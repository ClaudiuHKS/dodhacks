
#ifndef __MODULECONFIG_H__
#define __MODULECONFIG_H__

#define MODULE_NAME "DoD Hacks"
#define MODULE_VERSION "1.0.0.1"
#define MODULE_VERSION_MICROSOFT 1, 0, 0, 1
#define MODULE_AUTHOR "Hattrick HKS (claudiuhks)"
#define MODULE_URL "https://www.amxmodx.org/"
#define MODULE_LOGTAG "DODHACKS"
#define MODULE_LIBRARY "dodhacks"
#define MODULE_LIBCLASS ""
#define MODULE_DATE __DATE__

#define USE_METAMOD

#define FN_AMXX_CHECKGAME AmxxCheckGame
#define FN_AMXX_ATTACH OnAmxxAttach
#define FN_AMXX_DETACH OnAmxxDetach

#define FN_AMXX_PLUGINSLOADED OnPluginsLoaded

#ifdef USE_METAMOD

#define FN_DispatchKeyValue DispatchKeyValue
#define FN_ServerDeactivate_Post ServerDeactivate_Post

#endif

#endif
