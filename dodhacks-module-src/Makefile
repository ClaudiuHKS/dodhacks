CMD += gcc
OUT += dodhacks_amxx_i386.so

SRC += linux_compat.c
SRC += moduleconfig.cpp
SRC += Memory.cpp
SRC += amxxmodule.cpp
SRC += /mnt/d/scripting/subhook-master/subhook.c

DEF += -D__fastcall=/**__fastcall*/
DEF += -D__thiscall=/**__thiscall*/
DEF += -D_COMPILER_GCC
DEF += -D_FASTCALL_PARAM=/**void*,*/
DEF += -D_GCC
DEF += -D_GEOIP_STATIC
# DEF += -D_GLIBCXX_USE_CXX11_ABI=0
DEF += -D_GNU
DEF += -D_GNU_C
DEF += -D_GNU_SOURCE
DEF += -D_GNUC
DEF += -D_HAS_EXCEPTIONS=0
DEF += -D_HAVE_STDINT_H
DEF += -D_LINUX
DEF += -D_METAMOD_BUILD
DEF += -D_NDEBUG
DEF += -D_POSIX
DEF += -D_POSIX_SOURCE
DEF += -D_SOURCEMOD_BUILD
DEF += -D_SUBHOOK_STATIC
DEF += -D_USE_MATH_DEFINES
DEF += -D_stricmp=strcasecmp
DEF += -D_strnicmp=strncasecmp
DEF += -D_strtok_s=strtok_r

DEF += -D_fastcall=/**_fastcall*/
DEF += -D_thiscall=/**_thiscall*/
DEF += -DCOMPILER_GCC
DEF += -DFASTCALL_PARAM=/**void*,*/
DEF += -DGCC
DEF += -DGEOIP_STATIC
# DEF += -DGLIBCXX_USE_CXX11_ABI=0
DEF += -DGNU
DEF += -DGNU_C
DEF += -DGNU_SOURCE
DEF += -DGNUC
DEF += -DHAS_EXCEPTIONS=0
DEF += -DHAVE_STDINT_H
DEF += -DLINUX
DEF += -DMETAMOD_BUILD
DEF += -DNDEBUG
DEF += -DPOSIX
DEF += -DPOSIX_SOURCE
DEF += -DSOURCEMOD_BUILD
DEF += -DSUBHOOK_STATIC
DEF += -DUSE_MATH_DEFINES
DEF += -Dstricmp=strcasecmp
DEF += -Dstrnicmp=strncasecmp
DEF += -Dstrtok_s=strtok_r

HDR += -I.
HDR += -I./
HDR += -I..
HDR += -I../
HDR += -I/mnt/d/scripting/hlsdk-master/common
HDR += -I/mnt/d/scripting/hlsdk-master/dlls
HDR += -I/mnt/d/scripting/hlsdk-master/engine
HDR += -I/mnt/d/scripting/hlsdk-master/pm_shared
HDR += -I/mnt/d/scripting/hlsdk-master/public
HDR += -I/mnt/d/scripting/amxmodx-master/public
HDR += -I/mnt/d/scripting/amxmodx-master/public/sdk
HDR += -I/mnt/d/scripting/subhook-master
HDR += -I/mnt/d/scripting/metamod-hl1-master/metamod
HDR += -I/mnt/d/scripting/metamod-source-master/core/sourcehook

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
# By temporary disabling -s, you can see GLIB versions with the following command: strings binaryname.so | grep GLIB
OPT += -s
OPT += -shared
OPT += -static-libgcc
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
# OPT += -Wl,--wrap=fstat

default:

	rm -f "$(CURDIR)/*.so" "$(CURDIR)/*.0" "$(CURDIR)/*.o" "$(CURDIR)/*.log"

ifneq ($(OFL),)
	cp $(OFL) "$(CURDIR)"
endif

ifneq ($(LIB),)
	$(CMD) $(SRC) $(LIB) $(DEF) $(HDR) $(OPT) -o $(OUT)
else
	$(CMD) $(SRC) $(DEF) $(HDR) $(OPT) -o $(OUT)
endif
