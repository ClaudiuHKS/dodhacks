
#ifndef __MODULECONFIG_H__
#define __MODULECONFIG_H__

#define MODULE_NAME         "DoD Hacks"
#define MODULE_VERSION      "1.0.0.3"
#define MODULE_VERSION_MS   1,0,0,3
#define MODULE_YEAR_MS      "2026"
#define MODULE_AUTHOR       "Hattrick HKS (claudiuhks)"
#define MODULE_URL          "https://forums.alliedmods.net/"
#define MODULE_COMPANY      "AlliedModders LLC"
#define MODULE_LOGTAG       "DODHACKS"
#define MODULE_LIBRARY      "dodhacks"
#define MODULE_LIBCLASS     ""
#define MODULE_DATE         __DATE__

/// #define MODULE_RELOAD_ON_MAPCHANGE

#define USE_METAMOD

/// #define FN_AMXX_QUERY       OnAmxxQuery
/// #define FN_AMXX_CHECKGAME   AmxxCheckGame
#define FN_AMXX_ATTACH          OnAmxxAttach
#define FN_AMXX_DETACH          OnAmxxDetach

#define FN_AMXX_PLUGINSLOADED           OnPluginsLoaded
/// #define FN_AMXX_PLUGINSUNLOADING    OnPluginsUnloading
#define FN_AMXX_PLUGINSUNLOADED         OnPluginsUnloaded

#ifdef USE_METAMOD

/// #define FN_META_QUERY   OnMetaQuery
/// #define FN_META_ATTACH  OnMetaAttach
/// #define FN_META_DETACH  OnMetaDetach

#define FN_DispatchKeyValue DispatchKeyValue
#define FN_ServerActivate   ServerActivate

#endif

#endif
