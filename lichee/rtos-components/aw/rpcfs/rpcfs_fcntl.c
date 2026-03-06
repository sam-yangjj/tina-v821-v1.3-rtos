/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
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
#include "rpcfs_internal.h"

/*
 * FMODE_EXEC is 0x20
 * FMODE_NONOTIFY is 0x4000000
 * These cannot be used by userspace O_* until internal and external open
 * flags are split.
 * -Eric Paris
 */

/*
 * When introducing new O_* bits, please check its uniqueness in fcntl_init().
 */

#define RPCFS_O_ACCMODE	00000003
#define RPCFS_O_RDONLY	00000000
#define RPCFS_O_WRONLY	00000001
#define RPCFS_O_RDWR		00000002
#ifndef RPCFS_O_CREAT
#define RPCFS_O_CREAT		00000100	/* not fcntl */
#endif
#ifndef RPCFS_O_EXCL
#define RPCFS_O_EXCL		00000200	/* not fcntl */
#endif
#ifndef RPCFS_O_NOCTTY
#define RPCFS_O_NOCTTY	00000400	/* not fcntl */
#endif
#ifndef RPCFS_O_TRUNC
#define RPCFS_O_TRUNC		00001000	/* not fcntl */
#endif
#ifndef RPCFS_O_APPEND
#define RPCFS_O_APPEND	00002000
#endif
#ifndef RPCFS_O_NONBLOCK
#define RPCFS_O_NONBLOCK	00004000
#endif
#ifndef RPCFS_O_DSYNC
#define RPCFS_O_DSYNC		00010000	/* used to be O_SYNC, see below */
#endif
#ifndef RPCFS_FASYNC
#define RPCFS_FASYNC		00020000	/* fcntl, for BSD compatibility */
#endif
#ifndef RPCFS_O_DIRECT
#define RPCFS_O_DIRECT	00040000	/* direct disk access hint */
#endif
#ifndef RPCFS_O_LARGEFILE
#define RPCFS_O_LARGEFILE	00100000
#endif
#ifndef RPCFS_O_DIRECTORY
#define RPCFS_O_DIRECTORY	00200000	/* must be a directory */
#endif
#ifndef RPCFS_O_NOFOLLOW
#define RPCFS_O_NOFOLLOW	00400000	/* don't follow links */
#endif
#ifndef RPCFS_O_NOATIME
#define RPCFS_O_NOATIME	01000000
#endif
#ifndef RPCFS_O_CLOEXEC
#define RPCFS_O_CLOEXEC	02000000	/* set close_on_exec */
#endif

/*
 * Before Linux 2.6.33 only O_DSYNC semantics were implemented, but using
 * the O_SYNC flag.  We continue to use the existing numerical value
 * for O_DSYNC semantics now, but using the correct symbolic name for it.
 * This new value is used to request true Posix O_SYNC semantics.  It is
 * defined in this strange way to make sure applications compiled against
 * new headers get at least O_DSYNC semantics on older kernels.
 *
 * This has the nice side-effect that we can simply test for O_DSYNC
 * wherever we do not care if O_DSYNC or O_SYNC is used.
 *
 * Note: __O_SYNC must never be used directly.
 */
#ifndef RPCFS_O_SYNC
#define __RPCFS_O_SYNC	04000000
#define RPCFS_O_SYNC		(__RPCFS_O_SYNC|RPCFS_O_DSYNC)
#endif

#ifndef RPCFS_O_PATH
#define RPCFS_O_PATH		010000000
#endif

#ifndef __RPCFS_O_TMPFILE
#define __RPCFS_O_TMPFILE	020000000
#endif

/* a horrid kludge trying to make sure that this will fail on old kernels */
#define RPCFS_O_TMPFILE (__RPCFS_O_TMPFILE | RPCFS_O_DIRECTORY)
#define RPCFS_O_TMPFILE_MASK (__RPCFS_O_TMPFILE | RPCFS_O_DIRECTORY | RPCFS_O_CREAT)

#ifndef RPCFS_O_NDELAY
#define RPCFS_O_NDELAY	RPCFS_O_NONBLOCK
#endif

#define RPCFS_F_DUPFD		0	/* dup */
#define RPCFS_F_GETFD		1	/* get close_on_exec */
#define RPCFS_F_SETFD		2	/* set/clear close_on_exec */
#define RPCFS_F_GETFL		3	/* get file->f_flags */
#define RPCFS_F_SETFL		4	/* set file->f_flags */
#ifndef RPCFS_F_GETLK
#define RPCFS_F_GETLK		5
#define RPCFS_F_SETLK		6
#define RPCFS_F_SETLKW	7
#endif
#ifndef RPCFS_F_SETOWN
#define RPCFS_F_SETOWN	8	/* for sockets. */
#define RPCFS_F_GETOWN	9	/* for sockets. */
#endif
#ifndef RPCFS_F_SETSIG
#define RPCFS_F_SETSIG	10	/* for sockets. */
#define RPCFS_F_GETSIG	11	/* for sockets. */
#endif

#ifndef CONFIG_64BIT
#ifndef RPCFS_F_GETLK64
#define RPCFS_F_GETLK64	12	/*  using 'struct flock64' */
#define RPCFS_F_SETLK64	13
#define RPCFS_F_SETLKW64	14
#endif
#endif

#ifndef RPCFS_F_SETOWN_EX
#define RPCFS_F_SETOWN_EX	15
#define RPCFS_F_GETOWN_EX	16
#endif

#ifndef RPCFS_F_GETOWNER_UIDS
#define RPCFS_F_GETOWNER_UIDS	17
#endif

/*
 * Open File Description Locks
 *
 * Usually record locks held by a process are released on *any* close and are
 * not inherited across a fork().
 *
 * These cmd values will set locks that conflict with process-associated
 * record  locks, but are "owned" by the open file description, not the
 * process. This means that they are inherited across fork() like BSD (flock)
 * locks, and they are only released automatically when the last reference to
 * the the open file against which they were acquired is put.
 */
#define RPCFS_F_OFD_GETLK	36
#define RPCFS_F_OFD_SETLK	37
#define RPCFS_F_OFD_SETLKW	38

#define RPCFS_F_OWNER_TID	0
#define RPCFS_F_OWNER_PID	1
#define RPCFS_F_OWNER_PGRP	2

/* for F_[GET|SET]FL */
#define RPCFS_FD_CLOEXEC	1	/* actually anything with low bit set goes */

/* for posix fcntl() and lockf() */
#ifndef RPCFS_F_RDLCK
#define RPCFS_F_RDLCK		0
#define RPCFS_F_WRLCK		1
#define RPCFS_F_UNLCK		2
#endif

/* for old implementation of bsd flock () */
#ifndef RPCFS_F_EXLCK
#define RPCFS_F_EXLCK		4	/* or 3 */
#define RPCFS_F_SHLCK		8	/* or 4 */
#endif

/* operations for bsd flock(), also used by the kernel implementation */
#define RPCFS_LOCK_SH		1	/* shared lock */
#define RPCFS_LOCK_EX		2	/* exclusive lock */
#define RPCFS_LOCK_NB		4	/* or'd with one of the above to prevent
				   blocking */
#define RPCFS_LOCK_UN		8	/* remove lock */

#define RPCFS_LOCK_MAND	32	/* This is a mandatory flock ... */
#define RPCFS_LOCK_READ	64	/* which allows concurrent read operations */
#define RPCFS_LOCK_WRITE	128	/* which allows concurrent write operations */
#define RPCFS_LOCK_RW		192	/* which allows concurrent read & write ops */

#define RPCFS_F_LINUX_SPECIFIC_BASE	1024

#endif /* _AW_RPCFS_FCNTL_H */
