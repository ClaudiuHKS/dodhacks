
#include "amxxmodule.h"

#ifndef __linux__
#include <detours.h>
#else
#include <subhook.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <link.h>
#endif

#include <entity_state.h>
#include <Memory.h>

#define F_EToI(E) ((::size_t)       (E - ::g_pEntities))
#define F_IToE(I) ((::edict_s *)    (::g_pEntities + I))

enum DoD_Sig : unsigned char
{
    PlayerSpawn = false,
    GiveNamedItem,
    DropPlayerItem,
    GiveAmmo,
    SetWaveTime,
    GetWaveTime,
    RemovePlayerItem,
    AddPlayerItem,
    RemoveAllItems,
    SetBodygroup,
    DestroyItem,
    SubRemove,
    PackWeapon,
    WpnBoxKill,
    WpnBoxActivateThink,
    Create,
    InstallGameRules,
    UtilRemove,
    CreateNamedEntity,

    PatchFg42,
    PatchEnfield,

    OrigFg42_Byte,
    PatchFg42_Byte,

    OrigEnfield_Byte,
    PatchEnfield_Byte,

    Offs_AlliesAreBrit,
    Offs_AlliesArePara,
    Offs_AxisArePara,
    Offs_AlliesRespawnFactor,
    Offs_AxisRespawnFactor,
    Offs_AlliesInfiniteLives,
    Offs_AxisInfiniteLives,

    Offs_ItemScope,
    Offs_ApplyItemScope,

    Offs_ThinkFunc_Pfn,
    Offs_ThinkFunc_Delta,
};

enum DoD_Func : unsigned char
{
    Fn_PlayerSpawn = false,
    Fn_GiveNamedItem,
    Fn_DropPlayerItem,
    Fn_GiveAmmo,
    Fn_SetWaveTime,
    Fn_GetWaveTime,
    Fn_RemovePlayerItem,
    Fn_AddPlayerItem,
    Fn_RemoveAllItems,
    Fn_SetBodygroup,
    Fn_DestroyItem,
    Fn_SubRemove,
    Fn_PackWeapon,
    Fn_InstallGameRules,
    Fn_WpnBoxKill,
    Fn_WpnBoxActivateThink,
    Fn_Create,
    Fn_UtilRemove,
    Fn_CreateNamedEntity,
};

enum DoD_Size : unsigned char
{
    Int8 = false,
    UInt8,
    Int16,
    UInt16,

    Int32,
};

#ifdef __linux__
enum DoD_Suffix : unsigned char
{
    None = false,

    i386,
    i486,
    i686,
};
#endif

typedef void(__thiscall* DoD_PlayerSpawn_Type) (::size_t CDoDTeamPlay, ::size_t CBasePlayer);
typedef ::edict_s* (__thiscall* DoD_GiveNamedItem_Type) (::size_t CBasePlayer, const char* pItem);
typedef void(__thiscall* DoD_DropPlayerItem_Type) (::size_t CBasePlayer, char* pItem, bool Force);
typedef int(__thiscall* DoD_GiveAmmo_Type) (::size_t CBasePlayer, int Ammo, const char* pName, int Max);
typedef void(__thiscall* DoD_SetWaveTime_Type) (::size_t CDoDTeamPlay, int Team, float Time);
typedef float(__thiscall* DoD_GetWaveTime_Type) (::size_t CDoDTeamPlay, int Team);
typedef int(__thiscall* DoD_RemovePlayerItem_Type) (::size_t CBasePlayer, ::size_t CBasePlayerItem);
typedef int(__thiscall* DoD_AddPlayerItem_Type) (::size_t CBasePlayer, ::size_t CBasePlayerItem);
typedef void(__thiscall* DoD_RemoveAllItems_Type) (::size_t CBasePlayer, int RemoveSuit);
typedef void(__thiscall* DoD_SetBodygroup_Type) (::size_t CBasePlayer, int Group, int Value);
typedef void(__thiscall* DoD_DestroyItem_Type) (::size_t CBasePlayerItem);
typedef void(__thiscall* DoD_SubRemove_Type) (::size_t CBaseEntity);
typedef int(__thiscall* DoD_PackWeapon_Type) (::size_t CWeaponBox, ::size_t CBasePlayerItem);
typedef void(__thiscall* DoD_WpnBoxKill_Type) (::size_t CWeaponBox);
typedef void(__thiscall* DoD_WpnBoxActivateThink_Type) (::size_t CWeaponBox);

typedef ::size_t(*DoD_Create_Type) (char* pItem, const ::Vector* pOrigin, const ::Vector* pAngles, ::edict_s* pOwner);
typedef ::size_t(*DoD_InstallGameRules_Type) ();
typedef void (*DoD_UtilRemove_Type) (::size_t CBaseEntity);
typedef ::edict_s* (*DoD_CreateNamedEntity_Type) (::size_t Name);

struct AllocatedString
{
    ::SourceHook::String Buffer{ };
    ::size_t Index = false;
};

struct SignatureData
{
    ::SourceHook::String Symbol{ };
    ::SourceHook::CVector < unsigned char > Signature{ };
    bool IsSymbol = false;
    ::size_t Offs = false;
};

struct CustomKeyValue_Del
{
    ::SourceHook::String Class{ };
    ::SourceHook::String Key{ };
    ::SourceHook::String Value{ };
    ::SourceHook::String Map{ };
};

struct CustomKeyValue_Add : CustomKeyValue_Del
{
    bool Added = false;
    bool Handled = false;
};

/** AMXX Module CPP */
extern ::DLL_FUNCTIONS* g_pFunctionTable;
extern ::DLL_FUNCTIONS* g_pFunctionTable_Post;
extern ::enginefuncs_s* g_pengfuncsTable;
extern ::enginefuncs_s* g_pengfuncsTable_Post;
extern ::NEW_DLL_FUNCTIONS* g_pNewFunctionsTable;
extern ::NEW_DLL_FUNCTIONS* g_pNewFunctionsTable_Post;

::size_t g_CDoDTeamPlay = false;

::DoD_PlayerSpawn_Type DoD_PlayerSpawn = NULL;
::DoD_GiveNamedItem_Type DoD_GiveNamedItem = NULL;
::DoD_DropPlayerItem_Type DoD_DropPlayerItem = NULL;
::DoD_GiveAmmo_Type DoD_GiveAmmo = NULL;
::DoD_SetWaveTime_Type DoD_SetWaveTime = NULL;
::DoD_GetWaveTime_Type DoD_GetWaveTime = NULL;
::DoD_RemovePlayerItem_Type DoD_RemovePlayerItem = NULL;
::DoD_AddPlayerItem_Type DoD_AddPlayerItem = NULL;
::DoD_RemoveAllItems_Type DoD_RemoveAllItems = NULL;
::DoD_SetBodygroup_Type DoD_SetBodygroup = NULL;
::DoD_DestroyItem_Type DoD_DestroyItem = NULL;
::DoD_SubRemove_Type DoD_SubRemove = NULL;
::DoD_PackWeapon_Type DoD_PackWeapon = NULL;
::DoD_InstallGameRules_Type DoD_InstallGameRules = NULL;
::DoD_UtilRemove_Type DoD_UtilRemove = NULL;
::DoD_CreateNamedEntity_Type DoD_CreateNamedEntity = NULL;
::DoD_WpnBoxKill_Type DoD_WpnBoxKill = NULL;
::DoD_WpnBoxActivateThink_Type DoD_WpnBoxActivateThink = NULL;
::DoD_Create_Type DoD_Create = NULL;

bool g_DoDPlayerSpawn_Hook = false;
bool g_DoDGiveNamedItem_Hook = false;
bool g_DoDDropPlayerItem_Hook = false;
bool g_DoDGiveAmmo_Hook = false;
bool g_DoDSetWaveTime_Hook = false;
bool g_DoDGetWaveTime_Hook = false;
bool g_DoDRemovePlayerItem_Hook = false;
bool g_DoDAddPlayerItem_Hook = false;
bool g_DoDRemoveAllItems_Hook = false;
bool g_DoDSetBodygroup_Hook = false;
bool g_DoDDestroyItem_Hook = false;
bool g_DoDSubRemove_Hook = false;
bool g_DoDPackWeapon_Hook = false;
bool g_DoDInstallGameRules_Hook = false;
bool g_DoDUtilRemove_Hook = false;
bool g_DoDCreateNamedEntity_Hook = false;
bool g_DoDWpnBoxKill_Hook = false;
bool g_DoDWpnBoxActivateThink_Hook = false;
bool g_DoDCreate_Hook = false;

#ifdef __linux__
::subhook_t g_pDoDPlayerSpawn = NULL;
::subhook_t g_pDoDGiveNamedItem = NULL;
::subhook_t g_pDoDDropPlayerItem = NULL;
::subhook_t g_pDoDGiveAmmo = NULL;
::subhook_t g_pDoDSetWaveTime = NULL;
::subhook_t g_pDoDGetWaveTime = NULL;
::subhook_t g_pDoDRemovePlayerItem = NULL;
::subhook_t g_pDoDAddPlayerItem = NULL;
::subhook_t g_pDoDRemoveAllItems = NULL;
::subhook_t g_pDoDSetBodygroup = NULL;
::subhook_t g_pDoDDestroyItem = NULL;
::subhook_t g_pDoDSubRemove = NULL;
::subhook_t g_pDoDPackWeapon = NULL;
::subhook_t g_pDoDInstallGameRules = NULL;
::subhook_t g_pDoDUtilRemove = NULL;
::subhook_t g_pDoDCreateNamedEntity = NULL;
::subhook_t g_pDoDWpnBoxKill = NULL;
::subhook_t g_pDoDWpnBoxActivateThink = NULL;
::subhook_t g_pDoDCreate = NULL;
#endif

void* g_pDoDPlayerSpawn_Addr = NULL;
void* g_pDoDGiveNamedItem_Addr = NULL;
void* g_pDoDDropPlayerItem_Addr = NULL;
void* g_pDoDGiveAmmo_Addr = NULL;
void* g_pDoDSetWaveTime_Addr = NULL;
void* g_pDoDGetWaveTime_Addr = NULL;
void* g_pDoDRemovePlayerItem_Addr = NULL;
void* g_pDoDAddPlayerItem_Addr = NULL;
void* g_pDoDRemoveAllItems_Addr = NULL;
void* g_pDoDSetBodygroup_Addr = NULL;
void* g_pDoDDestroyItem_Addr = NULL;
void* g_pDoDSubRemove_Addr = NULL;
void* g_pDoDPackWeapon_Addr = NULL;
void* g_pDoDInstallGameRules_Addr = NULL;
void* g_pDoDUtilRemove_Addr = NULL;
void* g_pDoDCreateNamedEntity_Addr = NULL;
void* g_pDoDWpnBoxKill_Addr = NULL;
void* g_pDoDWpnBoxActivateThink_Addr = NULL;
void* g_pDoDCreate_Addr = NULL;

int g_fwPlayerSpawn = false;
int g_fwGiveNamedItem = false;
int g_fwDropPlayerItem = false;
int g_fwAddPlayerItem = false;
int g_fwRemovePlayerItem = false;
int g_fwGetWaveTime = false;
int g_fwSetWaveTime = false;
int g_fwRemoveAllItems = false;
int g_fwGiveAmmo = false;
int g_fwSetBodygroup = false;
int g_fwDestroyItem = false;
int g_fwSubRemove = false;
int g_fwPackWeapon = false;
int g_fwInstallGameRules = false;
int g_fwUtilRemove = false;
int g_fwCreateNamedEntity = false;
int g_fwWpnBoxKill = false;
int g_fwWpnBoxActivateThink = false;
int g_fwCreate = false;

int g_fwPlayerSpawn_Post = false;
int g_fwGiveNamedItem_Post = false;
int g_fwDropPlayerItem_Post = false;
int g_fwAddPlayerItem_Post = false;
int g_fwRemovePlayerItem_Post = false;
int g_fwGetWaveTime_Post = false;
int g_fwSetWaveTime_Post = false;
int g_fwRemoveAllItems_Post = false;
int g_fwGiveAmmo_Post = false;
int g_fwSetBodygroup_Post = false;
int g_fwDestroyItem_Post = false;
int g_fwSubRemove_Post = false;
int g_fwPackWeapon_Post = false;
int g_fwInstallGameRules_Post = false;
int g_fwUtilRemove_Post = false;
int g_fwCreateNamedEntity_Post = false;
int g_fwWpnBoxKill_Post = false;
int g_fwWpnBoxActivateThink_Post = false;
int g_fwCreate_Post = false;

unsigned char* g_pAutoScopeFG42Addr = NULL;
unsigned char* g_pAutoScopeEnfieldAddr = NULL;

bool g_selfNade = false;
bool g_selfNadeDoSolid = false;
short g_selfNadeSolid = false;
int g_selfNadeMode = false;
int g_selfNadeFx = false;
int g_selfNadeAmt = false;
::color24 g_selfNadeColor{ };

bool g_droppedNade = false;
bool g_droppedDoArmed = false;
bool g_droppedNadeDoSolid = false;
short g_droppedNadeSolid = false;
int g_droppedNadeMode = false;
int g_droppedNadeFx = false;
int g_droppedNadeAmt = false;
::color24 g_droppedNadeColor{ };

bool g_exclFromDroppedExploNadeGlow[33]{ };
bool g_exclFromExploNadeProjGlow[33]{ };

::SourceHook::CVector < ::SignatureData > g_Sigs{ };
::SourceHook::CVector < ::AllocatedString > g_Strings{ };
::SourceHook::CVector < ::CustomKeyValue_Add > g_CustomKeyValues_Add{ };
::SourceHook::CVector < ::CustomKeyValue_Del > g_CustomKeyValues_Del{ };

::edict_s* g_pEntities = NULL;

::size_t setupString(const char* pString)
{
    for (const auto& String : ::g_Strings)
    {
        if (false == String.Buffer.icmp(pString))
            return String.Index;
    }

    ::AllocatedString String;
    String.Buffer = pString;
    String.Index = (*::g_engfuncs.pfnAllocString) (pString);
    ::g_Strings.push_back(String);
    return String.Index;
}

void allowFullMemAccess ( void * pAddr, ::size_t Size )
{
#ifndef __linux__
    static unsigned long Access;
    ::VirtualProtect ( pAddr, Size, PAGE_EXECUTE_READWRITE, &Access );
#else
    static long Page;
    static ::size_t Addr, Begin, End;

    Addr = ( ::size_t ) pAddr;
    Page = ::sysconf ( _SC_PAGESIZE ) - true;
    Begin = Addr & ~Page; /// Would turn '0xABC777AB' into '0xABC77000'.
    End = ( Addr + Size + Page ) & ~Page; /// Would turn '0xABC777AB' into '0xABC78000', '0xABC79000', ...
    ::mprotect ( Begin, End - Begin /** 0x1000(4096), 0x2000(8192), ... */, PROT_READ | PROT_WRITE | PROT_EXEC );
#endif
}

#ifdef __linux__
void* dlsymComplex(void* pLib, const char* pSym)
{ /// On Linux, use DWARF module information as well when looking to reveal library symbols (not only '::dlsym').
    auto pAddr = ::dlsym(pLib, pSym);
    if (pAddr)
        return pAddr;
    auto pLinkMap = (::link_map*)pLib;
    auto binFile = ::open(pLinkMap->l_name, O_RDONLY);
    auto binSize = ::lseek(binFile, false, SEEK_END);
    auto pMemMap = ::mmap(NULL, binSize, PROT_READ, MAP_PRIVATE, binFile, false);
    ::close(binFile);
    auto pEHdr = (::ElfW(Ehdr)*) pMemMap;
    auto pSHdrs = (::ElfW(Shdr)*) ((char*)pMemMap + pEHdr->e_shoff);
    auto pSHStrTab = (char*)pMemMap + pSHdrs[pEHdr->e_shstrndx].sh_offset;
    for (unsigned short i = false; i < pEHdr->e_shnum; ++i)
    {
        auto pSecName = pSHStrTab + pSHdrs[i].sh_name;
        if (!::strcmp(pSecName, ".symtab"))
        {
            auto pSyms = (::ElfW(Sym)*) ((char*)pMemMap + pSHdrs[i].sh_offset);
            auto Syms = pSHdrs[i].sh_size / sizeof(::ElfW(Sym));
            auto& strTabHdr = pSHdrs[pSHdrs[i].sh_link];
            auto pStrTab = (char*)pMemMap + strTabHdr.sh_offset;
            for (::size_t j = false; j < Syms; ++j)
            {
                auto pName = pStrTab + pSyms[j].st_name;
                if (pName && *pName && !::strcmp(pName, pSym))
                {
                    pAddr = (void*)(pSyms[j].st_value + pLinkMap->l_addr);
                    goto endOfFunc;
                }
            }
        }
    }
endOfFunc:
    ::munmap(pMemMap, binSize);
    return pAddr;
}
#endif

#ifdef __linux__
void* openLib(const char* pName, ::DoD_Suffix Suffix)
{
    /**
     * Do not open a BOT library for sig. scanning if 'MDLL_Spawn' points into one.
     * Use the Game library instead (i.e. 'dod.so' or 'dod_i386.so').
     * Linux only. On Windows, this filtering isn't needed.
     */
    ::Dl_info memInfo;
    if (::strcasestr(pName, "dod"))
    {
        ::dladdr(MDLL_Spawn, &memInfo);
        pName = memInfo.dli_fname;
        if (::strcasestr(pName, "bot"))
        {
            switch (Suffix)
            {
            case ::DoD_Suffix::i386: pName = "dod_i386"; break;
            case ::DoD_Suffix::i486: pName = "dod_i486"; break;
            case ::DoD_Suffix::i686: pName = "dod_i686"; break;
            default: pName = "dod"; break;
            }
        }
    }
    else if (::strcasestr(pName, "mp") || ::strcasestr(pName, "cs"))
    {
        ::dladdr(MDLL_Spawn, &memInfo);
        pName = memInfo.dli_fname;
        if (::strcasestr(pName, "bot"))
        {
            switch (Suffix)
            {
            case ::DoD_Suffix::i386: pName = "cs_i386"; break;
            case ::DoD_Suffix::i486: pName = "cs_i486"; break;
            case ::DoD_Suffix::i686: pName = "cs_i686"; break;
            default: pName = "cs"; break;
            }
        }
    }
    else if (::strcasestr(pName, "tfc"))
    {
        ::dladdr(MDLL_Spawn, &memInfo);
        pName = memInfo.dli_fname;
        if (::strcasestr(pName, "bot"))
        {
            switch (Suffix)
            {
            case ::DoD_Suffix::i386: pName = "tfc_i386"; break;
            case ::DoD_Suffix::i486: pName = "tfc_i486"; break;
            case ::DoD_Suffix::i686: pName = "tfc_i686"; break;
            default: pName = "tfc"; break;
            }
        }
    }
    else if (::strcasestr(pName, "hl"))
    {
        ::dladdr(MDLL_Spawn, &memInfo);
        pName = memInfo.dli_fname;
        if (::strcasestr(pName, "bot"))
        {
            switch (Suffix)
            {
            case ::DoD_Suffix::i386: pName = "hl_i386"; break;
            case ::DoD_Suffix::i486: pName = "hl_i486"; break;
            case ::DoD_Suffix::i686: pName = "hl_i686"; break;
            default: pName = "hl"; break;
            }
        }
    }
    else if (::strcasestr(pName, "dmc"))
    {
        ::dladdr(MDLL_Spawn, &memInfo);
        pName = memInfo.dli_fname;
        if (::strcasestr(pName, "bot"))
        {
            switch (Suffix)
            {
            case ::DoD_Suffix::i386: pName = "dmc_i386"; break;
            case ::DoD_Suffix::i486: pName = "dmc_i486"; break;
            case ::DoD_Suffix::i686: pName = "dmc_i686"; break;
            default: pName = "dmc"; break;
            }
        }
    }
    else if (::strcasestr(pName, "ricochet"))
    {
        ::dladdr(MDLL_Spawn, &memInfo);
        pName = memInfo.dli_fname;
        if (::strcasestr(pName, "bot"))
        {
            switch (Suffix)
            {
            case ::DoD_Suffix::i386: pName = "ricochet_i386"; break;
            case ::DoD_Suffix::i486: pName = "ricochet_i486"; break;
            case ::DoD_Suffix::i686: pName = "ricochet_i686"; break;
            default: pName = "ricochet"; break;
            }
        }
    } /** If opening the Engine library, there's already an address called '::gpGlobals' which helps revealing the exact library file path. */
    else if (::strcasestr(pName, "engine") || ::strcasestr(pName, "swds"))
    {
        ::dladdr(::gpGlobals, &memInfo);
        pName = memInfo.dli_fname;
    }
    void* pLib = ::dlopen(pName, RTLD_LAZY | RTLD_NOLOAD | RTLD_NODELETE);
    if (pLib)
        return pLib;
    pLib = ::dlopen(pName, RTLD_NOW);
    if (pLib)
        return pLib;
    ::SourceHook::String Name = pName;
    Name.append(".so");
    pLib = ::dlopen(Name.c_str(), RTLD_LAZY | RTLD_NOLOAD | RTLD_NODELETE);
    if (pLib)
        return pLib;
    pLib = ::dlopen(Name.c_str(), RTLD_NOW);
    if (pLib)
        return pLib;
    char Buffer[256];
    ::g_fn_BuildPathnameR(Buffer, sizeof Buffer, "dlls/%s", pName);
    pLib = ::dlopen(Buffer, RTLD_LAZY | RTLD_NOLOAD | RTLD_NODELETE);
    if (pLib)
        return pLib;
    pLib = ::dlopen(Buffer, RTLD_NOW);
    if (pLib)
        return pLib;
    Name = Buffer;
    Name.append(".so");
    pLib = ::dlopen(Name.c_str(), RTLD_LAZY | RTLD_NOLOAD | RTLD_NODELETE);
    if (pLib)
        return pLib;
    pLib = ::dlopen(Name.c_str(), RTLD_NOW);
    return pLib;
}
#else
::HINSTANCE__* openLib(const char* pName, bool& Opened)
{
    Opened = false;
    ::HINSTANCE__* pLib = ::GetModuleHandleA(pName);
    if (pLib)
        return pLib;
    pLib = ::LoadLibraryA(pName);
    if (pLib)
    {
        Opened = true;
        return pLib;
    }
    ::SourceHook::String Name = pName;
    Name.append(".dll");
    pLib = ::GetModuleHandleA(Name.c_str());
    if (pLib)
        return pLib;
    pLib = ::LoadLibraryA(Name.c_str());
    if (pLib)
    {
        Opened = true;
        return pLib;
    }
    char Buffer[256];
    ::g_fn_BuildPathnameR(Buffer, sizeof Buffer, "dlls/%s", pName);
    pLib = ::GetModuleHandleA(Buffer);
    if (pLib)
        return pLib;
    pLib = ::LoadLibraryA(Buffer);
    if (pLib)
    {
        Opened = true;
        return pLib;
    }
    Name = Buffer;
    Name.append(".dll");
    pLib = ::GetModuleHandleA(Name.c_str());
    if (pLib)
        return pLib;
    pLib = ::LoadLibraryA(Name.c_str());
    if (pLib)
    {
        Opened = true;
        return pLib;
    }
    return pLib;
}
#endif

int baseToIndex(::size_t CBase)
{
    if (false == CBase)
        return -1;

    ::entvars_s* pVars = *(::entvars_s**)(CBase + 4);
    if (!pVars)
        return -1;

    ::edict_s* pEntity = pVars->pContainingEntity;
    if (!pEntity)
        return -1;

    return ::ENTINDEX(pEntity);
}

::size_t indexToBase(int Index)
{
    if (Index < 0 || Index > ::gpGlobals->maxEntities)
        return false;

    ::edict_s* pEntity = ::INDEXENT(Index);
    if (!pEntity)
        return false;

    void* pBase = pEntity->pvPrivateData;
    if (!pBase)
        return false;

    return (::size_t)pBase;
}

void ServerActivate(::edict_s* pEntities, int, int)
{
    ::g_pEntities = pEntities;
    RETURN_META(::MRES_IGNORED);
}

void DispatchKeyValue(::edict_s* pEntity, ::KeyValueData* pKvData)
{
    if (!pEntity)
    {
        RETURN_META(::MRES_IGNORED);
    }
    ::KeyValueData keyValData;
    ::SourceHook::String Map = STRING(::gpGlobals->mapname);
    ::SourceHook::String Class = STRING(pEntity->v.classname);
    for (auto& cusKeyVal : ::g_CustomKeyValues_Add)
    { /// Add/ update the desired key values to map.
        if (cusKeyVal.Added || cusKeyVal.Class.icmp(Class) || (false == cusKeyVal.Map.empty() && cusKeyVal.Map.icmp(Map)))
            continue;
        keyValData.szClassName = cusKeyVal.Class.c_str();
        keyValData.szKeyName = cusKeyVal.Key.c_str();
        keyValData.szValue = cusKeyVal.Value.c_str();
        keyValData.fHandled = false;
        ::gpGamedllFuncs->dllapi_table->pfnKeyValue(pEntity, &keyValData);
        if (false == keyValData.fHandled)
        {
            MF_Log("*** Failed adding K: '%s' & V: '%s' to '%s'! ***\n",
                keyValData.szKeyName, keyValData.szValue, keyValData.szClassName);
        }
        cusKeyVal.Added = true;
        cusKeyVal.Handled = keyValData.fHandled;
    }
    if (pKvData && pKvData->szKeyName && *pKvData->szKeyName)
    {
        for (const auto& cusKeyVal : ::g_CustomKeyValues_Add)
        { /// If this key value has already been inserted, do not insert it again on the same map.
            if (cusKeyVal.Class.icmp(Class) || (false == cusKeyVal.Map.empty() && cusKeyVal.Map.icmp(Map)) ||
                cusKeyVal.Key.icmp(pKvData->szKeyName))
            { /// Has nothing to do with what we are looking for.
                continue;
            }
            pKvData->fHandled = cusKeyVal.Handled;
            RETURN_META(::MRES_SUPERCEDE);
        }
        for (const auto& cusKeyVal : ::g_CustomKeyValues_Del)
        { /// If this is a key value that the user wants to remove, filter it out.
            if (cusKeyVal.Class.icmp(Class) || (false == cusKeyVal.Map.empty() && cusKeyVal.Map.icmp(Map)) ||
                cusKeyVal.Key.icmp(pKvData->szKeyName))
            { /// Has nothing to do with what we are looking for.
                continue;
            }
            if (cusKeyVal.Value.empty())
            { /// If there's no value specified by operator, delete it because it matched the key.
                RETURN_META(::MRES_SUPERCEDE);
            }
            if (!pKvData->szValue || false == *pKvData->szValue)
            { /// Operator wants to remove a specific non-empty value, skip this entry.
                continue;
            }
            if (false == cusKeyVal.Value.icmp(pKvData->szValue))
            { /// If the values matches too, delete this entry.
                RETURN_META(::MRES_SUPERCEDE);
            }
        }
    }
    RETURN_META(::MRES_IGNORED);
}

int AddToFullPack_Post(::entity_state_s* pState, int eIdx, ::edict_s* pEntity, ::edict_s* pHost, int hostFlags, int isPlayer, unsigned char* pSet)
{ /// Used (attached) only if a plugin requires it.
    static unsigned char Host;
    static const char* pClass;
    if (pState && pHost && pEntity && !isPlayer)
    {
        Host = (unsigned char)F_EToI(pHost);
        if (::g_selfNade && !::g_exclFromExploNadeProjGlow[Host] && pHost == pEntity->v.owner &&
            false == ::_strnicmp(STRING(pEntity->v.classname), "grenade", 7))
        {
            if (::g_selfNadeDoSolid)
                pState->solid = ::g_selfNadeSolid;
            pState->rendermode = ::g_selfNadeMode;
            pState->renderfx = ::g_selfNadeFx;
            pState->rendercolor = ::g_selfNadeColor;
            pState->renderamt = ::g_selfNadeAmt;
        }
        else if (::g_droppedNade && !::g_exclFromDroppedExploNadeGlow[Host])
        {
            pClass = STRING(pEntity->v.classname);
            switch (::g_droppedDoArmed)
            {
            case false:
            {
                if (false == ::_stricmp(pClass, "weapon_stickgrenade") ||
                    false == ::_stricmp(pClass, "weapon_handgrenade"))
                {
                    if (::g_droppedNadeDoSolid)
                        pState->solid = ::g_droppedNadeSolid;
                    pState->rendermode = ::g_droppedNadeMode;
                    pState->renderfx = ::g_droppedNadeFx;
                    pState->rendercolor = ::g_droppedNadeColor;
                    pState->renderamt = ::g_droppedNadeAmt;
                }
                break;
            }
            default:
            {
                if (false == ::_strnicmp(pClass, "weapon_stickgrenade", 19) ||
                    false == ::_strnicmp(pClass, "weapon_handgrenade", 18))
                {
                    if (::g_droppedNadeDoSolid)
                        pState->solid = ::g_droppedNadeSolid;
                    pState->rendermode = ::g_droppedNadeMode;
                    pState->renderfx = ::g_droppedNadeFx;
                    pState->rendercolor = ::g_droppedNadeColor;
                    pState->renderamt = ::g_droppedNadeAmt;
                }
                break;
            }
            }
        }
    }
    RETURN_META_VALUE(::MRES_IGNORED, false);
}

::cell DoD_PlayerSpawn_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDPlayerSpawn_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_PlayerSpawn not found!");
        return false;
    }

    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pPlayerBase = pPlayer->pvPrivateData;
    if (!pPlayerBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    if (pParam[2])
        ((::DoD_PlayerSpawn_Type) ::g_pDoDPlayerSpawn_Addr) (::g_CDoDTeamPlay, (::size_t)pPlayerBase);
    else
    {
        if (::g_DoDPlayerSpawn_Hook)
            ::DoD_PlayerSpawn(::g_CDoDTeamPlay, (::size_t)pPlayerBase);
        else
            ((::DoD_PlayerSpawn_Type) ::g_pDoDPlayerSpawn_Addr) (::g_CDoDTeamPlay, (::size_t)pPlayerBase);
    }
    return true;
}

::cell DoD_RemoveAllItems_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDRemoveAllItems_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_RemoveAllItems not found!");
        return false;
    }

    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerIngame(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
        return false;
    }

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pPlayerBase = pPlayer->pvPrivateData;
    if (!pPlayerBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    if (pParam[3])
        ((::DoD_RemoveAllItems_Type) ::g_pDoDRemoveAllItems_Addr) ((::size_t)pPlayerBase, pParam[2]);
    else
    {
        if (::g_DoDRemoveAllItems_Hook)
            ::DoD_RemoveAllItems((::size_t)pPlayerBase, pParam[2]);
        else
            ((::DoD_RemoveAllItems_Type) ::g_pDoDRemoveAllItems_Addr) ((::size_t)pPlayerBase, pParam[2]);
    }
    return true;
}

::cell DoD_SubRemove_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDSubRemove_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_SubRemove not found!");
        return false;
    }

    auto Entity = pParam[1];
    if (Entity < 0 || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

    if (pParam[2])
        ((::DoD_SubRemove_Type) ::g_pDoDSubRemove_Addr) ((::size_t)pEntityBase);
    else
    {
        if (::g_DoDSubRemove_Hook)
            ::DoD_SubRemove((::size_t)pEntityBase);
        else
            ((::DoD_SubRemove_Type) ::g_pDoDSubRemove_Addr) ((::size_t)pEntityBase);
    }
    return true;
}

::cell DoD_PackWeapon_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pRes)
        *pRes = false;

    if (!::g_pDoDPackWeapon_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_PackWeapon not found!");
        return false;
    }

    auto Entity = pParam[1];
    if (Entity <= ::gpGlobals->maxClients || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weaponbox entity index %d!", Entity);
        return false;
    }

    auto Weapon = pParam[2];
    if (Weapon <= ::gpGlobals->maxClients || Weapon > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon entity index %d!", Weapon);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity (weaponbox) %d has no edict!", Entity);
        return false;
    }

    auto pWeapon = ::INDEXENT(Weapon);
    if (!pWeapon)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity (weapon) %d has no edict!", Weapon);
        return false;
    }

    auto pEntityBase = pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity (weaponbox) %d has no private data!", Entity);
        return false;
    }

    auto pWeaponBase = pWeapon->pvPrivateData;
    if (!pWeaponBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity (weapon) %d has no private data!", Weapon);
        return false;
    }

    if (pRes)
    {
        if (pParam[4])
            *pRes = ((::DoD_PackWeapon_Type) ::g_pDoDPackWeapon_Addr) ((::size_t)pEntityBase, (::size_t)pWeaponBase);
        else
        {
            if (::g_DoDPackWeapon_Hook)
                *pRes = ::DoD_PackWeapon((::size_t)pEntityBase, (::size_t)pWeaponBase);
            else
                *pRes = ((::DoD_PackWeapon_Type) ::g_pDoDPackWeapon_Addr) ((::size_t)pEntityBase, (::size_t)pWeaponBase);
        }
    }
    else
    {
        if (pParam[4])
            ((::DoD_PackWeapon_Type) ::g_pDoDPackWeapon_Addr) ((::size_t)pEntityBase, (::size_t)pWeaponBase);
        else
        {
            if (::g_DoDPackWeapon_Hook)
                ::DoD_PackWeapon((::size_t)pEntityBase, (::size_t)pWeaponBase);
            else
                ((::DoD_PackWeapon_Type) ::g_pDoDPackWeapon_Addr) ((::size_t)pEntityBase, (::size_t)pWeaponBase);
        }
    }
    return true;
}

::cell DoD_DestroyItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDDestroyItem_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_DestroyItem not found!");
        return false;
    }

    auto Entity = pParam[1];
    if (Entity <= gpGlobals->maxClients || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

    if (pParam[2])
        ((::DoD_DestroyItem_Type) ::g_pDoDDestroyItem_Addr) ((::size_t)pEntityBase);
    else
    {
        if (::g_DoDDestroyItem_Hook)
            ::DoD_DestroyItem((::size_t)pEntityBase);
        else
            ((::DoD_DestroyItem_Type) ::g_pDoDDestroyItem_Addr) ((::size_t)pEntityBase);
    }
    return true;
}

::cell DoD_UtilRemove_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDUtilRemove_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_UtilRemove not found!");
        return false;
    }

    auto Entity = pParam[1];
    if (Entity < 0 || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

    if (pParam[2])
        ((::DoD_UtilRemove_Type) ::g_pDoDUtilRemove_Addr) ((::size_t)pEntityBase);
    else
    {
        if (::g_DoDUtilRemove_Hook)
            ::DoD_UtilRemove((::size_t)pEntityBase);
        else
            ((::DoD_UtilRemove_Type) ::g_pDoDUtilRemove_Addr) ((::size_t)pEntityBase);
    }
    return true;
}

::cell DoD_CreateNamedEntity_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    if (pRes)
        *pRes = -1;

    if (!::g_pDoDCreateNamedEntity_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_CreateNamedEntity not found!");
        return false;
    }

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity name string (null or empty)!");
        return false;
    }

    ::edict_s* pEntity;
    if (pParam[3])
        pEntity = ((::DoD_CreateNamedEntity_Type) ::g_pDoDCreateNamedEntity_Addr) (::setupString(pName));
    else
    {
        if (::g_DoDCreateNamedEntity_Hook)
            pEntity = ::DoD_CreateNamedEntity(::setupString(pName));
        else
            pEntity = ((::DoD_CreateNamedEntity_Type) ::g_pDoDCreateNamedEntity_Addr) (::setupString(pName));
    }
    if (pRes && pEntity)
        *pRes = ::ENTINDEX(pEntity);
    return true;
}

::cell DoD_AllocString_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pAlloc = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    if (pAlloc)
        *pAlloc = -1;

    int Len;
    auto pString = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pString || false == *pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid allocation string (null or empty)!");
        return false;
    }

    auto Alloc = ::setupString(pString);
    if (pAlloc)
        *pAlloc = ::cell(Alloc);
    return true;
}

::cell DoD_WpnBoxKill_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDWpnBoxKill_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_WpnBoxKill not found!");
        return false;
    }

    auto Entity = pParam[1];
    if (Entity <= ::gpGlobals->maxClients || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weaponbox entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

    if (pParam[2])
        ((::DoD_WpnBoxKill_Type) ::g_pDoDWpnBoxKill_Addr) ((::size_t)pEntityBase);
    else
    {
        if (::g_DoDWpnBoxKill_Hook)
            ::DoD_WpnBoxKill((::size_t)pEntityBase);
        else
            ((::DoD_WpnBoxKill_Type) ::g_pDoDWpnBoxKill_Addr) ((::size_t)pEntityBase);
    }
    return true;
}

::cell DoD_WpnBoxActivateThink_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDWpnBoxActivateThink_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_WpnBoxActivateThink not found!");
        return false;
    }

    auto Entity = pParam[1];
    if (Entity <= ::gpGlobals->maxClients || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weaponbox entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

    if (pParam[2])
        ((::DoD_WpnBoxActivateThink_Type) ::g_pDoDWpnBoxActivateThink_Addr) ((::size_t)pEntityBase);
    else
    {
        if (::g_DoDWpnBoxActivateThink_Hook)
            ::DoD_WpnBoxActivateThink((::size_t)pEntityBase);
        else
            ((::DoD_WpnBoxActivateThink_Type) ::g_pDoDWpnBoxActivateThink_Addr) ((::size_t)pEntityBase);
    }
    return true;
}

::cell DoD_Create_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pItemRes = ::g_fn_GetAmxAddr(pAmx, pParam[5]);
    if (pItemRes)
        *pItemRes = -1;

    if (!::g_pDoDCreate_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_Create not found!");
        return false;
    }

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item name string (null or empty)!");
        return false;
    }

    ::size_t Item;
    ::cell* pOrigin = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    ::cell* pAngles = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    ::cell Owner = pParam[4];
    ::edict_s* pOwner = ((Owner < 0 || Owner > ::gpGlobals->maxEntities) ? NULL : ::INDEXENT(Owner));
    ::Vector Origin = !pOrigin ?
        ::Vector(false, false, false) :
        ::Vector(::g_fn_CellToReal(pOrigin[0]), ::g_fn_CellToReal(pOrigin[1]), ::g_fn_CellToReal(pOrigin[2]));
    ::Vector Angles = !pAngles ?
        ::Vector(false, false, false) :
        ::Vector(::g_fn_CellToReal(pAngles[0]), ::g_fn_CellToReal(pAngles[1]), ::g_fn_CellToReal(pAngles[2]));

    if (pParam[6])
        Item = ((::DoD_Create_Type) ::g_pDoDCreate_Addr) ((char*)STRING(::setupString(pName)), &Origin, &Angles, pOwner);
    else
    {
        if (::g_DoDCreate_Hook)
            Item = ::DoD_Create((char*)STRING(::setupString(pName)), &Origin, &Angles, pOwner);
        else
            Item = ((::DoD_Create_Type) ::g_pDoDCreate_Addr) ((char*)STRING(::setupString(pName)), &Origin, &Angles, pOwner);
    }

    if (pItemRes)
        *pItemRes = ::baseToIndex(Item);
    return true;
}

::cell DoD_GiveNamedItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pItemRes = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pItemRes)
        *pItemRes = -1;

    if (!::g_pDoDGiveNamedItem_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_GiveNamedItem not found!");
        return false;
    }

    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    int Len;
    auto pItem = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (Len < 1 || !pItem || false == *pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item name string (null or empty)!");
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerIngame(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerAlive(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not alive!", Player);
        return false;
    }

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pPlayerBase = pPlayer->pvPrivateData;
    if (!pPlayerBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    ::edict_s* pEntity;
    if (false == ::_stricmp("weapon_scopedfg42", pItem))
    {
        if (pParam[4])
            pEntity = ((::DoD_GiveNamedItem_Type) ::g_pDoDGiveNamedItem_Addr) ((::size_t)pPlayerBase, STRING(::setupString("weapon_fg42")));
        else
        {
            if (::g_DoDGiveNamedItem_Hook)
                pEntity = ::DoD_GiveNamedItem((::size_t)pPlayerBase, STRING(::setupString("weapon_fg42")));
            else
                pEntity = ((::DoD_GiveNamedItem_Type) ::g_pDoDGiveNamedItem_Addr) ((::size_t)pPlayerBase, STRING(::setupString("weapon_fg42")));
        }
        if (pEntity)
        {
            auto pItemBase = (::size_t*)pEntity->pvPrivateData;
            if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & 1u))
            {
                *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= 1u;
                (*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::DoD_Sig::Offs_ApplyItemScope].Offs)) (pItemBase);
            }
        }
    }
    else if (false == ::_stricmp("weapon_scopedenfield", pItem))
    {
        if (pParam[4])
            pEntity = ((::DoD_GiveNamedItem_Type) ::g_pDoDGiveNamedItem_Addr) ((::size_t)pPlayerBase, STRING(::setupString("weapon_enfield")));
        else
        {
            if (::g_DoDGiveNamedItem_Hook)
                pEntity = ::DoD_GiveNamedItem((::size_t)pPlayerBase, STRING(::setupString("weapon_enfield")));
            else
                pEntity = ((::DoD_GiveNamedItem_Type) ::g_pDoDGiveNamedItem_Addr) ((::size_t)pPlayerBase, STRING(::setupString("weapon_enfield")));
        }
        if (pEntity)
        {
            auto pItemBase = (::size_t*)pEntity->pvPrivateData;
            if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & 1u))
            {
                *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= 1u;
                (*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::DoD_Sig::Offs_ApplyItemScope].Offs)) (pItemBase);
            }
        }
    }
    else
    {
        if (pParam[4])
            pEntity = ((::DoD_GiveNamedItem_Type) ::g_pDoDGiveNamedItem_Addr) ((::size_t)pPlayerBase, STRING(::setupString(pItem)));
        else
        {
            if (::g_DoDGiveNamedItem_Hook)
                pEntity = ::DoD_GiveNamedItem((::size_t)pPlayerBase, STRING(::setupString(pItem)));
            else
                pEntity = ((::DoD_GiveNamedItem_Type) ::g_pDoDGiveNamedItem_Addr) ((::size_t)pPlayerBase, STRING(::setupString(pItem)));
        }
    }

    if (pItemRes && pEntity)
        *pItemRes = ::cell(::ENTINDEX(pEntity));
    return true;
}

::cell DoD_DropPlayerItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDDropPlayerItem_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_DropPlayerItem not found!");
        return false;
    }

    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    int Len;
    auto pItem = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (!pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item name string (null)!");
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerIngame(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
        return false;
    }

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pPlayerBase = pPlayer->pvPrivateData;
    if (!pPlayerBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    if (pParam[4])
        ((::DoD_DropPlayerItem_Type) ::g_pDoDDropPlayerItem_Addr) ((::size_t)pPlayerBase, pItem, (bool)pParam[3]);
    else
    {
        if (::g_DoDDropPlayerItem_Hook)
            ::DoD_DropPlayerItem((::size_t)pPlayerBase, pItem, (bool)pParam[3]);
        else
            ((::DoD_DropPlayerItem_Type) ::g_pDoDDropPlayerItem_Addr) ((::size_t)pPlayerBase, pItem, (bool)pParam[3]);
    }
    return true;
}

::cell DoD_SetBodygroup_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDSetBodygroup_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_SetBodygroup not found!");
        return false;
    }

    auto Entity = pParam[1];
    if (Entity < 0 || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

    if (pParam[4])
        ((::DoD_SetBodygroup_Type) ::g_pDoDSetBodygroup_Addr) ((::size_t)pEntityBase, pParam[2], pParam[3]);
    else
    {
        if (::g_DoDSetBodygroup_Hook)
            ::DoD_SetBodygroup((::size_t)pEntityBase, pParam[2], pParam[3]);
        else
            ((::DoD_SetBodygroup_Type) ::g_pDoDSetBodygroup_Addr) ((::size_t)pEntityBase, pParam[2], pParam[3]);
    }
    return true;
}

::cell DoD_GiveAmmo_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pAmmoRes = ::g_fn_GetAmxAddr(pAmx, pParam[5]);
    if (pAmmoRes)
        *pAmmoRes = -1;

    if (!::g_pDoDGiveAmmo_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_GiveAmmo not found!");
        return false;
    }

    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[3], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid ammo name string (null or empty)!");
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerIngame(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerAlive(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not alive!", Player);
        return false;
    }

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pPlayerBase = pPlayer->pvPrivateData;
    if (!pPlayerBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    if (pParam[6])
    {
        if (pAmmoRes)
            *pAmmoRes = ((::DoD_GiveAmmo_Type) ::g_pDoDGiveAmmo_Addr) ((::size_t)pPlayerBase, pParam[2], pName, pParam[4]);
        else
            ((::DoD_GiveAmmo_Type) ::g_pDoDGiveAmmo_Addr) ((::size_t)pPlayerBase, pParam[2], pName, pParam[4]);
    }
    else
    {
        if (::g_DoDGiveAmmo_Hook)
        {
            if (pAmmoRes)
                *pAmmoRes = ::DoD_GiveAmmo((::size_t)pPlayerBase, pParam[2], pName, pParam[4]);
            else
                ::DoD_GiveAmmo((::size_t)pPlayerBase, pParam[2], pName, pParam[4]);
        }
        else
        {
            if (pAmmoRes)
                *pAmmoRes = ((::DoD_GiveAmmo_Type) ::g_pDoDGiveAmmo_Addr) ((::size_t)pPlayerBase, pParam[2], pName, pParam[4]);
            else
                ((::DoD_GiveAmmo_Type) ::g_pDoDGiveAmmo_Addr) ((::size_t)pPlayerBase, pParam[2], pName, pParam[4]);
        }
    }
    return true;
}

::cell DoD_AddHealthIfWounded_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pAdded = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pAdded)
        *pAdded = false;

    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerIngame(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerAlive(Player))
        return false;

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    if (pPlayer->v.health + (float)pParam[2] >= pPlayer->v.max_health)
    {
        if (pAdded)
            *pAdded = ::cell(pPlayer->v.max_health - pPlayer->v.health);
        pPlayer->v.health = pPlayer->v.max_health;
    }
    else
    {
        pPlayer->v.health += (float)pParam[2];
        if (pAdded)
            *pAdded = pParam[2];
    }
    return true;
}

::cell DoD_IsPlayerFullHealth_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerIngame(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerAlive(Player))
        return false;

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    return pPlayer->v.health == pPlayer->v.max_health;
}

::cell DoD_RemovePlayerItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pRes)
        *pRes = false;

    if (!::g_pDoDRemovePlayerItem_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_RemovePlayerItem not found!");
        return false;
    }

    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerIngame(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
        return false;
    }

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pPlayerBase = pPlayer->pvPrivateData;
    if (!pPlayerBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    auto Item = pParam[2];
    if (Item <= ::gpGlobals->maxClients || Item > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Item);
        return false;
    }

    auto pItem = ::INDEXENT(Item);
    if (!pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no edict!", Item);
        return false;
    }

    auto pItemBase = pItem->pvPrivateData;
    if (!pItemBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no private data!", Item);
        return false;
    }

    if (pRes)
    {
        if (pParam[4] || false == ::g_DoDRemovePlayerItem_Hook)
            *pRes = ((::DoD_RemovePlayerItem_Type) ::g_pDoDRemovePlayerItem_Addr) ((::size_t)pPlayerBase, (::size_t)pItemBase);
        else
            *pRes = ::DoD_RemovePlayerItem((::size_t)pPlayerBase, (::size_t)pItemBase);
    }
    else
    {
        if (pParam[4] || false == ::g_DoDRemovePlayerItem_Hook)
            ((::DoD_RemovePlayerItem_Type) ::g_pDoDRemovePlayerItem_Addr) ((::size_t)pPlayerBase, (::size_t)pItemBase);
        else
            ::DoD_RemovePlayerItem((::size_t)pPlayerBase, (::size_t)pItemBase);
    }
    return true;
}

::cell DoD_AddPlayerItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pRes)
        *pRes = false;

    if (!::g_pDoDAddPlayerItem_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_AddPlayerItem not found!");
        return false;
    }

    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }

    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (!::g_fn_IsPlayerIngame(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
        return false;
    }

    auto pPlayer = ::g_fn_GetPlayerEdict(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pPlayerBase = pPlayer->pvPrivateData;
    if (!pPlayerBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    auto Item = pParam[2];
    if (Item <= ::gpGlobals->maxClients || Item > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Item);
        return false;
    }

    auto pItem = ::INDEXENT(Item);
    if (!pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no edict!", Item);
        return false;
    }

    auto pItemBase = pItem->pvPrivateData;
    if (!pItemBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no private data!", Item);
        return false;
    }

    if (pRes)
    {
        if (pParam[4] || false == ::g_DoDAddPlayerItem_Hook)
            *pRes = ((::DoD_AddPlayerItem_Type) ::g_pDoDAddPlayerItem_Addr) ((::size_t)pPlayerBase, (::size_t)pItemBase);
        else
            *pRes = ::DoD_AddPlayerItem((::size_t)pPlayerBase, (::size_t)pItemBase);
    }
    else
    {
        if (pParam[4] || false == ::g_DoDAddPlayerItem_Hook)
            ((::DoD_AddPlayerItem_Type) ::g_pDoDAddPlayerItem_Addr) ((::size_t)pPlayerBase, (::size_t)pItemBase);
        else
            ::DoD_AddPlayerItem((::size_t)pPlayerBase, (::size_t)pItemBase);
    }
    return true;
}

::cell DoD_HasScope_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Item = pParam[1];
    if (Item <= ::gpGlobals->maxClients || Item > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Item);
        return false;
    }

    auto pItem = ::INDEXENT(Item);
    if (!pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no edict!", Item);
        return false;
    }

    auto pItemBase = (::size_t*)pItem->pvPrivateData;
    if (!pItemBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no private data!", Item);
        return false;
    }

    return *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & 1u;
}

::cell DoD_AddScope_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Item = pParam[1];
    if (Item <= ::gpGlobals->maxClients || Item > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Item);
        return false;
    }

    auto pItem = ::INDEXENT(Item);
    if (!pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no edict!", Item);
        return false;
    }

    auto pItemBase = (::size_t*)pItem->pvPrivateData;
    if (!pItemBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no private data!", Item);
        return false;
    }

    if (!(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & 1u))
    {
        *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= 1u;
        if (pParam[2])
            (*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::DoD_Sig::Offs_ApplyItemScope].Offs)) (pItemBase);
    }
    return true;
}

::cell DoD_DeployItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Item = pParam[1];
    if (Item <= ::gpGlobals->maxClients || Item > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Item);
        return false;
    }

    auto pItem = ::INDEXENT(Item);
    if (!pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no edict!", Item);
        return false;
    }

    auto pItemBase = (::size_t*)pItem->pvPrivateData;
    if (!pItemBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no private data!", Item);
        return false;
    }

    (*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::DoD_Sig::Offs_ApplyItemScope].Offs)) (pItemBase);
    return true;
}

::cell DoD_FindSignature_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto* pAddr = ::g_fn_GetAmxAddr(pAmx, pParam[4]);
    if (pAddr)
        *pAddr = false;

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid module name string (null or empty)!");
        return false;
    }

    auto pSig = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (Len < 1 || !pSig || false == *pSig)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid signature string (null or empty)!");
        return false;
    }

#ifndef __linux__
    bool Opened = false;
    auto pMod = ::openLib(pName, Opened);
    if (!pMod)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::GetModuleHandleA/ ::LoadLibraryA failed for '%s'!", pName);
        return false;
    }
    ::_MEMORY_BASIC_INFORMATION memInfo{ };
    if (false == ::VirtualQuery(pMod, &memInfo, sizeof memInfo) || !memInfo.AllocationBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::VirtualQuery failed for '%s'!", pName);
        if (Opened)
            ::FreeLibrary(pMod);
        return false;
    }
    ::size_t Addr = false;
    auto pImgDosHdr = (::_IMAGE_DOS_HEADER*)memInfo.AllocationBase;
    auto pImgNtHdr = (::_IMAGE_NT_HEADERS*)((::size_t)pImgDosHdr + (::size_t)pImgDosHdr->e_lfanew);
    ::SourceHook::CVector < unsigned char > Signature;
    ::vectorizeSignature(pSig, Signature);
    auto Res = ::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, Signature, &Addr, pParam[3]);
    if (Res && pAddr)
        *pAddr = Addr;
    if (Opened)
        ::FreeLibrary(pMod);
    return Res;
#else /** Linux below. Try opening with suffix initially, for compatibility (i.e. people trying to use outdated BOT plugins). */
    void* pMod = ::openLib(pName, ::DoD_Suffix::i386);
    if (!pMod)
    {
        pMod = ::openLib(pName, ::DoD_Suffix::i486);
        if (!pMod)
        {
            pMod = ::openLib(pName, ::DoD_Suffix::i686);
            if (!pMod)
            {
                pMod = ::openLib(pName, ::DoD_Suffix::None);
                if (!pMod)
                {
                    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::dlopen failed for '%s'!", pName);
                    return false;
                }
            }
        }
    }
    auto pLinkMap = (::link_map*)pMod;
    struct ::stat memData;
    if (::stat(pLinkMap->l_name, &memData))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::stat failed for '%s'!", pName);
        ::dlclose(pMod);
        return false;
    }
    ::size_t Addr = false;
    ::SourceHook::CVector < unsigned char > Signature;
    ::vectorizeSignature(pSig, Signature);
    auto Res = ::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, Signature, &Addr, pParam[3]);
    if (Res && pAddr)
        *pAddr = Addr;
    ::dlclose(pMod);
    return Res;
#endif
}

::cell DoD_AddExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    ::g_selfNade = true;
    ::g_selfNadeMode = pParam[1];
    ::g_selfNadeFx = pParam[2];
    auto pColor = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pColor)
    {
        ::g_selfNadeColor.r = ::byte(pColor[0]);
        ::g_selfNadeColor.g = ::byte(pColor[1]);
        ::g_selfNadeColor.b = ::byte(pColor[2]);
    }
    else
    {
        ::g_selfNadeColor.r = ::byte(20);
        ::g_selfNadeColor.g = ::byte(180);
        ::g_selfNadeColor.b = ::byte(200);
    }
    ::g_selfNadeAmt = pParam[4];
    ::g_selfNadeSolid = short(pParam[5]);
    ::g_selfNadeDoSolid = bool(pParam[6]);
    ::g_pFunctionTable_Post->pfnAddToFullPack = ::AddToFullPack_Post;
    return true;
}

::cell DoD_DelExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (::g_selfNade)
    {
        ::g_selfNade = false;
        if (!::g_droppedNade)
            ::g_pFunctionTable_Post->pfnAddToFullPack = NULL;
        return true;
    }
    return false;
}

::cell DoD_AddDroppedExploNadeGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    ::g_droppedNade = true;
    ::g_droppedDoArmed = bool(pParam[1]);
    ::g_droppedNadeMode = pParam[2];
    ::g_droppedNadeFx = pParam[3];
    auto pColor = ::g_fn_GetAmxAddr(pAmx, pParam[4]);
    if (pColor)
    {
        ::g_droppedNadeColor.r = ::byte(pColor[0]);
        ::g_droppedNadeColor.g = ::byte(pColor[1]);
        ::g_droppedNadeColor.b = ::byte(pColor[2]);
    }
    else
    {
        ::g_droppedNadeColor.r = ::byte(20);
        ::g_droppedNadeColor.g = ::byte(200);
        ::g_droppedNadeColor.b = ::byte(40);
    }
    ::g_droppedNadeAmt = pParam[5];
    ::g_droppedNadeSolid = short(pParam[6]);
    ::g_droppedNadeDoSolid = bool(pParam[7]);
    ::g_pFunctionTable_Post->pfnAddToFullPack = ::AddToFullPack_Post;
    return true;
}

::cell DoD_DelDroppedExploNadeGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (::g_droppedNade)
    {
        ::g_droppedNade = false;
        if (!::g_selfNade)
            ::g_pFunctionTable_Post->pfnAddToFullPack = NULL;
        return true;
    }
    return false;
}

::cell DoD_SetDroppedExploNadeGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }
    ::g_exclFromDroppedExploNadeGlow[Player] = bool(pParam[2]);
    return true;
}

::cell DoD_GetDroppedExploNadeGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }
    return ::g_exclFromDroppedExploNadeGlow[Player];
}

::cell DoD_SetExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }
    ::g_exclFromExploNadeProjGlow[Player] = bool(pParam[2]);
    return true;
}

::cell DoD_GetExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }
    return ::g_exclFromExploNadeProjGlow[Player];
}

::cell DoD_FindSymbol_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto* pAddr = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pAddr)
        *pAddr = false;

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid module name string (null or empty)!");
        return false;
    }

    auto pSym = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (Len < 1 || !pSym || false == *pSym)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid symbol string (null or empty)!");
        return false;
    }

#ifndef __linux__
    bool Opened = false;
    auto pMod = ::openLib(pName, Opened);
    if (!pMod)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::GetModuleHandleA/ ::LoadLibraryA failed for '%s'!", pName);
        return false;
    }
    auto Addr = (::size_t) ::GetProcAddress(pMod, pSym);
    if (Addr && pAddr)
        *pAddr = Addr;
    if (Opened)
        ::FreeLibrary(pMod);
    return false != Addr;
#else /** Linux below. Try opening with suffix initially, for compatibility (i.e. people trying to use outdated BOT plugins). */
    void* pMod = ::openLib(pName, ::DoD_Suffix::i386);
    if (!pMod)
    {
        pMod = ::openLib(pName, ::DoD_Suffix::i486);
        if (!pMod)
        {
            pMod = ::openLib(pName, ::DoD_Suffix::i686);
            if (!pMod)
            {
                pMod = ::openLib(pName, ::DoD_Suffix::None);
                if (!pMod)
                {
                    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::dlopen failed for '%s'!", pName);
                    return false;
                }
            }
        }
    }
    ::size_t Addr = (::size_t) ::dlsymComplex(pMod, pSym);
    if (Addr && pAddr)
        *pAddr = Addr;
    ::dlclose(pMod);
    return false != Addr;
#endif
}

::cell DoD_StoreToAddress_Native(::tagAMX* pAmx, ::cell* pParam)
{
    ::size_t Addr = (::size_t)pParam[1];
    if (Addr < 1)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid memory address provided!");
        return false;
    }
    switch (::DoD_Size(pParam[3]))
    {
    case ::DoD_Size::Int8:
    {
        ::allowFullMemAccess((void*)Addr, true);
        * (signed char*)Addr = (signed char)pParam[2];
        return true;
    }
    case ::DoD_Size::UInt8:
    {
        ::allowFullMemAccess((void*)Addr, true);
        * (unsigned char*)Addr = (unsigned char)pParam[2];
        return true;
    }
    case ::DoD_Size::Int16:
    {
        ::allowFullMemAccess((void*)Addr, sizeof (short));
        * (signed short*)Addr = (signed short)pParam[2];
        return true;
    }
    case ::DoD_Size::UInt16:
    {
        ::allowFullMemAccess((void*)Addr, sizeof (short));
        * (unsigned short*)Addr = (unsigned short)pParam[2];
        return true;
    }
    case ::DoD_Size::Int32:
    {
        ::allowFullMemAccess((void*)Addr, sizeof (int));
        * (signed int*)Addr = (signed int)pParam[2];
        return true;
    }
    }
    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid memory size provided!");
    return false;
}

::cell DoD_ReadFromAddress_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pRes)
        *pRes = false;

    ::size_t Addr = (::size_t)pParam[1];
    if (Addr < 1)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid memory address provided!");
        return false;
    }

    ::cell Res;
    auto Size = ::DoD_Size(pParam[2]);
    switch (Size)
    {
    case ::DoD_Size::Int8:
    {
        Res = (::cell) * (signed char*)Addr;
        if (pRes)
            *pRes = Res;
        return true;
    }
    case ::DoD_Size::UInt8:
    {
        Res = (::cell) * (unsigned char*)Addr;
        if (pRes)
            *pRes = Res;
        return true;
    }
    case ::DoD_Size::Int16:
    {
        Res = (::cell) * (signed short*)Addr;
        if (pRes)
            *pRes = Res;
        return true;
    }
    case ::DoD_Size::UInt16:
    {
        Res = (::cell) * (unsigned short*)Addr;
        if (pRes)
            *pRes = Res;
        return true;
    }
    case ::DoD_Size::Int32:
    {
        Res = (::cell) * (signed int*)Addr;
        if (pRes)
            *pRes = Res;
        return true;
    }
    }
    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid memory size provided!");
    return false;
}

::cell DoD_SetEntityThinkFunc_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Entity = pParam[1];
    if (Entity < 0 || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity index %d!", Entity);
        return false;
    }

    auto Time = ::g_fn_CellToReal(pParam[7]);
    if (Time < 0.f)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid think time %f!", Time);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = (::size_t*)pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

    *(::size_t*)((unsigned char*)pEntityBase + ::g_Sigs[::DoD_Sig::Offs_ThinkFunc_Pfn].Offs) = (::size_t)pParam[2];
    if (pParam[3])
        *(int*)((unsigned char*)pEntityBase + ::g_Sigs[::DoD_Sig::Offs_ThinkFunc_Delta].Offs) = (int)pParam[4];
    if (pParam[5])
        pEntity->v.nextthink = (pParam[6] ? (::gpGlobals->time + Time) : Time);
    return true;
}

::cell DoD_GetEntityThinkFunc_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pDelta = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    if (pDelta)
        *pDelta = false;

    auto Entity = pParam[1];
    if (Entity < 0 || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::INDEXENT(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = (::size_t*)pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

    if (pDelta)
        *pDelta = ::cell(*(int*)((unsigned char*)pEntityBase + ::g_Sigs[::DoD_Sig::Offs_ThinkFunc_Delta].Offs));
    return ::cell(*(::size_t*)((unsigned char*)pEntityBase + ::g_Sigs[::DoD_Sig::Offs_ThinkFunc_Pfn].Offs));
}

::cell DoD_SetWaveTime_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDSetWaveTime_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_SetWaveTime not found!");
        return false;
    }

    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto Time = ::g_fn_CellToReal(pParam[2]);

    if (pParam[3])
        ((::DoD_SetWaveTime_Type) ::g_pDoDSetWaveTime_Addr) (::g_CDoDTeamPlay, pParam[1], Time);
    else
    {
        if (::g_DoDSetWaveTime_Hook)
            ::DoD_SetWaveTime(::g_CDoDTeamPlay, pParam[1], Time);
        else
            ((::DoD_SetWaveTime_Type) ::g_pDoDSetWaveTime_Addr) (::g_CDoDTeamPlay, pParam[1], Time);
    }
    return true;
}

::cell DoD_GetWaveTime_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDGetWaveTime_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_GetWaveTime not found!");
        return false;
    }

    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    float Time;
    if (pParam[2])
        Time = ((::DoD_GetWaveTime_Type) ::g_pDoDGetWaveTime_Addr) (::g_CDoDTeamPlay, pParam[1]);
    else
    {
        if (::g_DoDGetWaveTime_Hook)
            Time = ::DoD_GetWaveTime(::g_CDoDTeamPlay, pParam[1]);
        else
            Time = ((::DoD_GetWaveTime_Type) ::g_pDoDGetWaveTime_Addr) (::g_CDoDTeamPlay, pParam[1]);
    }
    return ::g_fn_RealToCell(Time);
}

::cell DoD_AreAlliesBritish_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    return *(pAddr + ::g_Sigs[::DoD_Sig::Offs_AlliesAreBrit].Offs);
}

::cell DoD_AreAlliesParatroopers_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    return *(pAddr + ::g_Sigs[::DoD_Sig::Offs_AlliesArePara].Offs);
}

::cell DoD_AreAxisParatroopers_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    return *(pAddr + ::g_Sigs[::DoD_Sig::Offs_AxisArePara].Offs);
}

::cell DoD_HaveAxisInfiniteLives_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    return *(pAddr + ::g_Sigs[::DoD_Sig::Offs_AxisInfiniteLives].Offs);
}

::cell DoD_HaveAlliesInfiniteLives_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    return *(pAddr + ::g_Sigs[::DoD_Sig::Offs_AlliesInfiniteLives].Offs);
}

::cell DoD_GetAxisRespawnFactor_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (float*)(::g_CDoDTeamPlay + ::g_Sigs[::DoD_Sig::Offs_AxisRespawnFactor].Offs);
    return ::g_fn_RealToCell(*pAddr);
}

::cell DoD_GetAlliesRespawnFactor_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (float*)(::g_CDoDTeamPlay + ::g_Sigs[::DoD_Sig::Offs_AlliesRespawnFactor].Offs);
    return ::g_fn_RealToCell(*pAddr);
}

::cell DoD_ReadGameRulesBool_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    return *(pAddr + pParam[1]);
}

::cell DoD_ReadGameRulesInt_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (::size_t*)(::g_CDoDTeamPlay + pParam[1]);
    return (::cell)*pAddr;
}

::cell DoD_ReadGameRulesFloat_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (float*)(::g_CDoDTeamPlay + pParam[1]);
    return ::g_fn_RealToCell(*pAddr);
}

::cell DoD_ReadGameRulesStr_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    ::SourceHook::String Buffer;
    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    for (::size_t Iter = false; Iter < (::size_t)pParam[2]; Iter++)
        Buffer.append(*(pAddr + pParam[1] + Iter));

    if (pParam[5])
        return ::g_fn_SetAmxStringUTF8Char(pAmx, pParam[3], Buffer.c_str(), Buffer.size(), pParam[4]);
    return ::g_fn_SetAmxString(pAmx, pParam[3], Buffer.c_str(), pParam[4]);
}

::cell DoD_IsWeaponPrimary_Native(::tagAMX* pAmx, ::cell* pParam)
{
    static const char* Weapons[] =
    {
        "weapon_thompson", "weapon_sten", "weapon_mp40", "weapon_greasegun",
        "weapon_spring", "weapon_scopedkar", "weapon_mp44", "weapon_fg42",
        "weapon_scopedfg42", "weapon_bren", "weapon_bar", "weapon_kar",
        "weapon_garand", "weapon_enfield", "weapon_scopedenfield", "weapon_mg34",
        "weapon_mg42", "weapon_m1carbine", "weapon_k43", "weapon_30cal",
        "weapon_piat", "weapon_bazooka", "weapon_pschreck",

        "thompson", "sten", "mp40", "greasegun",
        "spring", "scopedkar", "mp44", "fg42",
        "scopedfg42", "bren", "bar", "kar",
        "garand", "enfield", "scopedenfield", "mg34",
        "mg42", "m1carbine", "k43", "30cal",
        "piat", "bazooka", "pschreck",
    };

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon name string (null or empty)!");
        return false;
    }

    for (const auto& Weapon : Weapons)
    {
        if (false == ::_stricmp(Weapon, pName))
            return true;
    }
    return false;
}

::cell DoD_IsWeaponKnife_Native(::tagAMX* pAmx, ::cell* pParam)
{
    static const char* Weapons[] =
    {
        "weapon_spade", "weapon_amerknife", "weapon_gerknife",
        "spade", "amerknife", "gerknife",
    };

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon name string (null or empty)!");
        return false;
    }

    for (const auto& Weapon : Weapons)
    {
        if (false == ::_stricmp(Weapon, pName))
            return true;
    }
    return false;
}

::cell DoD_IsWeaponSecondary_Native(::tagAMX* pAmx, ::cell* pParam)
{
    static const char* Weapons[] =
    {
        "weapon_webley", "weapon_colt", "weapon_luger",
        "webley", "colt", "luger",
    };

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon name string (null or empty)!");
        return false;
    }

    for (const auto& Weapon : Weapons)
    {
        if (false == ::_stricmp(Weapon, pName))
            return true;
    }
    return false;
}

::cell DoD_IsWeaponGrenade_Native(::tagAMX* pAmx, ::cell* pParam)
{
    static const char* Weapons[] =
    {
        "weapon_handgrenade", "weapon_handgrenade_ex",
        "weapon_stickgrenade", "weapon_stickgrenade_ex",
        "handgrenade", "handgrenade_ex",
        "stickgrenade", "stickgrenade_ex",
    };

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon name string (null or empty)!");
        return false;
    }

    for (const auto& Weapon : Weapons)
    {
        if (false == ::_stricmp(Weapon, pName))
            return true;
    }
    return false;
}

::cell DoD_AddKeyValDel_Native(::tagAMX* pAmx, ::cell* pParam)
{
    int Len;
    auto pString = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (!pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid map name string (null)!");
        return false;
    }

    ::CustomKeyValue_Del keyValData;
    keyValData.Map = pString;

    pString = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (Len < 1 || !pString || false == *pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity class name string (null or empty)!");
        return false;
    }
    keyValData.Class = pString;

    pString = ::g_fn_GetAmxString(pAmx, pParam[3], false, &Len);
    if (Len < 1 || !pString || false == *pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity key name string (null or empty)!");
        return false;
    }
    keyValData.Key = pString;

    pString = ::g_fn_GetAmxString(pAmx, pParam[4], false, &Len);
    if (!pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity value name string (null)!");
        return false;
    }
    keyValData.Value = pString;

    ::g_CustomKeyValues_Del.push_back(keyValData);
    return true;
}

::cell DoD_AddKeyValAdd_Native(::tagAMX* pAmx, ::cell* pParam)
{
    int Len;
    auto pString = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (!pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid map name string (null)!");
        return false;
    }

    ::CustomKeyValue_Add keyValData;
    keyValData.Added = false;
    keyValData.Map = pString;

    pString = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (Len < 1 || !pString || false == *pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity class name string (null or empty)!");
        return false;
    }
    keyValData.Class = pString;

    pString = ::g_fn_GetAmxString(pAmx, pParam[3], false, &Len);
    if (Len < 1 || !pString || false == *pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity key name string (null or empty)!");
        return false;
    }
    keyValData.Key = pString;

    pString = ::g_fn_GetAmxString(pAmx, pParam[4], false, &Len);
    if (Len < 1 || !pString || false == *pString)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity value name string (null or empty)!");
        return false;
    }
    keyValData.Value = pString;

    ::g_CustomKeyValues_Add.push_back(keyValData);
    return true;
}

::cell DoD_DisableAutoScoping_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pAutoScopeFG42Addr || !::g_pAutoScopeEnfieldAddr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Auto-scoping signature(s) not found!");
        return false;
    }
    ::allowFullMemAccess(::g_pAutoScopeFG42Addr, true);
    *::g_pAutoScopeFG42Addr = ::g_Sigs[::DoD_Sig::PatchFg42_Byte].Signature[0];

    ::allowFullMemAccess(::g_pAutoScopeEnfieldAddr, true);
    *::g_pAutoScopeEnfieldAddr = ::g_Sigs[::DoD_Sig::PatchEnfield_Byte].Signature[0];
    return true;
}

::cell DoD_EnableAutoScoping_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pAutoScopeFG42Addr || !::g_pAutoScopeEnfieldAddr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Auto-scoping signature(s) not found!");
        return false;
    }
    ::allowFullMemAccess(::g_pAutoScopeFG42Addr, true);
    *::g_pAutoScopeFG42Addr = ::g_Sigs[::DoD_Sig::OrigFg42_Byte].Signature[0];

    ::allowFullMemAccess(::g_pAutoScopeEnfieldAddr, true);
    *::g_pAutoScopeEnfieldAddr = ::g_Sigs[::DoD_Sig::OrigEnfield_Byte].Signature[0];
    return true;
}

::cell DoD_IsAutoScopingEnabled_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pAutoScopeFG42Addr || !::g_pAutoScopeEnfieldAddr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Auto-scoping signature(s) not found!");
        return false;
    }

    return *::g_pAutoScopeFG42Addr != ::g_Sigs[::DoD_Sig::PatchFg42_Byte].Signature[0];
}

::cell DoD_AreGameRulesReady_Native(::tagAMX* pAmx, ::cell* pParam)
{
    return false != ::g_CDoDTeamPlay;
}

::cell DoD_InstallGameRules_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDInstallGameRules_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_InstallGameRules not found!");
        return false;
    }

    if (pParam[1] || false == ::g_DoDInstallGameRules_Hook)
        return (::cell)((::DoD_InstallGameRules_Type) ::g_pDoDInstallGameRules_Addr) ();
    return (::cell) ::DoD_InstallGameRules();
}

::cell DoD_GetFunctionAddress_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (true == (bool)pParam[2])
    {
        switch (::DoD_Func(pParam[1]))
        {
        case ::DoD_Func::Fn_PlayerSpawn: return (::cell) ::g_pDoDPlayerSpawn_Addr;
        case ::DoD_Func::Fn_GiveNamedItem: return (::cell) ::g_pDoDGiveNamedItem_Addr;
        case ::DoD_Func::Fn_DropPlayerItem: return (::cell) ::g_pDoDDropPlayerItem_Addr;
        case ::DoD_Func::Fn_GiveAmmo: return (::cell) ::g_pDoDGiveAmmo_Addr;
        case ::DoD_Func::Fn_SetWaveTime: return (::cell) ::g_pDoDSetWaveTime_Addr;
        case ::DoD_Func::Fn_GetWaveTime: return (::cell) ::g_pDoDGetWaveTime_Addr;
        case ::DoD_Func::Fn_RemovePlayerItem: return (::cell) ::g_pDoDRemovePlayerItem_Addr;
        case ::DoD_Func::Fn_AddPlayerItem: return (::cell) ::g_pDoDAddPlayerItem_Addr;
        case ::DoD_Func::Fn_RemoveAllItems: return (::cell) ::g_pDoDRemoveAllItems_Addr;
        case ::DoD_Func::Fn_SetBodygroup: return (::cell) ::g_pDoDSetBodygroup_Addr;
        case ::DoD_Func::Fn_DestroyItem: return (::cell) ::g_pDoDDestroyItem_Addr;
        case ::DoD_Func::Fn_SubRemove: return (::cell) ::g_pDoDSubRemove_Addr;
        case ::DoD_Func::Fn_PackWeapon: return (::cell) ::g_pDoDPackWeapon_Addr;
        case ::DoD_Func::Fn_InstallGameRules: return (::cell) ::g_pDoDInstallGameRules_Addr;
        case ::DoD_Func::Fn_UtilRemove: return (::cell) ::g_pDoDUtilRemove_Addr;
        case ::DoD_Func::Fn_CreateNamedEntity: return (::cell) ::g_pDoDCreateNamedEntity_Addr;
        case ::DoD_Func::Fn_Create: return (::cell) ::g_pDoDCreate_Addr;
        case ::DoD_Func::Fn_WpnBoxKill: return (::cell) ::g_pDoDWpnBoxKill_Addr;
        case ::DoD_Func::Fn_WpnBoxActivateThink: return (::cell) ::g_pDoDWpnBoxActivateThink_Addr;
        }
    }

    switch (::DoD_Func(pParam[1]))
    {
    case ::DoD_Func::Fn_PlayerSpawn: return ::cell(::g_DoDPlayerSpawn_Hook ? ::DoD_PlayerSpawn : ::g_pDoDPlayerSpawn_Addr);
    case ::DoD_Func::Fn_GiveNamedItem: return ::cell(::g_DoDGiveNamedItem_Hook ? ::DoD_GiveNamedItem : ::g_pDoDGiveNamedItem_Addr);
    case ::DoD_Func::Fn_DropPlayerItem: return ::cell(::g_DoDDropPlayerItem_Hook ? ::DoD_DropPlayerItem : ::g_pDoDDropPlayerItem_Addr);
    case ::DoD_Func::Fn_GiveAmmo: return ::cell(::g_DoDGiveAmmo_Hook ? ::DoD_GiveAmmo : ::g_pDoDGiveAmmo_Addr);
    case ::DoD_Func::Fn_SetWaveTime: return ::cell(::g_DoDSetWaveTime_Hook ? ::DoD_SetWaveTime : ::g_pDoDSetWaveTime_Addr);
    case ::DoD_Func::Fn_GetWaveTime: return ::cell(::g_DoDGetWaveTime_Hook ? ::DoD_GetWaveTime : ::g_pDoDGetWaveTime_Addr);
    case ::DoD_Func::Fn_RemovePlayerItem: return ::cell(::g_DoDRemovePlayerItem_Hook ? ::DoD_RemovePlayerItem : ::g_pDoDRemovePlayerItem_Addr);
    case ::DoD_Func::Fn_AddPlayerItem: return ::cell(::g_DoDAddPlayerItem_Hook ? ::DoD_AddPlayerItem : ::g_pDoDAddPlayerItem_Addr);
    case ::DoD_Func::Fn_RemoveAllItems: return ::cell(::g_DoDRemoveAllItems_Hook ? ::DoD_RemoveAllItems : ::g_pDoDRemoveAllItems_Addr);
    case ::DoD_Func::Fn_SetBodygroup: return ::cell(::g_DoDSetBodygroup_Hook ? ::DoD_SetBodygroup : ::g_pDoDSetBodygroup_Addr);
    case ::DoD_Func::Fn_DestroyItem: return ::cell(::g_DoDDestroyItem_Hook ? ::DoD_DestroyItem : ::g_pDoDDestroyItem_Addr);
    case ::DoD_Func::Fn_SubRemove: return ::cell(::g_DoDSubRemove_Hook ? ::DoD_SubRemove : ::g_pDoDSubRemove_Addr);
    case ::DoD_Func::Fn_PackWeapon: return ::cell(::g_DoDPackWeapon_Hook ? ::DoD_PackWeapon : ::g_pDoDPackWeapon_Addr);
    case ::DoD_Func::Fn_InstallGameRules: return ::cell(::g_DoDInstallGameRules_Hook ? ::DoD_InstallGameRules : ::g_pDoDInstallGameRules_Addr);
    case ::DoD_Func::Fn_UtilRemove: return ::cell(::g_DoDUtilRemove_Hook ? ::DoD_UtilRemove : ::g_pDoDUtilRemove_Addr);
    case ::DoD_Func::Fn_CreateNamedEntity: return ::cell(::g_DoDCreateNamedEntity_Hook ? ::DoD_CreateNamedEntity : ::g_pDoDCreateNamedEntity_Addr);
    case ::DoD_Func::Fn_Create: return ::cell(::g_DoDCreate_Hook ? ::DoD_Create : ::g_pDoDCreate_Addr);
    case ::DoD_Func::Fn_WpnBoxKill: return ::cell(::g_DoDWpnBoxKill_Hook ? ::DoD_WpnBoxKill : ::g_pDoDWpnBoxKill_Addr);
    case ::DoD_Func::Fn_WpnBoxActivateThink: return ::cell(::g_DoDWpnBoxActivateThink_Hook ? ::DoD_WpnBoxActivateThink : ::g_pDoDWpnBoxActivateThink_Addr);
    }

    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid function %d!", pParam[1]);
    return false;
}

void __fastcall DoD_PlayerSpawn_Hook(::size_t CDoDTeamPlay, FASTCALL_PARAM::size_t CBasePlayer)
{
    if (false != CDoDTeamPlay)
        ::g_CDoDTeamPlay = CDoDTeamPlay;

    if (false == CBasePlayer)
    {
        ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
        return;
    }

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + 4);
    if (!pPlayerVars)
    {
        ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
        return;
    }

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
    {
        ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
        return;
    }

    auto Player = ::ENTINDEX(pPlayer);
    if (::g_fn_ExecuteForward(::g_fwPlayerSpawn, CDoDTeamPlay, &Player))
        return;

    ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
    ::g_fn_ExecuteForward(::g_fwPlayerSpawn_Post, CDoDTeamPlay, Player);
}

void __fastcall DoD_SetBodygroup_Hook(::size_t CBasePlayer, FASTCALL_PARAM int Group, int Value)
{
    if (false == CBasePlayer)
    {
        ::DoD_SetBodygroup(CBasePlayer, Group, Value);
        return;
    }

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + 4);
    if (!pPlayerVars)
    {
        ::DoD_SetBodygroup(CBasePlayer, Group, Value);
        return;
    }

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
    {
        ::DoD_SetBodygroup(CBasePlayer, Group, Value);
        return;
    }

    auto Player = ::ENTINDEX(pPlayer);
    if (::g_fn_ExecuteForward(::g_fwSetBodygroup, Player, &Group, &Value))
        return;

    ::DoD_SetBodygroup(CBasePlayer, Group, Value);
    ::g_fn_ExecuteForward(::g_fwSetBodygroup_Post, Player, Group, Value);
}

void __fastcall DoD_SubRemove_Hook(::size_t CBaseEntity FASTCALL_PARAM_ALONE)
{
    if (false == CBaseEntity)
    {
        ::DoD_SubRemove(CBaseEntity);
        return;
    }

    auto pEntityVars = *(::entvars_s**)(CBaseEntity + 4);
    if (!pEntityVars)
    {
        ::DoD_SubRemove(CBaseEntity);
        return;
    }

    auto pEntity = pEntityVars->pContainingEntity;
    if (!pEntity)
    {
        ::DoD_SubRemove(CBaseEntity);
        return;
    }

    auto Entity = ::ENTINDEX(pEntity);
    if (::g_fn_ExecuteForward(::g_fwSubRemove, Entity))
        return;

    ::DoD_SubRemove(CBaseEntity);
    ::g_fn_ExecuteForward(::g_fwSubRemove_Post, Entity);
}

int __fastcall DoD_PackWeapon_Hook(::size_t CWeaponBox, FASTCALL_PARAM::size_t CBasePlayerItem)
{
    if (false == CWeaponBox || false == CBasePlayerItem)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    auto pEntityVars = *(::entvars_s**)(CWeaponBox + 4);
    if (!pEntityVars)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    auto pWeaponVars = *(::entvars_s**)(CBasePlayerItem + 4);
    if (!pWeaponVars)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    auto pEntity = pEntityVars->pContainingEntity;
    if (!pEntity)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    auto pWeapon = pWeaponVars->pContainingEntity;
    if (!pWeapon)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    ::cell Override = false;
    auto Entity = ::ENTINDEX(pEntity), Weapon = ::ENTINDEX(pWeapon);
    if (::g_fn_ExecuteForward(::g_fwPackWeapon, Entity, &Weapon, &Override))
        return Override;

    auto Res = ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);
    ::g_fn_ExecuteForward(::g_fwPackWeapon_Post, Entity, Weapon, Res);
    return Res;
}

void DoD_UtilRemove_Hook(::size_t CBaseEntity)
{
    if (false == CBaseEntity)
    {
        ::DoD_UtilRemove(CBaseEntity);
        return;
    }

    auto pEntityVars = *(::entvars_s**)(CBaseEntity + 4);
    if (!pEntityVars)
    {
        ::DoD_UtilRemove(CBaseEntity);
        return;
    }

    auto pEntity = pEntityVars->pContainingEntity;
    if (!pEntity)
    {
        ::DoD_UtilRemove(CBaseEntity);
        return;
    }

    auto Entity = ::ENTINDEX(pEntity);
    if (::g_fn_ExecuteForward(::g_fwUtilRemove, &Entity))
        return;

    ::DoD_UtilRemove(CBaseEntity);
    ::g_fn_ExecuteForward(::g_fwUtilRemove_Post, Entity);
}

::edict_s* DoD_CreateNamedEntity_Hook(::size_t Name)
{
    if (false == Name)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_CreateNamedEntity_Hook called by game with an allocated string whose index is zero! Contact the author to enhance this module! ***");
        }
        return ::DoD_CreateNamedEntity(Name);
    }

    static char Buffer[64];
#ifndef __linux__
    ::strncpy_s(Buffer, sizeof Buffer, STRING(Name), _TRUNCATE);
#else
    ::snprintf(Buffer, sizeof Buffer, STRING(Name));
#endif
    if (false == Buffer[0])
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_CreateNamedEntity_Hook called by game with an empty allocated string! Contact the author to enhance this module! ***");
        }
        return ::DoD_CreateNamedEntity(Name);
    }

    ::cell Override = -1;
    if (::g_fn_ExecuteForward(::g_fwCreateNamedEntity, Buffer, sizeof Buffer, &Override) || Buffer[0] == false)
        return Override < 0 ? NULL : ::INDEXENT(Override);

    auto pEntity = ::DoD_CreateNamedEntity(::setupString(Buffer));
    ::g_fn_ExecuteForward(::g_fwCreateNamedEntity_Post, Buffer, pEntity ? ::ENTINDEX(pEntity) : -1);
    return pEntity;
}

void __fastcall DoD_WpnBoxKill_Hook(::size_t CWeaponBox FASTCALL_PARAM_ALONE)
{
    if (false == CWeaponBox)
    {
        ::DoD_WpnBoxKill(CWeaponBox);
        return;
    }

    auto pEntityVars = *(::entvars_s**)(CWeaponBox + 4);
    if (!pEntityVars)
    {
        ::DoD_WpnBoxKill(CWeaponBox);
        return;
    }

    auto pEntity = pEntityVars->pContainingEntity;
    if (!pEntity)
    {
        ::DoD_WpnBoxKill(CWeaponBox);
        return;
    }

    auto Entity = ::ENTINDEX(pEntity);
    if (::g_fn_ExecuteForward(::g_fwWpnBoxKill, Entity))
        return;

    ::DoD_WpnBoxKill(CWeaponBox);
    ::g_fn_ExecuteForward(::g_fwWpnBoxKill_Post, Entity);
}

void __fastcall DoD_WpnBoxActivateThink_Hook(::size_t CWeaponBox FASTCALL_PARAM_ALONE)
{
    if (false == CWeaponBox)
    {
        ::DoD_WpnBoxActivateThink(CWeaponBox);
        return;
    }

    auto pEntityVars = *(::entvars_s**)(CWeaponBox + 4);
    if (!pEntityVars)
    {
        ::DoD_WpnBoxActivateThink(CWeaponBox);
        return;
    }

    auto pEntity = pEntityVars->pContainingEntity;
    if (!pEntity)
    {
        ::DoD_WpnBoxActivateThink(CWeaponBox);
        return;
    }

    auto Entity = ::ENTINDEX(pEntity);
    if (::g_fn_ExecuteForward(::g_fwWpnBoxActivateThink, Entity))
        return;

    ::DoD_WpnBoxActivateThink(CWeaponBox);
    ::g_fn_ExecuteForward(::g_fwWpnBoxActivateThink_Post, Entity);
}

::size_t DoD_Create_Hook(char* pName, const ::Vector* pOrigin, const ::Vector* pAngles, ::edict_s* pOwner)
{
    if (!pName)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_Create_Hook called by game with a null item name! Contact the author to enhance this module! ***");
        }
        return ::DoD_Create(pName, pOrigin, pAngles, pOwner);
    }
    if (false == *pName)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_Create_Hook called by game with an empty item name! Contact the author to enhance this module! ***");
        }
        return ::DoD_Create(pName, pOrigin, pAngles, pOwner);
    }
    if (!pOrigin)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_Create_Hook called by game with a null origin parameter! Contact the author to enhance this module! ***");
        }
        return ::DoD_Create(pName, pOrigin, pAngles, pOwner);
    }
    if (!pAngles)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_Create_Hook called by game with a null angles parameter! Contact the author to enhance this module! ***");
        }
        return ::DoD_Create(pName, pOrigin, pAngles, pOwner);
    }

    static char Buffer[64];
#ifndef __linux__
    ::strncpy_s(Buffer, sizeof Buffer, pName, _TRUNCATE);
#else
    ::snprintf(Buffer, sizeof Buffer, pName);
#endif
    static ::cell Origin[3];
    Origin[0] = ::g_fn_RealToCell(pOrigin->x);
    Origin[1] = ::g_fn_RealToCell(pOrigin->y);
    Origin[2] = ::g_fn_RealToCell(pOrigin->z);
    static ::cell Angles[3];
    Angles[0] = ::g_fn_RealToCell(pAngles->x);
    Angles[1] = ::g_fn_RealToCell(pAngles->y);
    Angles[2] = ::g_fn_RealToCell(pAngles->z);
    auto Override = ::cell(-1);
    auto Owner = ::cell(pOwner ? ::ENTINDEX(pOwner) : -1);
    auto amxOrigin = ::g_fn_PrepareCellArrayA(Origin, ARRAYSIZE(Origin), true);
    auto amxAngles = ::g_fn_PrepareCellArrayA(Angles, ARRAYSIZE(Angles), true);
    if (::g_fn_ExecuteForward(::g_fwCreate, Buffer, sizeof Buffer, amxOrigin, amxAngles, &Owner, &Override) || false == Buffer[0])
        return ::indexToBase(Override);

    auto vecOrigin = ::Vector(::g_fn_CellToReal(Origin[0]), ::g_fn_CellToReal(Origin[1]), ::g_fn_CellToReal(Origin[2]));
    auto vecAngles = ::Vector(::g_fn_CellToReal(Angles[0]), ::g_fn_CellToReal(Angles[1]), ::g_fn_CellToReal(Angles[2]));
    ::edict_s* pNewOwner = ((Owner < 0 || Owner > ::gpGlobals->maxEntities) ? NULL : ::INDEXENT(Owner));
    auto Item = ::DoD_Create((char*)STRING(::setupString(Buffer)), &vecOrigin, &vecAngles, pNewOwner);
    amxOrigin = ::g_fn_PrepareCellArrayA(Origin, ARRAYSIZE(Origin), false);
    amxAngles = ::g_fn_PrepareCellArrayA(Angles, ARRAYSIZE(Angles), false);
    auto ItemIndex = ::cell(::baseToIndex(Item));
    ::g_fn_ExecuteForward(::g_fwCreate_Post, Buffer, amxOrigin, amxAngles, Owner, ItemIndex);
    return Item;
}

void __fastcall DoD_DestroyItem_Hook(::size_t CBasePlayerItem FASTCALL_PARAM_ALONE)
{
    if (false == CBasePlayerItem)
    {
        ::DoD_DestroyItem(CBasePlayerItem);
        return;
    }

    auto pEntityVars = *(::entvars_s**)(CBasePlayerItem + 4);
    if (!pEntityVars)
    {
        ::DoD_DestroyItem(CBasePlayerItem);
        return;
    }

    auto pEntity = pEntityVars->pContainingEntity;
    if (!pEntity)
    {
        ::DoD_DestroyItem(CBasePlayerItem);
        return;
    }

    auto Entity = ::ENTINDEX(pEntity);
    if (::g_fn_ExecuteForward(::g_fwDestroyItem, Entity))
        return;

    ::DoD_DestroyItem(CBasePlayerItem);
    ::g_fn_ExecuteForward(::g_fwDestroyItem_Post, Entity);
}

::edict_s* __fastcall DoD_GiveNamedItem_Hook(::size_t CBasePlayer, FASTCALL_PARAM const char* pItem)
{
    if (false == CBasePlayer)
        return ::DoD_GiveNamedItem(CBasePlayer, pItem);

    if (!pItem)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_GiveNamedItem_Hook called by game with a null item name! Contact the author to enhance this module! ***");
        }
        return ::DoD_GiveNamedItem(CBasePlayer, pItem);
    }
    if (false == *pItem)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_GiveNamedItem_Hook called by game with an empty item name! Contact the author to enhance this module! ***");
        }
        return ::DoD_GiveNamedItem(CBasePlayer, pItem);
    }

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + 4);
    if (!pPlayerVars)
        return ::DoD_GiveNamedItem(CBasePlayer, pItem);

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
        return ::DoD_GiveNamedItem(CBasePlayer, pItem);

    ::cell Override = -1;
    auto Player = ::ENTINDEX(pPlayer);
    static char Buffer[64];
#ifndef __linux__
    ::strncpy_s(Buffer, sizeof Buffer, pItem, _TRUNCATE);
#else
    ::snprintf(Buffer, sizeof Buffer, pItem);
#endif
    if (::g_fn_ExecuteForward(::g_fwGiveNamedItem, Player, Buffer, sizeof Buffer, &Override) || false == Buffer[0])
        return Override < 0 ? NULL : ::INDEXENT(Override);

    ::edict_s* pEntity;
    if (false == ::_stricmp("weapon_scopedfg42", Buffer))
    {
        pEntity = ::DoD_GiveNamedItem(CBasePlayer, STRING(::setupString("weapon_fg42")));
        if (pEntity)
        {
            auto pItemBase = (::size_t*)pEntity->pvPrivateData;
            if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & 1u))
            {
                *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= 1u;
                (*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::DoD_Sig::Offs_ApplyItemScope].Offs)) (pItemBase);
            }
        }
    }
    else if (false == ::_stricmp("weapon_scopedenfield", Buffer))
    {
        pEntity = ::DoD_GiveNamedItem(CBasePlayer, STRING(::setupString("weapon_enfield")));
        if (pEntity)
        {
            auto pItemBase = (::size_t*)pEntity->pvPrivateData;
            if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & 1u))
            {
                *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= 1u;
                (*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::DoD_Sig::Offs_ApplyItemScope].Offs)) (pItemBase);
            }
        }
    }
    else
        pEntity = ::DoD_GiveNamedItem(CBasePlayer, STRING(::setupString(Buffer)));

    ::g_fn_ExecuteForward(::g_fwGiveNamedItem_Post, Player, Buffer, pEntity ? ::ENTINDEX(pEntity) : -1);
    return pEntity;
}

int __fastcall DoD_GiveAmmo_Hook(::size_t CBasePlayer, FASTCALL_PARAM int Ammo, const char* pName, int Max)
{
    if (false == CBasePlayer)
        return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);

    if (!pName)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_GiveAmmo_Hook called by game with a null ammo name! Contact the author to enhance this module! ***");
        }
        return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);
    }
    if (false == *pName)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_GiveAmmo_Hook called by game with an empty ammo name! Contact the author to enhance this module! ***");
        }
        return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);
    }

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + 4);
    if (!pPlayerVars)
        return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
        return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);

    ::cell Override = -1;
    auto Player = ::ENTINDEX(pPlayer);
    static char Buffer[64];
#ifndef __linux__
    ::strncpy_s(Buffer, sizeof Buffer, pName, _TRUNCATE);
#else
    ::snprintf(Buffer, sizeof Buffer, pName);
#endif
    if (::g_fn_ExecuteForward(::g_fwGiveAmmo, Player, &Ammo, Buffer, sizeof Buffer, &Max, &Override) || false == Buffer[0])
        return Override;

    auto Res = ::DoD_GiveAmmo(CBasePlayer, Ammo, Buffer, Max);
    ::g_fn_ExecuteForward(::g_fwGiveAmmo_Post, Player, Ammo, Buffer, Max, Res);
    return Res;
}

void __fastcall DoD_DropPlayerItem_Hook(::size_t CBasePlayer, FASTCALL_PARAM char* pItem, bool Force)
{
    if (false == CBasePlayer)
    {
        ::DoD_DropPlayerItem(CBasePlayer, pItem, Force);
        return;
    }

    if (!pItem)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            Logged = true;
            ::MF_Log("*** Warning, ::DoD_DropPlayerItem_Hook called by game with a null item name! Contact the author to enhance this module! ***");
        }
        return ::DoD_DropPlayerItem(CBasePlayer, pItem, Force);
    }

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + 4);
    if (!pPlayerVars)
    {
        ::DoD_DropPlayerItem(CBasePlayer, pItem, Force);
        return;
    }

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
    {
        ::DoD_DropPlayerItem(CBasePlayer, pItem, Force);
        return;
    }

    auto newForce = (::cell)Force;
    auto Player = ::ENTINDEX(pPlayer);
    static char Buffer[64];
#ifndef __linux__
    ::strncpy_s(Buffer, sizeof Buffer, pItem, _TRUNCATE);
#else
    ::snprintf(Buffer, sizeof Buffer, pItem);
#endif
    if (::g_fn_ExecuteForward(::g_fwDropPlayerItem, Player, Buffer, sizeof Buffer, &newForce))
        return;

    ::DoD_DropPlayerItem(CBasePlayer, Buffer, bool(newForce));
    ::g_fn_ExecuteForward(::g_fwDropPlayerItem_Post, Player, Buffer, newForce);
}

int __fastcall DoD_RemovePlayerItem_Hook(::size_t CBasePlayer, FASTCALL_PARAM::size_t CBasePlayerItem)
{
    if (false == CBasePlayer || false == CBasePlayerItem)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + 4);
    if (!pPlayerVars)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    auto pItemVars = *(::entvars_s**)(CBasePlayerItem + 4);
    if (!pItemVars)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    auto pItem = pItemVars->pContainingEntity;
    if (!pItem)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    ::cell Override = false;
    auto Player = ::ENTINDEX(pPlayer);
    auto Item = ::ENTINDEX(pItem);
    if (::g_fn_ExecuteForward(::g_fwRemovePlayerItem, Player, &Item, &Override))
        return Override;

    auto Res = ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);
    ::g_fn_ExecuteForward(::g_fwRemovePlayerItem_Post, Player, Item, Res);
    return Res;
}

int __fastcall DoD_AddPlayerItem_Hook(::size_t CBasePlayer, FASTCALL_PARAM::size_t CBasePlayerItem)
{
    if (false == CBasePlayer || false == CBasePlayerItem)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + 4);
    if (!pPlayerVars)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    auto pItemVars = *(::entvars_s**)(CBasePlayerItem + 4);
    if (!pItemVars)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    auto pItem = pItemVars->pContainingEntity;
    if (!pItem)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    ::cell Override = false;
    auto Player = ::ENTINDEX(pPlayer);
    auto Item = ::ENTINDEX(pItem);
    if (::g_fn_ExecuteForward(::g_fwAddPlayerItem, Player, &Item, &Override))
        return Override;

    auto Res = ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);
    ::g_fn_ExecuteForward(::g_fwAddPlayerItem_Post, Player, Item, Res);
    return Res;
}

void __fastcall DoD_RemoveAllItems_Hook(::size_t CBasePlayer, FASTCALL_PARAM int RemoveSuit)
{
    if (false == CBasePlayer)
    {
        ::DoD_RemoveAllItems(CBasePlayer, RemoveSuit);
        return;
    }

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + 4);
    if (!pPlayerVars)
    {
        ::DoD_RemoveAllItems(CBasePlayer, RemoveSuit);
        return;
    }

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
    {
        ::DoD_RemoveAllItems(CBasePlayer, RemoveSuit);
        return;
    }

    auto Player = ::ENTINDEX(pPlayer);
    if (::g_fn_ExecuteForward(::g_fwRemoveAllItems, Player, &RemoveSuit))
        return;

    ::DoD_RemoveAllItems(CBasePlayer, RemoveSuit);
    ::g_fn_ExecuteForward(::g_fwRemoveAllItems_Post, Player, RemoveSuit);
}

float __fastcall DoD_GetWaveTime_Hook(::size_t CDoDTeamPlay, FASTCALL_PARAM int Team)
{
    if (false != CDoDTeamPlay)
        ::g_CDoDTeamPlay = CDoDTeamPlay;

    float Time = false;
    if (::g_fn_ExecuteForward(::g_fwGetWaveTime, CDoDTeamPlay, &Team, &Time))
        return Time;

    Time = ::DoD_GetWaveTime(CDoDTeamPlay, Team);
    ::g_fn_ExecuteForward(::g_fwGetWaveTime_Post, CDoDTeamPlay, Team, Time);
    return Time;
}

void __fastcall DoD_SetWaveTime_Hook(::size_t CDoDTeamPlay, FASTCALL_PARAM int Team, float Time)
{
    if (false != CDoDTeamPlay)
        ::g_CDoDTeamPlay = CDoDTeamPlay;

    if (::g_fn_ExecuteForward(::g_fwSetWaveTime, CDoDTeamPlay, &Team, &Time))
        return;

    ::DoD_SetWaveTime(CDoDTeamPlay, Team, Time);
    ::g_fn_ExecuteForward(::g_fwSetWaveTime_Post, CDoDTeamPlay, Team, Time);
}

::size_t DoD_InstallGameRules_Hook()
{
    ::cell Override = false;
    if (::g_fn_ExecuteForward(::g_fwInstallGameRules, &Override))
        return (::size_t)Override;

    ::g_CDoDTeamPlay = ::DoD_InstallGameRules();
    ::g_fn_ExecuteForward(::g_fwInstallGameRules_Post, ::g_CDoDTeamPlay);
    return ::g_CDoDTeamPlay;
}

::AMX_NATIVE_INFO DoDHacks_Natives[] =
{
    { "DoD_PlayerSpawn", ::DoD_PlayerSpawn_Native, },
    { "DoD_GiveNamedItem", ::DoD_GiveNamedItem_Native, },
    { "DoD_DropPlayerItem", ::DoD_DropPlayerItem_Native, },
    { "DoD_RemoveAllItems", ::DoD_RemoveAllItems_Native, },
    { "DoD_GiveAmmo", ::DoD_GiveAmmo_Native, },
    { "DoD_AddHealthIfWounded", ::DoD_AddHealthIfWounded_Native, },
    { "DoD_IsPlayerFullHealth", ::DoD_IsPlayerFullHealth_Native, },
    { "DoD_InstallGameRules", ::DoD_InstallGameRules_Native, },
    { "DoD_DeployItem", ::DoD_DeployItem_Native, },
    { "DoD_SetBodygroup", ::DoD_SetBodygroup_Native, },
    { "DoD_SubRemove", ::DoD_SubRemove_Native, },
    { "DoD_UtilRemove", ::DoD_UtilRemove_Native, },
    { "DoD_CreateNamedEntity", ::DoD_CreateNamedEntity_Native, },
    { "DoD_DestroyItem", ::DoD_DestroyItem_Native, },
    { "DoD_PackWeapon", ::DoD_PackWeapon_Native, },
    { "DoD_WpnBoxKill", ::DoD_WpnBoxKill_Native, },
    { "DoD_WpnBoxActivateThink", ::DoD_WpnBoxActivateThink_Native, },
    { "DoD_Create", ::DoD_Create_Native, },
    { "DoD_AllocString", ::DoD_AllocString_Native, },
    { "DoD_GetFunctionAddress", ::DoD_GetFunctionAddress_Native, },
    { "DoD_SetEntityThinkFunc", ::DoD_SetEntityThinkFunc_Native, },
    { "DoD_GetEntityThinkFunc", ::DoD_GetEntityThinkFunc_Native, },

    { "DoD_FindSignature", ::DoD_FindSignature_Native, },
    { "DoD_FindSymbol", ::DoD_FindSymbol_Native, },

    { "DoD_ReadFromAddress", ::DoD_ReadFromAddress_Native, },
    { "DoD_StoreToAddress", ::DoD_StoreToAddress_Native, },

    { "DoD_SetWaveTime", ::DoD_SetWaveTime_Native, },
    { "DoD_GetWaveTime", ::DoD_GetWaveTime_Native, },

    { "DoD_IsWeaponPrimary", ::DoD_IsWeaponPrimary_Native, },
    { "DoD_IsWeaponSecondary", ::DoD_IsWeaponSecondary_Native, },
    { "DoD_IsWeaponKnife", ::DoD_IsWeaponKnife_Native, },
    { "DoD_IsWeaponGrenade", ::DoD_IsWeaponGrenade_Native, },

    { "DoD_DisableAutoScoping", ::DoD_DisableAutoScoping_Native, },
    { "DoD_EnableAutoScoping", ::DoD_EnableAutoScoping_Native, },
    { "DoD_IsAutoScopingEnabled", ::DoD_IsAutoScopingEnabled_Native, },

    { "DoD_HasScope", ::DoD_HasScope_Native, },
    { "DoD_AddScope", ::DoD_AddScope_Native, },

    { "DoD_AddKeyValDel", ::DoD_AddKeyValDel_Native, },
    { "DoD_AddKeyValAdd", ::DoD_AddKeyValAdd_Native, },

    { "DoD_RemovePlayerItem", ::DoD_RemovePlayerItem_Native, },
    { "DoD_AddPlayerItem", ::DoD_AddPlayerItem_Native, },

    { "DoD_AddExploNadeProjGlow", ::DoD_AddExploNadeProjGlow_Native, },
    { "DoD_DelExploNadeProjGlow", ::DoD_DelExploNadeProjGlow_Native, },
    { "DoD_SetExploNadeProjGlow", ::DoD_SetExploNadeProjGlow_Native, },
    { "DoD_GetExploNadeProjGlow", ::DoD_GetExploNadeProjGlow_Native, },

    { "DoD_AddDroppedExploNadeGlow", ::DoD_AddDroppedExploNadeGlow_Native, },
    { "DoD_DelDroppedExploNadeGlow", ::DoD_DelDroppedExploNadeGlow_Native, },
    { "DoD_SetDroppedExploNadeGlow", ::DoD_SetDroppedExploNadeGlow_Native, },
    { "DoD_GetDroppedExploNadeGlow", ::DoD_GetDroppedExploNadeGlow_Native, },

    { "DoD_AreAlliesBritish", ::DoD_AreAlliesBritish_Native, },
    { "DoD_AreAlliesParatroopers", ::DoD_AreAlliesParatroopers_Native, },
    { "DoD_AreAxisParatroopers", ::DoD_AreAxisParatroopers_Native, },
    { "DoD_HaveAlliesInfiniteLives", ::DoD_HaveAlliesInfiniteLives_Native, },
    { "DoD_HaveAxisInfiniteLives", ::DoD_HaveAxisInfiniteLives_Native, },
    { "DoD_GetAlliesRespawnFactor", ::DoD_GetAlliesRespawnFactor_Native, },
    { "DoD_GetAxisRespawnFactor", ::DoD_GetAxisRespawnFactor_Native, },

    { "DoD_ReadGameRulesBool", ::DoD_ReadGameRulesBool_Native, },
    { "DoD_ReadGameRulesFloat", ::DoD_ReadGameRulesFloat_Native, },
    { "DoD_ReadGameRulesInt", ::DoD_ReadGameRulesInt_Native, },
    { "DoD_ReadGameRulesStr", ::DoD_ReadGameRulesStr_Native, },

    { "DoD_AreGameRulesReady", ::DoD_AreGameRulesReady_Native, },

    { NULL, NULL, },
};

bool ReadConfig(bool ForLinux)
{
    char Buffer[256];
    ::g_fn_BuildPathnameR(Buffer, sizeof Buffer, "%s/dod_hacks_signatures.ini", ::g_fn_GetLocalInfo("amxx_configsdir", "addons/amxmodx/configs"));
#ifndef __linux__
    ::_iobuf* pConfig;
    ::fopen_s(&pConfig, Buffer, "r");
#else
    ::FILE* pConfig = ::fopen(Buffer, "r");
#endif
    if (!pConfig)
    {
        ::MF_Log("Unable to open '%s'!", Buffer);
        return false;
    }

    ::SourceHook::String Line;
    ::SignatureData SigData;
    bool Linux;
    while (::fgets(Buffer, sizeof Buffer, pConfig))
    {
        Line = Buffer;
        Line.trim();
        if (Line.empty() || ';' == Line[0] || '/' == Line[0])
        {
            continue;
        }
        else if (false == Line.icmp("[Linux]"))
        {
            Linux = true;
            continue;
        }
        else if (false == Line.icmp("[Windows]"))
        {
            Linux = false;
            continue;
        }
        if (Linux != ForLinux)
        {
            continue;
        }
        if (false == Line.icmpn("Sig:", 4))
        {
            SigData.IsSymbol = false;
            ::vectorizeSignature(Line, SigData.Signature);
        }
        else if (false == Line.icmpn("OffsDec:", 8))
        {
            Line.erase(false, 8);
            Line.trim();
            SigData.Offs = ::strtoull(Line.c_str(), NULL, int(10));
        }
        else if (false == Line.icmpn("OffsHex:", 8))
        {
            Line.erase(false, 8);
            Line.trim();
            SigData.Offs = ::strtoull(Line.c_str(), NULL, int(16));
        }
        else
        {
            SigData.IsSymbol = true;
            SigData.Symbol = Line;
        }
        ::g_Sigs.push_back(SigData);
    }
    ::fclose(pConfig);
    return true;
}

void OnAmxxAttach()
{
#ifndef __linux__
    if (!::ReadConfig(false))
    {
        ::g_fn_AddNatives(::DoDHacks_Natives);
        return;
    }

    bool Opened = false;
    auto pDoD = ::openLib("dod", Opened);
    if (!pDoD)
    {
        ::MF_Log("::GetModuleHandleA/ ::LoadLibraryA failed! Use with caution!");
        ::g_fn_AddNatives(::DoDHacks_Natives);
        return;
    }

    ::_MEMORY_BASIC_INFORMATION memInfo{ };
    if (false == ::VirtualQuery(pDoD, &memInfo, sizeof memInfo) || !memInfo.AllocationBase)
    {
        ::MF_Log("::VirtualQuery failed!");
        ::g_fn_AddNatives(::DoDHacks_Natives);
        if (Opened)
            ::FreeLibrary(pDoD);
        return;
    }

    ::size_t Addr = false;
    auto pImgDosHdr = (::_IMAGE_DOS_HEADER*)memInfo.AllocationBase;
    auto pImgNtHdr = (::_IMAGE_NT_HEADERS*)((::size_t)pImgDosHdr + (::size_t)pImgDosHdr->e_lfanew);

    if (::g_Sigs[::DoD_Sig::PlayerSpawn].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::PlayerSpawn].Signature, &Addr, true))
            ::g_pDoDPlayerSpawn_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_PlayerSpawn signature not found!");
    }
    else if (!(::g_pDoDPlayerSpawn_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::PlayerSpawn].Symbol.c_str())))
        ::MF_Log("::DoD_PlayerSpawn symbol not found!");

    if (::g_Sigs[::DoD_Sig::GiveNamedItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::GiveNamedItem].Signature, &Addr, true))
            ::g_pDoDGiveNamedItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GiveNamedItem signature not found!");
    }
    else if (!(::g_pDoDGiveNamedItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::GiveNamedItem].Symbol.c_str())))
        ::MF_Log("::DoD_GiveNamedItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::DropPlayerItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::DropPlayerItem].Signature, &Addr, true))
            ::g_pDoDDropPlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_DropPlayerItem signature not found!");
    }
    else if (!(::g_pDoDDropPlayerItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::DropPlayerItem].Symbol.c_str())))
        ::MF_Log("::DoD_DropPlayerItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::GiveAmmo].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::GiveAmmo].Signature, &Addr, true))
            ::g_pDoDGiveAmmo_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GiveAmmo signature not found!");
    }
    else if (!(::g_pDoDGiveAmmo_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::GiveAmmo].Symbol.c_str())))
        ::MF_Log("::DoD_GiveAmmo symbol not found!");

    if (::g_Sigs[::DoD_Sig::UtilRemove].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::UtilRemove].Signature, &Addr, true))
            ::g_pDoDUtilRemove_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_UtilRemove signature not found!");
    }
    else if (!(::g_pDoDUtilRemove_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::UtilRemove].Symbol.c_str())))
        ::MF_Log("::DoD_UtilRemove symbol not found!");

    if (::g_Sigs[::DoD_Sig::CreateNamedEntity].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::CreateNamedEntity].Signature, &Addr, true))
            ::g_pDoDCreateNamedEntity_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_CreateNamedEntity signature not found!");
    }
    else if (!(::g_pDoDCreateNamedEntity_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::CreateNamedEntity].Symbol.c_str())))
        ::MF_Log("::DoD_CreateNamedEntity symbol not found!");

    if (::g_Sigs[::DoD_Sig::SubRemove].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::SubRemove].Signature, &Addr, true))
            ::g_pDoDSubRemove_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SubRemove signature not found!");
    }
    else if (!(::g_pDoDSubRemove_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::SubRemove].Symbol.c_str())))
        ::MF_Log("::DoD_SubRemove symbol not found!");

    if (::g_Sigs[::DoD_Sig::WpnBoxKill].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::WpnBoxKill].Signature, &Addr, true))
            ::g_pDoDWpnBoxKill_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_WpnBoxKill signature not found!");
    }
    else if (!(::g_pDoDWpnBoxKill_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::WpnBoxKill].Symbol.c_str())))
        ::MF_Log("::DoD_WpnBoxKill symbol not found!");

    if (::g_Sigs[::DoD_Sig::WpnBoxActivateThink].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::WpnBoxActivateThink].Signature, &Addr, true))
            ::g_pDoDWpnBoxActivateThink_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_WpnBoxActivateThink signature not found!");
    }
    else if (!(::g_pDoDWpnBoxActivateThink_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::WpnBoxActivateThink].Symbol.c_str())))
        ::MF_Log("::DoD_WpnBoxActivateThink symbol not found!");

    if (::g_Sigs[::DoD_Sig::Create].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Create].Signature, &Addr, true))
            ::g_pDoDCreate_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_Create signature not found!");
    }
    else if (!(::g_pDoDCreate_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Create].Symbol.c_str())))
        ::MF_Log("::DoD_Create symbol not found!");

    if (::g_Sigs[::DoD_Sig::PackWeapon].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::PackWeapon].Signature, &Addr, true))
            ::g_pDoDPackWeapon_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_PackWeapon signature not found!");
    }
    else if (!(::g_pDoDPackWeapon_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::PackWeapon].Symbol.c_str())))
        ::MF_Log("::DoD_PackWeapon symbol not found!");

    if (::g_Sigs[::DoD_Sig::DestroyItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::DestroyItem].Signature, &Addr, true))
            ::g_pDoDDestroyItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_DestroyItem signature not found!");
    }
    else if (!(::g_pDoDDestroyItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::DestroyItem].Symbol.c_str())))
        ::MF_Log("::DoD_DestroyItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::SetWaveTime].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::SetWaveTime].Signature, &Addr, true))
            ::g_pDoDSetWaveTime_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SetWaveTime signature not found!");
    }
    else if (!(::g_pDoDSetWaveTime_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::SetWaveTime].Symbol.c_str())))
        ::MF_Log("::DoD_SetWaveTime symbol not found!");

    if (::g_Sigs[::DoD_Sig::GetWaveTime].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::GetWaveTime].Signature, &Addr, true))
            ::g_pDoDGetWaveTime_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GetWaveTime signature not found!");
    }
    else if (!(::g_pDoDGetWaveTime_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::GetWaveTime].Symbol.c_str())))
        ::MF_Log("::DoD_GetWaveTime symbol not found!");

    if (::g_Sigs[::DoD_Sig::RemovePlayerItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::RemovePlayerItem].Signature, &Addr, true))
            ::g_pDoDRemovePlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_RemovePlayerItem signature not found!");
    }
    else if (!(::g_pDoDRemovePlayerItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::RemovePlayerItem].Symbol.c_str())))
        ::MF_Log("::DoD_RemovePlayerItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::AddPlayerItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::AddPlayerItem].Signature, &Addr, true))
            ::g_pDoDAddPlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_AddPlayerItem signature not found!");
    }
    else if (!(::g_pDoDAddPlayerItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::AddPlayerItem].Symbol.c_str())))
        ::MF_Log("::DoD_AddPlayerItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::RemoveAllItems].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::RemoveAllItems].Signature, &Addr, true))
            ::g_pDoDRemoveAllItems_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_RemoveAllItems signature not found!");
    }
    else if (!(::g_pDoDRemoveAllItems_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::RemoveAllItems].Symbol.c_str())))
        ::MF_Log("::DoD_RemoveAllItems symbol not found!");

    if (::g_Sigs[::DoD_Sig::SetBodygroup].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::SetBodygroup].Signature, &Addr, true))
            ::g_pDoDSetBodygroup_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SetBodygroup signature not found!");
    }
    else if (!(::g_pDoDSetBodygroup_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::SetBodygroup].Symbol.c_str())))
        ::MF_Log("::DoD_SetBodygroup symbol not found!");

    if (::g_Sigs[::DoD_Sig::InstallGameRules].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::InstallGameRules].Signature, &Addr, true))
            ::g_pDoDInstallGameRules_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_InstallGameRules signature not found!");
    }
    else if (!(::g_pDoDInstallGameRules_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::InstallGameRules].Symbol.c_str())))
        ::MF_Log("::DoD_InstallGameRules symbol not found!");

    if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::PatchFg42].Signature, &Addr, true))
        ::g_pAutoScopeFG42Addr = (unsigned char*)Addr;
    else
        ::MF_Log("::DoD_PatchAutoScope(FG42) signature not found!");

    if (::findInMemory((unsigned char*)pImgDosHdr, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::PatchEnfield].Signature, &Addr, true))
        ::g_pAutoScopeEnfieldAddr = (unsigned char*)Addr;
    else
        ::MF_Log("::DoD_PatchAutoScope(Enfield) signature not found!");

    if (::g_pDoDInstallGameRules_Addr)
    {
        ::DoD_InstallGameRules = (::DoD_InstallGameRules_Type) ::g_pDoDInstallGameRules_Addr;
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourAttach(&(void*&) ::DoD_InstallGameRules, ::DoD_InstallGameRules_Hook);
        ::DetourTransactionCommit();
        ::g_DoDInstallGameRules_Hook = true;
    }

    if (Opened)
        ::FreeLibrary(pDoD);
#else
    if (!::ReadConfig(true))
    {
        ::g_fn_AddNatives(::DoDHacks_Natives);
        return;
    }
    /** Try opening with suffix initially, for compatibility (i.e. people trying to use outdated BOT plugins). */
    void* pDoD = ::openLib("dod", ::DoD_Suffix::i386);
    if (!pDoD)
    {
        pDoD = ::openLib("dod", ::DoD_Suffix::i486);
        if (!pDoD)
        {
            pDoD = ::openLib("dod", ::DoD_Suffix::i686);
            if (!pDoD)
            {
                pDoD = ::openLib("dod", ::DoD_Suffix::None);
                if (!pDoD)
                {
                    ::MF_Log("::dlopen failed! Use with caution!");
                    ::g_fn_AddNatives(::DoDHacks_Natives);
                    return;
                }
            }
        }
    }

    auto pLinkMap = (::link_map*)pDoD;
    struct ::stat memData;
    if (::stat(pLinkMap->l_name, &memData))
    {
        ::MF_Log("::stat failed!");
        ::dlclose(pDoD);
        ::g_fn_AddNatives(::DoDHacks_Natives);
        return;
    }

    ::size_t Addr = false;

    if (::g_Sigs[::DoD_Sig::PlayerSpawn].IsSymbol)
    {
        if (!(::g_pDoDPlayerSpawn_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::PlayerSpawn].Symbol.c_str())))
            ::MF_Log("::DoD_PlayerSpawn symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::PlayerSpawn].Signature, &Addr, true))
            ::g_pDoDPlayerSpawn_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_PlayerSpawn signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::GiveNamedItem].IsSymbol)
    {
        if (!(::g_pDoDGiveNamedItem_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::GiveNamedItem].Symbol.c_str())))
            ::MF_Log("::DoD_GiveNamedItem symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::GiveNamedItem].Signature, &Addr, true))
            ::g_pDoDGiveNamedItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GiveNamedItem signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::SubRemove].IsSymbol)
    {
        if (!(::g_pDoDSubRemove_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::SubRemove].Symbol.c_str())))
            ::MF_Log("::DoD_SubRemove symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::SubRemove].Signature, &Addr, true))
            ::g_pDoDSubRemove_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SubRemove signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::WpnBoxKill].IsSymbol)
    {
        if (!(::g_pDoDWpnBoxKill_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::WpnBoxKill].Symbol.c_str())))
            ::MF_Log("::DoD_WpnBoxKill symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::WpnBoxKill].Signature, &Addr, true))
            ::g_pDoDWpnBoxKill_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_WpnBoxKill signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::WpnBoxActivateThink].IsSymbol)
    {
        if (!(::g_pDoDWpnBoxActivateThink_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::WpnBoxActivateThink].Symbol.c_str())))
            ::MF_Log("::DoD_WpnBoxActivateThink symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::WpnBoxActivateThink].Signature, &Addr, true))
            ::g_pDoDWpnBoxActivateThink_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_WpnBoxActivateThink signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::Create].IsSymbol)
    {
        if (!(::g_pDoDCreate_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Create].Symbol.c_str())))
            ::MF_Log("::DoD_Create symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Create].Signature, &Addr, true))
            ::g_pDoDCreate_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_Create signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::PackWeapon].IsSymbol)
    {
        if (!(::g_pDoDPackWeapon_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::PackWeapon].Symbol.c_str())))
            ::MF_Log("::DoD_PackWeapon symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::PackWeapon].Signature, &Addr, true))
            ::g_pDoDPackWeapon_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_PackWeapon signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::UtilRemove].IsSymbol)
    {
        if (!(::g_pDoDUtilRemove_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::UtilRemove].Symbol.c_str())))
            ::MF_Log("::DoD_UtilRemove symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::UtilRemove].Signature, &Addr, true))
            ::g_pDoDUtilRemove_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_UtilRemove signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::CreateNamedEntity].IsSymbol)
    {
        if (!(::g_pDoDCreateNamedEntity_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::CreateNamedEntity].Symbol.c_str())))
            ::MF_Log("::DoD_CreateNamedEntity symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::CreateNamedEntity].Signature, &Addr, true))
            ::g_pDoDCreateNamedEntity_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_CreateNamedEntity signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::DestroyItem].IsSymbol)
    {
        if (!(::g_pDoDDestroyItem_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::DestroyItem].Symbol.c_str())))
            ::MF_Log("::DoD_DestroyItem symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::DestroyItem].Signature, &Addr, true))
            ::g_pDoDDestroyItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_DestroyItem signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::DropPlayerItem].IsSymbol)
    {
        if (!(::g_pDoDDropPlayerItem_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::DropPlayerItem].Symbol.c_str())))
            ::MF_Log("::DoD_DropPlayerItem symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::DropPlayerItem].Signature, &Addr, true))
            ::g_pDoDDropPlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_DropPlayerItem signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::GiveAmmo].IsSymbol)
    {
        if (!(::g_pDoDGiveAmmo_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::GiveAmmo].Symbol.c_str())))
            ::MF_Log("::DoD_GiveAmmo symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::GiveAmmo].Signature, &Addr, true))
            ::g_pDoDGiveAmmo_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GiveAmmo signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::SetWaveTime].IsSymbol)
    {
        if (!(::g_pDoDSetWaveTime_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::SetWaveTime].Symbol.c_str())))
            ::MF_Log("::DoD_SetWaveTime symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::SetWaveTime].Signature, &Addr, true))
            ::g_pDoDSetWaveTime_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SetWaveTime signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::GetWaveTime].IsSymbol)
    {
        if (!(::g_pDoDGetWaveTime_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::GetWaveTime].Symbol.c_str())))
            ::MF_Log("::DoD_GetWaveTime symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::GetWaveTime].Signature, &Addr, true))
            ::g_pDoDGetWaveTime_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GetWaveTime signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::RemovePlayerItem].IsSymbol)
    {
        if (!(::g_pDoDRemovePlayerItem_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::RemovePlayerItem].Symbol.c_str())))
            ::MF_Log("::DoD_RemovePlayerItem symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::RemovePlayerItem].Signature, &Addr, true))
            ::g_pDoDRemovePlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_RemovePlayerItem signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::AddPlayerItem].IsSymbol)
    {
        if (!(::g_pDoDAddPlayerItem_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::AddPlayerItem].Symbol.c_str())))
            ::MF_Log("::DoD_AddPlayerItem symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::AddPlayerItem].Signature, &Addr, true))
            ::g_pDoDAddPlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_AddPlayerItem signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::RemoveAllItems].IsSymbol)
    {
        if (!(::g_pDoDRemoveAllItems_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::RemoveAllItems].Symbol.c_str())))
            ::MF_Log("::DoD_RemoveAllItems symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::RemoveAllItems].Signature, &Addr, true))
            ::g_pDoDRemoveAllItems_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_RemoveAllItems signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::SetBodygroup].IsSymbol)
    {
        if (!(::g_pDoDSetBodygroup_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::SetBodygroup].Symbol.c_str())))
            ::MF_Log("::DoD_SetBodygroup symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::SetBodygroup].Signature, &Addr, true))
            ::g_pDoDSetBodygroup_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SetBodygroup signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::InstallGameRules].IsSymbol)
    {
        if (!(::g_pDoDInstallGameRules_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::InstallGameRules].Symbol.c_str())))
            ::MF_Log("::DoD_InstallGameRules symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::InstallGameRules].Signature, &Addr, true))
            ::g_pDoDInstallGameRules_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_InstallGameRules signature not found!");
    }

    if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::PatchFg42].Signature, &Addr, true))
        ::g_pAutoScopeFG42Addr = (unsigned char*)Addr;
    else
        ::MF_Log("::DoD_PatchAutoScope(FG42) signature not found!");

    if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::PatchEnfield].Signature, &Addr, true))
        ::g_pAutoScopeEnfieldAddr = (unsigned char*)Addr;
    else
        ::MF_Log("::DoD_PatchAutoScope(Enfield) signature not found!");

    if (::g_pDoDInstallGameRules_Addr)
    {
        ::g_pDoDInstallGameRules = ::subhook_new(::g_pDoDInstallGameRules_Addr, ::DoD_InstallGameRules_Hook, ::SUBHOOK_TRAMPOLINE);
        ::subhook_install(::g_pDoDInstallGameRules);
        ::DoD_InstallGameRules = (::DoD_InstallGameRules_Type) ::subhook_get_trampoline(::g_pDoDInstallGameRules);
        ::g_DoDInstallGameRules_Hook = true;
    }

    ::dlclose(pDoD);
#endif

    ::g_fn_AddNatives(::DoDHacks_Natives);
}

void OnAmxxDetach()
{
#ifndef __linux__
    if (::g_DoDPlayerSpawn_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_PlayerSpawn, ::DoD_PlayerSpawn_Hook);
        ::DetourTransactionCommit();
        ::g_DoDPlayerSpawn_Hook = false;
    }
    if (::g_DoDGiveNamedItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_GiveNamedItem, ::DoD_GiveNamedItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDGiveNamedItem_Hook = false;
    }
    if (::g_DoDDropPlayerItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_DropPlayerItem, ::DoD_DropPlayerItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDDropPlayerItem_Hook = false;
    }
    if (::g_DoDRemovePlayerItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_RemovePlayerItem, ::DoD_RemovePlayerItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDRemovePlayerItem_Hook = false;
    }
    if (::g_DoDAddPlayerItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_AddPlayerItem, ::DoD_AddPlayerItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDAddPlayerItem_Hook = false;
    }
    if (::g_DoDRemoveAllItems_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_RemoveAllItems, ::DoD_RemoveAllItems_Hook);
        ::DetourTransactionCommit();
        ::g_DoDRemoveAllItems_Hook = false;
    }
    if (::g_DoDSetWaveTime_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_SetWaveTime, ::DoD_SetWaveTime_Hook);
        ::DetourTransactionCommit();
        ::g_DoDSetWaveTime_Hook = false;
    }
    if (::g_DoDGetWaveTime_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_GetWaveTime, ::DoD_GetWaveTime_Hook);
        ::DetourTransactionCommit();
        ::g_DoDGetWaveTime_Hook = false;
    }
    if (::g_DoDGiveAmmo_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_GiveAmmo, ::DoD_GiveAmmo_Hook);
        ::DetourTransactionCommit();
        ::g_DoDGiveAmmo_Hook = false;
    }
    if (::g_DoDSetBodygroup_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_SetBodygroup, ::DoD_SetBodygroup_Hook);
        ::DetourTransactionCommit();
        ::g_DoDSetBodygroup_Hook = false;
    }
    if (::g_DoDInstallGameRules_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_InstallGameRules, ::DoD_InstallGameRules_Hook);
        ::DetourTransactionCommit();
        ::g_DoDInstallGameRules_Hook = false;
    }
    if (::g_DoDUtilRemove_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_UtilRemove, ::DoD_UtilRemove_Hook);
        ::DetourTransactionCommit();
        ::g_DoDUtilRemove_Hook = false;
    }
    if (::g_DoDCreateNamedEntity_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_CreateNamedEntity, ::DoD_CreateNamedEntity_Hook);
        ::DetourTransactionCommit();
        ::g_DoDCreateNamedEntity_Hook = false;
    }
    if (::g_DoDSubRemove_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_SubRemove, ::DoD_SubRemove_Hook);
        ::DetourTransactionCommit();
        ::g_DoDSubRemove_Hook = false;
    }
    if (::g_DoDWpnBoxKill_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_WpnBoxKill, ::DoD_WpnBoxKill_Hook);
        ::DetourTransactionCommit();
        ::g_DoDWpnBoxKill_Hook = false;
    }
    if (::g_DoDWpnBoxActivateThink_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_WpnBoxActivateThink, ::DoD_WpnBoxActivateThink_Hook);
        ::DetourTransactionCommit();
        ::g_DoDWpnBoxActivateThink_Hook = false;
    }
    if (::g_DoDCreate_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_Create, ::DoD_Create_Hook);
        ::DetourTransactionCommit();
        ::g_DoDCreate_Hook = false;
    }
    if (::g_DoDPackWeapon_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_PackWeapon, ::DoD_PackWeapon_Hook);
        ::DetourTransactionCommit();
        ::g_DoDPackWeapon_Hook = false;
    }
    if (::g_DoDDestroyItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_DestroyItem, ::DoD_DestroyItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDDestroyItem_Hook = false;
    }
#else
    if (::g_DoDPlayerSpawn_Hook)
    {
        ::subhook_remove(::g_pDoDPlayerSpawn);
        ::subhook_free(::g_pDoDPlayerSpawn);
        ::g_DoDPlayerSpawn_Hook = false;
    }
    if (::g_DoDGiveNamedItem_Hook)
    {
        ::subhook_remove(::g_pDoDGiveNamedItem);
        ::subhook_free(::g_pDoDGiveNamedItem);
        ::g_DoDGiveNamedItem_Hook = false;
    }
    if (::g_DoDDropPlayerItem_Hook)
    {
        ::subhook_remove(::g_pDoDDropPlayerItem);
        ::subhook_free(::g_pDoDDropPlayerItem);
        ::g_DoDDropPlayerItem_Hook = false;
    }
    if (::g_DoDRemovePlayerItem_Hook)
    {
        ::subhook_remove(::g_pDoDRemovePlayerItem);
        ::subhook_free(::g_pDoDRemovePlayerItem);
        ::g_DoDRemovePlayerItem_Hook = false;
    }
    if (::g_DoDAddPlayerItem_Hook)
    {
        ::subhook_remove(::g_pDoDAddPlayerItem);
        ::subhook_free(::g_pDoDAddPlayerItem);
        ::g_DoDAddPlayerItem_Hook = false;
    }
    if (::g_DoDRemoveAllItems_Hook)
    {
        ::subhook_remove(::g_pDoDRemoveAllItems);
        ::subhook_free(::g_pDoDRemoveAllItems);
        ::g_DoDRemoveAllItems_Hook = false;
    }
    if (::g_DoDSetWaveTime_Hook)
    {
        ::subhook_remove(::g_pDoDSetWaveTime);
        ::subhook_free(::g_pDoDSetWaveTime);
        ::g_DoDSetWaveTime_Hook = false;
    }
    if (::g_DoDGetWaveTime_Hook)
    {
        ::subhook_remove(::g_pDoDGetWaveTime);
        ::subhook_free(::g_pDoDGetWaveTime);
        ::g_DoDGetWaveTime_Hook = false;
    }
    if (::g_DoDGiveAmmo_Hook)
    {
        ::subhook_remove(::g_pDoDGiveAmmo);
        ::subhook_free(::g_pDoDGiveAmmo);
        ::g_DoDGiveAmmo_Hook = false;
    }
    if (::g_DoDSetBodygroup_Hook)
    {
        ::subhook_remove(::g_pDoDSetBodygroup);
        ::subhook_free(::g_pDoDSetBodygroup);
        ::g_DoDSetBodygroup_Hook = false;
    }
    if (::g_DoDInstallGameRules_Hook)
    {
        ::subhook_remove(::g_pDoDInstallGameRules);
        ::subhook_free(::g_pDoDInstallGameRules);
        ::g_DoDInstallGameRules_Hook = false;
    }
    if (::g_DoDDestroyItem_Hook)
    {
        ::subhook_remove(::g_pDoDDestroyItem);
        ::subhook_free(::g_pDoDDestroyItem);
        ::g_DoDDestroyItem_Hook = false;
    }
    if (::g_DoDSubRemove_Hook)
    {
        ::subhook_remove(::g_pDoDSubRemove);
        ::subhook_free(::g_pDoDSubRemove);
        ::g_DoDSubRemove_Hook = false;
    }
    if (::g_DoDWpnBoxKill_Hook)
    {
        ::subhook_remove(::g_pDoDWpnBoxKill);
        ::subhook_free(::g_pDoDWpnBoxKill);
        ::g_DoDWpnBoxKill_Hook = false;
    }
    if (::g_DoDWpnBoxActivateThink_Hook)
    {
        ::subhook_remove(::g_pDoDWpnBoxActivateThink);
        ::subhook_free(::g_pDoDWpnBoxActivateThink);
        ::g_DoDWpnBoxActivateThink_Hook = false;
    }
    if (::g_DoDCreate_Hook)
    {
        ::subhook_remove(::g_pDoDCreate);
        ::subhook_free(::g_pDoDCreate);
        ::g_DoDCreate_Hook = false;
    }
    if (::g_DoDPackWeapon_Hook)
    {
        ::subhook_remove(::g_pDoDPackWeapon);
        ::subhook_free(::g_pDoDPackWeapon);
        ::g_DoDPackWeapon_Hook = false;
    }
    if (::g_DoDUtilRemove_Hook)
    {
        ::subhook_remove(::g_pDoDUtilRemove);
        ::subhook_free(::g_pDoDUtilRemove);
        ::g_DoDUtilRemove_Hook = false;
    }
    if (::g_DoDCreateNamedEntity_Hook)
    {
        ::subhook_remove(::g_pDoDCreateNamedEntity);
        ::subhook_free(::g_pDoDCreateNamedEntity);
        ::g_DoDCreateNamedEntity_Hook = false;
    }
#endif
    if (::g_pAutoScopeFG42Addr)
    {
        ::allowFullMemAccess(::g_pAutoScopeFG42Addr, true);
        *::g_pAutoScopeFG42Addr = ::g_Sigs[::DoD_Sig::OrigFg42_Byte].Signature[0];
    }
    if (::g_pAutoScopeEnfieldAddr)
    {
        ::allowFullMemAccess(::g_pAutoScopeEnfieldAddr, true);
        *::g_pAutoScopeEnfieldAddr = ::g_Sigs[::DoD_Sig::OrigEnfield_Byte].Signature[0];
    }
    ::g_pAutoScopeFG42Addr = NULL;
    ::g_pAutoScopeEnfieldAddr = NULL;

    ::g_pDoDPlayerSpawn_Addr = NULL;
    ::g_pDoDGiveNamedItem_Addr = NULL;
    ::g_pDoDDropPlayerItem_Addr = NULL;
    ::g_pDoDGiveAmmo_Addr = NULL;
    ::g_pDoDSetWaveTime_Addr = NULL;
    ::g_pDoDGetWaveTime_Addr = NULL;
    ::g_pDoDRemovePlayerItem_Addr = NULL;
    ::g_pDoDAddPlayerItem_Addr = NULL;
    ::g_pDoDRemoveAllItems_Addr = NULL;
    ::g_pDoDSetBodygroup_Addr = NULL;
    ::g_pDoDDestroyItem_Addr = NULL;
    ::g_pDoDSubRemove_Addr = NULL;
    ::g_pDoDPackWeapon_Addr = NULL;
    ::g_pDoDInstallGameRules_Addr = NULL;
    ::g_pDoDUtilRemove_Addr = NULL;
    ::g_pDoDCreateNamedEntity_Addr = NULL;
    ::g_pDoDCreate_Addr = NULL;
    ::g_pDoDWpnBoxActivateThink_Addr = NULL;
    ::g_pDoDWpnBoxKill_Addr = NULL;

    ::g_Sigs.clear();
}

void OnPluginsLoaded()
{
    ::g_fwPlayerSpawn = ::g_fn_RegisterForward("DoD_OnPlayerSpawn", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwGiveNamedItem = ::g_fn_RegisterForward("DoD_OnGiveNamedItem", ::ET_STOP, ::FP_CELL, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwDropPlayerItem = ::g_fn_RegisterForward("DoD_OnDropPlayerItem", ::ET_STOP, ::FP_CELL, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwAddPlayerItem = ::g_fn_RegisterForward("DoD_OnAddPlayerItem", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwRemovePlayerItem = ::g_fn_RegisterForward("DoD_OnRemovePlayerItem", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwRemoveAllItems = ::g_fn_RegisterForward("DoD_OnRemoveAllItems", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwGiveAmmo = ::g_fn_RegisterForward("DoD_OnGiveAmmo", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwGetWaveTime = ::g_fn_RegisterForward("DoD_OnGetWaveTime", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_FLOAT_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwSetWaveTime = ::g_fn_RegisterForward("DoD_OnSetWaveTime", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_FLOAT_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwSetBodygroup = ::g_fn_RegisterForward("DoD_OnSetBodygroup", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwInstallGameRules = ::g_fn_RegisterForward("DoD_OnInstallGameRules", ::ET_STOP, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwSubRemove = ::g_fn_RegisterForward("DoD_OnSubRemove", ::ET_STOP, ::FP_CELL, ::FP_DONE);
    ::g_fwUtilRemove = ::g_fn_RegisterForward("DoD_OnUtilRemove", ::ET_STOP, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwDestroyItem = ::g_fn_RegisterForward("DoD_OnDestroyItem", ::ET_STOP, ::FP_CELL, ::FP_DONE);
    ::g_fwPackWeapon = ::g_fn_RegisterForward("DoD_OnPackWeapon", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwWpnBoxKill = ::g_fn_RegisterForward("DoD_OnWpnBoxKill", ::ET_STOP, ::FP_CELL, ::FP_DONE);
    ::g_fwWpnBoxActivateThink = ::g_fn_RegisterForward("DoD_OnWpnBoxActivateThink", ::ET_STOP, ::FP_CELL, ::FP_DONE);
    ::g_fwCreate = ::g_fn_RegisterForward("DoD_OnCreate", ::ET_STOP, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_ARRAY, ::FP_ARRAY, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwCreateNamedEntity = ::g_fn_RegisterForward("DoD_OnCreateNamedEntity", ::ET_STOP, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);

    ::g_fwPlayerSpawn_Post = ::g_fn_RegisterForward("DoD_OnPlayerSpawn_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwGiveNamedItem_Post = ::g_fn_RegisterForward("DoD_OnGiveNamedItem_Post", ::ET_IGNORE, ::FP_CELL, ::FP_STRING, ::FP_CELL, ::FP_DONE);
    ::g_fwDropPlayerItem_Post = ::g_fn_RegisterForward("DoD_OnDropPlayerItem_Post", ::ET_IGNORE, ::FP_CELL, ::FP_STRING, ::FP_CELL, ::FP_DONE);
    ::g_fwAddPlayerItem_Post = ::g_fn_RegisterForward("DoD_OnAddPlayerItem_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwRemovePlayerItem_Post = ::g_fn_RegisterForward("DoD_OnRemovePlayerItem_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwRemoveAllItems_Post = ::g_fn_RegisterForward("DoD_OnRemoveAllItems_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwGiveAmmo_Post = ::g_fn_RegisterForward("DoD_OnGiveAmmo_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_STRING, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwGetWaveTime_Post = ::g_fn_RegisterForward("DoD_OnGetWaveTime_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_FLOAT, ::FP_DONE);
    ::g_fwSetWaveTime_Post = ::g_fn_RegisterForward("DoD_OnSetWaveTime_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_FLOAT, ::FP_DONE);
    ::g_fwSetBodygroup_Post = ::g_fn_RegisterForward("DoD_OnSetBodygroup_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwInstallGameRules_Post = ::g_fn_RegisterForward("DoD_OnInstallGameRules_Post", ::ET_IGNORE, ::FP_CELL, ::FP_DONE);
    ::g_fwSubRemove_Post = ::g_fn_RegisterForward("DoD_OnSubRemove_Post", ::ET_IGNORE, ::FP_CELL, ::FP_DONE);
    ::g_fwUtilRemove_Post = ::g_fn_RegisterForward("DoD_OnUtilRemove_Post", ::ET_IGNORE, ::FP_CELL, ::FP_DONE);
    ::g_fwDestroyItem_Post = ::g_fn_RegisterForward("DoD_OnDestroyItem_Post", ::ET_IGNORE, ::FP_CELL, ::FP_DONE);
    ::g_fwPackWeapon_Post = ::g_fn_RegisterForward("DoD_OnPackWeapon_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwWpnBoxKill_Post = ::g_fn_RegisterForward("DoD_OnWpnBoxKill_Post", ::ET_IGNORE, ::FP_CELL, ::FP_DONE);
    ::g_fwWpnBoxActivateThink_Post = ::g_fn_RegisterForward("DoD_OnWpnBoxActivateThink_Post", ::ET_IGNORE, ::FP_CELL, ::FP_DONE);
    ::g_fwCreate_Post = ::g_fn_RegisterForward("DoD_OnCreate_Post", ::ET_IGNORE, ::FP_STRING, ::FP_ARRAY, ::FP_ARRAY, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwCreateNamedEntity_Post = ::g_fn_RegisterForward("DoD_OnCreateNamedEntity_Post", ::ET_IGNORE, ::FP_STRING, ::FP_CELL, ::FP_DONE);

    ::tagAMX* pAmx;
    int Func, Iter = false;
    while (pAmx = ::g_fn_GetAmxScript(Iter++))
    {
#ifndef __linux__
        if (::g_pDoDPlayerSpawn_Addr && false == ::g_DoDPlayerSpawn_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnPlayerSpawn", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnPlayerSpawn_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_PlayerSpawn = (::DoD_PlayerSpawn_Type) ::g_pDoDPlayerSpawn_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_PlayerSpawn, ::DoD_PlayerSpawn_Hook);
            ::DetourTransactionCommit();
            ::g_DoDPlayerSpawn_Hook = true;
        }

        if (::g_pDoDGiveNamedItem_Addr && false == ::g_DoDGiveNamedItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnGiveNamedItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnGiveNamedItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_GiveNamedItem = (::DoD_GiveNamedItem_Type) ::g_pDoDGiveNamedItem_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_GiveNamedItem, ::DoD_GiveNamedItem_Hook);
            ::DetourTransactionCommit();
            ::g_DoDGiveNamedItem_Hook = true;
        }

        if (::g_pDoDDropPlayerItem_Addr && false == ::g_DoDDropPlayerItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnDropPlayerItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnDropPlayerItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_DropPlayerItem = (::DoD_DropPlayerItem_Type) ::g_pDoDDropPlayerItem_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_DropPlayerItem, ::DoD_DropPlayerItem_Hook);
            ::DetourTransactionCommit();
            ::g_DoDDropPlayerItem_Hook = true;
        }

        if (::g_pDoDAddPlayerItem_Addr && false == ::g_DoDAddPlayerItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnAddPlayerItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnAddPlayerItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_AddPlayerItem = (::DoD_AddPlayerItem_Type) ::g_pDoDAddPlayerItem_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_AddPlayerItem, ::DoD_AddPlayerItem_Hook);
            ::DetourTransactionCommit();
            ::g_DoDAddPlayerItem_Hook = true;
        }

        if (::g_pDoDRemovePlayerItem_Addr && false == ::g_DoDRemovePlayerItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnRemovePlayerItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnRemovePlayerItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_RemovePlayerItem = (::DoD_RemovePlayerItem_Type) ::g_pDoDRemovePlayerItem_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_RemovePlayerItem, ::DoD_RemovePlayerItem_Hook);
            ::DetourTransactionCommit();
            ::g_DoDRemovePlayerItem_Hook = true;
        }

        if (::g_pDoDRemoveAllItems_Addr && false == ::g_DoDRemoveAllItems_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnRemoveAllItems", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnRemoveAllItems_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_RemoveAllItems = (::DoD_RemoveAllItems_Type) ::g_pDoDRemoveAllItems_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_RemoveAllItems, ::DoD_RemoveAllItems_Hook);
            ::DetourTransactionCommit();
            ::g_DoDRemoveAllItems_Hook = true;
        }

        if (::g_pDoDGiveAmmo_Addr && false == ::g_DoDGiveAmmo_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnGiveAmmo", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnGiveAmmo_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_GiveAmmo = (::DoD_GiveAmmo_Type) ::g_pDoDGiveAmmo_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_GiveAmmo, ::DoD_GiveAmmo_Hook);
            ::DetourTransactionCommit();
            ::g_DoDGiveAmmo_Hook = true;
        }

        if (::g_pDoDGetWaveTime_Addr && false == ::g_DoDGetWaveTime_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnGetWaveTime", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnGetWaveTime_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_GetWaveTime = (::DoD_GetWaveTime_Type) ::g_pDoDGetWaveTime_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_GetWaveTime, ::DoD_GetWaveTime_Hook);
            ::DetourTransactionCommit();
            ::g_DoDGetWaveTime_Hook = true;
        }

        if (::g_pDoDSetWaveTime_Addr && false == ::g_DoDSetWaveTime_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnSetWaveTime", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnSetWaveTime_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_SetWaveTime = (::DoD_SetWaveTime_Type) ::g_pDoDSetWaveTime_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_SetWaveTime, ::DoD_SetWaveTime_Hook);
            ::DetourTransactionCommit();
            ::g_DoDSetWaveTime_Hook = true;
        }

        if (::g_pDoDSetBodygroup_Addr && false == ::g_DoDSetBodygroup_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnSetBodygroup", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnSetBodygroup_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_SetBodygroup = (::DoD_SetBodygroup_Type) ::g_pDoDSetBodygroup_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_SetBodygroup, ::DoD_SetBodygroup_Hook);
            ::DetourTransactionCommit();
            ::g_DoDSetBodygroup_Hook = true;
        }

        if (::g_pDoDSubRemove_Addr && false == ::g_DoDSubRemove_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnSubRemove", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnSubRemove_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_SubRemove = (::DoD_SubRemove_Type) ::g_pDoDSubRemove_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_SubRemove, ::DoD_SubRemove_Hook);
            ::DetourTransactionCommit();
            ::g_DoDSubRemove_Hook = true;
        }

        if (::g_pDoDWpnBoxKill_Addr && false == ::g_DoDWpnBoxKill_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnWpnBoxKill", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnWpnBoxKill_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_WpnBoxKill = (::DoD_WpnBoxKill_Type) ::g_pDoDWpnBoxKill_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_WpnBoxKill, ::DoD_WpnBoxKill_Hook);
            ::DetourTransactionCommit();
            ::g_DoDWpnBoxKill_Hook = true;
        }

        if (::g_pDoDWpnBoxActivateThink_Addr && false == ::g_DoDWpnBoxActivateThink_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnWpnBoxActivateThink", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnWpnBoxActivateThink_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_WpnBoxActivateThink = (::DoD_WpnBoxActivateThink_Type) ::g_pDoDWpnBoxActivateThink_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_WpnBoxActivateThink, ::DoD_WpnBoxActivateThink_Hook);
            ::DetourTransactionCommit();
            ::g_DoDWpnBoxActivateThink_Hook = true;
        }

        if (::g_pDoDCreate_Addr && false == ::g_DoDCreate_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnCreate", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnCreate_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_Create = (::DoD_Create_Type) ::g_pDoDCreate_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_Create, ::DoD_Create_Hook);
            ::DetourTransactionCommit();
            ::g_DoDCreate_Hook = true;
        }

        if (::g_pDoDPackWeapon_Addr && false == ::g_DoDPackWeapon_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnPackWeapon", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnPackWeapon_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_PackWeapon = (::DoD_PackWeapon_Type) ::g_pDoDPackWeapon_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_PackWeapon, ::DoD_PackWeapon_Hook);
            ::DetourTransactionCommit();
            ::g_DoDPackWeapon_Hook = true;
        }

        if (::g_pDoDUtilRemove_Addr && false == ::g_DoDUtilRemove_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnUtilRemove", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnUtilRemove_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_UtilRemove = (::DoD_UtilRemove_Type) ::g_pDoDUtilRemove_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_UtilRemove, ::DoD_UtilRemove_Hook);
            ::DetourTransactionCommit();
            ::g_DoDUtilRemove_Hook = true;
        }

        if (::g_pDoDCreateNamedEntity_Addr && false == ::g_DoDCreateNamedEntity_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnCreateNamedEntity", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnCreateNamedEntity_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_CreateNamedEntity = (::DoD_CreateNamedEntity_Type) ::g_pDoDCreateNamedEntity_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_CreateNamedEntity, ::DoD_CreateNamedEntity_Hook);
            ::DetourTransactionCommit();
            ::g_DoDCreateNamedEntity_Hook = true;
        }

        if (::g_pDoDDestroyItem_Addr && false == ::g_DoDDestroyItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnDestroyItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnDestroyItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_DestroyItem = (::DoD_DestroyItem_Type) ::g_pDoDDestroyItem_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_DestroyItem, ::DoD_DestroyItem_Hook);
            ::DetourTransactionCommit();
            ::g_DoDDestroyItem_Hook = true;
        }
#else
        if (::g_pDoDPlayerSpawn_Addr && false == ::g_DoDPlayerSpawn_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnPlayerSpawn", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnPlayerSpawn_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDPlayerSpawn = ::subhook_new(::g_pDoDPlayerSpawn_Addr, ::DoD_PlayerSpawn_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDPlayerSpawn);
            ::DoD_PlayerSpawn = (::DoD_PlayerSpawn_Type) ::subhook_get_trampoline(::g_pDoDPlayerSpawn);
            ::g_DoDPlayerSpawn_Hook = true;
        }

        if (::g_pDoDGiveNamedItem_Addr && false == ::g_DoDGiveNamedItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnGiveNamedItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnGiveNamedItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDGiveNamedItem = ::subhook_new(::g_pDoDGiveNamedItem_Addr, ::DoD_GiveNamedItem_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDGiveNamedItem);
            ::DoD_GiveNamedItem = (::DoD_GiveNamedItem_Type) ::subhook_get_trampoline(::g_pDoDGiveNamedItem);
            ::g_DoDGiveNamedItem_Hook = true;
        }

        if (::g_pDoDDropPlayerItem_Addr && false == ::g_DoDDropPlayerItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnDropPlayerItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnDropPlayerItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDDropPlayerItem = ::subhook_new(::g_pDoDDropPlayerItem_Addr, ::DoD_DropPlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDDropPlayerItem);
            ::DoD_DropPlayerItem = (::DoD_DropPlayerItem_Type) ::subhook_get_trampoline(::g_pDoDDropPlayerItem);
            ::g_DoDDropPlayerItem_Hook = true;
        }

        if (::g_pDoDAddPlayerItem_Addr && false == ::g_DoDAddPlayerItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnAddPlayerItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnAddPlayerItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDAddPlayerItem = ::subhook_new(::g_pDoDAddPlayerItem_Addr, ::DoD_AddPlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDAddPlayerItem);
            ::DoD_AddPlayerItem = (::DoD_AddPlayerItem_Type) ::subhook_get_trampoline(::g_pDoDAddPlayerItem);
            ::g_DoDAddPlayerItem_Hook = true;
        }

        if (::g_pDoDRemovePlayerItem_Addr && false == ::g_DoDRemovePlayerItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnRemovePlayerItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnRemovePlayerItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDRemovePlayerItem = ::subhook_new(::g_pDoDRemovePlayerItem_Addr, ::DoD_RemovePlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDRemovePlayerItem);
            ::DoD_RemovePlayerItem = (::DoD_RemovePlayerItem_Type) ::subhook_get_trampoline(::g_pDoDRemovePlayerItem);
            ::g_DoDRemovePlayerItem_Hook = true;
        }

        if (::g_pDoDRemoveAllItems_Addr && false == ::g_DoDRemoveAllItems_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnRemoveAllItems", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnRemoveAllItems_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDRemoveAllItems = ::subhook_new(::g_pDoDRemoveAllItems_Addr, ::DoD_RemoveAllItems_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDRemoveAllItems);
            ::DoD_RemoveAllItems = (::DoD_RemoveAllItems_Type) ::subhook_get_trampoline(::g_pDoDRemoveAllItems);
            ::g_DoDRemoveAllItems_Hook = true;
        }

        if (::g_pDoDGiveAmmo_Addr && false == ::g_DoDGiveAmmo_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnGiveAmmo", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnGiveAmmo_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDGiveAmmo = ::subhook_new(::g_pDoDGiveAmmo_Addr, ::DoD_GiveAmmo_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDGiveAmmo);
            ::DoD_GiveAmmo = (::DoD_GiveAmmo_Type) ::subhook_get_trampoline(::g_pDoDGiveAmmo);
            ::g_DoDGiveAmmo_Hook = true;
        }

        if (::g_pDoDGetWaveTime_Addr && false == ::g_DoDGetWaveTime_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnGetWaveTime", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnGetWaveTime_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDGetWaveTime = ::subhook_new(::g_pDoDGetWaveTime_Addr, ::DoD_GetWaveTime_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDGetWaveTime);
            ::DoD_GetWaveTime = (::DoD_GetWaveTime_Type) ::subhook_get_trampoline(::g_pDoDGetWaveTime);
            ::g_DoDGetWaveTime_Hook = true;
        }

        if (::g_pDoDSetWaveTime_Addr && false == ::g_DoDSetWaveTime_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnSetWaveTime", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnSetWaveTime_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDSetWaveTime = ::subhook_new(::g_pDoDSetWaveTime_Addr, ::DoD_SetWaveTime_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDSetWaveTime);
            ::DoD_SetWaveTime = (::DoD_SetWaveTime_Type) ::subhook_get_trampoline(::g_pDoDSetWaveTime);
            ::g_DoDSetWaveTime_Hook = true;
        }

        if (::g_pDoDSetBodygroup_Addr && false == ::g_DoDSetBodygroup_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnSetBodygroup", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnSetBodygroup_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDSetBodygroup = ::subhook_new(::g_pDoDSetBodygroup_Addr, ::DoD_SetBodygroup_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDSetBodygroup);
            ::DoD_SetBodygroup = (::DoD_SetBodygroup_Type) ::subhook_get_trampoline(::g_pDoDSetBodygroup);
            ::g_DoDSetBodygroup_Hook = true;
        }

        if (::g_pDoDSubRemove_Addr && false == ::g_DoDSubRemove_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnSubRemove", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnSubRemove_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDSubRemove = ::subhook_new(::g_pDoDSubRemove_Addr, ::DoD_SubRemove_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDSubRemove);
            ::DoD_SubRemove = (::DoD_SubRemove_Type) ::subhook_get_trampoline(::g_pDoDSubRemove);
            ::g_DoDSubRemove_Hook = true;
        }

        if (::g_pDoDCreate_Addr && false == ::g_DoDCreate_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnCreate", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnCreate_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDCreate = ::subhook_new(::g_pDoDCreate_Addr, ::DoD_Create_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDCreate);
            ::DoD_Create = (::DoD_Create_Type) ::subhook_get_trampoline(::g_pDoDCreate);
            ::g_DoDCreate_Hook = true;
        }

        if (::g_pDoDWpnBoxKill_Addr && false == ::g_DoDWpnBoxKill_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnWpnBoxKill", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnWpnBoxKill_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDWpnBoxKill = ::subhook_new(::g_pDoDWpnBoxKill_Addr, ::DoD_WpnBoxKill_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDWpnBoxKill);
            ::DoD_WpnBoxKill = (::DoD_WpnBoxKill_Type) ::subhook_get_trampoline(::g_pDoDWpnBoxKill);
            ::g_DoDWpnBoxKill_Hook = true;
        }

        if (::g_pDoDWpnBoxActivateThink_Addr && false == ::g_DoDWpnBoxActivateThink_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnWpnBoxActivateThink", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnWpnBoxActivateThink_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDWpnBoxActivateThink = ::subhook_new(::g_pDoDWpnBoxActivateThink_Addr, ::DoD_WpnBoxActivateThink_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDWpnBoxActivateThink);
            ::DoD_WpnBoxActivateThink = (::DoD_WpnBoxActivateThink_Type) ::subhook_get_trampoline(::g_pDoDWpnBoxActivateThink);
            ::g_DoDWpnBoxActivateThink_Hook = true;
        }

        if (::g_pDoDPackWeapon_Addr && false == ::g_DoDPackWeapon_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnPackWeapon", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnPackWeapon_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDPackWeapon = ::subhook_new(::g_pDoDPackWeapon_Addr, ::DoD_PackWeapon_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDPackWeapon);
            ::DoD_PackWeapon = (::DoD_PackWeapon_Type) ::subhook_get_trampoline(::g_pDoDPackWeapon);
            ::g_DoDPackWeapon_Hook = true;
        }

        if (::g_pDoDUtilRemove_Addr && false == ::g_DoDUtilRemove_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnUtilRemove", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnUtilRemove_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDUtilRemove = ::subhook_new(::g_pDoDUtilRemove_Addr, ::DoD_UtilRemove_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDUtilRemove);
            ::DoD_UtilRemove = (::DoD_UtilRemove_Type) ::subhook_get_trampoline(::g_pDoDUtilRemove);
            ::g_DoDUtilRemove_Hook = true;
        }

        if (::g_pDoDCreateNamedEntity_Addr && false == ::g_DoDCreateNamedEntity_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnCreateNamedEntity", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnCreateNamedEntity_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDCreateNamedEntity = ::subhook_new(::g_pDoDCreateNamedEntity_Addr, ::DoD_CreateNamedEntity_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDCreateNamedEntity);
            ::DoD_CreateNamedEntity = (::DoD_CreateNamedEntity_Type) ::subhook_get_trampoline(::g_pDoDCreateNamedEntity);
            ::g_DoDCreateNamedEntity_Hook = true;
        }

        if (::g_pDoDDestroyItem_Addr && false == ::g_DoDDestroyItem_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnDestroyItem", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnDestroyItem_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDDestroyItem = ::subhook_new(::g_pDoDDestroyItem_Addr, ::DoD_DestroyItem_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDDestroyItem);
            ::DoD_DestroyItem = (::DoD_DestroyItem_Type) ::subhook_get_trampoline(::g_pDoDDestroyItem);
            ::g_DoDDestroyItem_Hook = true;
        }
#endif
    }
}

void OnPluginsUnloaded()
{
    ::g_selfNade = false;
    ::g_droppedNade = false;
    ::g_pFunctionTable_Post->pfnAddToFullPack = NULL;
    ::g_Strings.clear();
    ::g_CustomKeyValues_Add.clear();
    ::g_CustomKeyValues_Del.clear();
    ::memset(&::g_exclFromExploNadeProjGlow, false, sizeof(::g_exclFromExploNadeProjGlow));
    ::memset(&::g_exclFromDroppedExploNadeGlow, false, sizeof(::g_exclFromDroppedExploNadeGlow));
#ifndef __linux__
    if (::g_DoDPlayerSpawn_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_PlayerSpawn, ::DoD_PlayerSpawn_Hook);
        ::DetourTransactionCommit();
        ::g_DoDPlayerSpawn_Hook = false;
    }
    if (::g_DoDGiveNamedItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_GiveNamedItem, ::DoD_GiveNamedItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDGiveNamedItem_Hook = false;
    }
    if (::g_DoDDropPlayerItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_DropPlayerItem, ::DoD_DropPlayerItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDDropPlayerItem_Hook = false;
    }
    if (::g_DoDRemovePlayerItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_RemovePlayerItem, ::DoD_RemovePlayerItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDRemovePlayerItem_Hook = false;
    }
    if (::g_DoDAddPlayerItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_AddPlayerItem, ::DoD_AddPlayerItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDAddPlayerItem_Hook = false;
    }
    if (::g_DoDRemoveAllItems_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_RemoveAllItems, ::DoD_RemoveAllItems_Hook);
        ::DetourTransactionCommit();
        ::g_DoDRemoveAllItems_Hook = false;
    }
    if (::g_DoDSetWaveTime_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_SetWaveTime, ::DoD_SetWaveTime_Hook);
        ::DetourTransactionCommit();
        ::g_DoDSetWaveTime_Hook = false;
    }
    if (::g_DoDGetWaveTime_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_GetWaveTime, ::DoD_GetWaveTime_Hook);
        ::DetourTransactionCommit();
        ::g_DoDGetWaveTime_Hook = false;
    }
    if (::g_DoDGiveAmmo_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_GiveAmmo, ::DoD_GiveAmmo_Hook);
        ::DetourTransactionCommit();
        ::g_DoDGiveAmmo_Hook = false;
    }
    if (::g_DoDSetBodygroup_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_SetBodygroup, ::DoD_SetBodygroup_Hook);
        ::DetourTransactionCommit();
        ::g_DoDSetBodygroup_Hook = false;
    }
    if (::g_DoDUtilRemove_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_UtilRemove, ::DoD_UtilRemove_Hook);
        ::DetourTransactionCommit();
        ::g_DoDUtilRemove_Hook = false;
    }
    if (::g_DoDCreateNamedEntity_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_CreateNamedEntity, ::DoD_CreateNamedEntity_Hook);
        ::DetourTransactionCommit();
        ::g_DoDCreateNamedEntity_Hook = false;
    }
    if (::g_DoDSubRemove_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_SubRemove, ::DoD_SubRemove_Hook);
        ::DetourTransactionCommit();
        ::g_DoDSubRemove_Hook = false;
    }
    if (::g_DoDWpnBoxKill_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_WpnBoxKill, ::DoD_WpnBoxKill_Hook);
        ::DetourTransactionCommit();
        ::g_DoDWpnBoxKill_Hook = false;
    }
    if (::g_DoDCreate_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_Create, ::DoD_Create_Hook);
        ::DetourTransactionCommit();
        ::g_DoDCreate_Hook = false;
    }
    if (::g_DoDWpnBoxActivateThink_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_WpnBoxActivateThink, ::DoD_WpnBoxActivateThink_Hook);
        ::DetourTransactionCommit();
        ::g_DoDWpnBoxActivateThink_Hook = false;
    }
    if (::g_DoDPackWeapon_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_PackWeapon, ::DoD_PackWeapon_Hook);
        ::DetourTransactionCommit();
        ::g_DoDPackWeapon_Hook = false;
    }
    if (::g_DoDDestroyItem_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_DestroyItem, ::DoD_DestroyItem_Hook);
        ::DetourTransactionCommit();
        ::g_DoDDestroyItem_Hook = false;
    }
#else
    if (::g_DoDPlayerSpawn_Hook)
    {
        ::subhook_remove(::g_pDoDPlayerSpawn);
        ::subhook_free(::g_pDoDPlayerSpawn);
        ::g_DoDPlayerSpawn_Hook = false;
    }
    if (::g_DoDGiveNamedItem_Hook)
    {
        ::subhook_remove(::g_pDoDGiveNamedItem);
        ::subhook_free(::g_pDoDGiveNamedItem);
        ::g_DoDGiveNamedItem_Hook = false;
    }
    if (::g_DoDDropPlayerItem_Hook)
    {
        ::subhook_remove(::g_pDoDDropPlayerItem);
        ::subhook_free(::g_pDoDDropPlayerItem);
        ::g_DoDDropPlayerItem_Hook = false;
    }
    if (::g_DoDRemovePlayerItem_Hook)
    {
        ::subhook_remove(::g_pDoDRemovePlayerItem);
        ::subhook_free(::g_pDoDRemovePlayerItem);
        ::g_DoDRemovePlayerItem_Hook = false;
    }
    if (::g_DoDAddPlayerItem_Hook)
    {
        ::subhook_remove(::g_pDoDAddPlayerItem);
        ::subhook_free(::g_pDoDAddPlayerItem);
        ::g_DoDAddPlayerItem_Hook = false;
    }
    if (::g_DoDRemoveAllItems_Hook)
    {
        ::subhook_remove(::g_pDoDRemoveAllItems);
        ::subhook_free(::g_pDoDRemoveAllItems);
        ::g_DoDRemoveAllItems_Hook = false;
    }
    if (::g_DoDSetWaveTime_Hook)
    {
        ::subhook_remove(::g_pDoDSetWaveTime);
        ::subhook_free(::g_pDoDSetWaveTime);
        ::g_DoDSetWaveTime_Hook = false;
    }
    if (::g_DoDGetWaveTime_Hook)
    {
        ::subhook_remove(::g_pDoDGetWaveTime);
        ::subhook_free(::g_pDoDGetWaveTime);
        ::g_DoDGetWaveTime_Hook = false;
    }
    if (::g_DoDGiveAmmo_Hook)
    {
        ::subhook_remove(::g_pDoDGiveAmmo);
        ::subhook_free(::g_pDoDGiveAmmo);
        ::g_DoDGiveAmmo_Hook = false;
    }
    if (::g_DoDSetBodygroup_Hook)
    {
        ::subhook_remove(::g_pDoDSetBodygroup);
        ::subhook_free(::g_pDoDSetBodygroup);
        ::g_DoDSetBodygroup_Hook = false;
    }
    if (::g_DoDDestroyItem_Hook)
    {
        ::subhook_remove(::g_pDoDDestroyItem);
        ::subhook_free(::g_pDoDDestroyItem);
        ::g_DoDDestroyItem_Hook = false;
    }
    if (::g_DoDSubRemove_Hook)
    {
        ::subhook_remove(::g_pDoDSubRemove);
        ::subhook_free(::g_pDoDSubRemove);
        ::g_DoDSubRemove_Hook = false;
    }
    if (::g_DoDCreate_Hook)
    {
        ::subhook_remove(::g_pDoDCreate);
        ::subhook_free(::g_pDoDCreate);
        ::g_DoDCreate_Hook = false;
    }
    if (::g_DoDWpnBoxKill_Hook)
    {
        ::subhook_remove(::g_pDoDWpnBoxKill);
        ::subhook_free(::g_pDoDWpnBoxKill);
        ::g_DoDWpnBoxKill_Hook = false;
    }
    if (::g_DoDWpnBoxActivateThink_Hook)
    {
        ::subhook_remove(::g_pDoDWpnBoxActivateThink);
        ::subhook_free(::g_pDoDWpnBoxActivateThink);
        ::g_DoDWpnBoxActivateThink_Hook = false;
    }
    if (::g_DoDPackWeapon_Hook)
    {
        ::subhook_remove(::g_pDoDPackWeapon);
        ::subhook_free(::g_pDoDPackWeapon);
        ::g_DoDPackWeapon_Hook = false;
    }
    if (::g_DoDUtilRemove_Hook)
    {
        ::subhook_remove(::g_pDoDUtilRemove);
        ::subhook_free(::g_pDoDUtilRemove);
        ::g_DoDUtilRemove_Hook = false;
    }
    if (::g_DoDCreateNamedEntity_Hook)
    {
        ::subhook_remove(::g_pDoDCreateNamedEntity);
        ::subhook_free(::g_pDoDCreateNamedEntity);
        ::g_DoDCreateNamedEntity_Hook = false;
    }
#endif
}
