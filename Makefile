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

.cpp.obj : .AUTODEPEND
	$(CXX) $(CXXFLAGS) -fo=$@ $[@

all : .SYMBOLIC kipcfg.exe

kipcfg.exe : kipcfg.obj daemon.obj dhcp_socks.obj dhcpc.obj &
             dhcp_options.obj ifconfig.obj router.obj log.obj
	$(LINK) $(LFLAGS) system os2v2 name $@ file { $< }

dist : .SYMBOLIC
	$(MAKE) clean
	$(MAKE) bin RELEASE=1 VER=$(VER)
	$(MAKE) src RELEASE=1
	$(ZIP) kipcfg$(VER) kipcfgsrc.zip
	-$(DEL) kipcfgsrc.zip

bin : .SYMBOLIC &
      kipcfg.exe readme.txt readme.eng
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

