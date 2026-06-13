
#include "amxxmodule.h"

#ifndef __linux__
#include <detours.h>
#else
#include <subhook.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <link.h>
#endif

#include <time.h>
#include <usercmd.h>
#include <entity_state.h>
#include <Memory.h>

#define DOD_BAR   ( 1 << 11 )
#define DOD_MG42  ( 1 << 17 )
#define DOD_30CAL ( 1 << 18 )
#define DOD_MG34  ( 1 << 21 )
#define DOD_FG42  ( 1 << 23 )
#define DOD_BREN  ( 1 << 27 )

enum DoD_Sig : unsigned char
{
    /// Game, Thiscall
    ///
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
    ChangePlayerTeam,
    ChooseRandomClass,

    /// Game, Cdecl
    ///
    Create,
    InstallGameRules,
    UtilRemove,
    CreateNamedEntity,

    /// Engine, Cdecl
    ///
    Engine_CalcPing_New,
    Engine_CalcPing_Old,
    Engine_CalcPing_Re, /// Thiscall (Win. ReHLDS)

    Engine_HostPing_New,
    Engine_HostPing_Old,
    Engine_HostPing_Re,

    Engine_HostStatus_New,
    Engine_HostStatus_Old,
    Engine_HostStatus_Re,

    Engine_HostStat_New,
    Engine_HostStat_Old,
    Engine_HostStat_Re,

#ifdef __linux__
    Engine_WriteByte_New,
    Engine_WriteByte_Old,
    Engine_WriteByte_Re,

    Engine_WriteBits_New,
    Engine_WriteBits_Old,
    Engine_WriteBits_Re,

    Engine_EmitPings_New,
    Engine_EmitPings_Old,
    Engine_EmitPings_Re,
#endif

    /// Misc. Signatures
    ///
    PatchFg42,
    PatchEnfield,

    /// Misc. Bytes
    ///
    OrigFg42_Byte,
    PatchFg42_Byte,

    OrigEnfield_Byte,
    PatchEnfield_Byte,

    /// Settings
    ///
    Setting_Engine_Client_Ptr_Min_Ofs_To_Entity_Ptr,
    Setting_Engine_Client_Ptr_Max_Ofs_To_Entity_Ptr,

    /// Offsets
    ///
    Offs_Entvars,

    Offs_AlliesAreBrit,
    Offs_AlliesArePara,
    Offs_AxisArePara,
    Offs_AlliesRespawnFactor,
    Offs_AxisRespawnFactor,
    Offs_AlliesInfiniteLives,
    Offs_AxisInfiniteLives,

    Offs_ItemScope,
    Offs_CBasePlayerItem_Index,
    Offs_ApplyItemScope,

    Offs_ThinkFunc_Pfn,
    Offs_ThinkFunc_Delta,

    Offs_CBasePlayer_NextClass,
    Offs_CBasePlayer_RandomClass,
    Offs_CBasePlayer_LastTeam,
    Offs_CBasePlayer_ActiveItem,
    Offs_CBasePlayer_RespawnFrames,
    Offs_CBasePlayer_FrameRate,
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
    Fn_WpnBoxKill,
    Fn_WpnBoxActivateThink,
    Fn_ChangePlayerTeam,
    Fn_ChooseRandomClass,
    Fn_Create,
    Fn_InstallGameRules,
    Fn_UtilRemove,
    Fn_CreateNamedEntity,
    Fn_Engine_CalcPing,
    Fn_Engine_HostPing,
    Fn_Engine_HostStatus,
    Fn_Engine_HostStat,

#ifdef __linux__
    Fn_Engine_WriteByte,
    Fn_Engine_WriteBits,
    Fn_Engine_EmitPings,
#endif
};

enum DoD_RandomClassAction : unsigned char
{
    DoD_RCA_None = false,
    DoD_RCA_Add,
    DoD_RCA_Remove,
};

enum DoD_Size : unsigned char
{
    Int8 = false,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
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
typedef void(__thiscall* DoD_ChangePlayerTeam_Type) (::size_t CDoDTeamPlay, ::size_t CBasePlayer, int Team, int Kill, int Gib);
typedef void(__thiscall* DoD_ChooseRandomClass_Type) (::size_t CDoDTeamPlay, ::size_t CBasePlayer);

#ifndef __linux__ /// Win. ReHLDS wrapper.
typedef int(__thiscall* DoD_Engine_CalcPing_Type_ReHLDS_Win) (::size_t client_s);
#endif

typedef ::size_t(*DoD_Create_Type) (char* pItem, const ::Vector* pOrigin, const ::Vector* pAngles, ::edict_s* pOwner);
typedef ::size_t(*DoD_InstallGameRules_Type) ();
typedef void (*DoD_UtilRemove_Type) (::size_t CBaseEntity);
typedef ::edict_s* (*DoD_CreateNamedEntity_Type) (::size_t Name);
typedef int (*DoD_Engine_CalcPing_Type) (::size_t client_s);
typedef void (*DoD_Engine_HostPing_Type) ();
typedef void (*DoD_Engine_HostStatus_Type) ();
typedef void (*DoD_Engine_HostStat_Type) ();

#ifdef __linux__
typedef void (*DoD_Engine_WriteByte_Type) (::size_t Message, int Value);
typedef void (*DoD_Engine_WriteBits_Type) (::size_t Data, int Bits);
typedef void (*DoD_Engine_EmitPings_Type) (::size_t client_s, ::size_t sizebuf_s);
#endif

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

#ifndef __linux__
bool g_ReHLDS = false;
#endif

::edict_s* g_pEntities = NULL;
::size_t g_entvarsOffs = false;
::size_t g_CDoDTeamPlay = false;
::size_t g_svPlayerEngPtrs[33]{ };
::size_t g_engConvOffs = ::size_t(-1);
::enginefuncs_s* g_pEngineHookTable = NULL;
::DLL_FUNCTIONS* g_pFunctionHookTable = NULL;
::NEW_DLL_FUNCTIONS* g_pNewFunctionHookTable = NULL;
::cvar_s g_Version{ "dodhacks_version", MODULE_VERSION " @ " MODULE_YEAR_MS, FCVAR_SERVER | FCVAR_SPONLY, };

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
::DoD_ChangePlayerTeam_Type DoD_ChangePlayerTeam = NULL;
::DoD_ChooseRandomClass_Type DoD_ChooseRandomClass = NULL;
::DoD_Create_Type DoD_Create = NULL;
::DoD_Engine_CalcPing_Type DoD_Engine_CalcPing = NULL;
::DoD_Engine_HostPing_Type DoD_Engine_HostPing = NULL;
::DoD_Engine_HostStatus_Type DoD_Engine_HostStatus = NULL;
::DoD_Engine_HostStat_Type DoD_Engine_HostStat = NULL;

#ifndef __linux__
::DoD_Engine_CalcPing_Type_ReHLDS_Win DoD_Engine_CalcPing_ReHLDS_Win = NULL;
#else
::DoD_Engine_WriteByte_Type DoD_Engine_WriteByte = NULL;
::DoD_Engine_WriteBits_Type DoD_Engine_WriteBits = NULL;
::DoD_Engine_EmitPings_Type DoD_Engine_EmitPings = NULL;
#endif

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
bool g_DoDChangePlayerTeam_Hook = false;
bool g_DoDChooseRandomClass_Hook = false;
bool g_DoDCreate_Hook = false;
bool g_DoDEngine_CalcPing_Hook = false;
bool g_DoDEngine_HostPing_Hook = false;
bool g_DoDEngine_HostStatus_Hook = false;
bool g_DoDEngine_HostStat_Hook = false;

#ifdef __linux__
bool g_DoDEngine_WriteByte_Hook = false;
bool g_DoDEngine_WriteBits_Hook = false;
bool g_DoDEngine_EmitPings_Hook = false;
#endif

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
::subhook_t g_pDoDChangePlayerTeam = NULL;
::subhook_t g_pDoDChooseRandomClass = NULL;
::subhook_t g_pDoDCreate = NULL;
::subhook_t g_pDoDEngine_CalcPing = NULL;
::subhook_t g_pDoDEngine_HostPing = NULL;
::subhook_t g_pDoDEngine_HostStatus = NULL;
::subhook_t g_pDoDEngine_HostStat = NULL;
::subhook_t g_pDoDEngine_WriteByte = NULL;
::subhook_t g_pDoDEngine_WriteBits = NULL;
::subhook_t g_pDoDEngine_EmitPings = NULL;
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
void* g_pDoDChangePlayerTeam_Addr = NULL;
void* g_pDoDChooseRandomClass_Addr = NULL;
void* g_pDoDCreate_Addr = NULL;
void* g_pDoDEngine_CalcPing_Addr = NULL;
void* g_pDoDEngine_HostPing_Addr = NULL;
void* g_pDoDEngine_HostStatus_Addr = NULL;
void* g_pDoDEngine_HostStat_Addr = NULL;

#ifdef __linux__
void* g_pDoDEngine_WriteByte_Addr = NULL;
void* g_pDoDEngine_WriteBits_Addr = NULL;
void* g_pDoDEngine_EmitPings_Addr = NULL;
#endif

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
int g_fwChangePlayerTeam = false;
int g_fwChooseRandomClass = false;
int g_fwCreate = false;
int g_fwEngine_CalcPing = false;
int g_fwEngine_HostPing = false;
int g_fwEngine_HostStatus = false;
int g_fwEngine_HostStat = false;

#ifdef __linux__
int g_fwEngine_WriteByte = false;
int g_fwEngine_WriteBits = false;
int g_fwEngine_EmitPings = false;
#endif

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
int g_fwChangePlayerTeam_Post = false;
int g_fwChooseRandomClass_Post = false;
int g_fwCreate_Post = false;
int g_fwEngine_CalcPing_Post = false;
int g_fwEngine_HostPing_Post = false;
int g_fwEngine_HostStatus_Post = false;
int g_fwEngine_HostStat_Post = false;

#ifdef __linux__
int g_fwEngine_WriteByte_Post = false;
int g_fwEngine_WriteBits_Post = false;
int g_fwEngine_EmitPings_Post = false;
#endif

float g_deadPlayerCorpsePevFrameRate = false;
float g_deadPlayerCorpseCBaseFrameRate = false;
float g_deadPlayerCorpseRespawnFramesSub = false;
bool g_deadPlayerCorpseAlterCBaseFrameRate = false;
bool g_deadPlayerCorpseEnableInterpolation = false;

unsigned char* g_pAutoScopeFG42Addr = NULL;
unsigned char* g_pAutoScopeEnfieldAddr = NULL;

bool g_selfNade = false;
bool g_selfNadeDoSolid = false;
short g_selfNadeSolid = false;
int g_selfNadeMode = false;
int g_selfNadeFx = false;
::color24 g_selfNadeColor{ };
int g_selfNadeAmt = false;
bool g_exclSelfNadeGlow[33]{ };

bool g_teamNade = false;
bool g_teamNadeDoSolid = false;
short g_teamNadeSolid = false;
int g_teamNadeMode = false;
int g_teamNadeFx = false;
::color24 g_teamNadeColor{ };
int g_teamNadeAmt = false;
bool g_exclTeamNadeGlow[33]{ };

bool g_droppedNade = false;
bool g_droppedNadeDoArmed = false;
bool g_droppedNadeDoSolid = false;
short g_droppedNadeSolid = false;
int g_droppedNadeMode = false;
int g_droppedNadeFx = false;
::color24 g_droppedNadeColor{ };
int g_droppedNadeAmt = false;
bool g_exclDroppedNadeGlow[33]{ };

::SourceHook::CVector < ::SignatureData > g_Sigs{ };
::SourceHook::CVector < ::AllocatedString > g_Strings{ };
::SourceHook::CVector < ::CustomKeyValue_Add > g_CustomKeyValues_Add{ };
::SourceHook::CVector < ::CustomKeyValue_Del > g_CustomKeyValues_Del{ };
::SourceHook::CVector < int > g_blockedFromPlayerCollision{ };

::size_t F_EToI(::edict_t* pEntity)
{
    return ::g_pEntities ? pEntity - ::g_pEntities : ::g_engfuncs.pfnIndexOfEdict(pEntity);
}

::edict_s* F_IToE(::size_t Entity)
{
    return ::g_pEntities ? ::g_pEntities + Entity : ::g_engfuncs.pfnPEntityOfEntIndex(Entity);
}

const char* teamNameByTeamIndex(int teamIndex, bool forAmx)
{
    switch (teamIndex)
    { /** AMX Mod X does not use/ need strings like "Unassigned" and "Spectators". */
    case 0:  return forAmx ? NULL : "Unassigned";
    case 1:  return "Allies";
    case 2:  return "Axis";
    default: return forAmx ? NULL : "Spectators";
    }
}

const char* classNameByClassIndex(int classIndex)
{
    switch (classIndex)
    {
        /** Allies classes. */
    case 1:  return "Rifleman";
    case 2:  return "Staff Sergeant";
    case 3:  return "Master Sergeant";
    case 4:  return "Sergeant";
    case 5:  return "Sniper";
    case 6:  return "Support Infantry";
    case 7:  return "Machine Gunner";
    case 8:  return "Bazooka";
        /// case 9:  return "Mortar"; /// Allies Mortar. Not implemented.

        /** Axis classes. */
    case 10: return "Grenadier";
    case 11: return "Stosstruppe";
    case 12: return "Unteroffizier";
    case 13: return "Sturmtruppe";
    case 14: return "Scharfschütze";
    case 15: return "Fg42-Zweibein";
    case 16: return "Fg42-Zielfernrohr";
    case 17: return "MG34-Schütze";
    case 18: return "MG42-Schütze";
    case 19: return "Panzerjäger";
        /// case 20: return "Mörserschütze"; /// Axis Mortar. Not implemented.

        /** British classes. */
    case 21: return "Rifleman";
    case 22: return "Sergeant Major";
    case 23: return "Marksman";
    case 24: return "Gunner";
    case 25: return "PIAT";
        /// case 26: return "Mortar"; /// British Mortar. Not implemented.

    default: return "Random"; /// Unknown class or no class at all.
    }
}

#if !defined (__linux__)
#if (defined (__i386__) || defined (_M_IX86)) /// x86 Windows.
const unsigned char* findStr /// x86 Windows.
(::_IMAGE_DOS_HEADER* pDosHdr, ::_IMAGE_NT_HEADERS* pNtHdr, const char* pString, ::size_t Len)
{
    const auto pSec = IMAGE_FIRST_SECTION(pNtHdr);
    if (!pSec)
        return NULL;
    ::size_t Addr;
    unsigned long Size;
    unsigned short Sec = false;
    const unsigned char* pBeg, * pEnd, * pIter;
    const auto Secs = pNtHdr->FileHeader.NumberOfSections;
    for (; Sec < Secs; Sec++)
    {
        const auto& Hdr = pSec[Sec];
        if (Hdr.Characteristics & IMAGE_SCN_MEM_EXECUTE)
            continue;
        Size = Hdr.Misc.VirtualSize;
        if (Size < Len)
            continue;
        pBeg = (const unsigned char*)pDosHdr + Hdr.VirtualAddress;
        pEnd = pBeg + (Size - Len);
        for (pIter = pBeg; pIter <= pEnd; ++pIter)
            if (*pIter == pString[0] && false == ::memcmp(pIter, pString, Len))
            {
                Addr = (::size_t)pIter;
                goto keepUp;
            }
    }
    return NULL;
keepUp:
    for (Sec = false; Sec < Secs; Sec++)
    {
        const auto& Hdr = pSec[Sec];
        if (!(Hdr.Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;
        Size = Hdr.Misc.VirtualSize;
        if (Size < 5)
            continue;
        pBeg = (const unsigned char*)pDosHdr + Hdr.VirtualAddress;
        pEnd = pBeg + (Size - 5);
        for (pIter = pBeg; pIter <= pEnd; ++pIter)
            if (*pIter == 0x68 && *(unsigned*)(pIter + true) == Addr)
                return pIter;
    }
    return NULL;
}
#else
#error "findStr() for x64 to be implemented!"
#endif
#endif

bool edict_s_Ptr_From_client_s_Ptr_Offs(::size_t client_s, ::size_t& Offs, ::size_t ofsFromIncl, ::size_t ofsTo)
{
    ::edict_s* pPlayer;
    ::size_t Iter = ofsFromIncl;
    unsigned char* pAddr = (unsigned char*)client_s, Player, Max = ::gpGlobals->maxClients;
    for (Offs = false; Iter < ofsTo; Iter++)
        for (pPlayer = *(::edict_s**)(pAddr + Iter), Player = 1; pPlayer && Player <= Max; Player++)
            if (::F_IToE(Player) == pPlayer)
            {
                Offs = Iter;
                return true;
            }
    return false;
}

void sendPClass(::edict_s* pPlayer, int Player, int Class)
{
    static auto PClass = ::gpMetaUtilFuncs->pfnGetUserMsgID(&::Plugin_info, "PClass", NULL);
    ::g_pEngineHookTable->pfnMessageBegin(pPlayer ? MSG_ONE_UNRELIABLE : MSG_BROADCAST, PClass, NULL, pPlayer);
    ::g_pEngineHookTable->pfnWriteByte(Player);
    ::g_pEngineHookTable->pfnWriteByte(Class);
    ::g_pEngineHookTable->pfnMessageEnd();
}

void sendTextMsg(::edict_s* pPlayer, const char* pPhrase, const char* pArg1, const char* pArg2, const char* pArg3, const char* pArg4)
{
    static auto TextMsg = ::gpMetaUtilFuncs->pfnGetUserMsgID(&::Plugin_info, "TextMsg", NULL);
    ::g_pEngineHookTable->pfnMessageBegin(pPlayer ? MSG_ONE_UNRELIABLE : MSG_BROADCAST, TextMsg, NULL, pPlayer);
    ::g_pEngineHookTable->pfnWriteByte(HUD_PRINTTALK);
    ::g_pEngineHookTable->pfnWriteString(pPhrase);
    if (pArg1) ::g_pEngineHookTable->pfnWriteString(pArg1);
    if (pArg2) ::g_pEngineHookTable->pfnWriteString(pArg2);
    if (pArg3) ::g_pEngineHookTable->pfnWriteString(pArg3);
    if (pArg4) ::g_pEngineHookTable->pfnWriteString(pArg4);
    ::g_pEngineHookTable->pfnMessageEnd();
}

bool playerActiveItem(::edict_s* pPlayer, int& activeItemIndex, bool& itemHasScopeAttached)
{
    activeItemIndex = false;
    itemHasScopeAttached = false;
    auto pPlayerBase = (unsigned char*)pPlayer->pvPrivateData;
    if (!pPlayerBase)
        return false;
    auto pActiveItemBase = *(unsigned char**)(pPlayerBase + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_ActiveItem].Offs);
    if (!pActiveItemBase)
        return false;
    activeItemIndex = (1 << *(int*)(pActiveItemBase + ::g_Sigs[::DoD_Sig::Offs_CBasePlayerItem_Index].Offs));
    itemHasScopeAttached = *(::size_t*)(pActiveItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & true;
    return true;
}

::size_t setupString(const char* pString)
{
    for (const auto& String : ::g_Strings)
    {
        if (false == String.Buffer.icmp(pString))
            return String.Index;
    }

    ::AllocatedString String;
    String.Buffer = pString;
    String.Index = ::g_engfuncs.pfnAllocString(pString);
    ::g_Strings.push_back(String);
    return String.Index;
}

void allowFullMemAccess(void* pAddr, ::size_t Size)
{
#ifndef __linux__
    static unsigned long Access;
    ::VirtualProtect(pAddr, Size, PAGE_EXECUTE_READWRITE, &Access);
#else
    static long Page;
    static ::size_t Addr, Begin, End;

    Addr = (::size_t)pAddr;
    Page = ::sysconf(_SC_PAGESIZE) - true;
    Begin = Addr & ~Page; /// Would turn '0xABC777AB' into '0xABC77000'.
    End = (Addr + Size + Page) & ~Page; /// Would turn '0xABC777AB' into '0xABC78000', '0xABC79000', ...
    ::mprotect(Begin, End - Begin /** 0x1000(4096), 0x2000(8192), ... */, PROT_READ | PROT_WRITE | PROT_EXEC);
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
     * Do not open a BOT library for sig. scanning if '::gpGamedllFuncs->dllapi_table->pfnSpawn' points into one.
     * Use the Game library instead (i.e. 'dod.so' or 'dod_i386.so').
     * Linux only. On Windows, this filtering isn't needed.
     */
    ::Dl_info memInfo;
    if (::strcasestr(pName, "dod"))
    {
        ::dladdr(::gpGamedllFuncs->dllapi_table->pfnSpawn, &memInfo);
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
        ::dladdr(::gpGamedllFuncs->dllapi_table->pfnSpawn, &memInfo);
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
        ::dladdr(::gpGamedllFuncs->dllapi_table->pfnSpawn, &memInfo);
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
        ::dladdr(::gpGamedllFuncs->dllapi_table->pfnSpawn, &memInfo);
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
        ::dladdr(::gpGamedllFuncs->dllapi_table->pfnSpawn, &memInfo);
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
        ::dladdr(::gpGamedllFuncs->dllapi_table->pfnSpawn, &memInfo);
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

    auto pVars = *(::entvars_s**)(CBase + ::g_entvarsOffs);
    if (!pVars)
        return -1;

    auto pEntity = pVars->pContainingEntity;
    if (!pEntity)
        return -1;

    return ::F_EToI(pEntity);
}

::size_t indexToBase(int Index)
{
    if (Index < 0 || Index > ::gpGlobals->maxEntities)
        return false;

    auto pEntity = ::F_IToE(Index);
    if (!pEntity)
        return false;

    auto pBase = pEntity->pvPrivateData;
    if (!pBase)
        return false;

    return (::size_t)pBase;
}

void ServerActivate(::edict_s* pEntities, int, int)
{ /// Always in-use.
    ::g_pEntities = pEntities;
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
}

void DispatchKeyValue(::edict_s* pEntity, ::KeyValueData* pKvData)
{ /// Always in-use.
    if (!pEntity)
    {
        ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
        return;
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
            ::gpMetaGlobals->mres = ::META_RES::MRES_SUPERCEDE;
            return;
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
                ::gpMetaGlobals->mres = ::META_RES::MRES_SUPERCEDE;
                return;
            }
            if (!pKvData->szValue || false == *pKvData->szValue)
            { /// Operator wants to remove a specific non-empty value, skip this entry.
                continue;
            }
            if (false == cusKeyVal.Value.icmp(pKvData->szValue))
            { /// If the values matches too, delete this entry.
                ::gpMetaGlobals->mres = ::META_RES::MRES_SUPERCEDE;
                return;
            }
        }
    }
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
}

void PlayerPostThink_Post(::edict_s* pPlayer)
{ /// Only if a plugin enables this.
    static ::entvars_s* pVars;
    static unsigned char* pBase;
    static float* pRespawnFrames;
    pVars = &pPlayer->v;
    if (pVars->deadflag || pVars->health <= 0.f)
    {
        if (::g_deadPlayerCorpseEnableInterpolation)
            pVars->effects &= ~EF_NOINTERP;
        pBase = (unsigned char*)pPlayer->pvPrivateData;
        if (pBase)
        {
            pRespawnFrames = (float*)(pBase + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_RespawnFrames].Offs);
            if (*pRespawnFrames > 0.f)
                *pRespawnFrames -= ::g_deadPlayerCorpseRespawnFramesSub;
            if (::g_deadPlayerCorpseAlterCBaseFrameRate)
                *(float*)(pBase + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_FrameRate].Offs) = ::g_deadPlayerCorpseCBaseFrameRate;
        }
        if (pVars->framerate > 0.f)
            pVars->framerate = ::g_deadPlayerCorpsePevFrameRate;
    }
    ::gpMetaGlobals->mres = ::MRES_IGNORED;
}

int AddToFullPack_Post
(::entity_state_s* pState, int eIdx, ::edict_s* pEntity, ::edict_s* pHost, int hostFlags, int isPlayer, unsigned char* pSet)
{ /// Used (attached) only if a plugin requires it.
    static unsigned char Host;
    static const char* pClass;
    static ::edict_s* pOwner;
    if (pState && pHost && pEntity && !isPlayer)
    {
        Host = (unsigned char) ::F_EToI(pHost);
        if (::g_selfNade && !::g_exclSelfNadeGlow[Host] && pHost == pEntity->v.owner &&
            false == ::_strnicmp(STRING(pEntity->v.classname), "grenade", 7))
        {
            if (::g_selfNadeDoSolid)
                pState->solid = ::g_selfNadeSolid;
            pState->rendermode = ::g_selfNadeMode;
            pState->renderfx = ::g_selfNadeFx;
            pState->rendercolor = ::g_selfNadeColor;
            pState->renderamt = ::g_selfNadeAmt;
        }
        else if (::g_teamNade && !::g_exclTeamNadeGlow[Host] && (pOwner = pEntity->v.owner) && pHost->v.team == pOwner->v.team &&
            false == ::_strnicmp(STRING(pEntity->v.classname), "grenade", 7))
        {
            if (::g_teamNadeDoSolid)
                pState->solid = ::g_teamNadeSolid;
            pState->rendermode = ::g_teamNadeMode;
            pState->renderfx = ::g_teamNadeFx;
            pState->rendercolor = ::g_teamNadeColor;
            pState->renderamt = ::g_teamNadeAmt;
        }
        else if (::g_droppedNade && !::g_exclDroppedNadeGlow[Host])
        {
            pClass = STRING(pEntity->v.classname);
            switch (::g_droppedNadeDoArmed)
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
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
    return false;
}

int ShouldCollide(::edict_s* pEntity, ::edict_s* pOther)
{ /// Only if a plugin enables this.
    ::gpMetaGlobals->mres =
        ((pEntity->v.flags & (FL_CLIENT | FL_FAKECLIENT)) && ::g_blockedFromPlayerCollision.hasVal(::F_EToI(pOther))) ||
        ((pOther->v.flags & (FL_CLIENT | FL_FAKECLIENT)) && ::g_blockedFromPlayerCollision.hasVal(::F_EToI(pEntity))) ?
        ::META_RES::MRES_SUPERCEDE : ::META_RES::MRES_IGNORED;
    return false;
}

void CmdStart(::edict_s* pPlayer, ::usercmd_s* pCmd, ::size_t randomSeed)
{ /// Only if a plugin enables this.
    static int Item;
    static bool scopeAttached;
    static ::entvars_s* pVars;
    pVars = &pPlayer->v;
    if (!pVars->deadflag && pVars->health > 0.f)
    {
        if ((pVars->weapons & (DOD_BAR | DOD_MG42 | DOD_30CAL | DOD_MG34 | DOD_FG42 | DOD_BREN)) &&
            ::playerActiveItem(pPlayer, Item, scopeAttached) && false == scopeAttached &&
            (Item & (DOD_BAR | DOD_MG42 | DOD_30CAL | DOD_MG34 | DOD_FG42 | DOD_BREN)))
        {
            if (pVars->vuser1.x != 2.f)
                pVars->vuser1.x = true;
        }
        else
            pVars->vuser1.x = false;
    }
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
}

::cell DoD_HookShouldCollide_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (::g_pNewFunctionsTable->pfnShouldCollide)
        return false;
    ::g_pNewFunctionsTable->pfnShouldCollide = ::ShouldCollide;
    return true;
}

::cell DoD_UnhookShouldCollide_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pNewFunctionsTable->pfnShouldCollide)
        return false;
    ::g_pNewFunctionsTable->pfnShouldCollide = NULL;
    if (pParam[1])
        ::memset(&::g_blockedFromPlayerCollision, false, sizeof(::g_blockedFromPlayerCollision));
    return true;
}

::cell DoD_BlockToPlayerCollision_Native(::tagAMX* pAmx, ::cell* pParam)
{
    const auto& Entity = pParam[1];
    if (::g_blockedFromPlayerCollision.hasVal(Entity))
        return false;
    ::g_blockedFromPlayerCollision.insert(::g_blockedFromPlayerCollision.end(), Entity);
    return true;
}

::cell DoD_UnblockFromPlayerCollision_Native(::tagAMX* pAmx, ::cell* pParam)
{
    const auto& Entity = pParam[1];
    const auto& End = ::g_blockedFromPlayerCollision.end();
    for (auto Iter = ::g_blockedFromPlayerCollision.begin(); Iter != End; Iter++)
    {
        if (*Iter == Entity)
        {
            ::g_blockedFromPlayerCollision.erase(Iter);
            return true;
        }
    }
    return false;
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
    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }

    auto pPlayer = ::F_IToE(Player);
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

::cell DoD_ChooseRandomClass_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    if (pRes)
        *pRes = -1;

    if (!::g_pDoDChooseRandomClass_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_ChooseRandomClass not found!");
        return false;
    }

    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto Player = pParam[1];
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

    auto pPlayer = ::F_IToE(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pBase = pPlayer->pvPrivateData;
    if (!pBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    if (pParam[8])
        ((::DoD_ChooseRandomClass_Type) ::g_pDoDChooseRandomClass_Addr) (::g_CDoDTeamPlay, (::size_t)pBase);
    else
    {
        if (::g_DoDChooseRandomClass_Hook)
            ::DoD_ChooseRandomClass(::g_CDoDTeamPlay, (::size_t)pBase);
        else
            ((::DoD_ChooseRandomClass_Type) ::g_pDoDChooseRandomClass_Addr) (::g_CDoDTeamPlay, (::size_t)pBase);
    }
    auto Class = *(::cell*)(((unsigned char*)pBase) + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_NextClass].Offs);
    if (pRes)
        *pRes = Class;
    if (pParam[3])
        pPlayer->v.playerclass = Class;
    if (pParam[4])
        ::sendPClass(NULL, Player, Class);
    if (pParam[5])
    {
        if (pParam[6])
            ::sendTextMsg(pPlayer, pPlayer->v.deadflag || pPlayer->v.health <= 0.f ? "#game_respawn_asrandom" : "#game_spawn_asrandom",
                NULL, NULL, NULL, NULL);
        else
            ::sendTextMsg(pPlayer, pPlayer->v.deadflag || pPlayer->v.health <= 0.f ? "#game_respawn_as" : "#game_spawn_as",
                ::classNameByClassIndex(Class), NULL, NULL, NULL);
    }
    switch (::DoD_RandomClassAction(pParam[7]))
    {
    case ::DoD_RandomClassAction::DoD_RCA_Add:
    {
        *(int*)(((unsigned char*)pBase) + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_RandomClass].Offs) = true;
        break;
    }
    case ::DoD_RandomClassAction::DoD_RCA_Remove:
    {
        *(int*)(((unsigned char*)pBase) + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_RandomClass].Offs) = false;
        break;
    }
    }
    return true;
}

::cell DoD_ChangePlayerTeam_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDChangePlayerTeam_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_ChangePlayerTeam not found!");
        return false;
    }

    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto Player = pParam[1];
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

    auto pPlayer = ::F_IToE(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }

    auto pBase = pPlayer->pvPrivateData;
    if (!pBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
        return false;
    }

    auto Team = pParam[2];
    if (pParam[9])
        ((::DoD_ChangePlayerTeam_Type) ::g_pDoDChangePlayerTeam_Addr) (::g_CDoDTeamPlay, (::size_t)pBase, Team, pParam[3], pParam[4]);
    else
    {
        if (::g_DoDChangePlayerTeam_Hook)
            ::DoD_ChangePlayerTeam(::g_CDoDTeamPlay, (::size_t)pBase, Team, pParam[3], pParam[4]);
        else
            ((::DoD_ChangePlayerTeam_Type) ::g_pDoDChangePlayerTeam_Addr) (::g_CDoDTeamPlay, (::size_t)pBase, Team, pParam[3], pParam[4]);
    }
    if (pParam[5])
        pPlayer->v.team = Team;
    if (pParam[6])
        ::g_fn_SetTeamInfo(Player, Team, ::teamNameByTeamIndex(Team, true));
    if (pParam[7])
        ::sendTextMsg(NULL, "#game_joined_team", STRING(pPlayer->v.netname), ::teamNameByTeamIndex(Team, false), NULL, NULL);
    if (pParam[8])
        *(int*)(((unsigned char*)pBase) + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_LastTeam].Offs) = true;
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

    auto pPlayer = ::F_IToE(Player);
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

    auto pEntity = ::F_IToE(Entity);
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

    auto pEntity = ::F_IToE(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity (weaponbox) %d has no edict!", Entity);
        return false;
    }

    auto pWeapon = ::F_IToE(Weapon);
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

    auto pEntity = ::F_IToE(Entity);
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

    auto pEntity = ::F_IToE(Entity);
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
        *pRes = ::F_EToI(pEntity);
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

    auto pEntity = ::F_IToE(Entity);
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

    auto pEntity = ::F_IToE(Entity);
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
    ::edict_s* pOwner = ((Owner < 0 || Owner > ::gpGlobals->maxEntities) ? NULL : ::F_IToE(Owner));
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

    int Len;
    auto pItem = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (Len < 1 || !pItem || false == *pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item name string (null or empty)!");
        return false;
    }

    auto Player = pParam[1];
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

    auto pPlayer = ::F_IToE(Player);
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
            if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & true))
            {
                *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= true;
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
            if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & true))
            {
                *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= true;
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
        *pItemRes = ::cell(::F_EToI(pEntity));
    return true;
}

::cell DoD_DropPlayerItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDDropPlayerItem_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_DropPlayerItem not found!");
        return false;
    }

    int Len;
    auto pItem = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (!pItem)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item name string (null)!");
        return false;
    }

    auto Player = pParam[1];
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

    auto pPlayer = ::F_IToE(Player);
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

    auto pEntity = ::F_IToE(Entity);
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

    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[3], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid ammo name string (null or empty)!");
        return false;
    }

    auto Player = pParam[1];
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

    auto pPlayer = ::F_IToE(Player);
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

::cell DoD_TraceLine_Native(::tagAMX* pAmx, ::cell* pParam)
{
    static ::TraceResult Trace;
    auto pFrom = ::g_fn_GetAmxAddr(pAmx, pParam[1]);
    auto pTo = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    auto From = pFrom ?
        ::Vector(::g_fn_CellToReal(pFrom[0]), ::g_fn_CellToReal(pFrom[1]), ::g_fn_CellToReal(pFrom[2])) :
        ::Vector(false, false, false);
    auto To = pTo ?
        ::Vector(::g_fn_CellToReal(pTo[0]), ::g_fn_CellToReal(pTo[1]), ::g_fn_CellToReal(pTo[2])) :
        ::Vector(false, false, false);
    auto Skip = pParam[4];
    auto pSkip = Skip < 0 || Skip > ::gpGlobals->maxEntities ? NULL : ::F_IToE(Skip);
    ::g_pEngineHookTable->pfnTraceLine(&From.x, &To.x, pParam[3], pSkip, &Trace);
    return true;
}

::cell DoD_TraceLineComplex_Native(::tagAMX* pAmx, ::cell* pParam)
{
    static ::TraceResult Trace;
    auto pFrom = ::g_fn_GetAmxAddr(pAmx, pParam[1]);
    auto pTo = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    auto From = pFrom ?
        ::Vector(::g_fn_CellToReal(pFrom[0]), ::g_fn_CellToReal(pFrom[1]), ::g_fn_CellToReal(pFrom[2])) :
        ::Vector(false, false, false);
    auto To = pTo ?
        ::Vector(::g_fn_CellToReal(pTo[0]), ::g_fn_CellToReal(pTo[1]), ::g_fn_CellToReal(pTo[2])) :
        ::Vector(false, false, false);
    auto Skip = pParam[4];
    auto pSkip = Skip < 0 || Skip > ::gpGlobals->maxEntities ? NULL : ::F_IToE(Skip);
    if (bool(pParam[6]))
        ::g_pEngineHookTable->pfnTraceLine(&From.x, &To.x, pParam[3], pSkip, &Trace);
    else
        TRACE_LINE(&From.x, &To.x, pParam[3], pSkip, &Trace);
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[5]);
    if (pRes)
    {
        *pRes = Trace.fAllSolid;
        *(::cell*)(pRes + 1) = Trace.fStartSolid;
        *(::cell*)(pRes + 2) = Trace.fInOpen;
        *(::cell*)(pRes + 3) = Trace.fInWater;
        *(::cell*)(pRes + 4) = ::g_fn_RealToCell(Trace.flFraction);
        *(::cell*)(pRes + 5) = ::g_fn_RealToCell(Trace.vecEndPos.x);
        *(::cell*)(pRes + 6) = ::g_fn_RealToCell(Trace.vecEndPos.y);
        *(::cell*)(pRes + 7) = ::g_fn_RealToCell(Trace.vecEndPos.z);
        *(::cell*)(pRes + 8) = ::g_fn_RealToCell(Trace.flPlaneDist);
        *(::cell*)(pRes + 9) = ::g_fn_RealToCell(Trace.vecPlaneNormal.x);
        *(::cell*)(pRes + 10) = ::g_fn_RealToCell(Trace.vecPlaneNormal.y);
        *(::cell*)(pRes + 11) = ::g_fn_RealToCell(Trace.vecPlaneNormal.z);
        *(::cell*)(pRes + 12) = Trace.pHit ? ::F_EToI(Trace.pHit) : -1;
        *(::cell*)(pRes + 13) = Trace.iHitgroup;
    }
    return true;
}

::cell DoD_AddHealthIfWounded_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pAdded = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pAdded)
        *pAdded = false;

    auto Player = pParam[1];
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

    auto pPlayer = ::F_IToE(Player);
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

    auto pPlayer = ::F_IToE(Player);
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

    auto pPlayer = ::F_IToE(Player);
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

    auto pItem = ::F_IToE(Item);
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

    auto pPlayer = ::F_IToE(Player);
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

    auto pItem = ::F_IToE(Item);
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

::cell DoD_Engine_HostPing_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDEngine_HostPing_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_Engine_HostPing not found!");
        return false;
    }
    if (pParam[1] || false == ::g_DoDEngine_HostPing_Hook)
        ((::DoD_Engine_HostPing_Type) ::g_pDoDEngine_HostPing_Addr) ();
    else
        ::DoD_Engine_HostPing();
    return true;
}

::cell DoD_Engine_HostStatus_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDEngine_HostStatus_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_Engine_HostStatus not found!");
        return false;
    }
    if (pParam[1] || false == ::g_DoDEngine_HostStatus_Hook)
        ((::DoD_Engine_HostStatus_Type) ::g_pDoDEngine_HostStatus_Addr) ();
    else
        ::DoD_Engine_HostStatus();
    return true;
}

::cell DoD_Engine_HostStat_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pDoDEngine_HostStat_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_Engine_HostStat not found!");
        return false;
    }
    if (pParam[1] || false == ::g_DoDEngine_HostStat_Hook)
        ((::DoD_Engine_HostStat_Type) ::g_pDoDEngine_HostStat_Addr) ();
    else
        ::DoD_Engine_HostStat();
    return true;
}

::cell DoD_Engine_WriteByte_Native(::tagAMX* pAmx, ::cell* pParam)
{
#ifndef __linux__
    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "DoD_Engine_WriteByte() native not implemented on Windows!");
    return false;
#else
    if (!::g_pDoDEngine_WriteByte_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_Engine_WriteByte not found!");
        return false;
    }
    if (pParam[3] || false == ::g_DoDEngine_WriteByte_Hook)
        ((::DoD_Engine_WriteByte_Type) ::g_pDoDEngine_WriteByte_Addr) (pParam[1], pParam[2]);
    else
        ::DoD_Engine_WriteByte(pParam[1], pParam[2]);
    return true;
#endif
}

::cell DoD_Engine_WriteBits_Native(::tagAMX* pAmx, ::cell* pParam)
{
#ifndef __linux__
    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "DoD_Engine_WriteBits() native not implemented on Windows!");
    return false;
#else
    if (!::g_pDoDEngine_WriteBits_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_Engine_WriteBits not found!");
        return false;
    }
    if (pParam[3] || false == ::g_DoDEngine_WriteBits_Hook)
        ((::DoD_Engine_WriteBits_Type) ::g_pDoDEngine_WriteBits_Addr) (pParam[1], pParam[2]);
    else
        ::DoD_Engine_WriteBits(pParam[1], pParam[2]);
    return true;
#endif
}

::cell DoD_Engine_EmitPings_Native(::tagAMX* pAmx, ::cell* pParam)
{
#ifndef __linux__
    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "DoD_Engine_EmitPings() native not implemented on Windows!");
    return false;
#else
    if (!::g_pDoDEngine_EmitPings_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_Engine_EmitPings not found!");
        return false;
    }
    if (pParam[3] || false == ::g_DoDEngine_EmitPings_Hook)
        ((::DoD_Engine_EmitPings_Type) ::g_pDoDEngine_EmitPings_Addr) (pParam[1], pParam[2]);
    else
        ::DoD_Engine_EmitPings(pParam[1], pParam[2]);
    return true;
#endif
}

::cell DoD_Engine_CalcPing_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    if (pRes)
        *pRes = -1;

    if (!::g_pDoDEngine_CalcPing_Addr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_Engine_CalcPing not found!");
        return false;
    }
    if (::size_t(-1) == ::g_engConvOffs)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Conversion offset (from ::client_s* to ::edict_t*) for ::DoD_Engine_CalcPing not found or not yet ready!");
        return false;
    }

    auto Player = pParam[1];
    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }
    if (false == ::g_svPlayerEngPtrs[Player])
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Engine not yet ready to reveal the ping of player %d!", Player);
        return false;
    }

#ifdef __linux__
    if (pRes)
    {
        if (pParam[3] || false == ::g_DoDEngine_CalcPing_Hook)
            *pRes = ((::DoD_Engine_CalcPing_Type) ::g_pDoDEngine_CalcPing_Addr) (::g_svPlayerEngPtrs[Player]);
        else
            *pRes = ::DoD_Engine_CalcPing(::g_svPlayerEngPtrs[Player]);
    }
    else
    {
        if (pParam[3] || false == ::g_DoDEngine_CalcPing_Hook)
            ((::DoD_Engine_CalcPing_Type) ::g_pDoDEngine_CalcPing_Addr) (::g_svPlayerEngPtrs[Player]);
        else
            ::DoD_Engine_CalcPing(::g_svPlayerEngPtrs[Player]);
    }
#else
    switch (::g_ReHLDS)
    {
    case false:
    {
        if (pRes)
        {
            if (pParam[3] || false == ::g_DoDEngine_CalcPing_Hook)
                *pRes = ((::DoD_Engine_CalcPing_Type) ::g_pDoDEngine_CalcPing_Addr) (::g_svPlayerEngPtrs[Player]);
            else
                *pRes = ::DoD_Engine_CalcPing(::g_svPlayerEngPtrs[Player]);
        }
        else
        {
            if (pParam[3] || false == ::g_DoDEngine_CalcPing_Hook)
                ((::DoD_Engine_CalcPing_Type) ::g_pDoDEngine_CalcPing_Addr) (::g_svPlayerEngPtrs[Player]);
            else
                ::DoD_Engine_CalcPing(::g_svPlayerEngPtrs[Player]);
        }
        break;
    }
    default:
    {
        if (pRes)
        {
            if (pParam[3] || false == ::g_DoDEngine_CalcPing_Hook)
                *pRes = ((::DoD_Engine_CalcPing_Type_ReHLDS_Win) ::g_pDoDEngine_CalcPing_Addr) (::g_svPlayerEngPtrs[Player]);
            else
                *pRes = ::DoD_Engine_CalcPing_ReHLDS_Win(::g_svPlayerEngPtrs[Player]);
        }
        else
        {
            if (pParam[3] || false == ::g_DoDEngine_CalcPing_Hook)
                ((::DoD_Engine_CalcPing_Type_ReHLDS_Win) ::g_pDoDEngine_CalcPing_Addr) (::g_svPlayerEngPtrs[Player]);
            else
                ::DoD_Engine_CalcPing_ReHLDS_Win(::g_svPlayerEngPtrs[Player]);
        }
        break;
    }
    }
#endif
    return true;
}

::cell DoD_GetPvDataBool_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Entity = pParam[1];
    if (Entity < 0 || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::F_IToE(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = (bool*)pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

#ifndef __linux__
    return *(pEntityBase + pParam[2]);
#else
    return *(pEntityBase + pParam[2] + pParam[3]);
#endif
}

::cell DoD_SetPvDataBool_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Entity = pParam[1];
    if (Entity < 0 || Entity > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity index %d!", Entity);
        return false;
    }

    auto pEntity = ::F_IToE(Entity);
    if (!pEntity)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no edict!", Entity);
        return false;
    }

    auto pEntityBase = (bool*)pEntity->pvPrivateData;
    if (!pEntityBase)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Entity %d has no private data!", Entity);
        return false;
    }

#ifndef __linux__
    * (pEntityBase + pParam[2]) = (bool)pParam[3];
#else
    * (pEntityBase + pParam[2] + pParam[4]) = (bool)pParam[3];
#endif
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

    auto pItem = ::F_IToE(Item);
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

    return *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & true;
}

::cell DoD_AddScope_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Item = pParam[1];
    if (Item <= ::gpGlobals->maxClients || Item > ::gpGlobals->maxEntities)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Item);
        return false;
    }

    auto pItem = ::F_IToE(Item);
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

    if (!(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & true))
    {
        *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= true;
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

    auto pItem = ::F_IToE(Item);
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

::cell DoD_ChangeMap_Native(::tagAMX* pAmx, ::cell* pParam)
{
    int Len;
    auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
    if (Len < 1 || !pName || false == *pName)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid map name string (null or empty)!");
        return false;
    }

    switch (pParam[2])
    {
    case false:
        ::g_engfuncs.pfnChangeLevel(pName, NULL);
        break;
    default:
        ::g_pEngineHookTable->pfnChangeLevel(pName, NULL);
        break;
    }
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
    auto pDosHdr = (::_IMAGE_DOS_HEADER*)memInfo.AllocationBase;
    auto pNtHdr = (::_IMAGE_NT_HEADERS*)((::size_t)pDosHdr + (::size_t)pDosHdr->e_lfanew);
    ::SourceHook::CVector < unsigned char > Signature;
    ::vectorizeSignature(pSig, Signature);
    auto Res = ::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, Signature, &Addr, pParam[3]);
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

::cell DoD_AddPlayerCorpseManager_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pFunctionTable_Post->pfnPlayerPostThink)
    {
        ::g_pFunctionTable_Post->pfnPlayerPostThink = ::PlayerPostThink_Post;

        ::g_deadPlayerCorpsePevFrameRate = ::g_fn_CellToReal(pParam[1]);
        ::g_deadPlayerCorpseCBaseFrameRate = ::g_fn_CellToReal(pParam[2]);
        ::g_deadPlayerCorpseRespawnFramesSub = ::g_fn_CellToReal(pParam[3]);
        ::g_deadPlayerCorpseAlterCBaseFrameRate = bool(pParam[4]);
        ::g_deadPlayerCorpseEnableInterpolation = bool(pParam[5]);

        return true;
    }
    return false;
}

::cell DoD_DelPlayerCorpseManager_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (::g_pFunctionTable_Post->pfnPlayerPostThink)
    {
        ::g_pFunctionTable_Post->pfnPlayerPostThink = NULL;

        return true;
    }
    return false;
}

::cell DoD_AddSelfExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    bool Res = !::g_selfNade;
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
    if (!::g_pFunctionTable_Post->pfnAddToFullPack)
        ::g_pFunctionTable_Post->pfnAddToFullPack = ::AddToFullPack_Post;
    return Res;
}

::cell DoD_AddTeamExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    bool Res = !::g_teamNade;
    ::g_teamNade = true;
    ::g_teamNadeMode = pParam[1];
    ::g_teamNadeFx = pParam[2];
    auto pColor = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
    if (pColor)
    {
        ::g_teamNadeColor.r = ::byte(pColor[0]);
        ::g_teamNadeColor.g = ::byte(pColor[1]);
        ::g_teamNadeColor.b = ::byte(pColor[2]);
    }
    else
    {
        ::g_teamNadeColor.r = ::byte(20);
        ::g_teamNadeColor.g = ::byte(180);
        ::g_teamNadeColor.b = ::byte(200);
    }
    ::g_teamNadeAmt = pParam[4];
    ::g_teamNadeSolid = short(pParam[5]);
    ::g_teamNadeDoSolid = bool(pParam[6]);
    if (!::g_pFunctionTable_Post->pfnAddToFullPack)
        ::g_pFunctionTable_Post->pfnAddToFullPack = ::AddToFullPack_Post;
    return Res;
}

::cell DoD_DelSelfExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (::g_selfNade)
    {
        ::g_selfNade = false;
        if (!::g_droppedNade && !::g_teamNade && ::g_pFunctionTable_Post->pfnAddToFullPack)
            ::g_pFunctionTable_Post->pfnAddToFullPack = NULL;
        return true;
    }
    return false;
}

::cell DoD_DelTeamExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (::g_teamNade)
    {
        ::g_teamNade = false;
        if (!::g_droppedNade && !::g_selfNade && ::g_pFunctionTable_Post->pfnAddToFullPack)
            ::g_pFunctionTable_Post->pfnAddToFullPack = NULL;
        return true;
    }
    return false;
}

::cell DoD_AddDroppedExploNadeGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    bool Res = !::g_droppedNade;
    ::g_droppedNade = true;
    ::g_droppedNadeDoArmed = bool(pParam[1]);
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
    if (!::g_pFunctionTable_Post->pfnAddToFullPack)
        ::g_pFunctionTable_Post->pfnAddToFullPack = ::AddToFullPack_Post;
    return Res;
}

::cell DoD_DelDroppedExploNadeGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (::g_droppedNade)
    {
        ::g_droppedNade = false;
        if (!::g_selfNade && !::g_teamNade && ::g_pFunctionTable_Post->pfnAddToFullPack)
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
    ::g_exclDroppedNadeGlow[Player] = bool(pParam[2]);
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
    return ::g_exclDroppedNadeGlow[Player];
}

::cell DoD_SetSelfExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }
    ::g_exclSelfNadeGlow[Player] = bool(pParam[2]);
    return true;
}

::cell DoD_GetSelfExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }
    return ::g_exclSelfNadeGlow[Player];
}

::cell DoD_SetTeamExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }
    ::g_exclTeamNadeGlow[Player] = bool(pParam[2]);
    return true;
}

::cell DoD_GetTeamExploNadeProjGlow_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (Player < 1 || Player > ::gpGlobals->maxClients)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
        return false;
    }
    return ::g_exclTeamNadeGlow[Player];
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

::cell DoD_FindStringPush_Native(::tagAMX* pAmx, ::cell* pParam)
{
#ifdef __linux__
    ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Not currently implemented on Linux!");
    return false;
#else
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

    auto pStr = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    if (Len < 1 || !pStr || false == *pStr)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid const. string (null or empty)!");
        return false;
    }

    auto Opened = false;
    auto pMod = ::openLib(pName, Opened);
    if (!pMod)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::GetModuleHandleA/ ::LoadLibraryA failed for '%s'!", pName);
        return false;
    }
    ::_MEMORY_BASIC_INFORMATION memInfo{ };
    if (!::VirtualQuery(pMod, &memInfo, sizeof memInfo) || !memInfo.AllocationBase)
    {
        if (Opened)
            ::FreeLibrary(pMod);
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::VirtualQuery failed for '%s'!", pName);
        return false;
    }
    auto pDosHdr = (::_IMAGE_DOS_HEADER*)memInfo.AllocationBase;
    auto pNtHdr = (::_IMAGE_NT_HEADERS*)((::size_t)pDosHdr + (::size_t)pDosHdr->e_lfanew);
    auto Addr = (::size_t) ::findStr(pDosHdr, pNtHdr, pStr, Len);
    if (Addr && pAddr)
        *pAddr = Addr;
    if (Opened)
        ::FreeLibrary(pMod);
    return false != Addr;
#endif
}

::cell DoD_StoreFloatToAddress_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Addr = (::size_t)pParam[1];
    if (Addr < 1)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid memory address provided!");
        return false;
    }

    ::allowFullMemAccess((void*)Addr, sizeof(float));
    *(float*)Addr = ::g_fn_CellToReal(pParam[2]);
    return true;
}

::cell DoD_ReadFloatFromAddress_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto pRes = ::g_fn_GetAmxAddr(pAmx, pParam[2]);
    if (pRes)
        *pRes = false;

    auto Addr = (::size_t)pParam[1];
    if (Addr < 1)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid memory address provided!");
        return false;
    }

    auto Res = (::cell) ::g_fn_RealToCell(*(float*)Addr);
    if (pRes)
        *pRes = Res;
    return true;
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
        *(signed char*)Addr = (signed char)pParam[2];
        return true;
    }
    case ::DoD_Size::UInt8:
    {
        ::allowFullMemAccess((void*)Addr, true);
        *(unsigned char*)Addr = (unsigned char)pParam[2];
        return true;
    }
    case ::DoD_Size::Int16:
    {
        ::allowFullMemAccess((void*)Addr, sizeof(short));
        *(signed short*)Addr = (signed short)pParam[2];
        return true;
    }
    case ::DoD_Size::UInt16:
    {
        ::allowFullMemAccess((void*)Addr, sizeof(short));
        *(unsigned short*)Addr = (unsigned short)pParam[2];
        return true;
    }
    case ::DoD_Size::Int32:
    {
        ::allowFullMemAccess((void*)Addr, sizeof(int));
        *(signed int*)Addr = (signed int)pParam[2];
        return true;
    }
    case ::DoD_Size::UInt32:
    {
        ::allowFullMemAccess((void*)Addr, sizeof(::size_t));
        *(::size_t*)Addr = (::size_t)pParam[2];
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
    case ::DoD_Size::UInt32:
    {
        Res = (::cell) * (::size_t*)Addr;
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

    auto pEntity = ::F_IToE(Entity);
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

    auto pEntity = ::F_IToE(Entity);
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

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return ::g_fn_RealToCell(*(float*)(pAddr + ::g_Sigs[::DoD_Sig::Offs_AxisRespawnFactor].Offs));
}

::cell DoD_GetAlliesRespawnFactor_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return ::g_fn_RealToCell(*(float*)(pAddr + ::g_Sigs[::DoD_Sig::Offs_AlliesRespawnFactor].Offs));
}

::cell DoD_ReadGameRulesBool_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    return bool(*(pAddr + pParam[1]));
}

::cell DoD_StoreGameRulesBool_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
    *(pAddr + pParam[1]) = bool(pParam[2]);
    return true;
}

::cell DoD_ReadGameRulesByte_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return *(char*)(pAddr + pParam[1]);
}

::cell DoD_StoreGameRulesByte_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    *(char*)(pAddr + pParam[1]) = char(pParam[2]);
    return true;
}

::cell DoD_ReadGameRulesUByte_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return *(unsigned char*)(pAddr + pParam[1]);
}

::cell DoD_StoreGameRulesUByte_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    *(unsigned char*)(pAddr + pParam[1]) = (unsigned char)pParam[2];
    return true;
}

::cell DoD_ReadGameRulesShort_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return *(short*)(pAddr + pParam[1]);
}

::cell DoD_StoreGameRulesShort_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    *(short*)(pAddr + pParam[1]) = short(pParam[2]);
    return true;
}

::cell DoD_ReadGameRulesUShort_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return *(unsigned short*)(pAddr + pParam[1]);
}

::cell DoD_StoreGameRulesUShort_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    *(unsigned short*)(pAddr + pParam[1]) = (unsigned short)pParam[2];
    return true;
}

::cell DoD_ReadGameRulesInt_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return *(::cell*)(pAddr + pParam[1]);
}

::cell DoD_StoreGameRulesInt_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    *(::cell*)(pAddr + pParam[1]) = ::cell(pParam[2]);
    return true;
}

::cell DoD_ReadGameRulesUInt_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return *(::size_t*)(pAddr + pParam[1]);
}

::cell DoD_StoreGameRulesUInt_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    *(::size_t*)(pAddr + pParam[1]) = (::size_t)pParam[2];
    return true;
}

::cell DoD_ReadGameRulesFloat_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    return ::g_fn_RealToCell(*(float*)(pAddr + pParam[1]));
}

::cell DoD_StoreGameRulesFloat_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    *(float*)(pAddr + pParam[1]) = ::g_fn_CellToReal(pParam[2]);
    return true;
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

::cell DoD_StoreGameRulesStr_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (false == ::g_CDoDTeamPlay)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
        return false;
    }

    int Len;
    auto pString = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
    auto pAddr = (unsigned char*) ::g_CDoDTeamPlay;
    if (Len < 1 || !pString || false == *pString)
        *(pAddr + pParam[1]) = false;
    else
    {
        ::size_t Iter = false;
        for (; Iter < (::size_t)Len; Iter++)
            *(pAddr + pParam[1] + Iter) = pString[Iter];
        *(pAddr + pParam[1] + Iter) = false;
    }
    return true;
}

::cell DoD_ClassIndexToName_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (pParam[4])
    {
        const auto pName = ::classNameByClassIndex(pParam[1]);
        return ::g_fn_SetAmxStringUTF8Char(pAmx, pParam[2], pName, ::strlen(pName), pParam[3]);
    }
    return ::g_fn_SetAmxString(pAmx, pParam[2], ::classNameByClassIndex(pParam[1]), pParam[3]);
}

::cell DoD_TeamIndexToName_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Team = pParam[1];
    if (Team < 0 || Team > 3)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid team index %d!", Team);
        return false;
    }

    if (pParam[4])
    {
        const auto pName = ::teamNameByTeamIndex(Team, false);
        return ::g_fn_SetAmxStringUTF8Char(pAmx, pParam[2], pName, ::strlen(pName), pParam[3]);
    }
    return ::g_fn_SetAmxString(pAmx, pParam[2], ::classNameByClassIndex(Team), pParam[3]);
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

::cell DoD_AddAdvancedDeploy_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (::g_pFunctionTable->pfnCmdStart)
        return false;
    ::g_pFunctionTable->pfnCmdStart = (decltype (::g_pFunctionTable->pfnCmdStart)) ::CmdStart;
    return true;
}

::cell DoD_DelAdvancedDeploy_Native(::tagAMX* pAmx, ::cell* pParam)
{
    if (!::g_pFunctionTable->pfnCmdStart)
        return false;
    ::g_pFunctionTable->pfnCmdStart = NULL;
    for (int Item, Player = 1; Player <= ::gpGlobals->maxClients; Player++)
    {
        auto pPlayer = ::F_IToE(Player);
        if (pPlayer && !pPlayer->v.deadflag && pPlayer->v.health > 0.f)
        {
            bool scopeAttached;
            if ((pPlayer->v.weapons & (DOD_BAR | DOD_MG42 | DOD_30CAL | DOD_MG34 | DOD_FG42 | DOD_BREN)) &&
                ::playerActiveItem(pPlayer, Item, scopeAttached) && false == scopeAttached &&
                (Item & (DOD_BAR | DOD_MG42 | DOD_30CAL | DOD_MG34 | DOD_FG42 | DOD_BREN)))
            {
                if (2.f != pPlayer->v.vuser1.x)
                    pPlayer->v.vuser1.x = false;
            }
            else
                pPlayer->v.vuser1.x = false;
        }
    }
    return true;
}

::cell DoD_PlayerOwnsDeployableGun_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }

    auto pPlayer = ::F_IToE(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }
    return (pPlayer->v.weapons & (DOD_BAR | DOD_MG42 | DOD_30CAL | DOD_MG34 | DOD_FG42 | DOD_BREN));
}

::cell DoD_PlayerHoldsDeployableGun_Native(::tagAMX* pAmx, ::cell* pParam)
{
    auto Player = pParam[1];
    if (!::g_fn_IsPlayerValid(Player))
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
        return false;
    }

    auto pPlayer = ::F_IToE(Player);
    if (!pPlayer)
    {
        ::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
        return false;
    }
    if (!(pPlayer->v.weapons & (DOD_BAR | DOD_MG42 | DOD_30CAL | DOD_MG34 | DOD_FG42 | DOD_BREN)))
        return false;
    int Item;
    bool scopeAttached;
    if (!::playerActiveItem(pPlayer, Item, scopeAttached) || scopeAttached)
        return false;
    return (Item & (DOD_BAR | DOD_MG42 | DOD_30CAL | DOD_MG34 | DOD_FG42 | DOD_BREN));
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
        case ::DoD_Func::Fn_WpnBoxKill: return (::cell) ::g_pDoDWpnBoxKill_Addr;
        case ::DoD_Func::Fn_WpnBoxActivateThink: return (::cell) ::g_pDoDWpnBoxActivateThink_Addr;
        case ::DoD_Func::Fn_ChangePlayerTeam: return (::cell) ::g_pDoDChangePlayerTeam_Addr;
        case ::DoD_Func::Fn_ChooseRandomClass: return (::cell) ::g_pDoDChooseRandomClass_Addr;
        case ::DoD_Func::Fn_Create: return (::cell) ::g_pDoDCreate_Addr;
        case ::DoD_Func::Fn_InstallGameRules: return (::cell) ::g_pDoDInstallGameRules_Addr;
        case ::DoD_Func::Fn_UtilRemove: return (::cell) ::g_pDoDUtilRemove_Addr;
        case ::DoD_Func::Fn_CreateNamedEntity: return (::cell) ::g_pDoDCreateNamedEntity_Addr;
        case ::DoD_Func::Fn_Engine_CalcPing: return (::cell) ::g_pDoDEngine_CalcPing_Addr;
        case ::DoD_Func::Fn_Engine_HostPing: return (::cell) ::g_pDoDEngine_HostPing_Addr;
        case ::DoD_Func::Fn_Engine_HostStatus: return (::cell) ::g_pDoDEngine_HostStatus_Addr;
        case ::DoD_Func::Fn_Engine_HostStat: return (::cell) ::g_pDoDEngine_HostStat_Addr;

#ifdef __linux__
        case ::DoD_Func::Fn_Engine_WriteByte: return (::cell) ::g_pDoDEngine_WriteByte_Addr;
        case ::DoD_Func::Fn_Engine_WriteBits: return (::cell) ::g_pDoDEngine_WriteBits_Addr;
        case ::DoD_Func::Fn_Engine_EmitPings: return (::cell) ::g_pDoDEngine_EmitPings_Addr;
#endif
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
    case ::DoD_Func::Fn_WpnBoxKill: return ::cell(::g_DoDWpnBoxKill_Hook ? ::DoD_WpnBoxKill : ::g_pDoDWpnBoxKill_Addr);
    case ::DoD_Func::Fn_WpnBoxActivateThink: return ::cell(::g_DoDWpnBoxActivateThink_Hook ? ::DoD_WpnBoxActivateThink : ::g_pDoDWpnBoxActivateThink_Addr);
    case ::DoD_Func::Fn_ChangePlayerTeam: return ::cell(::g_DoDChangePlayerTeam_Hook ? ::DoD_ChangePlayerTeam : ::g_pDoDChangePlayerTeam_Addr);
    case ::DoD_Func::Fn_ChooseRandomClass: return ::cell(::g_DoDChooseRandomClass_Hook ? ::DoD_ChooseRandomClass : ::g_pDoDChooseRandomClass_Addr);
    case ::DoD_Func::Fn_Create: return ::cell(::g_DoDCreate_Hook ? ::DoD_Create : ::g_pDoDCreate_Addr);
    case ::DoD_Func::Fn_InstallGameRules: return ::cell(::g_DoDInstallGameRules_Hook ? ::DoD_InstallGameRules : ::g_pDoDInstallGameRules_Addr);
    case ::DoD_Func::Fn_UtilRemove: return ::cell(::g_DoDUtilRemove_Hook ? ::DoD_UtilRemove : ::g_pDoDUtilRemove_Addr);
    case ::DoD_Func::Fn_CreateNamedEntity: return ::cell(::g_DoDCreateNamedEntity_Hook ? ::DoD_CreateNamedEntity : ::g_pDoDCreateNamedEntity_Addr);
#ifdef __linux__
    case ::DoD_Func::Fn_Engine_CalcPing: return ::cell(::g_DoDEngine_CalcPing_Hook ? ::DoD_Engine_CalcPing : ::g_pDoDEngine_CalcPing_Addr);
    case ::DoD_Func::Fn_Engine_WriteByte: return ::cell(::g_DoDEngine_WriteByte_Hook ? ::DoD_Engine_WriteByte : ::g_pDoDEngine_WriteByte_Addr);
    case ::DoD_Func::Fn_Engine_WriteBits: return ::cell(::g_DoDEngine_WriteBits_Hook ? ::DoD_Engine_WriteBits : ::g_pDoDEngine_WriteBits_Addr);
    case ::DoD_Func::Fn_Engine_EmitPings: return ::cell(::g_DoDEngine_EmitPings_Hook ? ::DoD_Engine_EmitPings : ::g_pDoDEngine_EmitPings_Addr);
#else
    case ::DoD_Func::Fn_Engine_CalcPing:
    {
        if (false == ::g_ReHLDS)
            return ::cell(::g_DoDEngine_CalcPing_Hook ? ::DoD_Engine_CalcPing : ::g_pDoDEngine_CalcPing_Addr);
        return ::cell(::g_DoDEngine_CalcPing_Hook ? ::DoD_Engine_CalcPing_ReHLDS_Win : ::g_pDoDEngine_CalcPing_Addr);
    }
#endif
    case ::DoD_Func::Fn_Engine_HostPing: return ::cell(::g_DoDEngine_HostPing_Hook ? ::DoD_Engine_HostPing : ::g_pDoDEngine_HostPing_Addr);
    case ::DoD_Func::Fn_Engine_HostStatus: return ::cell(::g_DoDEngine_HostStatus_Hook ? ::DoD_Engine_HostStatus : ::g_pDoDEngine_HostStatus_Addr);
    case ::DoD_Func::Fn_Engine_HostStat: return ::cell(::g_DoDEngine_HostStat_Hook ? ::DoD_Engine_HostStat : ::g_pDoDEngine_HostStat_Addr);
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

    auto pVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
    if (!pVars)
    {
        ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
        return;
    }

    auto pPlayer = pVars->pContainingEntity;
    if (!pPlayer)
    {
        ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
        return;
    }

    auto Player = (::cell) ::F_EToI(pPlayer);
    auto origPlayer = Player;
    if (::g_fn_ExecuteForward(::g_fwPlayerSpawn, CDoDTeamPlay, &Player))
        return;

    auto nonPlayer = Player < 1 || Player > ::gpGlobals->maxClients;
    switch (nonPlayer)
    {
    case false:
    {
        pPlayer = ::F_IToE(Player);
        if (pPlayer)
        {
            auto pBase = pPlayer->pvPrivateData;
            if (pBase)
            {
                ::DoD_PlayerSpawn(CDoDTeamPlay, (::size_t)pBase);
                ::g_fn_ExecuteForward(::g_fwPlayerSpawn_Post, CDoDTeamPlay, Player);
            }
            else
            {
                ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
                ::g_fn_ExecuteForward(::g_fwPlayerSpawn_Post, CDoDTeamPlay, origPlayer);
            }
        }
        else
        {
            ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
            ::g_fn_ExecuteForward(::g_fwPlayerSpawn_Post, CDoDTeamPlay, origPlayer);
        }
        break;
    }
    default:
    {
        ::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
        ::g_fn_ExecuteForward(::g_fwPlayerSpawn_Post, CDoDTeamPlay, origPlayer);
        break;
    }
    }
}

void __fastcall DoD_SetBodygroup_Hook(::size_t CBasePlayer, FASTCALL_PARAM int Group, int Value)
{
    if (false == CBasePlayer)
    {
        ::DoD_SetBodygroup(CBasePlayer, Group, Value);
        return;
    }

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
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

    auto Player = ::F_EToI(pPlayer);
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

    auto pEntityVars = *(::entvars_s**)(CBaseEntity + ::g_entvarsOffs);
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

    auto Entity = ::F_EToI(pEntity);
    if (::g_fn_ExecuteForward(::g_fwSubRemove, Entity))
        return;

    ::DoD_SubRemove(CBaseEntity);
    ::g_fn_ExecuteForward(::g_fwSubRemove_Post, Entity);
}

int __fastcall DoD_PackWeapon_Hook(::size_t CWeaponBox, FASTCALL_PARAM::size_t CBasePlayerItem)
{
    if (false == CWeaponBox || false == CBasePlayerItem)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    auto pEntityVars = *(::entvars_s**)(CWeaponBox + ::g_entvarsOffs);
    if (!pEntityVars)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    auto pWeaponVars = *(::entvars_s**)(CBasePlayerItem + ::g_entvarsOffs);
    if (!pWeaponVars)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    auto pEntity = pEntityVars->pContainingEntity;
    if (!pEntity)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    auto pWeapon = pWeaponVars->pContainingEntity;
    if (!pWeapon)
        return ::DoD_PackWeapon(CWeaponBox, CBasePlayerItem);

    ::cell Override = false;
    auto Entity = ::F_EToI(pEntity), Weapon = ::F_EToI(pWeapon);
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

    auto pEntityVars = *(::entvars_s**)(CBaseEntity + ::g_entvarsOffs);
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

    auto Entity = ::F_EToI(pEntity);
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
        return Override < 0 ? NULL : ::F_IToE(Override);

    auto pEntity = ::DoD_CreateNamedEntity(::setupString(Buffer));
    ::g_fn_ExecuteForward(::g_fwCreateNamedEntity_Post, Buffer, pEntity ? ::F_EToI(pEntity) : -1);
    return pEntity;
}

void __fastcall DoD_WpnBoxKill_Hook(::size_t CWeaponBox FASTCALL_PARAM_ALONE)
{
    if (false == CWeaponBox)
    {
        ::DoD_WpnBoxKill(CWeaponBox);
        return;
    }

    auto pEntityVars = *(::entvars_s**)(CWeaponBox + ::g_entvarsOffs);
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

    auto Entity = ::F_EToI(pEntity);
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

    auto pEntityVars = *(::entvars_s**)(CWeaponBox + ::g_entvarsOffs);
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

    auto Entity = ::F_EToI(pEntity);
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
    auto Owner = ::cell(pOwner ? ::F_EToI(pOwner) : -1);
    auto amxOrigin = ::g_fn_PrepareCellArrayA(Origin, ARRAYSIZE(Origin), true);
    auto amxAngles = ::g_fn_PrepareCellArrayA(Angles, ARRAYSIZE(Angles), true);
    if (::g_fn_ExecuteForward(::g_fwCreate, Buffer, sizeof Buffer, amxOrigin, amxAngles, &Owner, &Override) || false == Buffer[0])
        return ::indexToBase(Override);

    auto vecOrigin = ::Vector(::g_fn_CellToReal(Origin[0]), ::g_fn_CellToReal(Origin[1]), ::g_fn_CellToReal(Origin[2]));
    auto vecAngles = ::Vector(::g_fn_CellToReal(Angles[0]), ::g_fn_CellToReal(Angles[1]), ::g_fn_CellToReal(Angles[2]));
    ::edict_s* pNewOwner = ((Owner < 0 || Owner > ::gpGlobals->maxEntities) ? NULL : ::F_IToE(Owner));
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

    auto pEntityVars = *(::entvars_s**)(CBasePlayerItem + ::g_entvarsOffs);
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

    auto Entity = ::F_EToI(pEntity);
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

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
    if (!pPlayerVars)
        return ::DoD_GiveNamedItem(CBasePlayer, pItem);

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
        return ::DoD_GiveNamedItem(CBasePlayer, pItem);

    ::cell Override = -1;
    auto Player = ::F_EToI(pPlayer);
    static char Buffer[64];
#ifndef __linux__
    ::strncpy_s(Buffer, sizeof Buffer, pItem, _TRUNCATE);
#else
    ::snprintf(Buffer, sizeof Buffer, pItem);
#endif
    if (::g_fn_ExecuteForward(::g_fwGiveNamedItem, Player, Buffer, sizeof Buffer, &Override) || false == Buffer[0])
        return Override < 0 ? NULL : ::F_IToE(Override);

    ::edict_s* pEntity;
    if (false == ::_stricmp("weapon_scopedfg42", Buffer))
    {
        pEntity = ::DoD_GiveNamedItem(CBasePlayer, STRING(::setupString("weapon_fg42")));
        if (pEntity)
        {
            auto pItemBase = (::size_t*)pEntity->pvPrivateData;
            if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & true))
            {
                *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= true;
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
            if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) & true))
            {
                *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::DoD_Sig::Offs_ItemScope].Offs) |= true;
                (*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::DoD_Sig::Offs_ApplyItemScope].Offs)) (pItemBase);
            }
        }
    }
    else
        pEntity = ::DoD_GiveNamedItem(CBasePlayer, STRING(::setupString(Buffer)));

    ::g_fn_ExecuteForward(::g_fwGiveNamedItem_Post, Player, Buffer, pEntity ? ::F_EToI(pEntity) : -1);
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

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
    if (!pPlayerVars)
        return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
        return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);

    ::cell Override = -1;
    auto Player = ::F_EToI(pPlayer);
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

void __fastcall DoD_ChangePlayerTeam_Hook(::size_t CDoDTeamPlay, FASTCALL_PARAM::size_t CBasePlayer, int Team, int Kill, int Gib)
{
    if (false != CDoDTeamPlay)
        ::g_CDoDTeamPlay = CDoDTeamPlay;

    if (false == CBasePlayer)
    {
        ::DoD_ChangePlayerTeam(CDoDTeamPlay, CBasePlayer, Team, Kill, Gib);
        return;
    }

    auto pVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
    if (!pVars)
    {
        ::DoD_ChangePlayerTeam(CDoDTeamPlay, CBasePlayer, Team, Kill, Gib);
        return;
    }

    auto pPlayer = pVars->pContainingEntity;
    if (!pPlayer)
    {
        ::DoD_ChangePlayerTeam(CDoDTeamPlay, CBasePlayer, Team, Kill, Gib);
        return;
    }

    auto Player = (::cell) ::F_EToI(pPlayer);
    auto origPlayer = Player;
    if (::g_fn_ExecuteForward(::g_fwChangePlayerTeam, CDoDTeamPlay, &Player, &Team, &Kill, &Gib))
        return;

    auto nonPlayer = Player < 1 || Player > ::gpGlobals->maxClients;
    switch (nonPlayer)
    {
    case false:
    {
        pPlayer = ::F_IToE(Player);
        if (pPlayer)
        {
            auto pBase = pPlayer->pvPrivateData;
            if (pBase)
            {
                ::DoD_ChangePlayerTeam(CDoDTeamPlay, (::size_t)pBase, Team, Kill, Gib);
                ::g_fn_ExecuteForward(::g_fwChangePlayerTeam_Post, CDoDTeamPlay, Player, Team, Kill, Gib);
            }
            else
            {
                ::DoD_ChangePlayerTeam(CDoDTeamPlay, CBasePlayer, Team, Kill, Gib);
                ::g_fn_ExecuteForward(::g_fwChangePlayerTeam_Post, CDoDTeamPlay, origPlayer, Team, Kill, Gib);
            }
        }
        else
        {
            ::DoD_ChangePlayerTeam(CDoDTeamPlay, CBasePlayer, Team, Kill, Gib);
            ::g_fn_ExecuteForward(::g_fwChangePlayerTeam_Post, CDoDTeamPlay, origPlayer, Team, Kill, Gib);
        }
        break;
    }
    default:
    {
        ::DoD_ChangePlayerTeam(CDoDTeamPlay, CBasePlayer, Team, Kill, Gib);
        ::g_fn_ExecuteForward(::g_fwChangePlayerTeam_Post, CDoDTeamPlay, origPlayer, Team, Kill, Gib);
        break;
    }
    }
}

void __fastcall DoD_ChooseRandomClass_Hook(::size_t CDoDTeamPlay, FASTCALL_PARAM::size_t CBasePlayer)
{
    if (false != CDoDTeamPlay)
        ::g_CDoDTeamPlay = CDoDTeamPlay;

    if (false == CBasePlayer)
    {
        ::DoD_ChooseRandomClass(CDoDTeamPlay, CBasePlayer);
        return;
    }

    auto pVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
    if (!pVars)
    {
        ::DoD_ChooseRandomClass(CDoDTeamPlay, CBasePlayer);
        return;
    }

    auto pPlayer = pVars->pContainingEntity;
    if (!pPlayer)
    {
        ::DoD_ChooseRandomClass(CDoDTeamPlay, CBasePlayer);
        return;
    }

    auto Player = (::cell) ::F_EToI(pPlayer);
    auto origPlayer = Player;
    if (::g_fn_ExecuteForward(::g_fwChooseRandomClass, CDoDTeamPlay, &Player))
        return;

    auto nonPlayer = Player < 1 || Player > ::gpGlobals->maxClients;
    switch (nonPlayer)
    {
    case false:
    {
        pPlayer = ::F_IToE(Player);
        if (pPlayer)
        {
            auto pBase = pPlayer->pvPrivateData;
            if (pBase)
            {
                ::DoD_ChooseRandomClass(CDoDTeamPlay, (::size_t)pBase);
                auto Class = *(::cell*)(((unsigned char*)pBase) + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_NextClass].Offs);
                ::g_fn_ExecuteForward(::g_fwChooseRandomClass_Post, CDoDTeamPlay, Player, Class);
            }
            else
            {
                ::DoD_ChooseRandomClass(CDoDTeamPlay, CBasePlayer);
                auto Class = *(::cell*)(((unsigned char*)CBasePlayer) + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_NextClass].Offs);
                ::g_fn_ExecuteForward(::g_fwChooseRandomClass_Post, CDoDTeamPlay, origPlayer, Class);
            }
        }
        else
        {
            ::DoD_ChooseRandomClass(CDoDTeamPlay, CBasePlayer);
            auto Class = *(::cell*)(((unsigned char*)CBasePlayer) + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_NextClass].Offs);
            ::g_fn_ExecuteForward(::g_fwChooseRandomClass_Post, CDoDTeamPlay, origPlayer, Class);
        }
        break;
    }
    default:
    {
        ::DoD_ChooseRandomClass(CDoDTeamPlay, CBasePlayer);
        auto Class = *(::cell*)(((unsigned char*)CBasePlayer) + ::g_Sigs[::DoD_Sig::Offs_CBasePlayer_NextClass].Offs);
        ::g_fn_ExecuteForward(::g_fwChooseRandomClass_Post, CDoDTeamPlay, origPlayer, Class);
        break;
    }
    }
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

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
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
    auto Player = ::F_EToI(pPlayer);
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

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
    if (!pPlayerVars)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    auto pItemVars = *(::entvars_s**)(CBasePlayerItem + ::g_entvarsOffs);
    if (!pItemVars)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    auto pItem = pItemVars->pContainingEntity;
    if (!pItem)
        return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);

    ::cell Override = false;
    auto Player = ::F_EToI(pPlayer), Item = ::F_EToI(pItem);
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

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
    if (!pPlayerVars)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    auto pItemVars = *(::entvars_s**)(CBasePlayerItem + ::g_entvarsOffs);
    if (!pItemVars)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    auto pPlayer = pPlayerVars->pContainingEntity;
    if (!pPlayer)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    auto pItem = pItemVars->pContainingEntity;
    if (!pItem)
        return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);

    ::cell Override = false;
    auto Player = ::F_EToI(pPlayer), Item = ::F_EToI(pItem);
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

    auto pPlayerVars = *(::entvars_s**)(CBasePlayer + ::g_entvarsOffs);
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

    auto Player = ::F_EToI(pPlayer);
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

int DoD_Engine_CalcPing_Hook(::size_t client_s)
{
    static auto Found = ::edict_s_Ptr_From_client_s_Ptr_Offs(client_s, ::g_engConvOffs,
        ::g_Sigs[::DoD_Sig::Setting_Engine_Client_Ptr_Min_Ofs_To_Entity_Ptr].Offs,
        ::g_Sigs[::DoD_Sig::Setting_Engine_Client_Ptr_Max_Ofs_To_Entity_Ptr].Offs);
    if (false == Found)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            ::MF_Log("*** Warning, ::DoD_Engine_CalcPing_Hook() failed to convert ::client_s* to ::edict_s*!"
                " " "Contact the author to update the module! ***");
            Logged = true;
        }
        return ::DoD_Engine_CalcPing(client_s);
    }
    auto pAddr = (unsigned char*)client_s;
    auto pPlayer = *(::edict_s**)(pAddr + ::g_engConvOffs);
    if (!pPlayer)
        return ::DoD_Engine_CalcPing(client_s);
    ::cell Ping = false;
    auto Player = (unsigned char) ::F_EToI(pPlayer);
    ::g_svPlayerEngPtrs[Player] = client_s;
    if (::g_fn_ExecuteForward(::g_fwEngine_CalcPing, Player, &Ping))
        return Ping;
    Ping = ::DoD_Engine_CalcPing(client_s);
    ::g_fn_ExecuteForward(::g_fwEngine_CalcPing_Post, Player, Ping);
    return Ping;
}

#ifndef __linux__
int __fastcall DoD_Engine_CalcPing_Hook_ReHLDS_Win(::size_t client_s FASTCALL_PARAM_ALONE)
{ /// Wrapper for Windows ReHLDS (__thiscall/ __fastcall).
    static auto Found = ::edict_s_Ptr_From_client_s_Ptr_Offs(client_s, ::g_engConvOffs,
        ::g_Sigs[::DoD_Sig::Setting_Engine_Client_Ptr_Min_Ofs_To_Entity_Ptr].Offs,
        ::g_Sigs[::DoD_Sig::Setting_Engine_Client_Ptr_Max_Ofs_To_Entity_Ptr].Offs);
    if (false == Found)
    {
        static bool Logged = false;
        if (false == Logged)
        {
            ::MF_Log("*** Warning, ::DoD_Engine_CalcPing_Hook() failed to convert ::client_s* to ::edict_s*!"
                " " "Contact the author to update the module! ***");
            Logged = true;
        }
        return ::DoD_Engine_CalcPing_ReHLDS_Win(client_s);
    }
    auto pAddr = (unsigned char*)client_s;
    auto pPlayer = *(::edict_s**)(pAddr + ::g_engConvOffs);
    if (!pPlayer)
        return ::DoD_Engine_CalcPing_ReHLDS_Win(client_s);
    ::cell Ping = false;
    auto Player = (unsigned char) ::F_EToI(pPlayer);
    ::g_svPlayerEngPtrs[Player] = client_s;
    if (::g_fn_ExecuteForward(::g_fwEngine_CalcPing, Player, &Ping))
        return Ping;
    Ping = ::DoD_Engine_CalcPing_ReHLDS_Win(client_s);
    ::g_fn_ExecuteForward(::g_fwEngine_CalcPing_Post, Player, Ping);
    return Ping;
}
#endif

void DoD_Engine_HostPing_Hook()
{
    if (::g_fn_ExecuteForward(::g_fwEngine_HostPing))
        return;
    ::DoD_Engine_HostPing();
    ::g_fn_ExecuteForward(::g_fwEngine_HostPing_Post);
}

void DoD_Engine_HostStatus_Hook()
{
    if (::g_fn_ExecuteForward(::g_fwEngine_HostStatus))
        return;
    ::DoD_Engine_HostStatus();
    ::g_fn_ExecuteForward(::g_fwEngine_HostStatus_Post);
}

void DoD_Engine_HostStat_Hook()
{
    if (::g_fn_ExecuteForward(::g_fwEngine_HostStat))
        return;
    ::DoD_Engine_HostStat();
    ::g_fn_ExecuteForward(::g_fwEngine_HostStat_Post);
}

#ifdef __linux__
void DoD_Engine_WriteByte_Hook(::size_t Message, int Value)
{
    ::cell newValue = ::cell(Value);
    if (::g_fn_ExecuteForward(::g_fwEngine_WriteByte, ::cell(Message), &newValue))
        return;
    ::DoD_Engine_WriteByte(Message, newValue);
    ::g_fn_ExecuteForward(::g_fwEngine_WriteByte_Post, ::cell(Message), newValue);
}

void DoD_Engine_WriteBits_Hook(::size_t Data, int Bits)
{
    if (Data > __INT_MAX__)
    {
        ::DoD_Engine_WriteBits(Data, Bits);
        return;
    }
    ::cell newData = ::cell(Data), newBits = ::cell(Bits);
    if (::g_fn_ExecuteForward(::g_fwEngine_WriteBits, &newData, &newBits))
        return;
    ::DoD_Engine_WriteBits((::size_t)(newData), newBits);
    ::g_fn_ExecuteForward(::g_fwEngine_WriteBits_Post, newData, newBits);
}

void DoD_Engine_EmitPings_Hook(::size_t client_s, ::size_t sizebuf_s)
{
    if (::g_fn_ExecuteForward(::g_fwEngine_EmitPings, ::cell(client_s), ::cell(sizebuf_s)))
        return;
    ::DoD_Engine_EmitPings(client_s, sizebuf_s);
    ::g_fn_ExecuteForward(::g_fwEngine_EmitPings_Post, ::cell(client_s), ::cell(sizebuf_s));
}
#endif

::AMX_NATIVE_INFO DoDHacks_Natives[] =
{
    { "DoD_AddHealthIfWounded", ::DoD_AddHealthIfWounded_Native, },
    { "DoD_IsPlayerFullHealth", ::DoD_IsPlayerFullHealth_Native, },

    { "DoD_PlayerSpawn", ::DoD_PlayerSpawn_Native, },
    { "DoD_GiveNamedItem", ::DoD_GiveNamedItem_Native, },
    { "DoD_DropPlayerItem", ::DoD_DropPlayerItem_Native, },
    { "DoD_RemoveAllItems", ::DoD_RemoveAllItems_Native, },
    { "DoD_GiveAmmo", ::DoD_GiveAmmo_Native, },
    { "DoD_InstallGameRules", ::DoD_InstallGameRules_Native, },
    { "DoD_SetBodygroup", ::DoD_SetBodygroup_Native, },
    { "DoD_SubRemove", ::DoD_SubRemove_Native, },
    { "DoD_UtilRemove", ::DoD_UtilRemove_Native, },
    { "DoD_CreateNamedEntity", ::DoD_CreateNamedEntity_Native, },
    { "DoD_DestroyItem", ::DoD_DestroyItem_Native, },
    { "DoD_PackWeapon", ::DoD_PackWeapon_Native, },
    { "DoD_WpnBoxKill", ::DoD_WpnBoxKill_Native, },
    { "DoD_WpnBoxActivateThink", ::DoD_WpnBoxActivateThink_Native, },
    { "DoD_ChangePlayerTeam", ::DoD_ChangePlayerTeam_Native, },
    { "DoD_ChooseRandomClass", ::DoD_ChooseRandomClass_Native, },
    { "DoD_Create", ::DoD_Create_Native, },
    { "DoD_Engine_CalcPing", ::DoD_Engine_CalcPing_Native, },
    { "DoD_Engine_HostPing", ::DoD_Engine_HostPing_Native, },
    { "DoD_Engine_HostStatus", ::DoD_Engine_HostStatus_Native, },
    { "DoD_Engine_HostStat", ::DoD_Engine_HostStat_Native, },
    { "DoD_Engine_WriteByte", ::DoD_Engine_WriteByte_Native, },
    { "DoD_Engine_WriteBits", ::DoD_Engine_WriteBits_Native, },
    { "DoD_Engine_EmitPings", ::DoD_Engine_EmitPings_Native, },

    { "DoD_SetEntityThinkFunc", ::DoD_SetEntityThinkFunc_Native, },
    { "DoD_GetEntityThinkFunc", ::DoD_GetEntityThinkFunc_Native, },

    { "DoD_TeamIndexToName", ::DoD_TeamIndexToName_Native, },
    { "DoD_ClassIndexToName", ::DoD_ClassIndexToName_Native, },

    { "DoD_FindSignature", ::DoD_FindSignature_Native, },
    { "DoD_FindSymbol", ::DoD_FindSymbol_Native, },
    { "DoD_FindStringPush", ::DoD_FindStringPush_Native, },

    { "DoD_ReadFromAddress", ::DoD_ReadFromAddress_Native, },
    { "DoD_StoreToAddress", ::DoD_StoreToAddress_Native, },

    { "DoD_ReadFloatFromAddress", ::DoD_ReadFloatFromAddress_Native, },
    { "DoD_StoreFloatToAddress", ::DoD_StoreFloatToAddress_Native, },

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

    { "DoD_AddPlayerItem", ::DoD_AddPlayerItem_Native, },
    { "DoD_RemovePlayerItem", ::DoD_RemovePlayerItem_Native, },

    { "DoD_AddAdvancedDeploy", ::DoD_AddAdvancedDeploy_Native, },
    { "DoD_DelAdvancedDeploy", ::DoD_DelAdvancedDeploy_Native, },

    { "DoD_AddSelfExploNadeProjGlow", ::DoD_AddSelfExploNadeProjGlow_Native, },
    { "DoD_DelSelfExploNadeProjGlow", ::DoD_DelSelfExploNadeProjGlow_Native, },
    { "DoD_SetSelfExploNadeProjGlow", ::DoD_SetSelfExploNadeProjGlow_Native, },
    { "DoD_GetSelfExploNadeProjGlow", ::DoD_GetSelfExploNadeProjGlow_Native, },

    { "DoD_AddTeamExploNadeProjGlow", ::DoD_AddTeamExploNadeProjGlow_Native, },
    { "DoD_DelTeamExploNadeProjGlow", ::DoD_DelTeamExploNadeProjGlow_Native, },
    { "DoD_SetTeamExploNadeProjGlow", ::DoD_SetTeamExploNadeProjGlow_Native, },
    { "DoD_GetTeamExploNadeProjGlow", ::DoD_GetTeamExploNadeProjGlow_Native, },

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
    { "DoD_ReadGameRulesByte", ::DoD_ReadGameRulesByte_Native, },
    { "DoD_ReadGameRulesUByte", ::DoD_ReadGameRulesUByte_Native, },
    { "DoD_ReadGameRulesShort", ::DoD_ReadGameRulesShort_Native, },
    { "DoD_ReadGameRulesUShort", ::DoD_ReadGameRulesUShort_Native, },
    { "DoD_ReadGameRulesFloat", ::DoD_ReadGameRulesFloat_Native, },
    { "DoD_ReadGameRulesInt", ::DoD_ReadGameRulesInt_Native, },
    { "DoD_ReadGameRulesUInt", ::DoD_ReadGameRulesUInt_Native, },
    { "DoD_ReadGameRulesStr", ::DoD_ReadGameRulesStr_Native, },

    { "DoD_StoreGameRulesBool", ::DoD_StoreGameRulesBool_Native, },
    { "DoD_StoreGameRulesByte", ::DoD_StoreGameRulesByte_Native, },
    { "DoD_StoreGameRulesUByte", ::DoD_StoreGameRulesUByte_Native, },
    { "DoD_StoreGameRulesShort", ::DoD_StoreGameRulesShort_Native, },
    { "DoD_StoreGameRulesUShort", ::DoD_StoreGameRulesUShort_Native, },
    { "DoD_StoreGameRulesFloat", ::DoD_StoreGameRulesFloat_Native, },
    { "DoD_StoreGameRulesInt", ::DoD_StoreGameRulesInt_Native, },
    { "DoD_StoreGameRulesUInt", ::DoD_StoreGameRulesUInt_Native, },
    { "DoD_StoreGameRulesStr", ::DoD_StoreGameRulesStr_Native, },

    { "DoD_GetPvDataBool", ::DoD_GetPvDataBool_Native, },
    { "DoD_SetPvDataBool", ::DoD_SetPvDataBool_Native, },

    { "DoD_TraceLine", ::DoD_TraceLine_Native, },
    { "DoD_TraceLineComplex", ::DoD_TraceLineComplex_Native, },

    { "DoD_HookShouldCollide", ::DoD_HookShouldCollide_Native, },
    { "DoD_UnhookShouldCollide", ::DoD_UnhookShouldCollide_Native, },

    { "DoD_BlockToPlayerCollision", ::DoD_BlockToPlayerCollision_Native, },
    { "DoD_UnblockFromPlayerCollision", ::DoD_UnblockFromPlayerCollision_Native, },

    { "DoD_AddPlayerCorpseManager", ::DoD_AddPlayerCorpseManager_Native, },
    { "DoD_DelPlayerCorpseManager", ::DoD_DelPlayerCorpseManager_Native, },

    { "DoD_ChangeMap", ::DoD_ChangeMap_Native, },
    { "DoD_DeployItem", ::DoD_DeployItem_Native, },
    { "DoD_AllocString", ::DoD_AllocString_Native, },
    { "DoD_AreGameRulesReady", ::DoD_AreGameRulesReady_Native, },
    { "DoD_GetFunctionAddress", ::DoD_GetFunctionAddress_Native, },
    { "DoD_PlayerOwnsDeployableGun", ::DoD_PlayerOwnsDeployableGun_Native, },
    { "DoD_PlayerHoldsDeployableGun", ::DoD_PlayerHoldsDeployableGun_Native, },

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
    ::g_entvarsOffs = ::g_Sigs[::DoD_Sig::Offs_Entvars].Offs;
    return true;
}

void OnAmxxAttach()
{
    const auto Beg = double(::clock());
#ifndef __linux__
    if (!::ReadConfig(false))
    {
        ::g_fn_AddNatives(::DoDHacks_Natives);

        char Msg[128];
#ifdef __AVX2__
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#elif defined (__AVX__)
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#else
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#endif
        ::g_engfuncs.pfnServerPrint(Msg);
        return;
    }

    /// Engine module.
    auto Opened = false;
    auto pDoD = ::openLib("swds", Opened);
    if (pDoD)
    {
        ::_MEMORY_BASIC_INFORMATION memInfo{ };
        if (::VirtualQuery(pDoD, &memInfo, sizeof memInfo) && memInfo.AllocationBase)
        {
            ::size_t Addr = false;
            auto pDosHdr = (::_IMAGE_DOS_HEADER*)memInfo.AllocationBase;
            auto pNtHdr = (::_IMAGE_NT_HEADERS*)((::size_t)pDosHdr + (::size_t)pDosHdr->e_lfanew);

            if (::g_Sigs[::DoD_Sig::Engine_CalcPing_New].IsSymbol == false)
            {
                if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_CalcPing_New].Signature, &Addr, true))
                    ::g_pDoDEngine_CalcPing_Addr = (void*)Addr;
            }
            else
                ::g_pDoDEngine_CalcPing_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_CalcPing_New].Symbol.c_str());

            if (!::g_pDoDEngine_CalcPing_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_CalcPing_Old].IsSymbol == false)
                {
                    if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_CalcPing_Old].Signature, &Addr, true))
                        ::g_pDoDEngine_CalcPing_Addr = (void*)Addr;
                }
                else
                    ::g_pDoDEngine_CalcPing_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_CalcPing_Old].Symbol.c_str());

                if (!::g_pDoDEngine_CalcPing_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_CalcPing_Re].IsSymbol == false)
                    {
                        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_CalcPing_Re].Signature, &Addr, true))
                            ::g_pDoDEngine_CalcPing_Addr = (void*)Addr;
                    }
                    else
                        ::g_pDoDEngine_CalcPing_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_CalcPing_Re].Symbol.c_str());

                    if (!::g_pDoDEngine_CalcPing_Addr)
                        ::MF_Log("::DoD_Engine_CalcPing symbol/ signature not found!");
                    else
                        ::g_ReHLDS = true;
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_HostPing_New].IsSymbol == false)
            {
                if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostPing_New].Signature, &Addr, true))
                    ::g_pDoDEngine_HostPing_Addr = (void*)Addr;
            }
            else
                ::g_pDoDEngine_HostPing_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostPing_New].Symbol.c_str());

            if (!::g_pDoDEngine_HostPing_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_HostPing_Old].IsSymbol == false)
                {
                    if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostPing_Old].Signature, &Addr, true))
                        ::g_pDoDEngine_HostPing_Addr = (void*)Addr;
                }
                else
                    ::g_pDoDEngine_HostPing_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostPing_Old].Symbol.c_str());

                if (!::g_pDoDEngine_HostPing_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_HostPing_Re].IsSymbol == false)
                    {
                        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostPing_Re].Signature, &Addr, true))
                            ::g_pDoDEngine_HostPing_Addr = (void*)Addr;
                    }
                    else
                        ::g_pDoDEngine_HostPing_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostPing_Re].Symbol.c_str());

                    if (!::g_pDoDEngine_HostPing_Addr)
                        ::MF_Log("::DoD_Engine_HostPing symbol/ signature not found!");
                    else
                        ::g_ReHLDS = true;
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_HostStatus_New].IsSymbol == false)
            {
                if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostStatus_New].Signature, &Addr, true))
                    ::g_pDoDEngine_HostStatus_Addr = (void*)Addr;
            }
            else
                ::g_pDoDEngine_HostStatus_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStatus_New].Symbol.c_str());

            if (!::g_pDoDEngine_HostStatus_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_HostStatus_Old].IsSymbol == false)
                {
                    if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostStatus_Old].Signature, &Addr, true))
                        ::g_pDoDEngine_HostStatus_Addr = (void*)Addr;
                }
                else
                    ::g_pDoDEngine_HostStatus_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStatus_Old].Symbol.c_str());

                if (!::g_pDoDEngine_HostStatus_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_HostStatus_Re].IsSymbol == false)
                    {
                        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostStatus_Re].Signature, &Addr, true))
                            ::g_pDoDEngine_HostStatus_Addr = (void*)Addr;
                    }
                    else
                        ::g_pDoDEngine_HostStatus_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStatus_Re].Symbol.c_str());

                    if (!::g_pDoDEngine_HostStatus_Addr)
                        ::MF_Log("::DoD_Engine_HostStatus symbol/ signature not found!");
                    else
                        ::g_ReHLDS = true;
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_HostStat_New].IsSymbol == false)
            {
                if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostStat_New].Signature, &Addr, true))
                    ::g_pDoDEngine_HostStat_Addr = (void*)Addr;
            }
            else
                ::g_pDoDEngine_HostStat_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStat_New].Symbol.c_str());

            if (!::g_pDoDEngine_HostStat_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_HostStat_Old].IsSymbol == false)
                {
                    if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostStat_Old].Signature, &Addr, true))
                        ::g_pDoDEngine_HostStat_Addr = (void*)Addr;
                }
                else
                    ::g_pDoDEngine_HostStat_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStat_Old].Symbol.c_str());

                if (!::g_pDoDEngine_HostStat_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_HostStat_Re].IsSymbol == false)
                    {
                        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Engine_HostStat_Re].Signature, &Addr, true))
                            ::g_pDoDEngine_HostStat_Addr = (void*)Addr;
                    }
                    else
                        ::g_pDoDEngine_HostStat_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStat_Re].Symbol.c_str());

                    if (!::g_pDoDEngine_HostStat_Addr)
                        ::MF_Log("::DoD_Engine_HostStat symbol/ signature not found!");
                    else
                        ::g_ReHLDS = true;
                }
            }
        }
        if (Opened)
            ::FreeLibrary(pDoD);
    }

    /// Game module.
    Opened = false;
    pDoD = ::openLib("dod", Opened);
    if (!pDoD)
    {
        ::MF_Log("::GetModuleHandleA/ ::LoadLibraryA failed! Use with caution!");
        ::g_fn_AddNatives(::DoDHacks_Natives);

        char Msg[128];
#ifdef __AVX2__
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#elif defined (__AVX__)
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#else
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#endif
        ::g_engfuncs.pfnServerPrint(Msg);
        return;
    }

    ::_MEMORY_BASIC_INFORMATION memInfo{ };
    if (false == ::VirtualQuery(pDoD, &memInfo, sizeof memInfo) || !memInfo.AllocationBase)
    {
        ::MF_Log("::VirtualQuery failed!");
        ::g_fn_AddNatives(::DoDHacks_Natives);

        char Msg[128];
#ifdef __AVX2__
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#elif defined (__AVX__)
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#else
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#endif
        ::g_engfuncs.pfnServerPrint(Msg);
        if (Opened)
            ::FreeLibrary(pDoD);
        return;
    }

    ::size_t Addr = false;
    auto pDosHdr = (::_IMAGE_DOS_HEADER*)memInfo.AllocationBase;
    auto pNtHdr = (::_IMAGE_NT_HEADERS*)((::size_t)pDosHdr + (::size_t)pDosHdr->e_lfanew);

    if (::g_Sigs[::DoD_Sig::PlayerSpawn].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::PlayerSpawn].Signature, &Addr, true))
            ::g_pDoDPlayerSpawn_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_PlayerSpawn signature not found!");
    }
    else if (!(::g_pDoDPlayerSpawn_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::PlayerSpawn].Symbol.c_str())))
        ::MF_Log("::DoD_PlayerSpawn symbol not found!");

    if (::g_Sigs[::DoD_Sig::GiveNamedItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::GiveNamedItem].Signature, &Addr, true))
            ::g_pDoDGiveNamedItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GiveNamedItem signature not found!");
    }
    else if (!(::g_pDoDGiveNamedItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::GiveNamedItem].Symbol.c_str())))
        ::MF_Log("::DoD_GiveNamedItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::DropPlayerItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::DropPlayerItem].Signature, &Addr, true))
            ::g_pDoDDropPlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_DropPlayerItem signature not found!");
    }
    else if (!(::g_pDoDDropPlayerItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::DropPlayerItem].Symbol.c_str())))
        ::MF_Log("::DoD_DropPlayerItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::GiveAmmo].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::GiveAmmo].Signature, &Addr, true))
            ::g_pDoDGiveAmmo_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GiveAmmo signature not found!");
    }
    else if (!(::g_pDoDGiveAmmo_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::GiveAmmo].Symbol.c_str())))
        ::MF_Log("::DoD_GiveAmmo symbol not found!");

    if (::g_Sigs[::DoD_Sig::UtilRemove].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::UtilRemove].Signature, &Addr, true))
            ::g_pDoDUtilRemove_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_UtilRemove signature not found!");
    }
    else if (!(::g_pDoDUtilRemove_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::UtilRemove].Symbol.c_str())))
        ::MF_Log("::DoD_UtilRemove symbol not found!");

    if (::g_Sigs[::DoD_Sig::CreateNamedEntity].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::CreateNamedEntity].Signature, &Addr, true))
            ::g_pDoDCreateNamedEntity_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_CreateNamedEntity signature not found!");
    }
    else if (!(::g_pDoDCreateNamedEntity_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::CreateNamedEntity].Symbol.c_str())))
        ::MF_Log("::DoD_CreateNamedEntity symbol not found!");

    if (::g_Sigs[::DoD_Sig::SubRemove].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::SubRemove].Signature, &Addr, true))
            ::g_pDoDSubRemove_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SubRemove signature not found!");
    }
    else if (!(::g_pDoDSubRemove_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::SubRemove].Symbol.c_str())))
        ::MF_Log("::DoD_SubRemove symbol not found!");

    if (::g_Sigs[::DoD_Sig::WpnBoxKill].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::WpnBoxKill].Signature, &Addr, true))
            ::g_pDoDWpnBoxKill_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_WpnBoxKill signature not found!");
    }
    else if (!(::g_pDoDWpnBoxKill_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::WpnBoxKill].Symbol.c_str())))
        ::MF_Log("::DoD_WpnBoxKill symbol not found!");

    if (::g_Sigs[::DoD_Sig::WpnBoxActivateThink].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::WpnBoxActivateThink].Signature, &Addr, true))
            ::g_pDoDWpnBoxActivateThink_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_WpnBoxActivateThink signature not found!");
    }
    else if (!(::g_pDoDWpnBoxActivateThink_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::WpnBoxActivateThink].Symbol.c_str())))
        ::MF_Log("::DoD_WpnBoxActivateThink symbol not found!");

    if (::g_Sigs[::DoD_Sig::ChangePlayerTeam].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::ChangePlayerTeam].Signature, &Addr, true))
            ::g_pDoDChangePlayerTeam_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_ChangePlayerTeam signature not found!");
    }
    else if (!(::g_pDoDChangePlayerTeam_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::ChangePlayerTeam].Symbol.c_str())))
        ::MF_Log("::DoD_ChangePlayerTeam symbol not found!");

    if (::g_Sigs[::DoD_Sig::ChooseRandomClass].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::ChooseRandomClass].Signature, &Addr, true))
            ::g_pDoDChooseRandomClass_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_ChooseRandomClass signature not found!");
    }
    else if (!(::g_pDoDChooseRandomClass_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::ChooseRandomClass].Symbol.c_str())))
        ::MF_Log("::DoD_ChooseRandomClass symbol not found!");

    if (::g_Sigs[::DoD_Sig::Create].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::Create].Signature, &Addr, true))
            ::g_pDoDCreate_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_Create signature not found!");
    }
    else if (!(::g_pDoDCreate_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::Create].Symbol.c_str())))
        ::MF_Log("::DoD_Create symbol not found!");

    if (::g_Sigs[::DoD_Sig::PackWeapon].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::PackWeapon].Signature, &Addr, true))
            ::g_pDoDPackWeapon_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_PackWeapon signature not found!");
    }
    else if (!(::g_pDoDPackWeapon_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::PackWeapon].Symbol.c_str())))
        ::MF_Log("::DoD_PackWeapon symbol not found!");

    if (::g_Sigs[::DoD_Sig::DestroyItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::DestroyItem].Signature, &Addr, true))
            ::g_pDoDDestroyItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_DestroyItem signature not found!");
    }
    else if (!(::g_pDoDDestroyItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::DestroyItem].Symbol.c_str())))
        ::MF_Log("::DoD_DestroyItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::SetWaveTime].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::SetWaveTime].Signature, &Addr, true))
            ::g_pDoDSetWaveTime_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SetWaveTime signature not found!");
    }
    else if (!(::g_pDoDSetWaveTime_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::SetWaveTime].Symbol.c_str())))
        ::MF_Log("::DoD_SetWaveTime symbol not found!");

    if (::g_Sigs[::DoD_Sig::GetWaveTime].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::GetWaveTime].Signature, &Addr, true))
            ::g_pDoDGetWaveTime_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_GetWaveTime signature not found!");
    }
    else if (!(::g_pDoDGetWaveTime_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::GetWaveTime].Symbol.c_str())))
        ::MF_Log("::DoD_GetWaveTime symbol not found!");

    if (::g_Sigs[::DoD_Sig::RemovePlayerItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::RemovePlayerItem].Signature, &Addr, true))
            ::g_pDoDRemovePlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_RemovePlayerItem signature not found!");
    }
    else if (!(::g_pDoDRemovePlayerItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::RemovePlayerItem].Symbol.c_str())))
        ::MF_Log("::DoD_RemovePlayerItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::AddPlayerItem].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::AddPlayerItem].Signature, &Addr, true))
            ::g_pDoDAddPlayerItem_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_AddPlayerItem signature not found!");
    }
    else if (!(::g_pDoDAddPlayerItem_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::AddPlayerItem].Symbol.c_str())))
        ::MF_Log("::DoD_AddPlayerItem symbol not found!");

    if (::g_Sigs[::DoD_Sig::RemoveAllItems].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::RemoveAllItems].Signature, &Addr, true))
            ::g_pDoDRemoveAllItems_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_RemoveAllItems signature not found!");
    }
    else if (!(::g_pDoDRemoveAllItems_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::RemoveAllItems].Symbol.c_str())))
        ::MF_Log("::DoD_RemoveAllItems symbol not found!");

    if (::g_Sigs[::DoD_Sig::SetBodygroup].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::SetBodygroup].Signature, &Addr, true))
            ::g_pDoDSetBodygroup_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_SetBodygroup signature not found!");
    }
    else if (!(::g_pDoDSetBodygroup_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::SetBodygroup].Symbol.c_str())))
        ::MF_Log("::DoD_SetBodygroup symbol not found!");

    if (::g_Sigs[::DoD_Sig::InstallGameRules].IsSymbol == false)
    {
        if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::InstallGameRules].Signature, &Addr, true))
            ::g_pDoDInstallGameRules_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_InstallGameRules signature not found!");
    }
    else if (!(::g_pDoDInstallGameRules_Addr = (void*) ::GetProcAddress(pDoD, ::g_Sigs[::DoD_Sig::InstallGameRules].Symbol.c_str())))
        ::MF_Log("::DoD_InstallGameRules symbol not found!");

    if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::PatchFg42].Signature, &Addr, true))
        ::g_pAutoScopeFG42Addr = (unsigned char*)Addr;
    else
        ::MF_Log("::DoD_PatchAutoScope(FG42) signature not found!");

    if (::findInMemory((unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::DoD_Sig::PatchEnfield].Signature, &Addr, true))
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

        char Msg[128];
#ifdef __AVX2__
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#elif defined (__AVX__)
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#else
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#endif
        ::g_engfuncs.pfnServerPrint(Msg);
        return;
    }

    /// Engine module.
    auto pDoD = ::openLib("engine", ::DoD_Suffix::i486);
    if (pDoD)
    {
        auto pLinkMap = (::link_map*)pDoD;
        struct ::stat memData;
        if (!::stat(pLinkMap->l_name, &memData))
        {
            ::size_t Addr = false;

            if (::g_Sigs[::DoD_Sig::Engine_CalcPing_New].IsSymbol)
                ::g_pDoDEngine_CalcPing_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_CalcPing_New].Symbol.c_str());
            else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_CalcPing_New].Signature, &Addr, true))
                ::g_pDoDEngine_CalcPing_Addr = (void*)Addr;

            if (!::g_pDoDEngine_CalcPing_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_CalcPing_Old].IsSymbol)
                    ::g_pDoDEngine_CalcPing_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_CalcPing_Old].Symbol.c_str());
                else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_CalcPing_Old].Signature, &Addr, true))
                    ::g_pDoDEngine_CalcPing_Addr = (void*)Addr;

                if (!::g_pDoDEngine_CalcPing_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_CalcPing_Re].IsSymbol)
                        ::g_pDoDEngine_CalcPing_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_CalcPing_Re].Symbol.c_str());
                    else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_CalcPing_Re].Signature, &Addr, true))
                        ::g_pDoDEngine_CalcPing_Addr = (void*)Addr;

                    if (!::g_pDoDEngine_CalcPing_Addr)
                        ::MF_Log("::DoD_Engine_CalcPing symbol/ signature not found!");
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_HostPing_New].IsSymbol)
                ::g_pDoDEngine_HostPing_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostPing_New].Symbol.c_str());
            else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostPing_New].Signature, &Addr, true))
                ::g_pDoDEngine_HostPing_Addr = (void*)Addr;

            if (!::g_pDoDEngine_HostPing_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_HostPing_Old].IsSymbol)
                    ::g_pDoDEngine_HostPing_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostPing_Old].Symbol.c_str());
                else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostPing_Old].Signature, &Addr, true))
                    ::g_pDoDEngine_HostPing_Addr = (void*)Addr;

                if (!::g_pDoDEngine_HostPing_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_HostPing_Re].IsSymbol)
                        ::g_pDoDEngine_HostPing_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostPing_Re].Symbol.c_str());
                    else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostPing_Re].Signature, &Addr, true))
                        ::g_pDoDEngine_HostPing_Addr = (void*)Addr;

                    if (!::g_pDoDEngine_HostPing_Addr)
                        ::MF_Log("::DoD_Engine_HostPing symbol/ signature not found!");
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_HostStatus_New].IsSymbol)
                ::g_pDoDEngine_HostStatus_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStatus_New].Symbol.c_str());
            else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostStatus_New].Signature, &Addr, true))
                ::g_pDoDEngine_HostStatus_Addr = (void*)Addr;

            if (!::g_pDoDEngine_HostStatus_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_HostStatus_Old].IsSymbol)
                    ::g_pDoDEngine_HostStatus_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStatus_Old].Symbol.c_str());
                else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostStatus_Old].Signature, &Addr, true))
                    ::g_pDoDEngine_HostStatus_Addr = (void*)Addr;

                if (!::g_pDoDEngine_HostStatus_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_HostStatus_Re].IsSymbol)
                        ::g_pDoDEngine_HostStatus_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStatus_Re].Symbol.c_str());
                    else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostStatus_Re].Signature, &Addr, true))
                        ::g_pDoDEngine_HostStatus_Addr = (void*)Addr;

                    if (!::g_pDoDEngine_HostStatus_Addr)
                        ::MF_Log("::DoD_Engine_HostStatus symbol/ signature not found!");
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_HostStat_New].IsSymbol)
                ::g_pDoDEngine_HostStat_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStat_New].Symbol.c_str());
            else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostStat_New].Signature, &Addr, true))
                ::g_pDoDEngine_HostStat_Addr = (void*)Addr;

            if (!::g_pDoDEngine_HostStat_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_HostStat_Old].IsSymbol)
                    ::g_pDoDEngine_HostStat_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStat_Old].Symbol.c_str());
                else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostStat_Old].Signature, &Addr, true))
                    ::g_pDoDEngine_HostStat_Addr = (void*)Addr;

                if (!::g_pDoDEngine_HostStat_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_HostStat_Re].IsSymbol)
                        ::g_pDoDEngine_HostStat_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_HostStat_Re].Symbol.c_str());
                    else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_HostStat_Re].Signature, &Addr, true))
                        ::g_pDoDEngine_HostStat_Addr = (void*)Addr;

                    if (!::g_pDoDEngine_HostStat_Addr)
                        ::MF_Log("::DoD_Engine_HostStat symbol/ signature not found!");
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_WriteByte_New].IsSymbol)
                ::g_pDoDEngine_WriteByte_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_WriteByte_New].Symbol.c_str());
            else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_WriteByte_New].Signature, &Addr, true))
                ::g_pDoDEngine_WriteByte_Addr = (void*)Addr;

            if (!::g_pDoDEngine_WriteByte_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_WriteByte_Old].IsSymbol)
                    ::g_pDoDEngine_WriteByte_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_WriteByte_Old].Symbol.c_str());
                else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_WriteByte_Old].Signature, &Addr, true))
                    ::g_pDoDEngine_WriteByte_Addr = (void*)Addr;

                if (!::g_pDoDEngine_WriteByte_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_WriteByte_Re].IsSymbol)
                        ::g_pDoDEngine_WriteByte_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_WriteByte_Re].Symbol.c_str());
                    else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_WriteByte_Re].Signature, &Addr, true))
                        ::g_pDoDEngine_WriteByte_Addr = (void*)Addr;

                    if (!::g_pDoDEngine_WriteByte_Addr)
                        ::MF_Log("::DoD_Engine_WriteByte symbol/ signature not found!");
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_WriteBits_New].IsSymbol)
                ::g_pDoDEngine_WriteBits_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_WriteBits_New].Symbol.c_str());
            else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_WriteBits_New].Signature, &Addr, true))
                ::g_pDoDEngine_WriteBits_Addr = (void*)Addr;

            if (!::g_pDoDEngine_WriteBits_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_WriteBits_Old].IsSymbol)
                    ::g_pDoDEngine_WriteBits_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_WriteBits_Old].Symbol.c_str());
                else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_WriteBits_Old].Signature, &Addr, true))
                    ::g_pDoDEngine_WriteBits_Addr = (void*)Addr;

                if (!::g_pDoDEngine_WriteBits_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_WriteBits_Re].IsSymbol)
                        ::g_pDoDEngine_WriteBits_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_WriteBits_Re].Symbol.c_str());
                    else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_WriteBits_Re].Signature, &Addr, true))
                        ::g_pDoDEngine_WriteBits_Addr = (void*)Addr;

                    if (!::g_pDoDEngine_WriteBits_Addr)
                        ::MF_Log("::DoD_Engine_WriteBits symbol/ signature not found!");
                }
            }

            if (::g_Sigs[::DoD_Sig::Engine_EmitPings_New].IsSymbol)
                ::g_pDoDEngine_EmitPings_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_EmitPings_New].Symbol.c_str());
            else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_EmitPings_New].Signature, &Addr, true))
                ::g_pDoDEngine_EmitPings_Addr = (void*)Addr;

            if (!::g_pDoDEngine_EmitPings_Addr)
            {
                if (::g_Sigs[::DoD_Sig::Engine_EmitPings_Old].IsSymbol)
                    ::g_pDoDEngine_EmitPings_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_EmitPings_Old].Symbol.c_str());
                else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_EmitPings_Old].Signature, &Addr, true))
                    ::g_pDoDEngine_EmitPings_Addr = (void*)Addr;

                if (!::g_pDoDEngine_EmitPings_Addr)
                {
                    if (::g_Sigs[::DoD_Sig::Engine_EmitPings_Re].IsSymbol)
                        ::g_pDoDEngine_EmitPings_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::Engine_EmitPings_Re].Symbol.c_str());
                    else if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::Engine_EmitPings_Re].Signature, &Addr, true))
                        ::g_pDoDEngine_EmitPings_Addr = (void*)Addr;

                    if (!::g_pDoDEngine_EmitPings_Addr)
                        ::MF_Log("::DoD_Engine_EmitPings symbol/ signature not found!");
                }
            }
        }
        ::dlclose(pDoD);
    }

    /// Game module.
    /** Try opening with suffix initially, for compatibility (i.e. people trying to use outdated BOT plugins). */
    pDoD = ::openLib("dod", ::DoD_Suffix::i386);
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

                    char Msg[128];
#ifdef __AVX2__
#ifndef __linux__
                    ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
                    ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#elif defined (__AVX__)
#ifndef __linux__
                    ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
                    ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#else
#ifndef __linux__
                    ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
                    ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#endif
                    ::g_engfuncs.pfnServerPrint(Msg);
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

        char Msg[128];
#ifdef __AVX2__
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#elif defined (__AVX__)
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#else
#ifndef __linux__
        ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
        ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#endif
        ::g_engfuncs.pfnServerPrint(Msg);
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

    if (::g_Sigs[::DoD_Sig::ChangePlayerTeam].IsSymbol)
    {
        if (!(::g_pDoDChangePlayerTeam_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::ChangePlayerTeam].Symbol.c_str())))
            ::MF_Log("::DoD_ChangePlayerTeam symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::ChangePlayerTeam].Signature, &Addr, true))
            ::g_pDoDChangePlayerTeam_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_ChangePlayerTeam signature not found!");
    }

    if (::g_Sigs[::DoD_Sig::ChooseRandomClass].IsSymbol)
    {
        if (!(::g_pDoDChooseRandomClass_Addr = ::dlsymComplex(pDoD, ::g_Sigs[::DoD_Sig::ChooseRandomClass].Symbol.c_str())))
            ::MF_Log("::DoD_ChooseRandomClass symbol not found!");
    }
    else
    {
        if (::findInMemory((unsigned char*)pLinkMap->l_addr, memData.st_size, ::g_Sigs[::DoD_Sig::ChooseRandomClass].Signature, &Addr, true))
            ::g_pDoDChooseRandomClass_Addr = (void*)Addr;
        else
            ::MF_Log("::DoD_ChooseRandomClass signature not found!");
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

    char Msg[128];
#ifdef __AVX2__
#ifndef __linux__
    ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
    ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX2).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#elif defined (__AVX__)
#ifndef __linux__
    ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
    ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (AVX).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#else
#ifndef __linux__
    ::_snprintf_s(Msg, sizeof Msg, _TRUNCATE, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#else
    ::_snprintf(Msg, sizeof Msg, "[INFO] " MODULE_NAME " took %f sec. to load (SSE).\n", float((double(::clock()) - Beg) / double(CLOCKS_PER_SEC)));
#endif
#endif
    ::g_engfuncs.pfnServerPrint(Msg);
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
    if (::g_DoDEngine_CalcPing_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        if (::g_ReHLDS)
            ::DetourDetach(&(void*&) ::DoD_Engine_CalcPing_ReHLDS_Win, ::DoD_Engine_CalcPing_Hook_ReHLDS_Win);
        else
            ::DetourDetach(&(void*&) ::DoD_Engine_CalcPing, ::DoD_Engine_CalcPing_Hook);
        ::DetourTransactionCommit();
        ::g_DoDEngine_CalcPing_Hook = false;
    }
    if (::g_DoDEngine_HostPing_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_Engine_HostPing, ::DoD_Engine_HostPing_Hook);
        ::DetourTransactionCommit();
        ::g_DoDEngine_HostPing_Hook = false;
    }
    if (::g_DoDEngine_HostStatus_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_Engine_HostStatus, ::DoD_Engine_HostStatus_Hook);
        ::DetourTransactionCommit();
        ::g_DoDEngine_HostStatus_Hook = false;
    }
    if (::g_DoDEngine_HostStat_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_Engine_HostStat, ::DoD_Engine_HostStat_Hook);
        ::DetourTransactionCommit();
        ::g_DoDEngine_HostStat_Hook = false;
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
    if (::g_DoDChangePlayerTeam_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_ChangePlayerTeam, ::DoD_ChangePlayerTeam_Hook);
        ::DetourTransactionCommit();
        ::g_DoDChangePlayerTeam_Hook = false;
    }
    if (::g_DoDChooseRandomClass_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_ChooseRandomClass, ::DoD_ChooseRandomClass_Hook);
        ::DetourTransactionCommit();
        ::g_DoDChooseRandomClass_Hook = false;
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
    if (::g_DoDEngine_CalcPing_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_CalcPing);
        ::subhook_free(::g_pDoDEngine_CalcPing);
        ::g_DoDEngine_CalcPing_Hook = false;
    }
    if (::g_DoDEngine_HostPing_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_HostPing);
        ::subhook_free(::g_pDoDEngine_HostPing);
        ::g_DoDEngine_HostPing_Hook = false;
    }
    if (::g_DoDEngine_HostStatus_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_HostStatus);
        ::subhook_free(::g_pDoDEngine_HostStatus);
        ::g_DoDEngine_HostStatus_Hook = false;
    }
    if (::g_DoDEngine_HostStat_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_HostStat);
        ::subhook_free(::g_pDoDEngine_HostStat);
        ::g_DoDEngine_HostStat_Hook = false;
    }
    if (::g_DoDEngine_WriteByte_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_WriteByte);
        ::subhook_free(::g_pDoDEngine_WriteByte);
        ::g_DoDEngine_WriteByte_Hook = false;
    }
    if (::g_DoDEngine_WriteBits_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_WriteBits);
        ::subhook_free(::g_pDoDEngine_WriteBits);
        ::g_DoDEngine_WriteBits_Hook = false;
    }
    if (::g_DoDEngine_EmitPings_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_EmitPings);
        ::subhook_free(::g_pDoDEngine_EmitPings);
        ::g_DoDEngine_EmitPings_Hook = false;
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
    if (::g_DoDChangePlayerTeam_Hook)
    {
        ::subhook_remove(::g_pDoDChangePlayerTeam);
        ::subhook_free(::g_pDoDChangePlayerTeam);
        ::g_DoDChangePlayerTeam_Hook = false;
    }
    if (::g_DoDChooseRandomClass_Hook)
    {
        ::subhook_remove(::g_pDoDChooseRandomClass);
        ::subhook_free(::g_pDoDChooseRandomClass);
        ::g_DoDChooseRandomClass_Hook = false;
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
    ::g_pDoDChangePlayerTeam_Addr = NULL;
    ::g_pDoDChooseRandomClass_Addr = NULL;
    ::g_pDoDWpnBoxKill_Addr = NULL;
    ::g_pDoDEngine_CalcPing_Addr = NULL;
    ::g_pDoDEngine_HostPing_Addr = NULL;
    ::g_pDoDEngine_HostStatus_Addr = NULL;
    ::g_pDoDEngine_HostStat_Addr = NULL;

#ifdef __linux__
    ::g_pDoDEngine_WriteByte_Addr = NULL;
    ::g_pDoDEngine_WriteBits_Addr = NULL;
    ::g_pDoDEngine_EmitPings_Addr = NULL;
#endif

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
    ::g_fwChangePlayerTeam = ::g_fn_RegisterForward("DoD_OnChangePlayerTeam", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwChooseRandomClass = ::g_fn_RegisterForward("DoD_OnChooseRandomClass", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwCreate = ::g_fn_RegisterForward("DoD_OnCreate", ::ET_STOP, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_ARRAY, ::FP_ARRAY, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwCreateNamedEntity = ::g_fn_RegisterForward("DoD_OnCreateNamedEntity", ::ET_STOP, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwEngine_CalcPing = ::g_fn_RegisterForward("DoD_OnEngine_CalcPing", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwEngine_HostPing = ::g_fn_RegisterForward("DoD_OnEngine_HostPing", ::ET_STOP, ::FP_DONE);
    ::g_fwEngine_HostStatus = ::g_fn_RegisterForward("DoD_OnEngine_HostStatus", ::ET_STOP, ::FP_DONE);
    ::g_fwEngine_HostStat = ::g_fn_RegisterForward("DoD_OnEngine_HostStat", ::ET_STOP, ::FP_DONE);

#ifdef __linux__
    ::g_fwEngine_WriteByte = ::g_fn_RegisterForward("DoD_OnEngine_WriteByte", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwEngine_WriteBits = ::g_fn_RegisterForward("DoD_OnEngine_WriteBits", ::ET_STOP, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
    ::g_fwEngine_EmitPings = ::g_fn_RegisterForward("DoD_OnEngine_EmitPings", ::ET_STOP, ::FP_CELL, ::FP_CELL, ::FP_DONE);
#endif

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
    ::g_fwChangePlayerTeam_Post = ::g_fn_RegisterForward("DoD_OnChangePlayerTeam_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_CELL, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwChooseRandomClass_Post = ::g_fn_RegisterForward("DoD_OnChooseRandomClass_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwCreate_Post = ::g_fn_RegisterForward("DoD_OnCreate_Post", ::ET_IGNORE, ::FP_STRING, ::FP_ARRAY, ::FP_ARRAY, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwCreateNamedEntity_Post = ::g_fn_RegisterForward("DoD_OnCreateNamedEntity_Post", ::ET_IGNORE, ::FP_STRING, ::FP_CELL, ::FP_DONE);
    ::g_fwEngine_CalcPing_Post = ::g_fn_RegisterForward("DoD_OnEngine_CalcPing_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwEngine_HostPing_Post = ::g_fn_RegisterForward("DoD_OnEngine_HostPing_Post", ::ET_IGNORE, ::FP_DONE);
    ::g_fwEngine_HostStatus_Post = ::g_fn_RegisterForward("DoD_OnEngine_HostStatus_Post", ::ET_IGNORE, ::FP_DONE);
    ::g_fwEngine_HostStat_Post = ::g_fn_RegisterForward("DoD_OnEngine_HostStat_Post", ::ET_IGNORE, ::FP_DONE);

#ifdef __linux__
    ::g_fwEngine_WriteByte_Post = ::g_fn_RegisterForward("DoD_OnEngine_WriteByte_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwEngine_WriteBits_Post = ::g_fn_RegisterForward("DoD_OnEngine_WriteBits_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_DONE);
    ::g_fwEngine_EmitPings_Post = ::g_fn_RegisterForward("DoD_OnEngine_EmitPings_Post", ::ET_IGNORE, ::FP_CELL, ::FP_CELL, ::FP_DONE);
#endif

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

        if (::g_pDoDChangePlayerTeam_Addr && false == ::g_DoDChangePlayerTeam_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnChangePlayerTeam", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnChangePlayerTeam_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_ChangePlayerTeam = (::DoD_ChangePlayerTeam_Type) ::g_pDoDChangePlayerTeam_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_ChangePlayerTeam, ::DoD_ChangePlayerTeam_Hook);
            ::DetourTransactionCommit();
            ::g_DoDChangePlayerTeam_Hook = true;
        }

        if (::g_pDoDChooseRandomClass_Addr && false == ::g_DoDChooseRandomClass_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnChooseRandomClass", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnChooseRandomClass_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_ChooseRandomClass = (::DoD_ChooseRandomClass_Type) ::g_pDoDChooseRandomClass_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_ChooseRandomClass, ::DoD_ChooseRandomClass_Hook);
            ::DetourTransactionCommit();
            ::g_DoDChooseRandomClass_Hook = true;
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

        if (::g_pDoDEngine_CalcPing_Addr && false == ::g_DoDEngine_CalcPing_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_CalcPing", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_CalcPing_Post", &Func) == ::AMX_ERR_NONE))
        {
            if (::g_ReHLDS)
                ::DoD_Engine_CalcPing_ReHLDS_Win = (::DoD_Engine_CalcPing_Type_ReHLDS_Win) ::g_pDoDEngine_CalcPing_Addr;
            else
                ::DoD_Engine_CalcPing = (::DoD_Engine_CalcPing_Type) ::g_pDoDEngine_CalcPing_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            if (::g_ReHLDS)
                ::DetourAttach(&(void*&) ::DoD_Engine_CalcPing_ReHLDS_Win, ::DoD_Engine_CalcPing_Hook_ReHLDS_Win);
            else
                ::DetourAttach(&(void*&) ::DoD_Engine_CalcPing, ::DoD_Engine_CalcPing_Hook);
            ::DetourTransactionCommit();
            ::g_DoDEngine_CalcPing_Hook = true;
        }

        if (::g_pDoDEngine_HostPing_Addr && false == ::g_DoDEngine_HostPing_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostPing", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostPing_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_Engine_HostPing = (::DoD_Engine_HostPing_Type) ::g_pDoDEngine_HostPing_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_Engine_HostPing, ::DoD_Engine_HostPing_Hook);
            ::DetourTransactionCommit();
            ::g_DoDEngine_HostPing_Hook = true;
        }

        if (::g_pDoDEngine_HostStatus_Addr && false == ::g_DoDEngine_HostStatus_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostStatus", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostStatus_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_Engine_HostStatus = (::DoD_Engine_HostStatus_Type) ::g_pDoDEngine_HostStatus_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_Engine_HostStatus, ::DoD_Engine_HostStatus_Hook);
            ::DetourTransactionCommit();
            ::g_DoDEngine_HostStatus_Hook = true;
        }

        if (::g_pDoDEngine_HostStat_Addr && false == ::g_DoDEngine_HostStat_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostStat", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostStat_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::DoD_Engine_HostStat = (::DoD_Engine_HostStat_Type) ::g_pDoDEngine_HostStat_Addr;
            ::DetourTransactionBegin();
            ::DetourUpdateThread(::GetCurrentThread());
            ::DetourAttach(&(void*&) ::DoD_Engine_HostStat, ::DoD_Engine_HostStat_Hook);
            ::DetourTransactionCommit();
            ::g_DoDEngine_HostStat_Hook = true;
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

        if (::g_pDoDChangePlayerTeam_Addr && false == ::g_DoDChangePlayerTeam_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnChangePlayerTeam", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnChangePlayerTeam_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDChangePlayerTeam = ::subhook_new(::g_pDoDChangePlayerTeam_Addr, ::DoD_ChangePlayerTeam_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDChangePlayerTeam);
            ::DoD_ChangePlayerTeam = (::DoD_ChangePlayerTeam_Type) ::subhook_get_trampoline(::g_pDoDChangePlayerTeam);
            ::g_DoDChangePlayerTeam_Hook = true;
        }

        if (::g_pDoDChooseRandomClass_Addr && false == ::g_DoDChooseRandomClass_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnChooseRandomClass", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnChooseRandomClass_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDChooseRandomClass = ::subhook_new(::g_pDoDChooseRandomClass_Addr, ::DoD_ChooseRandomClass_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDChooseRandomClass);
            ::DoD_ChooseRandomClass = (::DoD_ChooseRandomClass_Type) ::subhook_get_trampoline(::g_pDoDChooseRandomClass);
            ::g_DoDChooseRandomClass_Hook = true;
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

        if (::g_pDoDEngine_CalcPing_Addr && false == ::g_DoDEngine_CalcPing_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_CalcPing", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_CalcPing_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDEngine_CalcPing = ::subhook_new(::g_pDoDEngine_CalcPing_Addr, ::DoD_Engine_CalcPing_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDEngine_CalcPing);
            ::DoD_Engine_CalcPing = (::DoD_Engine_CalcPing_Type) ::subhook_get_trampoline(::g_pDoDEngine_CalcPing);
            ::g_DoDEngine_CalcPing_Hook = true;
        }

        if (::g_pDoDEngine_HostPing_Addr && false == ::g_DoDEngine_HostPing_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostPing", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostPing_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDEngine_HostPing = ::subhook_new(::g_pDoDEngine_HostPing_Addr, ::DoD_Engine_HostPing_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDEngine_HostPing);
            ::DoD_Engine_HostPing = (::DoD_Engine_HostPing_Type) ::subhook_get_trampoline(::g_pDoDEngine_HostPing);
            ::g_DoDEngine_HostPing_Hook = true;
        }

        if (::g_pDoDEngine_HostStatus_Addr && false == ::g_DoDEngine_HostStatus_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostStatus", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostStatus_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDEngine_HostStatus = ::subhook_new(::g_pDoDEngine_HostStatus_Addr, ::DoD_Engine_HostStatus_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDEngine_HostStatus);
            ::DoD_Engine_HostStatus = (::DoD_Engine_HostStatus_Type) ::subhook_get_trampoline(::g_pDoDEngine_HostStatus);
            ::g_DoDEngine_HostStatus_Hook = true;
        }

        if (::g_pDoDEngine_HostStat_Addr && false == ::g_DoDEngine_HostStat_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostStat", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_HostStat_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDEngine_HostStat = ::subhook_new(::g_pDoDEngine_HostStat_Addr, ::DoD_Engine_HostStat_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDEngine_HostStat);
            ::DoD_Engine_HostStat = (::DoD_Engine_HostStat_Type) ::subhook_get_trampoline(::g_pDoDEngine_HostStat);
            ::g_DoDEngine_HostStat_Hook = true;
        }

        if (::g_pDoDEngine_WriteByte_Addr && false == ::g_DoDEngine_WriteByte_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_WriteByte", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_WriteByte_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDEngine_WriteByte = ::subhook_new(::g_pDoDEngine_WriteByte_Addr, ::DoD_Engine_WriteByte_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDEngine_WriteByte);
            ::DoD_Engine_WriteByte = (::DoD_Engine_WriteByte_Type) ::subhook_get_trampoline(::g_pDoDEngine_WriteByte);
            ::g_DoDEngine_WriteByte_Hook = true;
        }

        if (::g_pDoDEngine_WriteBits_Addr && false == ::g_DoDEngine_WriteBits_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_WriteBits", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_WriteBits_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDEngine_WriteBits = ::subhook_new(::g_pDoDEngine_WriteBits_Addr, ::DoD_Engine_WriteBits_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDEngine_WriteBits);
            ::DoD_Engine_WriteBits = (::DoD_Engine_WriteBits_Type) ::subhook_get_trampoline(::g_pDoDEngine_WriteBits);
            ::g_DoDEngine_WriteBits_Hook = true;
        }

        if (::g_pDoDEngine_EmitPings_Addr && false == ::g_DoDEngine_EmitPings_Hook &&
            (::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_EmitPings", &Func) == ::AMX_ERR_NONE ||
                ::g_fn_AmxFindPublic(pAmx, "DoD_OnEngine_EmitPings_Post", &Func) == ::AMX_ERR_NONE))
        {
            ::g_pDoDEngine_EmitPings = ::subhook_new(::g_pDoDEngine_EmitPings_Addr, ::DoD_Engine_EmitPings_Hook, ::SUBHOOK_TRAMPOLINE);
            ::subhook_install(::g_pDoDEngine_EmitPings);
            ::DoD_Engine_EmitPings = (::DoD_Engine_EmitPings_Type) ::subhook_get_trampoline(::g_pDoDEngine_EmitPings);
            ::g_DoDEngine_EmitPings_Hook = true;
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
    ::g_pEntities = NULL;
    ::g_selfNade = false;
    ::g_droppedNade = false;
    ::g_pFunctionTable->pfnCmdStart = NULL;
    ::g_pNewFunctionsTable->pfnShouldCollide = NULL;
    ::g_pFunctionTable_Post->pfnAddToFullPack = NULL;
    ::g_pFunctionTable_Post->pfnPlayerPostThink = NULL;
    ::g_Strings.clear();
    ::g_CustomKeyValues_Add.clear();
    ::g_CustomKeyValues_Del.clear();
    ::g_blockedFromPlayerCollision.clear();
    ::memset(&::g_svPlayerEngPtrs, false, sizeof(::g_svPlayerEngPtrs));
    ::memset(&::g_exclSelfNadeGlow, false, sizeof(::g_exclSelfNadeGlow));
    ::memset(&::g_exclDroppedNadeGlow, false, sizeof(::g_exclDroppedNadeGlow));
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
    if (::g_DoDEngine_CalcPing_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        if (::g_ReHLDS)
            ::DetourDetach(&(void*&) ::DoD_Engine_CalcPing_ReHLDS_Win, ::DoD_Engine_CalcPing_Hook_ReHLDS_Win);
        else
            ::DetourDetach(&(void*&) ::DoD_Engine_CalcPing, ::DoD_Engine_CalcPing_Hook);
        ::DetourTransactionCommit();
        ::g_DoDEngine_CalcPing_Hook = false;
    }
    if (::g_DoDEngine_HostPing_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_Engine_HostPing, ::DoD_Engine_HostPing_Hook);
        ::DetourTransactionCommit();
        ::g_DoDEngine_HostPing_Hook = false;
    }
    if (::g_DoDEngine_HostStatus_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_Engine_HostStatus, ::DoD_Engine_HostStatus_Hook);
        ::DetourTransactionCommit();
        ::g_DoDEngine_HostStatus_Hook = false;
    }
    if (::g_DoDEngine_HostStat_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_Engine_HostStat, ::DoD_Engine_HostStat_Hook);
        ::DetourTransactionCommit();
        ::g_DoDEngine_HostStat_Hook = false;
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
    if (::g_DoDChangePlayerTeam_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_ChangePlayerTeam, ::DoD_ChangePlayerTeam_Hook);
        ::DetourTransactionCommit();
        ::g_DoDChangePlayerTeam_Hook = false;
    }
    if (::g_DoDChooseRandomClass_Hook)
    {
        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());
        ::DetourDetach(&(void*&) ::DoD_ChooseRandomClass, ::DoD_ChooseRandomClass_Hook);
        ::DetourTransactionCommit();
        ::g_DoDChooseRandomClass_Hook = false;
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
    if (::g_DoDChangePlayerTeam_Hook)
    {
        ::subhook_remove(::g_pDoDChangePlayerTeam);
        ::subhook_free(::g_pDoDChangePlayerTeam);
        ::g_DoDChangePlayerTeam_Hook = false;
    }
    if (::g_DoDChooseRandomClass_Hook)
    {
        ::subhook_remove(::g_pDoDChooseRandomClass);
        ::subhook_free(::g_pDoDChooseRandomClass);
        ::g_DoDChooseRandomClass_Hook = false;
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
    if (::g_DoDEngine_CalcPing_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_CalcPing);
        ::subhook_free(::g_pDoDEngine_CalcPing);
        ::g_DoDEngine_CalcPing_Hook = false;
    }
    if (::g_DoDEngine_HostPing_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_HostPing);
        ::subhook_free(::g_pDoDEngine_HostPing);
        ::g_DoDEngine_HostPing_Hook = false;
    }
    if (::g_DoDEngine_HostStatus_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_HostStatus);
        ::subhook_free(::g_pDoDEngine_HostStatus);
        ::g_DoDEngine_HostStatus_Hook = false;
    }
    if (::g_DoDEngine_HostStat_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_HostStat);
        ::subhook_free(::g_pDoDEngine_HostStat);
        ::g_DoDEngine_HostStat_Hook = false;
    }
    if (::g_DoDEngine_WriteByte_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_WriteByte);
        ::subhook_free(::g_pDoDEngine_WriteByte);
        ::g_DoDEngine_WriteByte_Hook = false;
    }
    if (::g_DoDEngine_WriteBits_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_WriteBits);
        ::subhook_free(::g_pDoDEngine_WriteBits);
        ::g_DoDEngine_WriteBits_Hook = false;
    }
    if (::g_DoDEngine_EmitPings_Hook)
    {
        ::subhook_remove(::g_pDoDEngine_EmitPings);
        ::subhook_free(::g_pDoDEngine_EmitPings);
        ::g_DoDEngine_EmitPings_Hook = false;
    }
#endif
}

void OnMetaAttach()
{
    ::gpMetaUtilFuncs->pfnGetHookTables(&::Plugin_info, &::g_pEngineHookTable, &::g_pFunctionHookTable, &::g_pNewFunctionHookTable);
    ::g_engfuncs.pfnCVarRegister(&::g_Version);
}
