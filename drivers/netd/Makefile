SW_ROOT_DIR=../..
include $(SW_ROOT_DIR)/make.inc

BUILD_DIR = $(SW_ROOT_DIR)/build/$(HW)

NETD_OBJS = netd.o intr.o sched.o tap.o task.o\
				stack/loopback.o	\
				stack/icmp.o \
				stack/net.o \
				stack/ether.o \
				stack/arp.o \
				stack/ip.o \
				stack/udp.o \
				stack/tcp.o  \
				stack/util.o \
				stack/sock.o \
				stack/dhcp.o 

NETD = $(BUILD_DIR)/drivers/netd

$(NETD): $(NETD_OBJS) \
		$(EWOK_LIBC_A) 
	mkdir -p $(BUILD_DIR)/drivers
	$(LD) -Ttext=100 $(NETD_OBJS) -o $(NETD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(NETD_OBJS)
