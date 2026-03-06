ifeq ($(CONFIG_KERNEL_FREERTOS), y)
subdir-ccflags-y += -I components/common/thirdparty/openamp/include
subdir-ccflags-y += -I components/common/aw/rpbuf/include
subdir-ccflags-y += -I components/common/thirdparty/rtt_memheap
endif

obj-y += modules/sensor/
obj-y += modules/sensor/sensor_register.o
ifeq ($(CONFIG_ARCH_SUN55IW3), y)
obj-y += platform/vin_config_sun55iw3.o
else ifeq ($(CONFIG_ARCH_SUN300IW1), y)
obj-y += platform/vin_config_sun300iw1.o
else ifeq ($(CONFIG_ARCH_SUN8IW20), y)
obj-y += platform/vin_config_sun8iw20p1.o
else
obj-y += platform/vin_config_sun8iw21p1.o
endif
obj-y += utility/config.o
obj-y += utility/vin_supply.o
obj-y += vin_cci/cci_helper.o
ifeq ($(CONFIG_ARCH_SUN8IW20), y)
obj-y += vin_mipi/combo_csi/combo_csi_reg_null.o
else
obj-y += vin_mipi/combo_csi/combo_csi_reg.o
endif
obj-y += vin_mipi/sunxi_mipi.o
obj-y += vin_csi/parser_reg.o
obj-y += vin_csi/sunxi_csi.o
obj-y += vin_tdm/tdm200/tdm200_reg.o
obj-y += vin_tdm/vin_tdm.o
ifeq ($(CONFIG_ARCH_SUN20IW3), y)
obj-y += vin_isp/isp600/isp600_reg_cfg.o
else ifeq ($(CONFIG_ARCH_SUN55IW3), y)
obj-y += vin_isp/isp600/isp600_reg_cfg.o
else ifeq ($(CONFIG_ARCH_SUN300IW1), y)
obj-y += vin_isp/isp600/isp600_reg_cfg.o
endif
ifneq ($(CONFIG_ARCH_SUN8IW20), y)
ifneq ($(CONFIG_VIDEO_SUNXI_VIN_SPECIAL), y)
obj-y += vin_isp/sunxi_isp.o
obj-y += vin_stat/vin_h3a.o
endif
endif
obj-y += vin_vipp/vipp200/vipp200_reg.o
ifneq ($(CONFIG_VIDEO_SUNXI_VIN_SPECIAL), y)
obj-y += vin_vipp/sunxi_scaler.o
endif
ifneq ($(CONFIG_ARCH_SUN8IW20), y)
obj-y += vin_video/dma140/dma140_reg.o
else
obj-y += vin_video/dma130/dma_reg.o
endif
obj-y += vin_video/vin_core.o
obj-y += vin_video/vin_video.o
obj-y += top_reg.o
obj-y += vin.o

obj-$(CONFIG_VIDEO_SUNXI_VIN_SPECIAL) += vin_rpmsg/vin_rpmsg.o

ifneq ($(CONFIG_ARCH_SUN8IW20), y)
ifneq ($(CONFIG_VIDEO_SUNXI_VIN_SPECIAL), y)
obj-y += vin_isp/vin_link_server.o
obj-y += vin_isp/isp_server/isp_server.o
obj-y += vin_isp/isp_server/isp_math/matrix.o
obj-y += vin_isp/isp_server/isp_math/isp_math_util.o
obj-y += vin_isp/isp_server/isp_cfg/isp_ini_parse.o
obj-y += vin_isp/isp_server/isp_tuning/isp_tuning.o
obj-y += vin_isp/isp_server/isp_manage/isp_helper.o
obj-y += vin_isp/isp_server/isp_manage/isp_manage.o
endif
endif

#obj-y += vin_isp/isp_server/isp_algo/
