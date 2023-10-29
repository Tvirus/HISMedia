
include ./configs/hismedia_config_$(PRODUCT).mk


INC :=
SRC :=
LIB :=
CFLAGS := -Wall


INC += -I../../include
INC += -I../
INC += -I./
SRC += ../hismedia_cache.c
SRC += ./hismedia.c

LIB += -lmi_sys
LIB += -lcam_os_wrapper
LIB += -lcam_fs_wrapper
LIB += -lmi_rgn
LIB += -lmi_sensor
LIB += -lmi_vif
LIB += -lmi_isp
LIB += -lcus3a
LIB += -lispalgo
LIB += -lmi_vpe
LIB += -lmi_scl
LIB += -lmi_venc
LIB += -lmi_vdf
LIB += -lOD_LINUX
LIB += -lMD_LINUX
LIB += -lVG_LINUX
LIB += -lmi_shadow
LIB += -lmi_ive
LIB += -lmi_ai
LIB += -lmi_common
LIB += -lmi_iqserver
LIB += -lpthread


.PHONY: all
all:
	cp -f ./configs/hismedia_config_$(PRODUCT).h ./hismedia_config.h
	$(CROSS_COMPILE)gcc -shared -fPIC -o libhismedia.so $(CFLAGS) $(INC) $(SRC) $(LIB)
	mkdir -p ../../lib/; cp -f ./libhismedia.so ../../lib/