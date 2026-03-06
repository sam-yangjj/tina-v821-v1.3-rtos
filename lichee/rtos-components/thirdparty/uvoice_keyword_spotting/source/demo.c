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
#include "console.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include "../include/uvoice.h"
#include "../include/uvoice_license.h"

#define RPMSG 1
#define RPBUF 1
#define MAX_3SEC_BUF_SIZE (16000 * 1 * 2 * 3)  // 根据实际参数调整
//static char received_buf_in0[MAX_3SEC_BUF_SIZE] = {0};
//static int ai_start = 0;
//static int rpbuf_start = 0;

#if RPBUF
#include "../include/aw_rpbuf.h"

LIST_HEAD(aw_rpbuf_demo_buffers);
static hal_mutex_t g_rpbuf_demo_buffers_mutex;

struct rpbuf_demo_buffer_entry {
	int controller_id;
	struct rpbuf_buffer *buffer;
	struct list_head list;
};
void *handle = NULL;

static struct rpbuf_demo_buffer_entry *find_buffer_entry(const char *name)
{
	struct rpbuf_demo_buffer_entry *buf_entry;

	list_for_each_entry(buf_entry, &aw_rpbuf_demo_buffers, list)
	{
		if (0 == strcmp(rpbuf_buffer_name(buf_entry->buffer), name))
			return buf_entry;
	}
	return NULL;
}

static void rpbuf_demo_buffer_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printf("buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}
//static int data_cnt = 0;


static uint16_t crc16(const unsigned char *data, uint32_t len) {
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

typedef struct {
    uint32_t mSeq;
    uint16_t crc;
} AudioFrameHeader;


static uint32_t last_seq = 0;
static int is_first_frame = 1;

static int rpbuf_demo_buffer_rx_cb(struct rpbuf_buffer *buffer,
	void *data, int data_len, void *priv)
{
	if(data != NULL)
	{

	    AudioFrameHeader *header = (AudioFrameHeader*)data;
	    uint32_t current_seq = header->mSeq;
	    uint16_t received_crc = header->crc;
    	int audio_data_len = data_len - sizeof(AudioFrameHeader);
    	unsigned char *audio_data = (unsigned char*)data + sizeof(AudioFrameHeader);
	    uint16_t calculated_crc = crc16(audio_data, audio_data_len);
	    if (calculated_crc != received_crc) {
	        printf("CRC check failed! mSeq=%u, calc=0x%04X, recv=0x%04X\n",
	               current_seq, calculated_crc, received_crc);
	        return -1;
	    }

		if (!is_first_frame) {
			if (current_seq != last_seq + 1) {
				uint32_t lost_frames = current_seq - last_seq - 1;
				printf("Sequence discontinuous! Last=%u, Current=%u, Lost=%u frames\n",
					   last_seq, current_seq, lost_frames);
			}
		} else {
			is_first_frame = 0;
		}
		last_seq = current_seq;

		//printf("Calculated: 0x%04X, Received: 0x%04X, mSeq=%u, CRC check passed\n", calculated_crc, received_crc, current_seq);

	    int frame_size = 320;
	    int frame_count = audio_data_len / (frame_size*2);
		int remaining = audio_data_len % frame_size;
	    if (remaining > 0) {
	        printf("Remaining unprocessed: %d bytes (need %d)\n", remaining, frame_size);
	    }
	    for(int i = 0; i < frame_count; i++)
	    {
	        //unsigned char *micbuf = (unsigned char*)data + i * 160;
	        //unsigned char *refbuf = (unsigned char*)data + data_len / 2 + i * 160;
	        struct audio_buf audio;
	        memset(&audio, 0, sizeof(struct audio_buf));
	        //short buf_ref0[160] = {0};

	        audio.audioin[0] = (short*)(audio_data + i * frame_size);
	        audio.audioref[0] = (short*)(audio_data + audio_data_len/2 + i * frame_size);
	        audio.audioin[1] = NULL;

#if 0
	    if( (current_seq % 399)==0 ){
		 printf("[CRC] recv_seq: %d\n",current_seq);
		 printf("[CRC] data_len:%d bytes.\n",audio_data_len);
		 printf("[CRC] sample: ");
		 for(int j=0;j<5;j++){
		     printf("%d,",audio.audioin[0][j]);
		 }
		 printf("\n");
	    }
#endif
	        UVoice_HomeSpeech_Process(handle, &audio, frame_size);

	        //usleep(10000);
	    }
	}
	return 0;
}



static int rpbuf_demo_buffer_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printf("buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));
	return 0;
}

static const struct rpbuf_buffer_cbs rpbuf_demo_cbs = {
    .available_cb = rpbuf_demo_buffer_available_cb,
    .rx_cb = rpbuf_demo_buffer_rx_cb,
    .destroyed_cb = rpbuf_demo_buffer_destroyed_cb,
};

int rpbuf_demo_create(int controller_id, const char *name, int len)
{
	int ret;
	struct rpbuf_demo_buffer_entry *buf_entry = NULL;
	struct rpbuf_controller *controller = NULL;
	struct rpbuf_buffer *buffer = NULL;
	//if (!g_rpbuf_demo_buffers_mutex)
	g_rpbuf_demo_buffers_mutex = hal_mutex_create();

	hal_mutex_lock(g_rpbuf_demo_buffers_mutex);
	buf_entry = find_buffer_entry(name);
	if (buf_entry) {
		printf("Buffer named \"%s\" already exists\n", name);
		hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);

	buf_entry = hal_malloc(sizeof(struct rpbuf_demo_buffer_entry));
	if (!buf_entry) {
		RPBUF_DEMO_LOG(controller_id, name, len,
			"Failed to allocate memory for buffer entry\n");
		ret = -ENOMEM;
		goto err_out;
	}
	buf_entry->controller_id = controller_id;

	controller = rpbuf_get_controller_by_id(controller_id);
	if (!controller) {
		RPBUF_DEMO_LOG(controller_id, name, len,
			"Failed to get controller%d, controller_id\n", controller_id);
		ret = -ENOENT;
		goto err_free_buf_entry;
	}

	buffer = rpbuf_alloc_buffer(controller, name, len, NULL, &rpbuf_demo_cbs, NULL);
	if (!buffer) {
		RPBUF_DEMO_LOG(controller_id, name, len, "rpbuf_alloc_buffer failed\n");
		ret = -ENOENT;
		goto err_free_buf_entry;
	}
	buf_entry->buffer = buffer;
	rpbuf_buffer_set_sync(buffer, true);

	hal_mutex_lock(g_rpbuf_demo_buffers_mutex);
	list_add_tail(&buf_entry->list, &aw_rpbuf_demo_buffers);
	hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);

	return 0;

err_free_buf_entry:
	hal_free(buf_entry);
err_out:
	return ret;
}

int rpbuf_demo_destroy(const char *name)
{
	int ret;
	struct rpbuf_demo_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;

	if (g_rpbuf_demo_buffers_mutex)
		hal_mutex_lock(g_rpbuf_demo_buffers_mutex);

	buf_entry = find_buffer_entry(name);
	if (!buf_entry) {
		printf("Buffer named \"%s\" not found\n", name);
		hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	buffer = buf_entry->buffer;

	ret = rpbuf_free_buffer(buffer);
	if (ret < 0) {
		RPBUF_DEMO_LOG(buf_entry->controller_id,
			rpbuf_buffer_name(buffer),
			rpbuf_buffer_len(buffer),
			"rpbuf_free_buffer failed\n");
		hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);
		goto err_out;
	}

	list_del(&buf_entry->list);
	hal_free(buf_entry);

	hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);

	return 0;

err_out:
	return ret;
}

/* rpbuf_demo_transmit */
int rpbuf_demo_transmit(void *data, int data_len, int offset)
{
	int ret;
	struct rpbuf_demo_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;

	hal_mutex_lock(g_rpbuf_demo_buffers_mutex);
	buf_entry = find_buffer_entry(RPBUF_BUFFER_NAME_DEFAULT);
	if (!buf_entry) {
		printf("Buffer named \"%s\" not found\n", RPBUF_BUFFER_NAME_DEFAULT);
		hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	buffer = buf_entry->buffer;

	/*
	 * Before putting data to buffer or sending buffer to remote, we should
	 * ensure that the buffer is available.
	 */
	if (!rpbuf_buffer_is_available(buffer)) {
		RPBUF_DEMO_LOG(buf_entry->controller_id,
			rpbuf_buffer_name(buffer),
			rpbuf_buffer_len(buffer),
			"buffer not available\n");
		hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);
		ret = -EACCES;
		goto err_out;
	}

	if (data_len > rpbuf_buffer_len(buffer)) {
		RPBUF_DEMO_LOG(buf_entry->controller_id,
			rpbuf_buffer_name(buffer),
			rpbuf_buffer_len(buffer),
			"data length too long\n");
		hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);
		ret = -EACCES;
		goto err_out;
	}
	memcpy(rpbuf_buffer_va(buffer) + offset, data, data_len);

	ret = rpbuf_transmit_buffer(buffer, offset, data_len);
	if (ret < 0) {
		RPBUF_DEMO_LOG(buf_entry->controller_id,
			rpbuf_buffer_name(buffer),
			rpbuf_buffer_len(buffer),
			"rpbuf_transmit_buffer failed\n");
		hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);
		goto err_out;
	}
	hal_mutex_unlock(g_rpbuf_demo_buffers_mutex);

	return 0;

err_out:
	return ret;
}

#endif

#if RPMSG
#include "../include/aw_rpmsg.h"
#define BIND_CNT_MAX 10

const char *rpmsg_name = "rpmsg_lp";
//static uint32_t tx_delay_ms = 500;

//static uint32_t tx_len = 32;
char tmpbuf[RPMSG_BUFFER_SIZE];

#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_WAIT
hal_sem_t wait_cpux_sem;
#endif

//short received_buf_in0[320] = {0};
typedef enum {
    RPBUF_UNKNOWN = -1,  // 未知命令
    RPBUF_OK = 0,        // 操作成功
    RPBUF_ERROR,         // 操作失败
    RPBUF_START,         // 开始信号
    RPBUF_STOP           // 停止信号
} RPMSG_CMD;


struct ept_test_entry *eptdev = NULL;
static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	printf("rpmsg%d: rx %zu Bytes\r\n", (int)eptdev->client->id, len);
	RPMSG_CMD cmd = RPBUF_UNKNOWN;

	memcpy(&cmd, data, sizeof(RPMSG_CMD));
	printf("Received command: %d ", cmd);

	char *data_str = (char*)data;
	int int_param = 6400;  // 默认整数参数，若解析失败使用此值

	char *space_pos = strchr(data_str, ' ');
	if (space_pos != NULL && space_pos < data_str + len) {
		*space_pos = '\0';
		cmd = atoi(data_str);

		char *gdata = space_pos + 1;
		char *endptr;

		long val = strtol(gdata, &endptr, 10);

		if (endptr == gdata) {
			printf("gdata is not a number: %s\n", gdata);
		} else if (*endptr != '\0') {
			printf("gdata has extra characters: %s (only using %.*s)\n",
				   gdata, (int)(endptr - gdata), gdata);
		} else if (val < INT_MIN || val > INT_MAX) {
			printf("gdata value %ld is out of int range\n", val);
		} else {
			int_param = (int)val;
			printf("gdata converted to int: %d\n", int_param);
		}
	} else {
		cmd = atoi(data_str);
		printf("No gdata provided, using default int_param: %d\n", int_param);
	}

	switch (cmd) {
        case RPBUF_OK:
            printf("(RPBUF_OK)\n");
            break;
        case RPBUF_ERROR:
            printf("(RPBUF_ERROR)\n");
            break;
        case RPBUF_START:
			printf("rpbuf_start success !\n");
			printf("int_param = %d\n", int_param);
			if (rpbuf_demo_create(0, "rpbuf_test", int_param) == 0) {
		        low_power_rpmsg_to_cpux((char *)&(RPMSG_CMD){RPBUF_OK}, sizeof(RPMSG_CMD));
		    }
            break;
        case RPBUF_STOP:
            printf("(RPBUF_STOP)\n");
            break;
        default:
            printf("(UNKNOWN)\n");
            cmd = RPBUF_UNKNOWN;
            break;
    }
	return 0;
}

static int rpmsg_bind_cb(struct rpmsg_ept_client *client)
{
	printf("rpmsg%d: binding\r\n", client->id);

	eptdev = hal_malloc(sizeof(*eptdev));
	if (!eptdev) {
		openamp_err("failed to alloc client entry\r\n");
		return -ENOMEM;
	}

	memset(eptdev, 0, sizeof(*eptdev));
	eptdev->client = client;
	client->ept->priv = eptdev;
	eptdev = eptdev;
	eptdev->print_perf_data = (unsigned long)client->priv;

	return 0;
}

static int rpmsg_unbind_cb(struct rpmsg_ept_client *client)
{
	struct ept_test_entry *eptdev = client->ept->priv;

	printf("rpmsg%d: unbinding\r\n", client->id);

#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_WAIT
	hal_sem_clear(wait_cpux_sem);
	hal_sem_delete(wait_cpux_sem);
#endif
	hal_free(eptdev);
	eptdev = NULL;
	return 0;
}

int low_power_rpmsg_init(rpmsg_ept_cb cb)
{
	int cnt = 10;
	unsigned long print_perf_data = 0;
	int bind_state = -1;
	int bind_cnt = 0;

#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_WAIT
	wait_cpux_sem = hal_sem_create(0);
	if (!wait_cpux_sem) {
		printf("Failed to create rpmsg wait_cpux_sem\n");
		return -1;
	}
#endif

rebind:
	printf("wait cpux bind: %s, try cnt: %d/%d\r\n",
		rpmsg_name, ++bind_cnt, BIND_CNT_MAX);
	bind_state = rpmsg_client_bind(rpmsg_name, cb, rpmsg_bind_cb,
		rpmsg_unbind_cb, cnt, (void *)print_perf_data);
	if (bind_state != 0) {
		if (bind_cnt >= BIND_CNT_MAX) return -1;
		hal_msleep(500);
		goto rebind;
	}

#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_WAIT
	if (low_power_rpmsg_wait_cpux_ok() == 0) {
		printf("cpux init ok\n");
	}
#endif
	return 0;
}

int low_power_rpmsg_deinit(void)
{
	printf("unbind cpux: %s\r\n", rpmsg_name);
	rpmsg_client_unbind(rpmsg_name); /* self-unbind */
	return 0;
}

int low_power_rpmsg_to_cpux(char *send_msg, int len)
{
	if (eptdev == NULL) {
		printf("warn:eptdev not init\n");
		return -1;
	}
	int ret = 0;
	if (len <= 0 || len > sizeof(tmpbuf)) {
		printf("invalid len: %d\n", len);
		return -1;
	}
	memcpy(tmpbuf, send_msg, len);
	//printf("tmpbuf binary data %s, len = %d\n", tmpbuf, len);
	ret = rpmsg_send(eptdev->client->ept, tmpbuf, len);
	if (ret < 0) {
		printf("rpmsg%d: Failed to send data\n", eptdev->client->id);
	}
	return ret;  // 返回实际发送结果（原代码固定返回0，不合理）
}

#ifdef CONFIG_COMPONENTS_LOW_POWER_RPMSG_WAIT
/* wait w27l2 ok */
int low_power_rpmsg_wait_cpux_ok(void)
{
	hal_sem_timedwait(wait_cpux_sem, HAL_WAIT_FOREVER);
	return 0;
}
#endif

int low_power_rpmsg_set_keepalive_state(int state)
{
	if (eptdev == NULL) {
		printf("warn:eptdev not init\n");
		return -1;
	}
	eptdev->keep_alive = state;
	return 0;
}

int low_power_rpmsg_get_sleep_state(void)
{
	if (eptdev == NULL) {
		printf("warn:eptdev not init\n");
		return -1;
	}
	return eptdev->keep_alive;
}

#endif
void error(void *handle, int errorcode, void *usrdata)
{
    printf("errorcode = <%d>\n", errorcode);
}

// 唤醒回调函数，成功唤醒时调用此函数
//void kws(void *handle, int id, float confidence, void *usrdata)
void kws(void *handle, char* pbuf, void *usrdata)
{
    //printf("kws id = <%d>\n", id);
	//rpbuf_demo_transmit("audio detect success <xiaoyoutongxue> ", 128, 0);
	//low_power_rpmsg_to_cpux("audio_detect_ok", 32);
	low_power_rpmsg_to_cpux(pbuf, 32);
}

// 授权回调函数实现样例
const char* my_demo_auth_cb(const char* body)
{
    printf("Body is:%s\n",body);
    char curl_cmd[1024];
    int rc = snprintf(curl_cmd,1024,"curl -k -s --data-binary '%s' %s",body, "https://srv01.51asr.com:8007/asrsn_active2");
    printf("curl_cmd: %s\n",curl_cmd);
    if ( rc >0 ) {
        return strdup(curl_cmd);
    }
    return "";
}

int ai_main(void)
{
    //int input_mode = 1;  // 2mic1ref

	printf("ai_main in\n");

	low_power_rpmsg_init(rpmsg_ept_callback);
	//rpbuf_demo_create(0, "rpbuf_test", 1024);

    // 设置回调信息
    struct callback_function* function_cb;
    function_cb = (struct callback_function*)malloc(sizeof(struct callback_function));
    memset(function_cb,0,sizeof(struct callback_function));
    function_cb->kws_cb = kws;
    function_cb->error_cb = error;

    // 设置授权信息
    uv_activate_param param;
    param.license = "";
    param.uuid = "";
    param.license_path = "";
    param.activate_type = 0;
    param.auth_cb = my_demo_auth_cb;

    handle = UVoice_HomeSpeech_Init((void*)(&param),NULL);

    if (handle == NULL) {
        printf("handle init fail\n");
        return -1;
    }
    printf("demo init ok\n");
    printf("SDK Version: %s\n",UVoice_HomeSpeech_GetVersion());

    UVoice_HomeSpeech_SetCallback(handle, function_cb);
    UVoice_HomeSpeech_SetUserData(handle, NULL);

    printf("callback set ok\n");

    struct audio_buf audio;
    memset(&audio, 0, sizeof(struct audio_buf));
    short buf_in0[160] = {0};  // 麦克风1数据
    //short buf_in1[160] = {0};  // 麦克风2数据
    short buf_ref0[160] = {0}; // 参考音频数据

    // 设置音频缓冲区
    audio.audioin[0] = buf_in0;
    audio.audioref[0] = buf_ref0;
    audio.audioin[1] = NULL;

	//int cnt = 0;
//	RPMSG_CMD rpmsg_cmd = RPBUF_OK;
    while(1)
    {
    #if 0
		if(rpbuf_start == 1)
		{
			printf("rpbuf_start success !\n");
			int ret = rpbuf_demo_create(0, "rpbuf_test", 1024);
			rpbuf_start= 0;
			if(ret == 0)
				low_power_rpmsg_to_cpux((char *)&rpmsg_cmd, 32);
		}

    	if(ai_start == 1)
    	{
    		int total_len = MAX_3SEC_BUF_SIZE;
		    int frame_size = 320;
		    int frame_count = total_len / frame_size;

		    for (int i = 0; i < frame_count; i++)
		    {
		    	printf("i=%d: num=%d short\n", i, frame_size / sizeof(short));
		        short* current_frame = (short *)received_buf_in0 + (i * frame_size / sizeof(short));

		        audio.audioin[0] = current_frame;
				audio.audioref[0] = buf_ref0;
		        UVoice_HomeSpeech_Process(handle, &audio, frame_size);

		        usleep(10000);
		    }
			ai_start=0;
			//break;
    	}
    #endif
        usleep(10000);  // 10ms
		//printf("cnt = %d\n", cnt++);
    }

    // 理论上不会执行到这里，如需退出机制可添加信号处理
    UVoice_HomeSpeech_Fini(handle);
    free(function_cb);
    return 0;
}

static void ksw_main(void *arg)
{
	ai_main();
}

int kws_test(int argc, char *argv[])
{
	hal_thread_t ksw_main_thread;
	ksw_main_thread = hal_thread_create(ksw_main,
							NULL,
							"ksw_main",
							2048,
							17);
	if (!ksw_main_thread) {
		printf("run ksw_main_thread\n");
		hal_thread_start(ksw_main_thread);
	}
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(kws_test, kws_test, kws_test);
