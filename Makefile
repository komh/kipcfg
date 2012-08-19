.ERASE

.SUFFIXES :
.SUFFIXES : .exe .obj .h .cpp

CXX = wpp386
CXXFLAGS = -zq -wx -we -bm

LINK = wlink
LFLAGS = option quiet, map

!ifdef RELEASE
#The optimization of Open Watcom v1.8 is buggy. Do not use it.
CXXFLAGS += -d0
!else
CXXFLAGS += -d2 -DDEBUG
LFLAGS += debug watcom all
!endif

ZIP = zip

DEL = del

.cpp.obj :
	$(CXX) $(CXXFLAGS) -fo=$@ $[@

all : .SYMBOLIC kipcfg.exe

kipcfg.exe : kipcfg.obj daemon.obj dhcp_socks.obj dhcpc.obj &
             dhcp_options.obj ifconfig.obj router.obj log.obj
	$(LINK) $(LFLAGS) system os2v2 name $@ file { $< }

kipcfg.obj : kipcfg.cpp daemon.h

daemon.obj : daemon.cpp dhcp.h dhcpc.h dhcp_socks.h dhcp_options.h &
             ifconfig.h router.h log.h daemon.h

dhcp_socks.obj : dhcp_socks.cpp ifconfig.h log.h dhcp_socks.h

dhcpc.obj : dhcpc.cpp daemon.h dhcp.h dhcp_options.h dhcp_socks.h log.h &
            dhcpc.h

dhcp_options.obj: dhcp_options.cpp dhcp.h log.h dhcp_options.h

ifconfig.obj : ifconfig.cpp router.h log.h ifconfig.h

router.obj : router.cpp log.h router.h

log.obj : log.cpp log.h

dist : .SYMBOLIC
	$(MAKE) clean
	$(MAKE) bin RELEASE=1 VER=$(VER)
	$(MAKE) src RELEASE=1
	$(ZIP) kipcfg$(VER) kipcfgsrc.zip
	-$(DEL) kipcfgsrc.zip

bin : .SYMBOLIC kipcfg.exe readme.txt readme.eng
	-$(DEL) kipcfg$(VER).zip
	$(ZIP) kipcfg$(VER) $<

src : .SYMBOLIC &
      kipcfg.cpp dhcp.h daemon.cpp daemon.h dhcp_socks.cpp dhcp_socks.h &
      dhcpc.cpp dhcpc.h dhcp_options.cpp dhcp_options.h ifconfig.cpp &
      ifconfig.h router.cpp router.h log.cpp log.h Makefile COPYING
	-$(DEL) kipcfgsrc.zip
	$(ZIP) kipcfgsrc $<

clean : .SYMBOLIC
	-$(DEL) *.map
	-$(DEL) *.obj
	-$(DEL) *.exe
	-$(DEL) *.zip
	-$(DEL) *~?

