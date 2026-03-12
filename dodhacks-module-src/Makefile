
# Linux stuff below. For Windows, it's better to use Visual Studio.
#
# To get Linux on Windows, use a software like VMWare Workstation or
# VMLite Workstation and a Linux ISO file, like Debian.
# To get Linux on Windows, you can also enable WSL (Windows Subsystem
# for Linux) and then install Debian from Windows Store application.
#
# To setup Debian for C/ C++ compilations, run a command in Terminal
# app like "sudo apt-get update -y ; sudo apt-get upgrade -y ; sudo
# apt-get install g++ -y ; sudo apt-get install g++-multilib -y ; sudo
# apt-get install make -y ; sudo apt-get install patchelf -y".
#
# If no access to update the system, type "su" and hit ENTER.
# Type "exit" and hit ENTER to leave "root" access once updated.

CMD ?= gcc
OUT ?= dodhacks_amxx_i386.so

# If building outside of GitHub, you need to manually download these
# dependencies (click on "< > Code" button then select Download ZIP).
#
# https://github.com/alliedmodders/hlsdk
# https://github.com/alliedmodders/metamod-hl1
# https://github.com/alliedmodders/amxmodx
# https://github.com/ClaudiuHKS/subhook

# Root paths for external dependencies below.
#
# If specified file/ folder doesn't exist, define non-local paths.
ifeq ($(wildcard hlsdk),)
HLSDK_ROOT ?= /mnt/d/scripting/hlsdk-master
HL1MM_ROOT ?= /mnt/d/scripting/metamod-hl1-master
AMXX_ROOT  ?= /mnt/d/scripting/amxmodx-master
SHOOK_ROOT ?= /mnt/d/scripting/subhook-master
else
HLSDK_ROOT ?= ./hlsdk
HL1MM_ROOT ?= ./metamod-hl1
AMXX_ROOT  ?= ./amxmodx
SHOOK_ROOT ?= ./subhook
endif

SRC += linux_compat.c
SRC += dodhacks.cpp
SRC += Memory.cpp
SRC += $(AMXX_ROOT)/public/sdk/amxxmodule.cpp
SRC += $(SHOOK_ROOT)/subhook.c

DEF += -D__fastcall=/**__fastcall*/
DEF += -D__thiscall=/**__thiscall*/
DEF += -D_COMPILER_GCC
DEF += -D_FASTCALL_PARAM=/**void*,*/
DEF += -D_FASTCALL_PARAM_ALONE=/**,void**/
DEF += -D_GCC
# DEF += -D_GEOIP_STATIC
# DEF += -D_GLIBCXX_USE_CXX11_ABI=0
DEF += -D_GNU
DEF += -D_GNU_C
DEF += -D_GNU_SOURCE
DEF += -D_GNUC
DEF += -D_HAS_EXCEPTIONS=0
DEF += -D_HAVE_STDINT_H
DEF += -D_LINUX
# DEF += -D_METAMOD_BUILD
DEF += -D_NDEBUG
DEF += -D_POSIX
DEF += -D_POSIX_SOURCE
# DEF += -D_SOURCEMOD_BUILD
DEF += -D_SUBHOOK_STATIC
# DEF += -D_USE_MATH_DEFINES
DEF += -D_snprintf=snprintf
DEF += -D_stricmp=strcasecmp
DEF += -D_stristr=strcasestr
DEF += -D_strnicmp=strncasecmp
DEF += -D_strtok_s=strtok_r
DEF += -D_StrStrIA=strcasestr

DEF += -D_fastcall=/**_fastcall*/
DEF += -D_thiscall=/**_thiscall*/
DEF += -DCOMPILER_GCC
DEF += -DFASTCALL_PARAM=/**void*,*/
DEF += -DFASTCALL_PARAM_ALONE=/**,void**/
DEF += -DGCC
# DEF += -DGEOIP_STATIC
# DEF += -DGLIBCXX_USE_CXX11_ABI=0
DEF += -DGNU
DEF += -DGNU_C
DEF += -DGNU_SOURCE
DEF += -DGNUC
DEF += -DHAS_EXCEPTIONS=0
DEF += -DHAVE_STDINT_H
DEF += -DLINUX
# DEF += -DMETAMOD_BUILD
DEF += -DNDEBUG
DEF += -DPOSIX
DEF += -DPOSIX_SOURCE
# DEF += -DSOURCEMOD_BUILD
DEF += -DSUBHOOK_STATIC
# DEF += -DUSE_MATH_DEFINES
DEF += -Dsnprintf=snprintf
DEF += -Dstricmp=strcasecmp
DEF += -Dstristr=strcasestr
DEF += -Dstrnicmp=strncasecmp
DEF += -Dstrtok_s=strtok_r
DEF += -DStrStrIA=strcasestr

HDR += -I.
HDR += -I./
HDR += -I..
HDR += -I../
HDR += -I$(HLSDK_ROOT)/common
HDR += -I$(HLSDK_ROOT)/dlls
HDR += -I$(HLSDK_ROOT)/engine
HDR += -I$(HLSDK_ROOT)/pm_shared
HDR += -I$(HLSDK_ROOT)/public
HDR += -I$(HL1MM_ROOT)/metamod
HDR += -I$(AMXX_ROOT)/public
HDR += -I$(AMXX_ROOT)/public/sdk
HDR += -I$(SHOOK_ROOT)

OPT += -fno-exceptions
OPT += -fno-rtti
OPT += -fno-stack-protector
OPT += -fno-strict-aliasing
OPT += -fno-threadsafe-statics
OPT += -fvisibility-inlines-hidden
OPT += -fvisibility=hidden
OPT += -fpermissive
OPT += -ldl
OPT += -lm
OPT += -lpthread
OPT += -lstdc++
OPT += -m32
OPT += -mfpmath=sse
OPT += -msse
OPT += -o3
OPT += -pipe
OPT += -pthread
#
# By temporary disabling -s, you can see GCC versions (of each function) with the following commands:
#
# strings binaryname.so | grep GLI
# strings binaryname.so | grep ABI
# strings binaryname.so | grep CXX
#
OPT += -s
OPT += -shared
# OPT += -static
OPT += -static-libgcc
OPT += -static-libstdc++
OPT += -std=c++11
# OPT += -std=c++14
# OPT += -std=c++17
# OPT += -std=c++20
# OPT += -std=c99
OPT += -std=c11
# OPT += -std=c17
OPT += -w
OPT += -Wl,--wrap=stat
OPT += -Wl,--wrap=dlopen
OPT += -Wl,--wrap=dlclose
OPT += -Wl,--wrap=dlsym
OPT += -Wl,--wrap=dladdr
OPT += -Wl,--wrap=__isoc23_strtoull
OPT += -Wl,--wrap=__isoc23_sscanf
# OPT += -Wl,--wrap=fstat

default:

	rm -r -f "$(CURDIR)/$(OUT)" "$(CURDIR)/*.so" "$(CURDIR)/*.0" "$(CURDIR)/*.o" "$(CURDIR)/*.log" "$(CURDIR)/Linux-Release" "$(CURDIR)/Linux-Release-AVX2"
	rm -f    "$(CURDIR)/$(OUT)" "$(CURDIR)/*.so" "$(CURDIR)/*.0" "$(CURDIR)/*.o" "$(CURDIR)/*.log"

ifneq ($(OFL),)
	cp $(OFL) "$(CURDIR)"
endif

ifneq ($(LIB),)
	mkdir "$(CURDIR)/Linux-Release"      ; $(CMD) $(SRC) $(LIB) $(DEF) $(HDR) $(OPT)        -o "$(CURDIR)/Linux-Release/$(OUT)"
	mkdir "$(CURDIR)/Linux-Release-AVX2" ; $(CMD) $(SRC) $(LIB) $(DEF) $(HDR) $(OPT) -mavx2 -o "$(CURDIR)/Linux-Release-AVX2/$(OUT)"
else
	mkdir "$(CURDIR)/Linux-Release"      ; $(CMD) $(SRC)        $(DEF) $(HDR) $(OPT)        -o "$(CURDIR)/Linux-Release/$(OUT)"
	mkdir "$(CURDIR)/Linux-Release-AVX2" ; $(CMD) $(SRC)        $(DEF) $(HDR) $(OPT) -mavx2 -o "$(CURDIR)/Linux-Release-AVX2/$(OUT)"
endif
