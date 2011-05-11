obj-$(CONFIG_DIAG_CHAR) := diagchar.o
diagchar-objs := diagchar_core.o diagchar_hdlc.o diagfwd.o diagmem.o
ifeq ($(CONFIG_HUAWEI_FEATURE_PHUDIAG),y)
diagchar-objs += phudiagchar_core.o phudiagfwd.o
endif
