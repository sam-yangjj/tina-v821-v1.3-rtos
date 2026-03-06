/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <awlog.h>
#include <stdlib.h>
#include <sunxi_hal_common.h>
#include <hal_mem.h>
#include <hal_thread.h>
#include <console.h>
#include <trace_event.h>
#include <hal_atomic.h>

#define ev_err(fmt, ...)	printf("[EVENT]" fmt, ##__VA_ARGS__)

#define MAX_EVENT_ARGS		(CONFIG_MAX_EVENT_ARGS)
#define MAX_BUFFER_SIZE		(CONFIG_MAX_EVENT_BUFFER_SIZE * 1024)
#define MAX_BUFFER_UNITS	(MAX_BUFFER_SIZE / (sizeof(uint32_t)))

#define events_get_prio(tcb) \
		({ \
			unsigned int prio; \
			if (ev_get_irq_nest()) \
				prio = uxTaskPriorityGetFromISR(tcb); \
			else \
				prio = uxTaskPriorityGet(tcb); \
			prio; \
		})

#define events_get_stat(tcb) \
		({ \
			eTaskState stat; \
			if (ev_get_irq_nest()) \
				stat = eRunning; \
			else \
				stat = eTaskGetState(tcb); \
			stat; \
		})

#define get_val(ev, mask, shift) \
		({ \
			unsigned long val; \
			val = (ev)->stat & (mask); \
			val >>= (shift); \
			val; \
		 })

#define get_arg_cnt(ev)			get_val(ev, ARGCNT_MASK, ARGCNT_SHIFT)

#define get_arg_sz(ev)			get_val(ev, ARGSZ_MASK, ARGSZ_SHIFT)

#define get_subsys(ev)			get_val(ev, SUBSYS_MASK, SUBSYS_SHIFT)

#define get_irq_nest(ev)		get_val(ev, NEST_MASK, NEST_SHIFT)

#define get_irqoff(ev)			get_val(ev, IRQOFF_MASK, IRQOFF_SHIFT)

#define get_event_type(ev)		get_val(ev, EV_TYPE_MASK, EV_TYPE_SHIFT)

#define event_size(argsz)		((sizeof(os_event_t) + argsz) / sizeof(uint32_t))

#define event_base_sz()			((sizeof(os_event_t)) / sizeof(uint32_t))
#define event_arg_sz(ev)		(get_arg_sz(ev))

#define get_task_pid(ev)		((ev)->pid)

#define get_task_tcb(ev)		pid2tcb((ev)->pid)

#define get_event_name(ev)		((ev)->name)

#define get_event_time(ev)		((ev)->time)

#define get_subsys_str(ev)		(subsys_class[get_subsys(ev)].name)

static uint32_t write_pos = 0;
static uint32_t events_buffer[MAX_BUFFER_UNITS] = { 0 };
int event_tracing = 0;

#define MAX_TASK_NAME_CNT			CONFIG_MAX_TASK_NAME_CNT
#define MAX_TASK_NAME_LEN			CONFIG_MAX_TASK_NAME_LEN
struct pid_map {
	void *tcb;
	unsigned int prio;
	eTaskState stat;
	char name[MAX_TASK_NAME_LEN];
};
static struct pid_map pid_maps[MAX_TASK_NAME_CNT] = { 0 };

static int tcb2pid(void *tcb)
{
	int i;

	for (i = 0; i < MAX_TASK_NAME_CNT; i++) {
		if (pid_maps[i].tcb == tcb)
			break;
		if (pid_maps[i].tcb == NULL) {
			const char *name = hal_thread_get_name(tcb);
			pid_maps[i].tcb = tcb;
			events_memcpy(pid_maps[i].name, name, MAX_TASK_NAME_LEN);
			pid_maps[i].name[MAX_TASK_NAME_LEN - 1] = '\0';
			pid_maps[i].prio = events_get_prio(tcb);
			pid_maps[i].stat = events_get_stat(tcb);
			break;
		}
	}

	return i;
}

static inline void *pid2tcb(int pid)
{
	return pid_maps[pid].tcb;
}

static inline const char *get_task_name(int pid)
{
	return pid_maps[pid].name;
}

static inline unsigned int get_task_prio(int pid)
{
	return pid_maps[pid].prio;
}

static void hexdump_event(os_event_t *ev, event_print pr)
{
	int i;
	uint32_t len;
	uint32_t *p = (uint32_t *)ev;

	len = event_base_sz() + event_arg_sz(ev);

	pr("hexdump event: %p, len=%u", p, len);

	p -= 16;
	len += 16;
	for (i = 0; i < len; i++) {
		uint32_t val = p[i];
		if ((i & 0x3) == 0)
			pr("\r\n0x%08lx: ", (long)(p + i));
		pr("0x%08lx ", (long)val);
	}
	pr("\r\n");
}

static int get_name_cnt(const char *from)
{
	// name formats: event_name:argN_type:argN_name:....:
	const char *p = from;
	int i = 0;

	while (*p != '\0') {
		if (*p == ':')
			i++;
		p++;
	}

	return i;
}

static char get_tcb_state(int pid)
{
	eTaskState stat = pid_maps[pid].stat;
	switch (stat) {
	case eRunning: return 'R';
	case eReady: return 'R';
	case eBlocked: return 'S';
	case eSuspended: return 'D';
	case eDeleted: return 'D';
	default: return 'X';
	}
}

static int get_name_from_names(const char *from, int n, char *to, int max)
{
	// name formats: event_name:argN_type:argN_name:....:
	const char *p = from;
	const char *pp = from;
	int i = 0;

	if (!from) {
		ev_err("BUG: from is NULL, n=%d, to=%p, max=%d\r\n", n, to, max);
		return -EINVAL;
	}

	while (*pp != '\0') {
		if (*pp == ':') {
			if (i == n) {
				int len = (((unsigned long)pp) - ((unsigned long)p));
				memcpy(to, p, len > max ? max : len);
				to[len] = '\0';
				return 0;
			}
			p = pp + 1;
			i++;
		}
		pp++;
	}

	if (i == n) {
		int len = (((unsigned long)pp) - ((unsigned long)p));
		memcpy(to, p, len > max ? max : len);
		to[len] = '\0';
		return 0;
	}

	/* no found */
	return 1;
}

static void *get_arg_from_args(TRACE_EVENT_ARG_UNIT *args, int *ofs, int type)
{
	void *retp = &args[*ofs];
	switch(type) {
	case ARG_PRINT_U:
		*ofs += _count_arg_type_sz(ARG_PRINT_U, *(unsigned int *)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_X:
		*ofs += _count_arg_type_sz(ARG_PRINT_X, *(unsigned int *)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_D:
		*ofs += _count_arg_type_sz(ARG_PRINT_D, *(unsigned int *)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_C:
		*ofs += _count_arg_type_sz(ARG_PRINT_C, *(unsigned int *)(retp)) / sizeof(uint32_t);
		break;

	case ARG_PRINT_LD:
		*ofs += _count_arg_type_sz(ARG_PRINT_LD, *(unsigned long)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_LU:
		*ofs += _count_arg_type_sz(ARG_PRINT_LU, *(unsigned long)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_LX:
		*ofs += _count_arg_type_sz(ARG_PRINT_LX, *(unsigned long)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_P:
		*ofs += _count_arg_type_sz(ARG_PRINT_P, *(unsigned long)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_STR:
		*ofs += _count_arg_type_sz(ARG_PRINT_STR, (const char *)(retp)) / sizeof(uint32_t);
		break;
	default:
		printf("Undown type %d ofs=%d ", type, *ofs); break;
	}

	return retp;
}

os_event_t *trace_get_ev_space(int argsz)
{
	unsigned long sz = event_size(argsz);

	if ((MAX_BUFFER_UNITS - write_pos) > sz)
		write_pos += sz;
	else {
		/* mark unused */
		events_buffer[write_pos] = ~(EVENT_MAGIC);
		write_pos = sz;
	}
	//printf("new write_pos:%d\r\n", write_pos);
	return (os_event_t *)(&events_buffer[write_pos - sz]);
}

os_event_t *trace_init_event(unsigned long argsz)
{
	os_event_t *event = trace_get_ev_space(argsz);
	event->magic = EVENT_MAGIC;
	event->time =  ev_get_time();
	event->pid = tcb2pid(ev_get_tcb());

	return event;
}

static void trace_event_dump_arg(event_print pr, int type, const char *name, void *p, bool is_counter)
{
	char c = '=';
	if (is_counter)
		c = '|';

	//pr("val addr:%p\r\n", p);
	switch(type) {
	case ARG_PRINT_U: {
			unsigned int val = *(unsigned int *)p;
			pr("%s%c%u ", name, c, val);
		}
		break;
	case ARG_PRINT_LU: {
			unsigned long val = *(unsigned long *)p;
			pr("%s%c%lu ", name, c, val); break;
		}
		break;
	case ARG_PRINT_X: {
			unsigned int val = *(unsigned int *)p;
			pr("%s%c0x%x ", name, c, val);
		}
		break;
	case ARG_PRINT_LX: {
			unsigned long val = *(unsigned long *)p;
			pr("%s%c0x%08lx ", name, c, val);
		}
		break;
	case ARG_PRINT_D: {
			int val = *(int *)p;
			pr("%s%c%d ", name, c, val);
		}
		break;
	case ARG_PRINT_LD: {
			long val = *(long *)p;
			pr("%s%c%ld ", name, c, val);
		}
		break;
	case ARG_PRINT_C: {
			char val = *(char *)p;
			pr("%s%c%c ", name, c, (char)val);
		}
		break;
	case ARG_PRINT_STR: {
			const char *val = (const char *)p;
			if (strcmp(name, "func"))
				pr("%s%c%s ", name, c, val);
			else
				pr("%s ", val);
		}
		break;
	case ARG_PRINT_P:
		pr("%s%c%lx ", name, c, *(uintptr_t*)p); break;
	default:
		pr("Undown type %d %s=%lx ", type, name, *(unsigned long *)p); break;
	}
}

static int trace_event_dump_one(os_event_t *ev, event_print pr)
{
	static int last_pid = -1;
	const char *name = get_event_name(ev);
	const char *subsys_str;
	int subsys;
	uint64_t time = get_event_time(ev);
	unsigned long pid;
	char name_buf[128];
	const int cpu = 0;  /* only support signal core */
	int arg_ofs = 0, ofs = 0;
	int i = 0;

	if (get_subsys(ev) > EV_NUM_SUBSYS) {
		pr("Invalid %s event subsys idex:%d, MAX:%d\r\n", name, get_subsys(ev), EV_NUM_SUBSYS);
		return -EINVAL;
	}

	subsys_str = get_subsys_str(ev);
	subsys = get_subsys(ev);
	pid = get_task_pid(ev);

	if (get_name_from_names(name, 0, name_buf, sizeof(name_buf)) != 0) {
		pr("can't get func name from event %p, %s, n=0\r\n", ev, name ? name : "NULL");
		hexdump_event(ev, pr);
		return -EINVAL;
	}

	/* the first is func_name, one arg <-> 2 arg: type:name */
	if (get_name_cnt(name) != (get_arg_cnt(ev) * 2 + 1)) {
		pr("[%s]: Invalid ArgCnt:%d, Name Cnt:%d\r\n", name, get_arg_cnt(ev),
						get_name_cnt(name));
		return -EINVAL;
	}

	if (subsys == EV_SCHE && name_buf[0] == 'o') {
		last_pid = pid;
		return 0;
	}

	pr("%16s-%lu  [%d] ", get_task_name(pid), pid, cpu);

	pr("%c%d ", get_irqoff(ev) ? 'd':'.', get_irq_nest(ev));

#ifdef CONFIG_EVENTS_PRINT_TIME_FMT_LONG
	pr("%08ld:", time);
#endif
#ifdef CONFIG_EVENTS_PRINT_TIME_FMT_FLOAT
	pr("%0.6f:", (float)time / 1000000000.0F);
#endif
#ifdef CONFIG_EVENTS_PRINT_TIME_FMT_DOUBLE
	pr("%0.6lf:", (double)time / 1000000000.0);
#endif

	switch(get_subsys(ev)) {
	case EV_SCHE:
		if (name_buf[0] == 'i') {
			if (last_pid != -1) {
				pr(" sched_switch: prev_comm=%s prev_pid=%lu prev_prio=%u",
						get_task_name(last_pid), last_pid, get_task_prio(last_pid));
				pr(" prev_state=%c ==> next_comm=%s next_pid=%lu next_prio=%u\r\n",
						get_tcb_state(last_pid), get_task_name(pid), pid, get_task_prio(pid));
			}
		}
		return 0;
	default: pr(" tracing_mark_write:"); break;
	}

	switch (get_event_type(ev)) {
	case EV_TYPE_BEGIN:
		pr(" B|%lu|", pid);
		pr("%s: ", subsys_str);
		if (name_buf[0] != '\0')
			pr("%s ", name_buf);
		break;
	case EV_TYPE_END:
		pr(" E|%lu|", pid);
		pr("%s: ", subsys_str);
		if (name_buf[0] != '\0')
			pr("%s ", name_buf);
		break;
	case EV_TYPE_MARK:
		pr(" I|%lu|", pid);
		pr("%s: ", subsys_str);
		if (name_buf[0] != '\0')
			pr("%s ", name_buf);
		break;
	case EV_TYPE_CNT:
		pr(" C|%lu|", ev->args[0]); ofs = 1; break;
	default: break;
	}

	//pr("sys=%s ", subsys);
	//pr("ev full name: %s\r\n", ev->name);

	for (i = 0; i < get_arg_cnt(ev); i++) {
		int type;
		void *p;

		if (get_name_from_names(name, i * 2 + 1, name_buf, sizeof(name_buf)) != 0) {
			pr("can't get arg%d type from %s\r\n", i, name ? name : "NULL");
			hexdump_event(ev, pr);
			return -EINVAL;
		}
		type =(int)(name_buf[0] - '0');
		//printf("type:%d\r\n", type);
		if (get_name_from_names(name, i * 2 + 2, name_buf, sizeof(name_buf)) != 0) {
			pr("can't get arg%d name from %s\r\n", i, name ? name : "NULL");
			hexdump_event(ev, pr);
			return -EINVAL;
		}
		//printf("name:%s\r\n", name_buf);
		//printf("arg_ofs:%d\r\n", arg_ofs);
		p = get_arg_from_args(ev->args, &arg_ofs, type);

		if (i < ofs)
			continue;
		trace_event_dump_arg(pr, type, name_buf, p, get_event_type(ev) == EV_TYPE_CNT);
	}
	pr("\r\n");

	return 0;
}

void trace_event_dump(event_print pr)
{
	unsigned long first, flags;
	uint32_t *tmp_buf = NULL, tmp_pos;
	os_event_t *ev;

	tmp_buf = hal_malloc(MAX_BUFFER_SIZE);
	if (!tmp_buf) { /* out of memory? */
		flags = hal_enter_critical();
		tmp_buf = events_buffer;
		tmp_pos = write_pos;
	} else {
		flags = hal_enter_critical();
		memcpy(tmp_buf, events_buffer, MAX_BUFFER_SIZE);
		tmp_pos = write_pos;
		hal_exit_critical(flags);
	}

	first = tmp_pos;
	while (first < (tmp_pos + 256)) {

		if (tmp_buf[first] == EVENT_MAGIC)
			break;

		first++;

		if (first >= MAX_BUFFER_UNITS) {
			first = 0;
			break;
		}

		if (first == tmp_pos)		/* no any envent */
			goto err_out;
	}

	if (first == (tmp_pos + 256))
		first = 0;

	if (tmp_buf[first] != EVENT_MAGIC) {
		pr("Not Any Events\r\n");
		goto err_out;
	}

	pr("# tracer: nop\r\n#\r\n");
	pr("#                        _---=> irqs-off (.: irq enable, d:irq disabled)\r\n");
	pr("#                       /  _--=> preempt-depth\r\n");
	pr("#                       | /\r\n");
	pr("#       TASK-PID   CPU# || TIMESTAMP\r\n");
	pr("#          | |       |  ||     |\r\n");

	while(1) {
		ev = (os_event_t *)(&tmp_buf[first]);

		trace_event_dump_one(ev, pr);
		/* find next event */
		first += event_base_sz() + event_arg_sz(ev);

		if (first == tmp_pos)
			break;

		if (tmp_buf[first] != EVENT_MAGIC) {
			if (tmp_buf[first] == ~(EVENT_MAGIC)) {
				/* is the unused, skip it */
				first = 0;
				if (tmp_buf[first] == EVENT_MAGIC)
					continue;
			}
			pr("Invalid Event Fromet, idx:%d\r\n", first * sizeof(uint32_t));
			goto err_out;
		}
	}

err_out:
	if (tmp_buf == events_buffer)
		hal_exit_critical(flags);
	else
		hal_free(tmp_buf);

	return;
}

void trace_event_panic_dump(event_print pr)
{
	unsigned long first;
	uint32_t *tmp_buf = NULL, tmp_pos;
	os_event_t *ev;

	tmp_buf = events_buffer;
	tmp_pos = write_pos;

	first = tmp_pos;
	while (first < (tmp_pos + 256)) {

		if (tmp_buf[first] == EVENT_MAGIC)
			break;

		first++;

		if (first >= MAX_BUFFER_UNITS) {
			first = 0;
			break;
		}

		if (first == tmp_pos)		/* no any envent */
			goto err_out;
	}

	if (first == (tmp_pos + 256))
		first = 0;

	pr("write_pos = %d\r\n", tmp_pos);
	pr("first = %d\r\n", first);
	if (tmp_buf[first] != EVENT_MAGIC) {
		pr("Not Any Events\r\n");
		goto err_out;
	}

	pr("# tracer: nop\r\n#\r\n");
	pr("#                        _---=> irqs-off (.: irq enable, d:irq disabled)\r\n");
	pr("#                       /  _--=> preempt-depth\r\n");
	pr("#                       | /\r\n");
	pr("#       TASK-PID   CPU# || TIMESTAMP\r\n");
	pr("#          | |       |  ||     |\r\n");

	while(1) {
		ev = (os_event_t *)(&tmp_buf[first]);

		trace_event_dump_one(ev, pr);
		/* find next event */
		first += event_base_sz() + event_arg_sz(ev);

		if (first == tmp_pos)
			break;

		if (tmp_buf[first] != EVENT_MAGIC) {
			if (tmp_buf[first] == ~(EVENT_MAGIC)) {
				/* is the unused, skip it */
				first = 0;
				if (tmp_buf[first] == EVENT_MAGIC)
					continue;
			}
			pr("Invalid Event Fromet, idx:%d\r\n", first * sizeof(uint32_t));
			goto err_out;
		}
	}

err_out:
	return;
}

static void trace_event_clear_buffer(void)
{
	unsigned long flags;

	flags = events_buffer_lock();
	memset(events_buffer, 0, MAX_BUFFER_SIZE);
	memset(pid_maps, 0, sizeof(pid_maps));
	write_pos = 0;
	events_buffer_unlock(flags);
}

void trace_event_dump_all_events(event_print pr)
{
	int i;
	struct subsys_entry *ev;

	for (i = 0; i < EV_NUM_SUBSYS; i++) {
		ev = &subsys_class[i];
		if (!ev->buildin)
			continue;
		pr("\t%-30s [%s]\r\n", ev->name, ev->enable ? "enable" : "disable");
	}
}

static void trace_event_set_sys(const char *sys, bool action)
{
	int i;
	unsigned long flags;

	if (!sys) {
		flags = events_buffer_lock();
		for (i = 0; i < EV_NUM_SUBSYS; i++) {
			if (!subsys_class[i].buildin)
				continue;
			if (action)
				subsys_class[i].enable = 1;
			else
				subsys_class[i].enable = 0;
		}
		events_buffer_unlock(flags);
		return;
	}

	for (i = 0; i < EV_NUM_SUBSYS; i++) {
		if (!subsys_class[i].buildin)
			continue;
		if (strcmp(sys, subsys_class[i].name))
			continue;
		flags = events_buffer_lock();
		if (action)
			subsys_class[i].enable = 1;
		else
			subsys_class[i].enable = 0;
		events_buffer_unlock(flags);
		return;
	}
	printf("Can't find %s subsys\r\n", sys);
}

static void event_print_help(void)
{
	printf("useage:\r\n");
	printf("  trace_events events        :dump all support subsys\r\n");
	printf("  trace_events dump          :dump events buffer\r\n");
	printf("  trace_events ctrl name 0/1 :ctrl wether enable sys event\r\n");
	printf("  trace_events disable       :disable all sys event\r\n");
	printf("  trace_events enable        :enable all sys event\r\n");
	printf("  trace_events clear         :clear event buffer\r\n");
}

int cmd_events(int argc, char ** argv)
{
	if (argc < 2) {
		event_print_help();
		return 0;
	}

	if (argc == 2) {
		if (!strcmp(argv[1], "dump"))
			trace_event_dump(printf);
		else if (!strcmp(argv[1], "events"))
			trace_event_dump_all_events(printf);
		else if (!strcmp(argv[1], "clear"))
			trace_event_clear_buffer();
		if (!strcmp(argv[1], "disable"))
			trace_event_set_sys(NULL, false);
		if (!strcmp(argv[1], "enable"))
			trace_event_set_sys(NULL, true);
		return 0;
	} else if (argc == 4) {
		if (!strcmp(argv[1], "ctrl")) {
			trace_event_set_sys(argv[2], argv[3][0] == '1' ? true : false);
		}
		return 0;
	}
	event_print_help();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_events, trace_events, Ctrl trace event);

void __attribute__((no_instrument_function))
__cyg_profile_func_enter(void *this_func, void *call_site)
{
	trace_event_begin(EV_USR0, "", ARG_PTR_RENAME(func, (unsigned long)(this_func)));
}

void __attribute__((no_instrument_function))
__cyg_profile_func_exit(void *this_func, void *call_site)
{
	trace_event_end(EV_USR0, "", ARG_PTR_RENAME(func, (unsigned long)(this_func)));
}
