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
#include "mqtt_client.h"
#include "lp_rpmsg.h"
#include "lp_ctrl_msg.h"
#include "public.h"
#include "capacity_read.h"

__standby_unsaved_data uint8_t mqtt_buffer[1024] = { 0 };
__standby_unsaved_data char client_id_buf[32] = { 0 };
__standby_unsaved_data char client_subscribe_topic[64] = { 0 };
char client_publish_topic[64] = { 0 };

typedef struct {
	MQTTContext_t mqttContext;
	NetworkContext_t networkContext;
	MQTTPubAckInfo_t pIncomingPublishRecords[INCOMING_PUBLISH_RECORD_LEN];
	MQTTPubAckInfo_t pOutgoingPublishRecords[OUTGOING_PUBLISH_RECORD_LEN];
	MQTTSubscribeInfo_t pGlobalSubscriptionList[1];
	MQTTSubAckStatus_t globalSubAckStatus;
	uint16_t globalSubscribePacketIdentifier;
	uint16_t globalAckPacketIdentifier;
	struct wakelock mqtt_wakelock;
	hal_sem_t reconnect_sem;
	int mqtt_network_error_flags;
	int mqtt_temperature_flags;
	int mqtt_ping_cnt;
	hal_thread_t receive_pt;
	hal_thread_t delay_thread;
	volatile int wakeup_status;
	int mqtt_receive_pt_exit;
	int mqtt_receive_pt_exit_ok;
	volatile int mqtt_network_ok;
	int mqtt_lp_rpmsg_init_flags;
}mqtt_keep;

static mqtt_keep lp_mqtt_keepalive[1] = { 0 };

static void lp_mqtt_keepalive_init(void)
{
	memset(lp_mqtt_keepalive, 0, sizeof(lp_mqtt_keepalive));
	lp_mqtt_keepalive->reconnect_sem = hal_sem_create(0);
	if (lp_mqtt_keepalive->reconnect_sem == NULL) {
		LP_LOG_INFO("creat reconnect_sem fail\n");
	}
}

static void updateSubAckStatus(MQTTPacketInfo_t *pPacketInfo);

void lp_mqtt_low_power_msg_proc(uint32_t event, uint32_t data, void *arg)
{
	int ret = -1;
	uint16_t type = EVENT_SUBTYPE(event);
	struct netif *nif = g_wlan_netif;
	LP_LOG_INFO("Low Power net event: %s\r\n", net_ctrl_msg_type_to_str(type));
	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA && NET_IS_IP4_VALID(nif)) {
			ret = wlan_ext_low_power_param_set_default(10);
			if (ret == -2) {
				LP_LOG_ERR("Set Dtim 10 invalid arg\n");
			} else if (ret == -1) {
				LP_LOG_ERR("Set Dtim 10 exec failed\n");
			}
		}
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		LP_LOG_INFO("### Network up\n");
		if (nif && wlan_if_get_mode(nif) == WLAN_MODE_STA) {
			lp_mqtt_keepalive->mqtt_network_ok = 1;
			ret = wlan_ext_low_power_param_set_default(10);
			if (ret == -2) {
				LP_LOG_ERR("Set Dtim 10 invalid arg\n");
			} else if (ret == -1) {
				LP_LOG_ERR("Set Dtim 10 exec failed\n");
			}

		}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		LP_LOG_INFO("### Wlan disconnect\n");

		break;
	case NET_CTRL_MSG_WLAN_SSID_NOT_FOUND:
		LP_LOG_INFO("### Wlan ssid not found\n");
		break;
	case NET_CTRL_MSG_WLAN_CONNECTION_LOSS:
		LP_LOG_INFO("### Wlan connect loss(BSS LOST)\n");
		break;
	default:
		LP_LOG_WARN("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

static void eventCallback(MQTTContext_t *pMqttContext,
	MQTTPacketInfo_t *pPacketInfo,
	MQTTDeserializedInfo_t *pDeserializedInfo)
{
	LP_LOG_INFO("pPacketInfo->type = %02X\n", pPacketInfo->type);
	if ((pPacketInfo->type & 0xF0U) == MQTT_PACKET_TYPE_PUBLISH) {
		LP_LOG_INFO("Incoming Publish Topic Name: %.*s matches subscribed topic.\n"
			"Incoming Publish message Packet Id is %u.\n"
			"Incoming Publish Message : %.*s.\n\n",
			pDeserializedInfo->pPublishInfo->topicNameLength,
			pDeserializedInfo->pPublishInfo->pTopicName,
			pDeserializedInfo->packetIdentifier,
			(int)pDeserializedInfo->pPublishInfo->payloadLength,
			(const char *)pDeserializedInfo->pPublishInfo->pPayload);
		char *pPayload = (char *)pDeserializedInfo->pPublishInfo->pPayload;
		if (strstr(pPayload, "wakeup") || strstr(pPayload, "wake up")) {
			LP_LOG_INFO("MQTT get wakeup ,will wakeup now\n");
			lp_mqtt_keepalive->wakeup_status = 1;
			pm_wakelocks_acquire(&(lp_mqtt_keepalive->mqtt_wakelock), PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER);
			pm_wakesrc_relax(lp_mqtt_keepalive->mqtt_wakelock.ws, PM_RELAX_WAKEUP);  /* wakeup cpux */
			if (lp_mqtt_keepalive->mqtt_wakelock.ref) {
				LP_LOG_INFO("%s:pm_wakelocks_release in %d\n", __FUNCTION__, __LINE__);
				pm_wakelocks_release(&(lp_mqtt_keepalive->mqtt_wakelock));
			}
		}
	} else {
		/* Handle other packets. */
		switch (pPacketInfo->type) {
		case MQTT_PACKET_TYPE_SUBACK:
			updateSubAckStatus(pPacketInfo);
			if (lp_mqtt_keepalive->globalSubAckStatus != MQTTSubAckFailure) {
				LP_LOG_INFO("Subscribed to the topic %.*s. with maximum QoS %u.\n\n",
					strlen(client_subscribe_topic),
					client_subscribe_topic,
					lp_mqtt_keepalive->globalSubAckStatus);
			}
			lp_mqtt_keepalive->globalAckPacketIdentifier = pDeserializedInfo->packetIdentifier;
			break;
		case MQTT_PACKET_TYPE_PINGRESP:
			LP_LOG_INFO("\tPINGRESP ok\n");
			if (lp_mqtt_keepalive->mqtt_ping_cnt <= 0) {
				LP_LOG_INFO("PINGRESP mqtt_ping_cnt = %d\n", lp_mqtt_keepalive->mqtt_ping_cnt);
			} else {
				LP_LOG_INFO("PINGRESP mqtt_ping_cnt = %d\n", --(lp_mqtt_keepalive->mqtt_ping_cnt));
			}
			lp_rpmsg_set_keepalive_state(1);

			if (lp_mqtt_keepalive->mqtt_wakelock.ref) {
				LP_LOG_INFO("%s:pm_wakelocks_release in %d\n", __FUNCTION__, __LINE__);
				pm_wakelocks_release(&(lp_mqtt_keepalive->mqtt_wakelock));
			}
			break;
		}
	}
}

static int init_mqtt_connect(MQTTContext_t *pMqttContext,
	NetworkContext_t *pNetworkContext)
{
	MQTTStatus_t mqttStatus;
	TransportInterface_t transport = { NULL };
	MQTTFixedBuffer_t networkBuffer;
	transport.pNetworkContext = pNetworkContext;
	transport.send = portTransportSend;
	transport.recv = portTransportRecv;
	transport.writev = NULL;
	networkBuffer.pBuffer = mqtt_buffer;
	networkBuffer.size = sizeof(mqtt_buffer);
	portNetworkInit(pNetworkContext, CONNECT_WITH_TLS);
	mqttStatus = MQTT_Init(pMqttContext,
		&transport,
		portGetTimeMs,
		eventCallback,
		&networkBuffer);
	if (mqttStatus != MQTTSuccess) return EXIT_FAILURE;
	mqttStatus = MQTT_InitStatefulQoS(pMqttContext,
		lp_mqtt_keepalive->pOutgoingPublishRecords,
		OUTGOING_PUBLISH_RECORD_LEN,
		lp_mqtt_keepalive->pIncomingPublishRecords,
		INCOMING_PUBLISH_RECORD_LEN);
	if (mqttStatus != MQTTSuccess) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

static int establishMqttSession(MQTTContext_t *pMqttContext,
	bool createCleanSession,
	bool *pSessionPresent)
{
	int returnStatus = EXIT_SUCCESS;
	MQTTStatus_t mqttStatus;
	MQTTConnectInfo_t connectInfo;

	/* get wlan mac */
	char UserName[64] = { 0 };
	struct sysinfo *sysinfo = sysinfo_get();
	sprintf(client_id_buf, "alive_%02X:%02X:%02X:%02X:%02X:%02X",
		sysinfo->mac_addr[0], sysinfo->mac_addr[1],
		sysinfo->mac_addr[2], sysinfo->mac_addr[3],
		sysinfo->mac_addr[4], sysinfo->mac_addr[5]);
	LP_LOG_INFO("client_id_buf = %s\n", client_id_buf);  /* for example: alive_38:2A:8C:03:F9:0D */

	sprintf(UserName, "%02X:%02X:%02X:%02X:%02X:%02X",
		sysinfo->mac_addr[0], sysinfo->mac_addr[1],
		sysinfo->mac_addr[2], sysinfo->mac_addr[3],
		sysinfo->mac_addr[4], sysinfo->mac_addr[5]);
	LP_LOG_INFO("UserName = %s\n", UserName);

	connectInfo.cleanSession = createCleanSession == true ? true : false;
	connectInfo.pClientIdentifier = client_id_buf;
	connectInfo.clientIdentifierLength = strlen(client_id_buf);
	connectInfo.keepAliveSeconds = MQTT_KEEP_ALIVE_INTERVAL_SECONDS_SERVER;
	connectInfo.pUserName = UserName;
	connectInfo.userNameLength = strlen(UserName);
	connectInfo.pPassword = NULL;
	connectInfo.passwordLength = 0U;

	/* Send MQTT CONNECT packet to broker. */
	mqttStatus = MQTT_Connect(pMqttContext, &connectInfo, NULL, CONNACK_RECV_TIMEOUT_MS,
		pSessionPresent);

	if (mqttStatus != MQTTSuccess) {
		returnStatus = EXIT_FAILURE;
		LogError(("Connection with MQTT broker failed with status %s.",
			MQTT_Status_strerror(mqttStatus)));
	} else {
		LP_LOG_INFO("MQTT connection successfully established with broker.\n\n");
	}

	return returnStatus;
}

static int connectToServer(NetworkContext_t *pNetworkContext,
	MQTTContext_t *pMqttContext,
	bool *pClientSessionPresent,
	bool *pBrokerSessionPresent)
{
	int returnStatus = EXIT_FAILURE;
	ServerInfo_t serverInfo;
	bool createCleanSession;

	serverInfo.pHostName = BROKER_ENDPOINT;
	serverInfo.hostNameLength = BROKER_ENDPOINT_LENGTH;
	serverInfo.port = BROKER_PORT;

	do {
		LP_LOG_INFO("Establishing a session to %.*s:%d.",
			BROKER_ENDPOINT_LENGTH,
			BROKER_ENDPOINT,
			BROKER_PORT);
		returnStatus = portConnectServer(pNetworkContext,
			&serverInfo,
			1000,
			1000);

		if (returnStatus == EXIT_SUCCESS) {
			createCleanSession = (*pClientSessionPresent == true) ? false : true;
			returnStatus = establishMqttSession(pMqttContext, createCleanSession,
				pBrokerSessionPresent);
			if (returnStatus == EXIT_FAILURE) {
				(void)portDisconnectServer(pNetworkContext);
				return returnStatus;
			}
		}
	} while (0);
	return returnStatus;
}

static int waitForPacketAck(MQTTContext_t *pMqttContext,
	uint16_t usPacketIdentifier,
	uint32_t ulTimeout)
{
	uint32_t ulMqttProcessLoopEntryTime;
	uint32_t ulMqttProcessLoopTimeoutTime;
	uint32_t ulCurrentTime;

	MQTTStatus_t eMqttStatus = MQTTSuccess;
	int returnStatus = EXIT_FAILURE;

	lp_mqtt_keepalive->globalAckPacketIdentifier = 0U;

	ulCurrentTime = pMqttContext->getTime();
	ulMqttProcessLoopEntryTime = ulCurrentTime;
	ulMqttProcessLoopTimeoutTime = ulCurrentTime + ulTimeout;

	while ((lp_mqtt_keepalive->globalAckPacketIdentifier != usPacketIdentifier) &&
		(ulCurrentTime < ulMqttProcessLoopTimeoutTime) &&
		(eMqttStatus == MQTTSuccess || eMqttStatus == MQTTNeedMoreBytes)) {
		eMqttStatus = MQTT_ProcessLoop(pMqttContext);
		ulCurrentTime = pMqttContext->getTime();
	}

	if (((eMqttStatus != MQTTSuccess) && (eMqttStatus != MQTTNeedMoreBytes)) ||
		(lp_mqtt_keepalive->globalAckPacketIdentifier != usPacketIdentifier)) {
		LogError(("MQTT_ProcessLoop failed to receive ACK packet: "
			"Expected ACK Packet ID = %02X, LoopDuration = %u, Status = %s",
			usPacketIdentifier,
			(ulCurrentTime - ulMqttProcessLoopEntryTime),
			MQTT_Status_strerror(eMqttStatus)));
	} else {
		returnStatus = EXIT_SUCCESS;
	}

	return returnStatus;
}

static int subscribeToTopic(MQTTContext_t *pMqttContext)
{
	int returnStatus = EXIT_SUCCESS;
	MQTTStatus_t mqttStatus;

	/* Start with everything at 0. */
	(void)memset((void *)lp_mqtt_keepalive->pGlobalSubscriptionList, 0x00, sizeof(lp_mqtt_keepalive->pGlobalSubscriptionList));

	sprintf(client_subscribe_topic, "webrtc/%s/jsonrpc", client_id_buf);
	LP_LOG_INFO("client_subscribe_topic = %s\n", client_subscribe_topic);
	/* This example subscribes to only one topic and uses QOS1. */
	lp_mqtt_keepalive->pGlobalSubscriptionList[0].qos = MQTTQoS1;
	lp_mqtt_keepalive->pGlobalSubscriptionList[0].pTopicFilter = client_subscribe_topic;
	lp_mqtt_keepalive->pGlobalSubscriptionList[0].topicFilterLength = strlen(client_subscribe_topic);

	/* Generate packet identifier for the SUBSCRIBE packet. */
	lp_mqtt_keepalive->globalSubscribePacketIdentifier = MQTT_GetPacketId(pMqttContext);

	/* Send SUBSCRIBE packet. */
	mqttStatus = MQTT_Subscribe(pMqttContext,
		lp_mqtt_keepalive->pGlobalSubscriptionList,
		sizeof(lp_mqtt_keepalive->pGlobalSubscriptionList) / sizeof(MQTTSubscribeInfo_t),
		lp_mqtt_keepalive->globalSubscribePacketIdentifier);

	if (mqttStatus != MQTTSuccess) {
		LogError(("Failed to send SUBSCRIBE packet to broker with error = %s.",
			MQTT_Status_strerror(mqttStatus)));
		returnStatus = EXIT_FAILURE;
	} else {
		if (returnStatus == EXIT_SUCCESS) {
			returnStatus = waitForPacketAck(pMqttContext,
				lp_mqtt_keepalive->globalSubscribePacketIdentifier,
				1000);
			if (returnStatus == EXIT_SUCCESS) {
				LP_LOG_INFO("SUBSCRIBE sent for topic %.*s to broker.\n\n",
					strlen(client_subscribe_topic),
					client_subscribe_topic);
			}
		}
	}
	return returnStatus;
}

static int publishToTopic(MQTTContext_t *pMqttContext, char *payload)
{
	int returnStatus = EXIT_SUCCESS;
	MQTTStatus_t mqttStatus = MQTTSuccess;
	MQTTPublishInfo_t pubInfo;
	uint16_t packetId;

	sprintf(client_publish_topic, "webrtc/%s/jsonrpc-reply", client_id_buf);
	LP_LOG_INFO("client_publish_topic = %s\n", client_publish_topic);
	/* This example publishes to only one topic and uses QOS1. */
	pubInfo.qos = MQTTQoS1;
	pubInfo.pTopicName = client_publish_topic;
	pubInfo.topicNameLength = strlen(client_publish_topic);
	pubInfo.pPayload = payload;
	pubInfo.payloadLength = strlen(payload);

	/* Get a new packet id. */
	packetId = MQTT_GetPacketId(pMqttContext);
	/* Send PUBLISH packet. */
	mqttStatus = MQTT_Publish(pMqttContext, &pubInfo, packetId);

	if (mqttStatus != MQTTSuccess) {
		LogError(("Failed to send PUBLISH packet to broker with error = %s.",
			MQTT_Status_strerror(mqttStatus)));
		returnStatus = EXIT_FAILURE;
	} else {
		LP_LOG_INFO("PUBLISH sent for topic %.*s to broker with packet ID %u.\n\n",
			strlen(client_publish_topic),
			client_publish_topic,
			packetId);
	}

	return returnStatus;
}

static void updateSubAckStatus(MQTTPacketInfo_t *pPacketInfo)
{
	uint8_t *pPayload = NULL;
	size_t pSize = 0;

	MQTTStatus_t mqttStatus = MQTT_GetSubAckStatusCodes(pPacketInfo, &pPayload, &pSize);

	if (mqttStatus != MQTTSuccess) LP_LOG_INFO("MQTT_GetSubAckStatusCodes error\n");

	(void)mqttStatus;

	lp_mqtt_keepalive->globalSubAckStatus = (MQTTSubAckStatus_t)pPayload[0];
}

static int disconnectMqttSession(MQTTContext_t *pMqttContext, NetworkContext_t *pNetworkContext)
{
	LP_LOG_INFO("run %s:%d\n", __FUNCTION__, __LINE__);
	MQTTStatus_t mqttStatus = MQTTSuccess;
	int returnStatus = EXIT_SUCCESS;

	/* Send DISCONNECT */
	if (pMqttContext != NULL) {
		mqttStatus = MQTT_Disconnect(pMqttContext);
		if (mqttStatus != MQTTSuccess) {
			LogError(("Sending MQTT DISCONNECT failed with status=%s.",
				MQTT_Status_strerror(mqttStatus)));
			returnStatus = EXIT_FAILURE;
		}
	}
	low_power_set_keepalive_use_strategy(pNetworkContext->Socket, 0, DEFAULLT_USE_STRATEGY);
	if (pNetworkContext != NULL) {
		if (portDisconnectServer(pNetworkContext) != EXIT_SUCCESS) {
			LP_LOG_INFO("portDisconnectServer error\n");
			returnStatus = EXIT_FAILURE;
		}
	}
	return returnStatus;
}

static int wlan_keepalive_wakeup_init(unsigned int sec, void *arg);
static void wlan_wakeup_callback(void *arg)
{
	pm_wakelocks_acquire(&(lp_mqtt_keepalive->mqtt_wakelock), PM_WL_TYPE_WAIT_ONCE, OS_WAIT_FOREVER);
	if (lp_mqtt_keepalive->mqtt_network_error_flags == 1) {
		hal_sem_post(lp_mqtt_keepalive->reconnect_sem);
		wlan_keepalive_wakeup_init(MQTT_RECONNECT_INTERVAL, NULL);
	} else {
		wlan_keepalive_wakeup_init(KEEPALIVE_INTERVAL_MS, NULL);
	}
	lp_mqtt_keepalive->mqtt_temperature_flags = 1;
}

static int wlan_keepalive_wakeup_init(unsigned int sec, void *arg)
{
	wlan_ext_host_keepalive_param_t param;

	param.time = sec;
	param.callback = wlan_wakeup_callback;
	param.arg = arg;

	LP_LOG_INFO("set wake after %ds\n", sec);
	return wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_KEEPALIVE_PRTIOD, (uint32_t)&param);
}

static int wlan_keepalive_wakeup_deinit(void)
{
	wlan_ext_host_keepalive_param_t param;

	param.time = 0;
	param.callback = NULL;
	param.arg = NULL;

	LP_LOG_INFO("wlan_keepalive_wakeup_deinit\n");
	return wlan_ext_request(g_wlan_netif, WLAN_EXT_CMD_SET_KEEPALIVE_PRTIOD, (uint32_t)&param);
}

static int mqtt_reconnect(void)
{
	int attempts = 0;
	bool clientSessionPresent = true, brokerSessionPresent = false;
	int returnStatus = EXIT_SUCCESS;

	(void)disconnectMqttSession(&(lp_mqtt_keepalive->mqttContext), &(lp_mqtt_keepalive->networkContext));

	returnStatus = init_mqtt_connect(&(lp_mqtt_keepalive->mqttContext), &(lp_mqtt_keepalive->networkContext));
	LP_LOG_INFO("init_mqtt_connect Status = %s\n", returnStatus == EXIT_SUCCESS ? "EXIT_SUCCESS" : "EXIT_FAILURE");
	while (attempts < MQTT_MAX_RECONNECT_ATTEMPTS) {
		LP_LOG_INFO("%s try times %d\n", __FUNCTION__, attempts + 1);
		returnStatus = connectToServer(&(lp_mqtt_keepalive->networkContext),
			&(lp_mqtt_keepalive->mqttContext),
			&clientSessionPresent,
			&brokerSessionPresent);
		if (returnStatus == EXIT_SUCCESS) {
			LP_LOG_INFO("connectToServer success\n");
			returnStatus = subscribeToTopic(&(lp_mqtt_keepalive->mqttContext));
			if (returnStatus == EXIT_SUCCESS) {
				LP_LOG_INFO("subscribeToTopic success\n");
				publishToTopic(&(lp_mqtt_keepalive->mqttContext), "hello v821 online");
				low_power_set_keepalive_use_strategy(lp_mqtt_keepalive->networkContext.Socket, 1, DEFAULLT_USE_STRATEGY);
				return EXIT_SUCCESS;
			}
		}
		attempts++;
	}
	LP_LOG_INFO("Max reconnect attempts reached\n");
	return EXIT_FAILURE;
}

/* PING and receive loop */
static void mqtt_receive_loop_thread(void *arg)
{
	if (NULL == arg) {
		LP_LOG_INFO("[%u] : arg is NULL\n", (unsigned int)pthread_self());
		return;
	}
	MQTTContext_t *pMqttContext = (MQTTContext_t *)arg;
	MQTTStatus_t eMqttStatus = MQTTSuccess;

	while (!lp_mqtt_keepalive->mqtt_receive_pt_exit) {
		if (lp_mqtt_keepalive->mqtt_network_error_flags == 0) {
			while ((eMqttStatus == MQTTSuccess || eMqttStatus == MQTTNeedMoreBytes) && !lp_mqtt_keepalive->mqtt_receive_pt_exit) {
				if (lp_mqtt_keepalive->mqtt_temperature_flags == 1) {
					eMqttStatus = MQTT_Ping(pMqttContext);
					if (eMqttStatus == MQTTSuccess) {
						LP_LOG_INFO("\tMQTT_Ping ok\n");
						lp_mqtt_keepalive->mqtt_temperature_flags = 0;
						LP_LOG_INFO("MQTT_Ping mqtt_ping_cnt = %d\n", ++(lp_mqtt_keepalive->mqtt_ping_cnt));
						if (lp_mqtt_keepalive->mqtt_ping_cnt > 2) {
							LP_LOG_INFO("MQTT_Ping mqtt_ping_cnt more then 2, need reconnect\n");
							break;
						}
#ifdef CONFIG_COMPONENTS_LOW_POWER_STRATEGY_STANDBY_NOT_WAIT_SER_REPLY
						/* fast to ultra standby */
						if (lp_mqtt_keepalive->mqtt_wakelock.ref) {
							LP_LOG_INFO("%s:pm_wakelocks_release in %d\n",
								__FUNCTION__, __LINE__);
							pm_wakelocks_release(&(lp_mqtt_keepalive->mqtt_wakelock));
						}
#endif
					} else {
						LP_LOG_INFO("MQTT_Ping error\n");
						lp_rpmsg_set_keepalive_state(0);
						break;
					}
#if CONFIG_COMPONENTS_LOW_POWER_APP_MQTT_CAPACITY_READ
					uint8_t cap;
					if (capacity_read(&cap) != TWI_STATUS_OK)
						LP_LOG_INFO("capacity_read failed\n");
					else
						LP_LOG_INFO("the capacity is %d\n", cap);
#endif
				}
				eMqttStatus = MQTT_ReceiveLoop(pMqttContext);
				if (lp_mqtt_keepalive->wakeup_status == 1) {
					lp_mqtt_keepalive->wakeup_status = 0;
					// lp_rpmsg_init();  //call in resume
				}
			}
			if (lp_mqtt_keepalive->mqtt_receive_pt_exit) {
				(void)disconnectMqttSession(&(lp_mqtt_keepalive->mqttContext), &(lp_mqtt_keepalive->networkContext));
				break;
			}
			wlan_keepalive_wakeup_init(0, NULL);
			LP_LOG_INFO("MQTT Ping or Receive error ,eMqttStatus = %s\n",
				MQTT_Status_strerror(eMqttStatus));
		}

		if (!mqtt_reconnect()) {
			eMqttStatus = MQTTSuccess;
			lp_mqtt_keepalive->mqtt_network_error_flags = 0;
			lp_mqtt_keepalive->mqtt_ping_cnt = 0;
			lp_rpmsg_set_keepalive_state(1);
			LP_LOG_INFO("reconnect success\n");
			wlan_keepalive_wakeup_init(KEEPALIVE_INTERVAL_MS, NULL);
			continue;
		} else {
			/* Need to set up 5 minute to reconnect once */
			lp_mqtt_keepalive->mqtt_network_error_flags = 1;
			LP_LOG_INFO("reconnect error, reconnect every 5 minutes\n");
			wlan_keepalive_wakeup_init(MQTT_RECONNECT_INTERVAL, NULL);
			if (lp_mqtt_keepalive->mqtt_wakelock.ref) {
				LP_LOG_INFO("%s:pm_wakelocks_release in %d\n", __FUNCTION__, __LINE__);
				pm_wakelocks_release(&(lp_mqtt_keepalive->mqtt_wakelock));
			}
			hal_sem_timedwait(lp_mqtt_keepalive->reconnect_sem, HAL_WAIT_FOREVER);
		}
	}
	lp_mqtt_keepalive->mqtt_receive_pt_exit_ok = 1;
	LP_LOG_INFO("mqtt_receive_loop_thread exit !\n");
	hal_thread_stop(NULL);
}

#ifdef CONFIG_COMPONENTS_PM
void mqtt_delay_thread(void *arg)
{
	LP_LOG_INFO("mqtt_delay_thread run\n");
	vTaskDelay(pdMS_TO_TICKS(1000));
	lp_mqtt_keepalive->mqtt_lp_rpmsg_init_flags = 1;
	lp_rpmsg_init();
	hal_thread_stop(NULL);
}

static int mqtt_lp_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	LP_LOG_INFO("### mqtt_lp_suspend run\n");
	lp_mqtt_keepalive->mqtt_lp_rpmsg_init_flags = 0;
	return 0;
}

static int mqtt_lp_resume(struct pm_device *dev, suspend_mode_t mode)
{
	LP_LOG_INFO("### mqtt_lp_resume run\n");
	lp_mqtt_keepalive->delay_thread = hal_thread_create(mqtt_delay_thread, NULL, "mqtt_delay_thread", 256, 17);
	if (!lp_mqtt_keepalive->delay_thread) {
		LP_LOG_ERR("mqtt_delay_thread create error\n");
	}
	hal_thread_start(lp_mqtt_keepalive->delay_thread);
	return 0;
}

static struct pm_devops mqtt_lp_ops = {
	.suspend = mqtt_lp_suspend,
	.resume = mqtt_lp_resume,
};

static struct pm_device mqtt_lp_pm = {
	.name = "mqtt_lp",
	.ops = &mqtt_lp_ops,
};
#endif

int mqtt_low_power_keepalive_demo(void)
{
	int ret = -1;
	int keepalive_state = 0;
	bool clientSessionPresent = true, brokerSessionPresent = false;
	int returnStatus = EXIT_SUCCESS;

#if CONFIG_COMPONENTS_LOW_POWER_APP_MQTT_CAPACITY_READ
	capacity_read_init();
#endif

	if (lp_rpmsg_init()) {
		LP_LOG_INFO("lp_rpmsg_init failed\n");
		return -1;
	}

	lp_ctrl_msg_register(lp_mqtt_low_power_msg_proc);

	lp_mqtt_keepalive_init();

	while (!lp_mqtt_keepalive->mqtt_network_ok) {
		hal_msleep(100);
	}
	LP_LOG_INFO("Mqtt network is OK, go on now\n");

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&mqtt_lp_pm);
#endif

	/* The first attempt to connect to the MQTT server */
	returnStatus = init_mqtt_connect(&(lp_mqtt_keepalive->mqttContext), &(lp_mqtt_keepalive->networkContext));
	if (returnStatus == EXIT_SUCCESS) {
		returnStatus = connectToServer(&(lp_mqtt_keepalive->networkContext),
			&(lp_mqtt_keepalive->mqttContext),
			&clientSessionPresent,
			&brokerSessionPresent);
		if (returnStatus == EXIT_SUCCESS) {
			LP_LOG_INFO("MQTT connectToServer success\n");
			returnStatus = subscribeToTopic(&(lp_mqtt_keepalive->mqttContext));
			if (returnStatus == EXIT_SUCCESS) {
				LP_LOG_INFO("subscribeToTopic ok\n");
				returnStatus = publishToTopic(&(lp_mqtt_keepalive->mqttContext), "hello v821 online");
				if (returnStatus == EXIT_SUCCESS) {
					LP_LOG_INFO("publishToTopic ok\n");
					keepalive_state = 1;
					low_power_set_keepalive_use_strategy(lp_mqtt_keepalive->networkContext.Socket, 1, DEFAULLT_USE_STRATEGY);
				}
			}
		} else {
			lp_mqtt_keepalive->mqtt_network_error_flags = 1;
			LP_LOG_INFO("MQTT connectToServer error and reconnect later\n");
		}
	} else {
		return returnStatus;
	}

	lp_mqtt_keepalive->receive_pt = hal_thread_create(mqtt_receive_loop_thread,
		(void *)(&(lp_mqtt_keepalive->mqttContext)),
		"mqtt_receive_loop_thread",
		1024,
		17);
	if (!lp_mqtt_keepalive->receive_pt) {
		(void)disconnectMqttSession(&(lp_mqtt_keepalive->mqttContext), &(lp_mqtt_keepalive->networkContext));
		ret = -1;
		goto free_thread1;
	}
	hal_thread_start(lp_mqtt_keepalive->receive_pt);

	if (pm_task_register(lp_mqtt_keepalive->receive_pt, PM_TASK_TYPE_WLAN)) {
		LP_LOG_INFO("receive_pt pm_task_register error\n");
		ret = -1;
		goto free_thread1;
	}

	wlan_keepalive_wakeup_init(KEEPALIVE_INTERVAL_MS, NULL);

	if (keepalive_state == 1)
		lp_rpmsg_set_keepalive_state(1);

	LP_LOG_INFO("mqtt_low_power_keepalive_demo run\n");
	return 0;
free_thread1:
	hal_sem_delete(lp_mqtt_keepalive->reconnect_sem);
	hal_thread_stop(lp_mqtt_keepalive->receive_pt);
	LP_LOG_INFO("hal_thread_stop in %s:%d\n", __FUNCTION__, __LINE__);
	return ret;
}

int mqtt_low_power_keepalive_demo_deinit(void)
{
	wlan_keepalive_wakeup_deinit();
	lp_mqtt_keepalive->mqtt_receive_pt_exit = 1;
	lp_rpmsg_set_keepalive_state(0);
#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&mqtt_lp_pm);
#endif
	if (lp_mqtt_keepalive->receive_pt) {
		if (pm_task_unregister(lp_mqtt_keepalive->receive_pt)) {
			LP_LOG_INFO("receive_pt pm_task_unregister error\n");
		}
		while (!lp_mqtt_keepalive->mqtt_receive_pt_exit_ok) {
			hal_msleep(10);
		}
		lp_mqtt_keepalive->receive_pt = NULL;
	}

	if (lp_mqtt_keepalive->reconnect_sem != NULL) {
		hal_sem_delete(lp_mqtt_keepalive->reconnect_sem);
		lp_mqtt_keepalive->reconnect_sem = NULL;
		LP_LOG_INFO("semaphore deleted\n");
	}
	lp_rpmsg_deinit();
	memset(lp_mqtt_keepalive, 0, sizeof(*lp_mqtt_keepalive));
	LP_LOG_INFO("mqtt_low_power_keepalive_demo_deinit complete\n");
	return 0;
}
