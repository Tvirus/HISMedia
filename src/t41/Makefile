
include ./configs/hismedia_config_$(PRODUCT).mk


INC :=
SRC :=
LIB :=
CFLAGS := -Wall


ifeq ($(FEATURE_ALGO_MOVE), y)
CFLAGS += -DALGO_MOVE
LIBMXU := y
endif
ifeq ($(FEATURE_ALGO_PERSONVEHICLEPET), y)
CFLAGS += -DALGO_PERSONVEHICLEPET
LIBMXU := y
endif


INC += -I../../include
INC += -I../
INC += -I./
SRC += ../hismedia_cache.c
SRC += ./hismedia.c

LIB += -limp
LIB += -lalog
LIB += -lpthread
LIB += -lrt
LIB += -lm
LIB += -lstdc++
ifeq ($(FEATURE_ALGO_MOVE), y)
LIB += -lmove_inf
endif
ifeq ($(FEATURE_ALGO_PERSONVEHICLEPET), y)
LIB += -lpersonvehiclepetDet_inf
LIB += -ldrivers
LIB += -laip
LIB += -lvenus
endif
ifeq ($(LIBMXU), y)
LIB += -lmxu_contrib
LIB += -lmxu_merge
LIB += -lmxu_video
LIB += -lmxu_imgproc
LIB += -lmxu_core
endif


.PHONY: all
all:
	cp -f ./configs/hismedia_config_$(PRODUCT).h ./hismedia_config.h
	$(CROSS_COMPILE)gcc -shared -fPIC -o libhismedia.so $(CFLAGS) $(INC) $(SRC) $(LIB)
	mkdir -p ../../lib/; cp -f ./libhismedia.so ../../lib/