GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
ifeq ($(GCC_VERSION),0404)
CFLAGS_REMOVE_msm_vfe8x.o = -Wframe-larger-than=1024
endif

obj-$(CONFIG_MSM_CAMERA) += msm_camera.o msm_v4l2.o msm_axi_qos.o
obj-$(CONFIG_MSM_CAMERA_FLASH) += flash.o
obj-$(CONFIG_ARCH_MSM_ARM11) += msm_vfe7x.o msm_io7x.o
obj-$(CONFIG_ARCH_MSM7X30) += msm_vfe31.o msm_io_vfe31.o
obj-$(CONFIG_ARCH_QSD8X50) += msm_vfe8x.o msm_vfe8x_proc.o msm_io8x.o
obj-$(CONFIG_ARCH_MSM8X60) += msm_vfe31.o msm_io_8x60.o
obj-$(CONFIG_MT9T013) += mt9t013.o mt9t013_reg.o
obj-$(CONFIG_MT9D112) += mt9d112.o mt9d112_reg.o
obj-$(CONFIG_SN12M0PZ) += sn12m0pz.o sn12m0pz_reg.o
obj-$(CONFIG_HUAWEI_SENSOR_OV7690) += ov7690.o
obj-$(CONFIG_HUAWEI_SENSOR_HIMAX0356) += himax0356.o
obj-$(CONFIG_HUAWEI_SENSOR_S5K4E1GX) += s5k4e1gx.o
obj-$(CONFIG_HUAWEI_SENSOR_OV5647_SUNNY) += ov5647_sunny.o
obj-$(CONFIG_HUAWEI_FEATURE_RIGHT_TPA2028D1_AMPLIFIER)	+= tpa2028d1_r.o
obj-$(CONFIG_MT9P012) += mt9p012_reg.o
obj-$(CONFIG_MSM_CAMERA_AF_FOXCONN) += mt9p012_fox.o
obj-$(CONFIG_MSM_CAMERA_AF_BAM) += mt9p012_bam.o
obj-$(CONFIG_MT9P012_KM) += mt9p012_km.o mt9p012_km_reg.o
obj-$(CONFIG_S5K3E2FX) += s5k3e2fx.o
obj-$(CONFIG_VX6953) += vx6953.o vx6953_reg.o
obj-$(CONFIG_VB6801) += vb6801.o
obj-$(CONFIG_IMX074) += imx074.o imx074_reg.o
obj-$(CONFIG_MSM_GEMINI) += msm_gemini_dev.o msm_gemini_sync.o msm_gemini_core.o msm_gemini_hw.o msm_gemini_platform.o
