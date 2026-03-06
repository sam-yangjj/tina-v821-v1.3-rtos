obj-y += rpc.o
obj-$(CONFIG_COMPONENTS_AW_RPC_SERVER) += rpc_servers.o threadpool.o
obj-$(CONFIG_COMPONENTS_AW_RPC_CLIENT) += mempool.o
obj-y += find_bit.o bitmap.o
ifeq ($(CONFIG_COMPONENTS_AW_RPC_TEST_CMD),y)
ifeq ($(CONFIG_COMPONENTS_AW_RPC_SERVER),y)
obj-y += test_service.o
endif
ifeq ($(CONFIG_COMPONENTS_AW_RPC_CLIENT),y)
obj-y += test_client.o
endif
endif

subdir-ccflags-y += -I components/common/aw/sunxi-rpc/include
subdir-ccflags-y += -Wno-address-of-packed-member
