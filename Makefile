.ERASE

.SUFFIXES :
.SUFFIXES : .exe .obj .h .c

CC = wcc386
CFLAGS = -zq -wx -we -bm

LINK = wlink
LFLAGS = option quiet, map

!ifdef RELEASE
#The optimization of Open Watcom v1.8 is buggy. Do not use it.
CFLAGS += -d0
!else
CFLAGS += -d2 -DDEBUG
LFLAGS += debug watcom all
!endif

ZIP = zip

DEL = del

.c.obj :
	$(CC) $(CFLAGS) -fo=$@ $[@

all : .SYMBOLIC kipcfg.exe

kipcfg.exe : kipcfg.obj daemon.obj dhcp_socks.obj dhcpc.obj dhcp_options.obj &
             ifconfig.obj router.obj log.obj
	$(LINK) $(LFLAGS) system os2v2 name $@ file { $< }

kipcfg.obj : kipcfg.c daemon.h

daemon.obj : daemon.c dhcp.h dhcpc.h dhcp_socks.h dhcp_options.h ifconfig.h &
             router.h log.h daemon.h

dhcp_socks.obj : dhcp_socks.c ifconfig.h log.h dhcp_socks.h

dhcpc.obj : dhcpc.c daemon.h dhcp.h dhcp_options.h dhcp_socks.h log.h dhcpc.h

dhcp_options.o: dhcp_options.c dhcp.h log.h dhcp_options.h

ifconfig.obj : ifconfig.c router.h log.h ifconfig.h

router.obj : router.c log.h router.h

log.obj : log.c log.h

dist : .SYMBOLIC
	$(MAKE) clean
	$(MAKE) bin RELEASE=1 VER=$(VER)
	$(MAKE) src RELEASE=1
	$(ZIP) kipcfg$(VER) kipcfgsrc.zip
	-$(DEL) kipcfgsrc.zip

bin : .SYMBOLIC kipcfg.exe readme.txt readme.eng
	-$(DEL) kipcfg$(VER).zip
	$(ZIP) kipcfg$(VER) $<

src : .SYMBOLIC kipcfg.c dhcp.h daemon.c daemon.h dhcp_socks.c dhcp_socks.h &
      dhcpc.c dhcpc.h dhcp_options.c dhcp_options.h ifconfig.c ifconfig.h &
      router.c router.h log.h Makefile COPYING
	-$(DEL) kipcfgsrc.zip
	$(ZIP) kipcfgsrc $<

clean : .SYMBOLIC
	-$(DEL) *.map
	-$(DEL) *.obj
	-$(DEL) *.exe
	-$(DEL) *.zip
	-$(DEL) *~?

