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
#ifndef __UVOICE_H__
#define __UVOICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define UVOICE_ERROR_PARAMETER -4
#define UVOICE_ERROR_NULLPTR   -3
#define UVOICE_ERROR_GENER     -2
#define UVOICE_ERROR_UNINIT    -1
#define UVOICE_NORMAL           0

#define UVOICE_ERROR_CODE_ALLZEROS -10
#define UVOICE_ERROR_CODE_NONVALID_LICENSE -11


typedef enum uvoice_paramterId_e {
	uvoice_agc,
	uvoice_cmd,   /* control multi turn cmd activation, set it true in cmdcallback. */
	uvoice_vad,
	uvoice_mode,
        uvoice_reset_timeout,
        uvoice_mic_distance,
        uvoice_duplex_mode,
} uvoice_paramterId;

typedef void (*audioOut)(void *handle, short *buf, int len, int vad_status, void *usrdata);
//typedef void (*kwscallback)(void *handle, int id, float confidence, void *usrdata);
typedef void (*kwscallback)(void *handle, char* pbuf, void *usrdata);
typedef void (*cmdcallback)(void *handle, int id, float confidence, void *usrdata);
typedef void (*errorcallback)(void *handle, int errorcode, void *usrdata);
typedef int (*log_print_fun)(const char *format, ...);

struct callback_function {
	audioOut copydata_cb;
	kwscallback kws_cb;
	cmdcallback cmd_cb;
	errorcallback error_cb;
};

#define MAX_MICIN_NUM 8
#define MAX_REFIN_NUM 2

struct audio_buf {
    short *audioin[MAX_MICIN_NUM];
    short *audioref[MAX_REFIN_NUM];
};

// get SDK version
char *UVoice_HomeSpeech_GetVersion();

// re-direct SDK log info to log_print_func
void UVoice_HomeSpeech_SetLogFun(void* handle, log_print_fun fun);

// init homespeech SDK handle
void *UVoice_HomeSpeech_Init(void* param, const char* model_data_path);

// input audio stream to SDK
void UVoice_HomeSpeech_Process(void *handle, void *audio, int len);

// callback function setting
void UVoice_HomeSpeech_SetCallback(void *handle, void *cb);

// clear SDK memory
void UVoice_HomeSpeech_Fini(void *handle);

// set callback function user data
void UVoice_HomeSpeech_SetUserData(void *handle, void *usrdata);

// SDK configure setting, to enable VAD, etc.
void UVoice_HomeSpeech_Ctrl(void *handle, uvoice_paramterId id, void *value);

// predefine keywords
// params:
//	word_str: text of new keywords, text of words seperate by '\n'
//	model_data_path: path to store new keywords model
//int UVoice_HomeSpeech_MakeKwsFst(const char* word_str, const char* model_data_path);

#ifdef __cplusplus
}
#endif

#endif
