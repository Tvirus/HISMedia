
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

LIB += -lss_mpi_aibnr
LIB += -lsvp_acl
LIB += -lprotobuf-c
LIB += -lss_mpi_isp
LIB += -lot_mpi_isp
LIB += -lss_mpi_ae
LIB += -lss_mpi_awb
LIB += -lextend_stats
LIB += -ldrc
LIB += -lldci
LIB += -lir_auto
LIB += -lcalcflicker
LIB += -lacs
LIB += -ldehaze
LIB += -lbnr
LIB += -lsns_os04a10
LIB += -lss_mpi_audio
LIB += -lvoice_engine
LIB += -lupvqe
LIB += -ldnvqe

LIB += -lss_mpi_sysbind
LIB += -lss_mpi_sysmem
LIB += -lss_mpi
LIB += -lsecurec

LIB += -lstdc++


.PHONY: all
all:
	cp -f ./configs/hismedia_config_$(PRODUCT).h ./hismedia_config.h
	$(CROSS_COMPILE)gcc -shared -fPIC -o libhismedia.so $(CFLAGS) $(INC) $(SRC) $(LIB)
	mkdir -p ../../lib/; cp -f ./libhismedia.so ../../lib/