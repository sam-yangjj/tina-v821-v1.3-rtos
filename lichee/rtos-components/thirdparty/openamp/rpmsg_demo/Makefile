CFLAGS += -I $(BASE)/components/common/thirdparty/openamp/include
CFLAGS += -I components/common/thirdparty/md5
CFLAGS += -I $(BASE)/components/common/aw/pm/include

obj-$(CONFIG_RPMSG_DEMO)      += demo.o
obj-$(CONFIG_RPMSG_NOTIFY)      += notify.o
obj-$(CONFIG_RPMSG_SPEEDTEST) += speed.o
obj-$(CONFIG_RPMSG_HEARBEAT) += hearbeat.o
obj-$(CONFIG_RPMSG_CLIENT) += rpmsg_ctrl/rpmsg_master.o
obj-$(CONFIG_RPMSG_CLIENT) += rpmsg_ctrl/rpmsg_client.o
obj-$(CONFIG_RPMSG_EPT_TEST_CMD) += rpmsg_ctrl/test_cmd.o
obj-$(CONFIG_RPMSG_CLIENT_TEST) += rpmsg_ctrl/test/
obj-$(CONFIG_RPMSG_TEST) += rpmsg_test.o
obj-$(CONFIG_RPMSG_TEST) += rpmsg_test_extend.o

