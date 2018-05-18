#
# PF_RING
#
PFRINGDIR  = ./PF_RING/userland/lib
LIBPFRING  = ${PFRINGDIR}/libpfring.a

#
# PF_RING aware libpcap
#
O_FLAG     = -O2 -DHAVE_PF_RING -g -g -rdynamic
EXTRA_LIBS =
PCAPDIR    = ./PF_RING/userland/libpcap
LIBPCAP    = ${PCAPDIR}/libpcap.a  ${EXTRA_LIBS}

#
# Search directories
#
PFRING_KERNEL=./PF_RING/kernel
INCLUDE    = -I${PFRING_KERNEL} -I${PFRINGDIR} -I${PCAPDIR} -Ithird-party `./PF_RING/userland/lib/pfring_config --include`

#
# C compiler and flags
#
#
# CROSS_COMPILE=arm-mv5sft-linux-gnueabi-
#
CC         = ${CROSS_COMPILE}colorgcc #--platform=native
CFLAGS     =  ${O_FLAG} -Wall ${INCLUDE} -D ENABLE_BPF   -O2  -g -fno-builtin -w -I./inc/ -g -rdynamic
# LDFLAGS  =

#
# User and System libraries
#
LIBS       = ${LIBPCAP} ${LIBPFRING} ${LIBPCAP} ${LIBPFRING} `./PF_RING/userland/lib/pfring_config --libs` -lpthread  -lrt -ldl  -lrt ./argparse/libargparse.a -lm
# `./PF_RING/userland/libpcap/pcap-config --additional-libs --static` -lpthread  -lrt -ldl  -lrt

# How to make an object file
%.o: %.c pfutils.c
#       @echo "=*= making object $@ =*="
	${CC} ${CFLAGS} -c $< -o $@

#
# Main targets
#
PFPROGS   = mux mux_server

PCAPPROGS =
TARGETS   = ${PFPROGS} ${PCAPPROGS}

RCOBJS    = ringc.o interval.o
RSOBJS    = rings.o interval.o
RMOBJS    = ringm.o interval.o
RPOBJS    = pcaps.o interval.o

all: ${TARGETS}

MUX_CLIENT_OBJECTS = clog.o mux_climain.o    mux_ifmanager.o mux_ifreaders.o  \
                     mux_util.o mux_cfgutil.o mux_br_fdb.o timers.o           \
		     mux_ifwriters.o mux_ifcfg_manual.o mux_ether_arpdb.o     \
		     mux_ethernet.o mux_ipconf.o mux_ip.o mux_discovery.o     \
		     mux_muxtun.o mux_gwdiscovery.o mux_proto.o               \
		     mux_tr_raw.o

MUX_SERVER_OBJECTS = clog.o mux_server.o  mux_ifmanager.o mux_ifreaders.o     \
                     mux_util.o mux_cfgutil.o mux_br_fdb.o timers.o           \
                     mux_ifwriters.o mux_ifcfg_manual.o mux_ether_arpdb.o     \
		     mux_ethernet.o mux_ipconf.o mux_ip.o mux_discovery.o     \
		     mux_muxtun.o mux_gwdiscovery.o mux_proto.o               \
		     mux_tr_raw.o


mux: ${MUX_CLIENT_OBJECTS} ${LIBPFRING}
	${CC} ${CFLAGS} ${MUX_CLIENT_OBJECTS} ${LIBS} -o $@

mux_server: ${MUX_SERVER_OBJECTS} ${LIBPFRING}
	${CC} ${CFLAGS} ${MUX_SERVER_OBJECTS} ${LIBS}  -o $@


clean:
	@rm -f ${TARGETS} *.o *~

