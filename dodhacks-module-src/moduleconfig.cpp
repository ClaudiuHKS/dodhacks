
#include "amxxmodule.h"

#ifndef __linux__
#include <detours.h>
#else
#include <subhook.h>
#include <sys/mman.h>
#endif

#include <Memory.h>

enum : unsigned char
{
	SIG_PLAYERSPAWN = false,
	SIG_GIVENAMEDITEM,
	SIG_DROPPLAYERITEM,
	SIG_GIVEAMMO,
	SIG_SETWAVETIME,
	SIG_GETWAVETIME,
	SIG_REMOVEPLAYERITEM,
	SIG_ADDPLAYERITEM,
	SIG_REMOVEALLITEMS,
	SIG_SETBODYGROUP,
	SIG_INSTALLGAMERULES,

	SIG_PATCHFG42,
	SIG_PATCHENFIELD,

	SIG_ORIGFG42_BYTE,
	SIG_PATCHFG42_BYTE,

	SIG_ORIGENFIELD_BYTE,
	SIG_PATCHENFIELD_BYTE,

	SIG_OFFS_ALLIESAREBRIT,
	SIG_OFFS_ALLIESAREPARA,
	SIG_OFFS_AXISAREPARA,
	SIG_OFFS_ALLIESRESPAWNFACTOR,
	SIG_OFFS_AXISRESPAWNFACTOR,
	SIG_OFFS_ALLIESINFINITELIVES,
	SIG_OFFS_AXISINFINITELIVES,

	SIG_OFFS_ITEMSCOPE,
	SIG_OFFS_APPLYITEMSCOPE,
};

typedef void(__thiscall* DoD_PlayerSpawn_Type) (::size_t CDoDTeamPlay, ::size_t CBasePlayer);
typedef ::edict_t* (__thiscall* DoD_GiveNamedItem_Type) (::size_t CBasePlayer, const char* pItem);
typedef void(__thiscall* DoD_DropPlayerItem_Type) (::size_t CBasePlayer, char* pItem, bool Force);
typedef int(__thiscall* DoD_GiveAmmo_Type) (::size_t CBasePlayer, int Ammo, const char* pName, int Max);
typedef void(__thiscall* DoD_SetWaveTime_Type) (::size_t CDoDTeamPlay, int Team, float Time);
typedef float(__thiscall* DoD_GetWaveTime_Type) (::size_t CDoDTeamPlay, int Team);
typedef int(__thiscall* DoD_RemovePlayerItem_Type) (::size_t CBasePlayer, ::size_t CBasePlayerItem);
typedef int(__thiscall* DoD_AddPlayerItem_Type) (::size_t CBasePlayer, ::size_t CBasePlayerItem);
typedef void(__thiscall* DoD_RemoveAllItems_Type) (::size_t CBasePlayer, int RemoveSuit);
typedef void(__thiscall* DoD_SetBodygroup_Type) (::size_t CBasePlayer, int Group, int Value);
typedef ::size_t(*DoD_InstallGameRules_Type) ();

struct AllocatedString
{
	::SourceHook::String Buffer;
	::size_t Index;
};

struct SignatureData
{
	::SourceHook::String Symbol;
	::SourceHook::CVector < unsigned char > Signature;
	bool IsSymbol;
	::size_t Offs;
};

struct CustomKeyValue_Del
{
	::SourceHook::String Class;
	::SourceHook::String Key;
	::SourceHook::String Value;
	::SourceHook::String Map;
};

struct CustomKeyValue_Add : CustomKeyValue_Del
{
	bool Added;
};

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
::DoD_InstallGameRules_Type DoD_InstallGameRules = NULL;

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
::subhook_t g_pDoDInstallGameRules = NULL;
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
int g_fwInstallGameRules = false;

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
int g_fwInstallGameRules_Post = false;

unsigned char* g_pAutoScopeFG42Addr = NULL;
unsigned char* g_pAutoScopeEnfieldAddr = NULL;

::SourceHook::CVector < ::SignatureData > g_Sigs;
::SourceHook::CVector < ::AllocatedString > g_Strings;
::SourceHook::CVector < ::CustomKeyValue_Add > g_CustomKeyValues_Add;
::SourceHook::CVector < ::CustomKeyValue_Del > g_CustomKeyValues_Del;

::size_t setupString(const char* pString)
{
	for (const auto& String : ::g_Strings)
	{
		if (false == String.Buffer.icmp(pString))
		{
			return String.Index;
		}
	}

	::AllocatedString String;
	String.Buffer = pString;
	String.Index = (*::g_engfuncs.pfnAllocString) (pString);
	::g_Strings.push_back(String);
	return String.Index;
}

void ServerDeactivate_Post()
{
	::g_Strings.clear();
	::g_CustomKeyValues_Add.clear();
	::g_CustomKeyValues_Del.clear();

	RETURN_META(::MRES_IGNORED);
}

void DispatchKeyValue(::edict_t* pEntity, ::KeyValueData* pKvData)
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
		{
			continue;
		}
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
			pKvData->fHandled = true;
			RETURN_META(::MRES_SUPERCEDE);
		}
		for (const auto& cusKeyVal : ::g_CustomKeyValues_Del)
		{ /// If this is a key value that the user wants to remove, filter it out.
			if (cusKeyVal.Class.icmp(Class) || (false == cusKeyVal.Map.empty() && cusKeyVal.Map.icmp(Map)) ||
				cusKeyVal.Key.icmp(pKvData->szKeyName))
			{ /// Has nothing to do with what we are looking for.
				continue;
			}
			if (!pKvData->szValue || false == *pKvData->szValue || cusKeyVal.Value.empty() ||
				false == cusKeyVal.Value.icmp(pKvData->szValue))
			{ /// If there's no value, delete it because it matched the key. If the values matches, delete as well.
				RETURN_META(::MRES_SUPERCEDE);
			}
		}
	}
	RETURN_META(::MRES_IGNORED);
}

static ::cell DoD_PlayerSpawn_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::DoD_PlayerSpawn)
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

	::DoD_PlayerSpawn(::g_CDoDTeamPlay, (::size_t)pPlayerBase);
	return true;
}

static ::cell DoD_RemoveAllItems_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::DoD_RemoveAllItems)
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

	::DoD_RemoveAllItems((::size_t)pPlayerBase, pParam[2]);
	return true;
}

static ::cell DoD_GiveNamedItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
	auto pItemRes = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
	if (pItemRes)
	{
		*pItemRes = false;
	}

	auto Player = pParam[1];
	if (Player < 1 || Player > ::gpGlobals->maxClients)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
		return false;
	}

	int Len = false;
	auto pItem = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
	if (Len < 1 || !pItem || false == *pItem)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item name string!");
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

	::edict_t* pEntity = NULL;

	if (false == ::_stricmp("weapon_scopedfg42", pItem))
	{
		pEntity = (*::g_engfuncs.pfnCreateNamedEntity) (::setupString("weapon_fg42"));
		if (pEntity /** && false != (*::g_engfuncs.pfnEntOffsetOfPEntity) (pEntity) */)
		{
			pEntity->v.origin = pPlayer->v.origin;
			pEntity->v.spawnflags |= 0x40000000u;

			::gpGamedllFuncs->dllapi_table->pfnSpawn(pEntity);
			::gpGamedllFuncs->dllapi_table->pfnTouch(pEntity, pPlayer);

			auto pItemBase = (::size_t*)pEntity->pvPrivateData;
			if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) & 1u))
			{
				*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) |= 1u;
				(*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::SIG_OFFS_APPLYITEMSCOPE].Offs)) (pItemBase);
			}
		}
		else
		{
			::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Could not create an entity for item '%s'!", pItem);
			return false;
		}
	}

	else if (false == ::_stricmp("weapon_scopedenfield", pItem))
	{
		pEntity = (*::g_engfuncs.pfnCreateNamedEntity) (::setupString("weapon_enfield"));
		if (pEntity /** && false != (*::g_engfuncs.pfnEntOffsetOfPEntity) (pEntity) */)
		{
			pEntity->v.origin = pPlayer->v.origin;
			pEntity->v.spawnflags |= 0x40000000u;

			::gpGamedllFuncs->dllapi_table->pfnSpawn(pEntity);
			::gpGamedllFuncs->dllapi_table->pfnTouch(pEntity, pPlayer);

			auto pItemBase = (::size_t*)pEntity->pvPrivateData;
			if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) & 1u))
			{
				*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) |= 1u;
				(*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::SIG_OFFS_APPLYITEMSCOPE].Offs)) (pItemBase);
			}
		}
		else
		{
			::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Could not create an entity for item '%s'!", pItem);
			return false;
		}
	}

	else
	{
		pEntity = (*::g_engfuncs.pfnCreateNamedEntity) (::setupString(pItem));
		if (pEntity /** && false != (*::g_engfuncs.pfnEntOffsetOfPEntity) (pEntity) */)
		{
			pEntity->v.origin = pPlayer->v.origin;
			pEntity->v.spawnflags |= 0x40000000u;

			::gpGamedllFuncs->dllapi_table->pfnSpawn(pEntity);
			::gpGamedllFuncs->dllapi_table->pfnTouch(pEntity, pPlayer);
		}
		else
		{
			::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Could not create an entity for item '%s'!", pItem);
			return false;
		}
	}

	if (pItemRes)
	{
		*pItemRes = (::cell) ::ENTINDEX(pEntity);
	}
	return true;
}

static ::cell DoD_DropPlayerItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::DoD_DropPlayerItem)
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

	int Len = false;
	auto pItem = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
	if (!pItem)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item name string!");
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

	::DoD_DropPlayerItem((::size_t)pPlayerBase, pItem, (bool)pParam[3]);
	return true;
}

static ::cell DoD_SetBodygroup_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::DoD_SetBodygroup)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_SetBodygroup not found!");
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

	::DoD_SetBodygroup((::size_t)pPlayerBase, pParam[2], pParam[3]);
	return true;
}

static ::cell DoD_GiveAmmo_Native(::tagAMX* pAmx, ::cell* pParam)
{
	auto pAmmoRes = ::g_fn_GetAmxAddr(pAmx, pParam[5]);
	if (pAmmoRes)
	{
		*pAmmoRes = false;
	}

	if (!::DoD_GiveAmmo)
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

	auto Ammo = pParam[2];
	if (Ammo < 1 || Ammo > 32767)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid ammo amount %d!", Ammo);
		return false;
	}

	auto Max = pParam[4];
	if (Max < Ammo || Max > 32767)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid maximum ammo amount %d!", Ammo);
		return false;
	}

	int Len = false;
	auto pName = ::g_fn_GetAmxString(pAmx, pParam[3], false, &Len);
	if (Len < 1 || !pName || false == *pName)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid ammo name string!");
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

	if (pAmmoRes)
	{
		*pAmmoRes = ::DoD_GiveAmmo((::size_t)pPlayerBase, Ammo, pName, Max);
	}
	else
	{
		::DoD_GiveAmmo((::size_t)pPlayerBase, Ammo, pName, Max);
	}
	return true;
}

static ::cell DoD_AddHealthIfWounded_Native(::tagAMX* pAmx, ::cell* pParam)
{
	auto pAdded = ::g_fn_GetAmxAddr(pAmx, pParam[3]);
	if (pAdded)
	{
		*pAdded = false;
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
	if (!::g_fn_IsPlayerAlive(Player))
	{
		return false;
	}

	auto pPlayer = ::g_fn_GetPlayerEdict(Player);
	if (!pPlayer)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
		return false;
	}

	if (pPlayer->v.health + (float)pParam[2] >= pPlayer->v.max_health)
	{
		if (pAdded)
		{
			*pAdded = ::cell(pPlayer->v.max_health - pPlayer->v.health);
		}
		pPlayer->v.health = pPlayer->v.max_health;
	}
	else
	{
		pPlayer->v.health += (float)pParam[2];
		if (pAdded)
		{
			*pAdded = pParam[2];
		}
	}
	return true;
}

static ::cell DoD_IsPlayerFullHealth_Native(::tagAMX* pAmx, ::cell* pParam)
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
	{
		return false;
	}

	auto pPlayer = ::g_fn_GetPlayerEdict(Player);
	if (!pPlayer)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
		return false;
	}

	return pPlayer->v.health == pPlayer->v.max_health;
}

static ::cell DoD_RemovePlayerItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::DoD_RemovePlayerItem)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_RemovePlayerItem not found!");
		return -1;
	}

	auto Player = pParam[1];
	if (Player < 1 || Player > ::gpGlobals->maxClients)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
		return -1;
	}

	if (!::g_fn_IsPlayerValid(Player))
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
		return -1;
	}
	if (!::g_fn_IsPlayerIngame(Player))
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
		return -1;
	}

	auto pPlayer = ::g_fn_GetPlayerEdict(Player);
	if (!pPlayer)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
		return -1;
	}

	auto pPrivateData = pPlayer->pvPrivateData;
	if (!pPrivateData)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
		return -1;
	}

	auto Item = pParam[2];
	if (Item <= ::gpGlobals->maxClients || Item > ::gpGlobals->maxEntities)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Item);
		return -1;
	}

	auto pItem = ::INDEXENT(Item);
	if (!pItem)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no edict!", Item);
		return -1;
	}

	auto pItemBase = pItem->pvPrivateData;
	if (!pItemBase)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no private data!", Item);
		return -1;
	}

	return ::DoD_RemovePlayerItem((::size_t)pPrivateData, (::size_t)pItemBase);
}

static ::cell DoD_AddPlayerItem_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::DoD_AddPlayerItem)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_AddPlayerItem not found!");
		return -1;
	}

	auto Player = pParam[1];
	if (Player < 1 || Player > ::gpGlobals->maxClients)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid player index %d!", Player);
		return -1;
	}

	if (!::g_fn_IsPlayerValid(Player))
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is invalid!", Player);
		return -1;
	}
	if (!::g_fn_IsPlayerIngame(Player))
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d is not in-game!", Player);
		return -1;
	}

	auto pPlayer = ::g_fn_GetPlayerEdict(Player);
	if (!pPlayer)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no edict!", Player);
		return -1;
	}

	auto pPrivateData = pPlayer->pvPrivateData;
	if (!pPrivateData)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Player %d has no private data!", Player);
		return -1;
	}

	auto Item = pParam[2];
	if (Item <= ::gpGlobals->maxClients || Item > ::gpGlobals->maxEntities)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid item entity index %d!", Item);
		return -1;
	}

	auto pItem = ::INDEXENT(Item);
	if (!pItem)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no edict!", Item);
		return -1;
	}

	auto pItemBase = pItem->pvPrivateData;
	if (!pItemBase)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Item %d has no private data!", Item);
		return -1;
	}

	return ::DoD_AddPlayerItem((::size_t)pPrivateData, (::size_t)pItemBase);
}

static ::cell DoD_HasScope_Native(::tagAMX* pAmx, ::cell* pParam)
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

	return *(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) & 1u;
}

static ::cell DoD_AddScope_Native(::tagAMX* pAmx, ::cell* pParam)
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

	if (!(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) & 1u))
	{
		*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) |= 1u;
		(*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::SIG_OFFS_APPLYITEMSCOPE].Offs)) (pItemBase);
	}
	return true;
}

static ::cell DoD_DeployItem_Native(::tagAMX* pAmx, ::cell* pParam)
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

	(*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::SIG_OFFS_APPLYITEMSCOPE].Offs)) (pItemBase);
	return true;
}

static ::cell DoD_SetWaveTime_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::DoD_SetWaveTime)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_SetWaveTime not found!");
		return false;
	}

	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto Team = pParam[1];
	if (Team < 1 || Team > 2)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid team index %d!", Team);
		return false;
	}

	auto Time = ::g_fn_CellToReal(pParam[2]);
	if (Time < 0.0f)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid wave time %f!", Time);
		return false;
	}

	::DoD_SetWaveTime(::g_CDoDTeamPlay, Team, Time);
	return true;
}

static ::cell DoD_GetWaveTime_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::DoD_GetWaveTime)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Signature for ::DoD_GetWaveTime not found!");
		return false;
	}

	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto Team = pParam[1];
	if (Team < 1 || Team > 2)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid team index %d!", Team);
		return false;
	}

	auto Time = ::DoD_GetWaveTime(::g_CDoDTeamPlay, Team);
	return ::g_fn_RealToCell(Time);
}

static ::cell DoD_AreAlliesBritish_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
	return *(pAddr + ::g_Sigs[::SIG_OFFS_ALLIESAREBRIT].Offs);
}

static ::cell DoD_AreAlliesParatroopers_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
	return *(pAddr + ::g_Sigs[::SIG_OFFS_ALLIESAREPARA].Offs);
}

static ::cell DoD_AreAxisParatroopers_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
	return *(pAddr + ::g_Sigs[::SIG_OFFS_AXISAREPARA].Offs);
}

static ::cell DoD_HaveAxisInfiniteLives_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
	return *(pAddr + ::g_Sigs[::SIG_OFFS_AXISINFINITELIVES].Offs);
}

static ::cell DoD_HaveAlliesInfiniteLives_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
	return *(pAddr + ::g_Sigs[::SIG_OFFS_ALLIESINFINITELIVES].Offs);
}

static ::cell DoD_GetAxisRespawnFactor_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (float*)(::g_CDoDTeamPlay + ::g_Sigs[::SIG_OFFS_AXISRESPAWNFACTOR].Offs);
	return ::g_fn_RealToCell(*pAddr);
}

static ::cell DoD_GetAlliesRespawnFactor_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (float*)(::g_CDoDTeamPlay + ::g_Sigs[::SIG_OFFS_ALLIESRESPAWNFACTOR].Offs);
	return ::g_fn_RealToCell(*pAddr);
}

static ::cell DoD_ReadGameRulesBool_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
	return *(pAddr + pParam[1]);
}

static ::cell DoD_ReadGameRulesInt_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (::size_t*)(::g_CDoDTeamPlay + pParam[1]);
	return (::cell)*pAddr;
}

static ::cell DoD_ReadGameRulesFloat_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	auto pAddr = (float*)(::g_CDoDTeamPlay + pParam[1]);
	return ::g_fn_RealToCell(*pAddr);
}

static ::cell DoD_ReadGameRulesStr_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (false == ::g_CDoDTeamPlay)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "::CDoDTeamPlay (Game Rules) pointer is null at the moment!");
		return false;
	}

	::SourceHook::String Buffer;
	auto pAddr = (unsigned char*)(::g_CDoDTeamPlay);
	for (::size_t Iter = false; Iter < (::size_t)pParam[2]; Iter++)
	{
		Buffer.append(*(pAddr + pParam[1] + Iter));
	}

	if (pParam[5])
	{
		return ::g_fn_SetAmxStringUTF8Char(pAmx, pParam[3], Buffer.c_str(), Buffer.size(), pParam[4]);
	}
	return ::g_fn_SetAmxString(pAmx, pParam[3], Buffer.c_str(), pParam[4]);
}

static ::cell DoD_IsWeaponPrimary_Native(::tagAMX* pAmx, ::cell* pParam)
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

	int Len = false;
	auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
	if (Len < 1 || !pName || false == *pName)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon name string!");
		return false;
	}

	for (const auto& Weapon : Weapons)
	{
		if (false == ::_stricmp(Weapon, pName))
		{
			return true;
		}
	}
	return false;
}

static ::cell DoD_IsWeaponKnife_Native(::tagAMX* pAmx, ::cell* pParam)
{
	static const char* Weapons[] =
	{
		"weapon_spade", "weapon_amerknife", "weapon_gerknife",
		"spade", "amerknife", "gerknife",
	};

	int Len = false;
	auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
	if (Len < 1 || !pName || false == *pName)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon name string!");
		return false;
	}

	for (const auto& Weapon : Weapons)
	{
		if (false == ::_stricmp(Weapon, pName))
		{
			return true;
		}
	}
	return false;
}

static ::cell DoD_IsWeaponSecondary_Native(::tagAMX* pAmx, ::cell* pParam)
{
	static const char* Weapons[] =
	{
		"weapon_webley", "weapon_colt", "weapon_luger",
		"webley", "colt", "luger",
	};

	int Len = false;
	auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
	if (Len < 1 || !pName || false == *pName)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon name string!");
		return false;
	}

	for (const auto& Weapon : Weapons)
	{
		if (false == ::_stricmp(Weapon, pName))
		{
			return true;
		}
	}
	return false;
}

static ::cell DoD_IsWeaponGrenade_Native(::tagAMX* pAmx, ::cell* pParam)
{
	static const char* Weapons[] =
	{
		"weapon_handgrenade", "weapon_handgrenade_ex",
		"weapon_stickgrenade", "weapon_stickgrenade_ex",
		"handgrenade", "handgrenade_ex",
		"stickgrenade", "stickgrenade_ex",
	};

	int Len = false;
	auto pName = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
	if (Len < 1 || !pName || false == *pName)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid weapon name string!");
		return false;
	}

	for (const auto& Weapon : Weapons)
	{
		if (false == ::_stricmp(Weapon, pName))
		{
			return true;
		}
	}
	return false;
}

static ::cell DoD_AddKeyValDel_Native(::tagAMX* pAmx, ::cell* pParam)
{
	int Len = false;
	auto pString = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
	if (!pString)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid map name string!");
		return false;
	}

	::CustomKeyValue_Del keyValData;
	keyValData.Map = pString;

	pString = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
	if (Len < 1 || !pString || false == *pString)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity class name string!");
		return false;
	}
	keyValData.Class = pString;

	pString = ::g_fn_GetAmxString(pAmx, pParam[3], false, &Len);
	if (Len < 1 || !pString || false == *pString)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity key name string!");
		return false;
	}
	keyValData.Key = pString;

	pString = ::g_fn_GetAmxString(pAmx, pParam[4], false, &Len);
	if (!pString)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity value name string!");
		return false;
	}
	keyValData.Value = pString;

	::g_CustomKeyValues_Del.push_back(keyValData);
	return true;
}

static ::cell DoD_AddKeyValAdd_Native(::tagAMX* pAmx, ::cell* pParam)
{
	int Len = false;
	auto pString = ::g_fn_GetAmxString(pAmx, pParam[1], false, &Len);
	if (!pString)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid map name string!");
		return false;
	}

	::CustomKeyValue_Add keyValData;
	keyValData.Added = false;
	keyValData.Map = pString;

	pString = ::g_fn_GetAmxString(pAmx, pParam[2], false, &Len);
	if (Len < 1 || !pString || false == *pString)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity class name string!");
		return false;
	}
	keyValData.Class = pString;

	pString = ::g_fn_GetAmxString(pAmx, pParam[3], false, &Len);
	if (Len < 1 || !pString || false == *pString)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity key name string!");
		return false;
	}
	keyValData.Key = pString;

	pString = ::g_fn_GetAmxString(pAmx, pParam[4], false, &Len);
	if (Len < 1 || !pString || false == *pString)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Invalid entity value name string!");
		return false;
	}
	keyValData.Value = pString;

	::g_CustomKeyValues_Add.push_back(keyValData);
	return true;
}

static ::cell DoD_DisableAutoScoping_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::g_pAutoScopeFG42Addr || !::g_pAutoScopeEnfieldAddr)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Auto-scoping signature(s) not found!");
		return false;
	}

#ifndef __linux__
	unsigned long origProtection_FG42 = false;
	::VirtualProtect(::g_pAutoScopeFG42Addr, true, PAGE_EXECUTE_READWRITE, &origProtection_FG42);
	*::g_pAutoScopeFG42Addr = ::g_Sigs[::SIG_PATCHFG42_BYTE].Signature[0];
	unsigned long oldProtection_FG42 = false;
	::VirtualProtect(::g_pAutoScopeFG42Addr, true, origProtection_FG42, &oldProtection_FG42);

	unsigned long origProtection_Enfield = false;
	::VirtualProtect(::g_pAutoScopeEnfieldAddr, true, PAGE_EXECUTE_READWRITE, &origProtection_Enfield);
	*::g_pAutoScopeEnfieldAddr = ::g_Sigs[::SIG_PATCHENFIELD_BYTE].Signature[0];
	unsigned long oldProtection_Enfield = false;
	::VirtualProtect(::g_pAutoScopeEnfieldAddr, true, origProtection_Enfield, &oldProtection_Enfield);
#else
	::mprotect(::g_pAutoScopeFG42Addr, true, PROT_READ | PROT_WRITE | PROT_EXEC);
	*::g_pAutoScopeFG42Addr = ::g_Sigs[::SIG_PATCHFG42_BYTE].Signature[0];
	::mprotect(::g_pAutoScopeFG42Addr, true, PROT_READ | PROT_EXEC);

	::mprotect(::g_pAutoScopeEnfieldAddr, true, PROT_READ | PROT_WRITE | PROT_EXEC);
	*::g_pAutoScopeEnfieldAddr = ::g_Sigs[::SIG_PATCHENFIELD_BYTE].Signature[0];
	::mprotect(::g_pAutoScopeEnfieldAddr, true, PROT_READ | PROT_EXEC);
#endif

	return true;
}

static ::cell DoD_EnableAutoScoping_Native(::tagAMX* pAmx, ::cell* pParam)
{
	if (!::g_pAutoScopeFG42Addr || !::g_pAutoScopeEnfieldAddr)
	{
		::MF_LogError(pAmx, ::AMX_ERR_NATIVE, "Auto-scoping signature(s) not found!");
		return false;
	}

#ifndef __linux__
	unsigned long origProtection_FG42 = false;
	::VirtualProtect(::g_pAutoScopeFG42Addr, true, PAGE_EXECUTE_READWRITE, &origProtection_FG42);
	*::g_pAutoScopeFG42Addr = ::g_Sigs[::SIG_ORIGFG42_BYTE].Signature[0];
	unsigned long oldProtection_FG42 = false;
	::VirtualProtect(::g_pAutoScopeFG42Addr, true, origProtection_FG42, &oldProtection_FG42);

	unsigned long origProtection_Enfield = false;
	::VirtualProtect(::g_pAutoScopeEnfieldAddr, true, PAGE_EXECUTE_READWRITE, &origProtection_Enfield);
	*::g_pAutoScopeEnfieldAddr = ::g_Sigs[::SIG_ORIGENFIELD_BYTE].Signature[0];
	unsigned long oldProtection_Enfield = false;
	::VirtualProtect(::g_pAutoScopeEnfieldAddr, true, origProtection_Enfield, &oldProtection_Enfield);
#else
	::mprotect(::g_pAutoScopeFG42Addr, true, PROT_READ | PROT_WRITE | PROT_EXEC);
	*::g_pAutoScopeFG42Addr = ::g_Sigs[::SIG_ORIGFG42_BYTE].Signature[0];
	::mprotect(::g_pAutoScopeFG42Addr, true, PROT_READ | PROT_EXEC);

	::mprotect(::g_pAutoScopeEnfieldAddr, true, PROT_READ | PROT_WRITE | PROT_EXEC);
	*::g_pAutoScopeEnfieldAddr = ::g_Sigs[::SIG_ORIGENFIELD_BYTE].Signature[0];
	::mprotect(::g_pAutoScopeEnfieldAddr, true, PROT_READ | PROT_EXEC);
#endif

	return true;
}

static ::cell DoD_AreGameRulesReady_Native(::tagAMX* pAmx, ::cell* pParam)
{
	return false != ::g_CDoDTeamPlay;
}

static ::cell DoD_InstallGameRules_Native(::tagAMX* pAmx, ::cell* pParam)
{
	return (::cell) ::DoD_InstallGameRules();
}

void __fastcall DoD_PlayerSpawn_Hook(::size_t CDoDTeamPlay, FASTCALL_PARAM::size_t CBasePlayer)
{
	if (false != CDoDTeamPlay)
	{
		::g_CDoDTeamPlay = CDoDTeamPlay;
	}

	if (false == CBasePlayer)
	{
		::DoD_PlayerSpawn(CDoDTeamPlay, CBasePlayer);
		return;
	}

	auto pPlayerVars = *(entvars_t**)(CBasePlayer + 4);
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
	if (::g_fn_ExecuteForward(::g_fwPlayerSpawn, CDoDTeamPlay, Player))
	{
		return;
	}

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

	auto pPlayerVars = *(entvars_t**)(CBasePlayer + 4);
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
	{
		return;
	}

	::DoD_SetBodygroup(CBasePlayer, Group, Value);
	::g_fn_ExecuteForward(::g_fwSetBodygroup_Post, Player, Group, Value);
}

::edict_t* __fastcall DoD_GiveNamedItem_Hook(::size_t CBasePlayer, FASTCALL_PARAM const char* pItem)
{
	if (false == CBasePlayer)
	{
		return ::DoD_GiveNamedItem(CBasePlayer, pItem);
	}

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

	auto pPlayerVars = *(::entvars_t**)(CBasePlayer + 4);
	if (!pPlayerVars)
	{
		return ::DoD_GiveNamedItem(CBasePlayer, pItem);
	}

	auto pPlayer = pPlayerVars->pContainingEntity;
	if (!pPlayer)
	{
		return ::DoD_GiveNamedItem(CBasePlayer, pItem);
	}

#ifndef __linux__
	auto Player = ::ENTINDEX(pPlayer);
	static char Buffer[128];
	::strncpy_s(Buffer, sizeof Buffer, pItem, _TRUNCATE);
	if (::g_fn_ExecuteForward(::g_fwGiveNamedItem, Player, Buffer, sizeof Buffer) || false == Buffer[0])
	{
		return NULL;
	}
#else
	auto Player = ::ENTINDEX(pPlayer);
	static char Buffer[128];
	::snprintf(Buffer, sizeof Buffer, pItem);
	if (::g_fn_ExecuteForward(::g_fwGiveNamedItem, Player, Buffer, sizeof Buffer) || false == Buffer[0])
	{
		return NULL;
	}
#endif

	::edict_t* pEntity = NULL;

	if (false == ::_stricmp("weapon_scopedfg42", Buffer))
	{
		pEntity = (*::g_engfuncs.pfnCreateNamedEntity) (::setupString("weapon_fg42"));
		if (pEntity /** && false != (*::g_engfuncs.pfnEntOffsetOfPEntity) (pEntity) */)
		{
			pEntity->v.origin = pPlayer->v.origin;
			pEntity->v.spawnflags |= 0x40000000u;

			::gpGamedllFuncs->dllapi_table->pfnSpawn(pEntity);
			::gpGamedllFuncs->dllapi_table->pfnTouch(pEntity, pPlayer);

			auto pItemBase = (::size_t*)pEntity->pvPrivateData;
			if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) & 1u))
			{
				*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) |= 1u;
				(*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::SIG_OFFS_APPLYITEMSCOPE].Offs)) (pItemBase);
			}
		}
	}

	else if (false == ::_stricmp("weapon_scopedenfield", Buffer))
	{
		pEntity = (*::g_engfuncs.pfnCreateNamedEntity) (::setupString("weapon_enfield"));
		if (pEntity /** && false != (*::g_engfuncs.pfnEntOffsetOfPEntity) (pEntity) */)
		{
			pEntity->v.origin = pPlayer->v.origin;
			pEntity->v.spawnflags |= 0x40000000u;

			::gpGamedllFuncs->dllapi_table->pfnSpawn(pEntity);
			::gpGamedllFuncs->dllapi_table->pfnTouch(pEntity, pPlayer);

			auto pItemBase = (::size_t*)pEntity->pvPrivateData;
			if (pItemBase && !(*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) & 1u))
			{
				*(::size_t*)((unsigned char*)pItemBase + ::g_Sigs[::SIG_OFFS_ITEMSCOPE].Offs) |= 1u;
				(*(void(__thiscall**) (::size_t*)) (*pItemBase + ::g_Sigs[::SIG_OFFS_APPLYITEMSCOPE].Offs)) (pItemBase);
			}
		}
	}

	else
	{
		pEntity = (*::g_engfuncs.pfnCreateNamedEntity) (::setupString(Buffer));
		if (pEntity /** && false != (*::g_engfuncs.pfnEntOffsetOfPEntity) (pEntity) */)
		{
			pEntity->v.origin = pPlayer->v.origin;
			pEntity->v.spawnflags |= 0x40000000u;

			::gpGamedllFuncs->dllapi_table->pfnSpawn(pEntity);
			::gpGamedllFuncs->dllapi_table->pfnTouch(pEntity, pPlayer);
		}
	}

	::g_fn_ExecuteForward(::g_fwGiveNamedItem_Post, Player, pItem, pEntity ? ::ENTINDEX(pEntity) : false);
	return pEntity;
}

int __fastcall DoD_GiveAmmo_Hook(::size_t CBasePlayer, FASTCALL_PARAM int Ammo, const char* pName, int Max)
{
	if (false == CBasePlayer)
	{
		return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);
	}

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

	auto pPlayerVars = *(::entvars_t**)(CBasePlayer + 4);
	if (!pPlayerVars)
	{
		return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);
	}

	auto pPlayer = pPlayerVars->pContainingEntity;
	if (!pPlayer)
	{
		return ::DoD_GiveAmmo(CBasePlayer, Ammo, pName, Max);
	}

#ifndef __linux__
	auto Player = ::ENTINDEX(pPlayer);
	static char Buffer[128];
	::strncpy_s(Buffer, sizeof Buffer, pName, _TRUNCATE);
	if (::g_fn_ExecuteForward(::g_fwGiveAmmo, Player, &Ammo, Buffer, sizeof Buffer, &Max) || false == Buffer[0])
	{
		return -1;
	}
#else
	auto Player = ::ENTINDEX(pPlayer);
	static char Buffer[128];
	::snprintf(Buffer, sizeof Buffer, pName);
	if (::g_fn_ExecuteForward(::g_fwGiveAmmo, Player, &Ammo, Buffer, sizeof Buffer, &Max) || false == Buffer[0])
	{
		return -1;
	}
#endif

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

	auto pPlayerVars = *(::entvars_t**)(CBasePlayer + 4);
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

#ifndef __linux__
	auto Player = ::ENTINDEX(pPlayer);
	static char Buffer[128];
	::strncpy_s(Buffer, sizeof Buffer, pItem, _TRUNCATE);
	if (::g_fn_ExecuteForward(::g_fwDropPlayerItem, Player, Buffer, sizeof Buffer, &Force))
	{
		return;
	}
#else
	auto Player = ::ENTINDEX(pPlayer);
	static char Buffer[128];
	::snprintf(Buffer, sizeof Buffer, pItem);
	if (::g_fn_ExecuteForward(::g_fwDropPlayerItem, Player, Buffer, sizeof Buffer, &Force))
	{
		return;
	}
#endif

	::DoD_DropPlayerItem(CBasePlayer, Buffer, Force);
	::g_fn_ExecuteForward(::g_fwDropPlayerItem_Post, Player, Buffer, Force);
}

int __fastcall DoD_RemovePlayerItem_Hook(::size_t CBasePlayer, FASTCALL_PARAM::size_t CBasePlayerItem)
{
	if (false == CBasePlayer || false == CBasePlayerItem)
	{
		return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto pPlayerVars = *(entvars_t**)(CBasePlayer + 4);
	if (!pPlayerVars)
	{
		return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto pItemVars = *(::entvars_t**)(CBasePlayerItem + 4);
	if (!pItemVars)
	{
		return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto pPlayer = pPlayerVars->pContainingEntity;
	if (!pPlayer)
	{
		return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto pItem = pItemVars->pContainingEntity;
	if (!pItem)
	{
		return ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto Player = ::ENTINDEX(pPlayer);
	auto Item = ::ENTINDEX(pItem);
	if (::g_fn_ExecuteForward(::g_fwRemovePlayerItem, Player, Item))
	{
		return false;
	}

	auto Res = ::DoD_RemovePlayerItem(CBasePlayer, CBasePlayerItem);
	::g_fn_ExecuteForward(::g_fwRemovePlayerItem_Post, Player, Item, Res);
	return Res;
}

int __fastcall DoD_AddPlayerItem_Hook(::size_t CBasePlayer, FASTCALL_PARAM::size_t CBasePlayerItem)
{
	if (false == CBasePlayer || false == CBasePlayerItem)
	{
		return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto pPlayerVars = *(::entvars_t**)(CBasePlayer + 4);
	if (!pPlayerVars)
	{
		return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto pItemVars = *(::entvars_t**)(CBasePlayerItem + 4);
	if (!pItemVars)
	{
		return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto pPlayer = pPlayerVars->pContainingEntity;
	if (!pPlayer)
	{
		return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto pItem = pItemVars->pContainingEntity;
	if (!pItem)
	{
		return ::DoD_AddPlayerItem(CBasePlayer, CBasePlayerItem);
	}

	auto Player = ::ENTINDEX(pPlayer);
	auto Item = ::ENTINDEX(pItem);
	if (::g_fn_ExecuteForward(::g_fwAddPlayerItem, Player, Item))
	{
		return false;
	}

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

	auto pPlayerVars = *(::entvars_t**)(CBasePlayer + 4);
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
	{
		return;
	}

	::DoD_RemoveAllItems(CBasePlayer, RemoveSuit);
	::g_fn_ExecuteForward(::g_fwRemoveAllItems_Post, Player, RemoveSuit);
}

float __fastcall DoD_GetWaveTime_Hook(::size_t CDoDTeamPlay, FASTCALL_PARAM int Team)
{
	if (false != CDoDTeamPlay)
	{
		::g_CDoDTeamPlay = CDoDTeamPlay;
	}

	float Time = false;
	if (::g_fn_ExecuteForward(::g_fwGetWaveTime, CDoDTeamPlay, &Team, &Time))
	{
		return Time;
	}

	Time = ::DoD_GetWaveTime(CDoDTeamPlay, Team);
	::g_fn_ExecuteForward(::g_fwGetWaveTime_Post, CDoDTeamPlay, Team, Time);
	return Time;
}

void __fastcall DoD_SetWaveTime_Hook(::size_t CDoDTeamPlay, FASTCALL_PARAM int Team, float Time)
{
	if (false != CDoDTeamPlay)
	{
		::g_CDoDTeamPlay = CDoDTeamPlay;
	}

	if (::g_fn_ExecuteForward(::g_fwSetWaveTime, CDoDTeamPlay, &Team, &Time))
	{
		return;
	}

	::DoD_SetWaveTime(CDoDTeamPlay, Team, Time);
	::g_fn_ExecuteForward(::g_fwSetWaveTime_Post, CDoDTeamPlay, Team, Time);
}

::size_t DoD_InstallGameRules_Hook()
{
	if (::g_fn_ExecuteForward(::g_fwInstallGameRules))
	{
		return false;
	}

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

	{ "DoD_SetWaveTime", ::DoD_SetWaveTime_Native, },
	{ "DoD_GetWaveTime", ::DoD_GetWaveTime_Native, },

	{ "DoD_IsWeaponPrimary", ::DoD_IsWeaponPrimary_Native, },
	{ "DoD_IsWeaponSecondary", ::DoD_IsWeaponSecondary_Native, },
	{ "DoD_IsWeaponKnife", ::DoD_IsWeaponKnife_Native, },
	{ "DoD_IsWeaponGrenade", ::DoD_IsWeaponGrenade_Native, },

	{ "DoD_DisableAutoScoping", ::DoD_DisableAutoScoping_Native, },
	{ "DoD_EnableAutoScoping", ::DoD_EnableAutoScoping_Native, },

	{ "DoD_HasScope", ::DoD_HasScope_Native, },
	{ "DoD_AddScope", ::DoD_AddScope_Native, },

	{ "DoD_AddKeyValDel", ::DoD_AddKeyValDel_Native, },
	{ "DoD_AddKeyValAdd", ::DoD_AddKeyValAdd_Native, },

	{ "DoD_RemovePlayerItem", ::DoD_RemovePlayerItem_Native, },
	{ "DoD_AddPlayerItem", ::DoD_AddPlayerItem_Native, },

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

void OnAmxxAttach()
{
#ifndef __linux__
	char Buffer[256];
	::g_fn_BuildPathnameR(Buffer, sizeof Buffer, "%s/dod_hacks_signatures.ini", ::g_fn_GetLocalInfo("amxx_configsdir", "addons/amxmodx/configs"));
	::_iobuf* pConfig = NULL;
	::fopen_s(&pConfig, Buffer, "r");
	if (!pConfig)
	{
		::MF_Log("Unable to open '%s'!", Buffer);
		::g_fn_AddNatives(::DoDHacks_Natives);
		return;
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
		if (true == Linux)
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

	auto pDoD = ::GetModuleHandleA("dod.dll");
	if (!pDoD)
	{
		::MF_Log("::GetModuleHandleA failed!");
		::g_fn_AddNatives(::DoDHacks_Natives);
		return;
	}

	::_MEMORY_BASIC_INFORMATION memInfo{ };
	if (false == ::VirtualQuery(pDoD, &memInfo, sizeof memInfo) || !memInfo.AllocationBase)
	{
		::MF_Log("::VirtualQuery failed!");
		::g_fn_AddNatives(::DoDHacks_Natives);
		return;
	}

	::size_t Addr = false;
	auto pImgDosHdr = (::_IMAGE_DOS_HEADER*)memInfo.AllocationBase;
	auto pImgNtHdr = (::_IMAGE_NT_HEADERS*)((::size_t)pImgDosHdr + (::size_t)pImgDosHdr->e_lfanew);

	if (::g_Sigs[::SIG_PLAYERSPAWN].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_PLAYERSPAWN].Signature, &Addr, true))
		{
			::DoD_PlayerSpawn = (::DoD_PlayerSpawn_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_PlayerSpawn, ::DoD_PlayerSpawn_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_PlayerSpawn signature not found!");
		}
	}
	else
	{
		if (::DoD_PlayerSpawn = (::DoD_PlayerSpawn_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_PLAYERSPAWN].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_PlayerSpawn, ::DoD_PlayerSpawn_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_PlayerSpawn symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_GIVENAMEDITEM].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_GIVENAMEDITEM].Signature, &Addr, true))
		{
			::DoD_GiveNamedItem = (::DoD_GiveNamedItem_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_GiveNamedItem, ::DoD_GiveNamedItem_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_GiveNamedItem signature not found!");
		}
	}
	else
	{
		if (::DoD_GiveNamedItem = (::DoD_GiveNamedItem_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_GIVENAMEDITEM].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_GiveNamedItem, ::DoD_GiveNamedItem_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_GiveNamedItem symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_DROPPLAYERITEM].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_DROPPLAYERITEM].Signature, &Addr, true))
		{
			::DoD_DropPlayerItem = (::DoD_DropPlayerItem_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_DropPlayerItem, ::DoD_DropPlayerItem_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_DropPlayerItem signature not found!");
		}
	}
	else
	{
		if (::DoD_DropPlayerItem = (::DoD_DropPlayerItem_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_DROPPLAYERITEM].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_DropPlayerItem, ::DoD_DropPlayerItem_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_DropPlayerItem symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_GIVEAMMO].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_GIVEAMMO].Signature, &Addr, true))
		{
			::DoD_GiveAmmo = (::DoD_GiveAmmo_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_GiveAmmo, ::DoD_GiveAmmo_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_GiveAmmo signature not found!");
		}
	}
	else
	{
		if (::DoD_GiveAmmo = (::DoD_GiveAmmo_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_GIVEAMMO].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_GiveAmmo, ::DoD_GiveAmmo_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_GiveAmmo symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_SETWAVETIME].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_SETWAVETIME].Signature, &Addr, true))
		{
			::DoD_SetWaveTime = (::DoD_SetWaveTime_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_SetWaveTime, ::DoD_SetWaveTime_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_SetWaveTime signature not found!");
		}
	}
	else
	{
		if (::DoD_SetWaveTime = (::DoD_SetWaveTime_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_SETWAVETIME].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_SetWaveTime, ::DoD_SetWaveTime_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_SetWaveTime symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_GETWAVETIME].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_GETWAVETIME].Signature, &Addr, true))
		{
			::DoD_GetWaveTime = (::DoD_GetWaveTime_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_GetWaveTime, ::DoD_GetWaveTime_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_GetWaveTime signature not found!");
		}
	}
	else
	{
		if (::DoD_GetWaveTime = (::DoD_GetWaveTime_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_GETWAVETIME].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_GetWaveTime, ::DoD_GetWaveTime_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_GetWaveTime symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_REMOVEPLAYERITEM].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_REMOVEPLAYERITEM].Signature, &Addr, true))
		{
			::DoD_RemovePlayerItem = (::DoD_RemovePlayerItem_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_RemovePlayerItem, ::DoD_RemovePlayerItem_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_RemovePlayerItem signature not found!");
		}
	}
	else
	{
		if (::DoD_RemovePlayerItem = (::DoD_RemovePlayerItem_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_REMOVEPLAYERITEM].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_RemovePlayerItem, ::DoD_RemovePlayerItem_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_RemovePlayerItem symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_ADDPLAYERITEM].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_ADDPLAYERITEM].Signature, &Addr, true))
		{
			::DoD_AddPlayerItem = (::DoD_AddPlayerItem_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_AddPlayerItem, ::DoD_AddPlayerItem_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_AddPlayerItem signature not found!");
		}
	}
	else
	{
		if (::DoD_AddPlayerItem = (::DoD_AddPlayerItem_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_ADDPLAYERITEM].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_AddPlayerItem, ::DoD_AddPlayerItem_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_AddPlayerItem symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_REMOVEALLITEMS].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_REMOVEALLITEMS].Signature, &Addr, true))
		{
			::DoD_RemoveAllItems = (::DoD_RemoveAllItems_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_RemoveAllItems, ::DoD_RemoveAllItems_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_RemoveAllItems signature not found!");
		}
	}
	else
	{
		if (::DoD_RemoveAllItems = (::DoD_RemoveAllItems_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_REMOVEALLITEMS].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_RemoveAllItems, ::DoD_RemoveAllItems_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_RemoveAllItems symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_SETBODYGROUP].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_SETBODYGROUP].Signature, &Addr, true))
		{
			::DoD_SetBodygroup = (::DoD_SetBodygroup_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_SetBodygroup, ::DoD_SetBodygroup_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_SetBodygroup signature not found!");
		}
	}
	else
	{
		if (::DoD_SetBodygroup = (::DoD_SetBodygroup_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_SETBODYGROUP].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_SetBodygroup, ::DoD_SetBodygroup_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_SetBodygroup symbol not found!");
		}
	}

	if (::g_Sigs[::SIG_INSTALLGAMERULES].IsSymbol == false)
	{
		if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_INSTALLGAMERULES].Signature, &Addr, true))
		{
			::DoD_InstallGameRules = (::DoD_InstallGameRules_Type)Addr;
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_InstallGameRules, ::DoD_InstallGameRules_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_InstallGameRules signature not found!");
		}
	}
	else
	{
		if (::DoD_InstallGameRules = (::DoD_InstallGameRules_Type) ::GetProcAddress(pDoD, ::g_Sigs[::SIG_INSTALLGAMERULES].Symbol.c_str()))
		{
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
			::DetourAttach(&(void*&) ::DoD_InstallGameRules, ::DoD_InstallGameRules_Hook);
			::DetourTransactionCommit();
		}
		else
		{
			::MF_Log("::DoD_InstallGameRules symbol not found!");
		}
	}

	if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_PATCHFG42].Signature, &Addr, true))
	{
		::g_pAutoScopeFG42Addr = (unsigned char*)Addr;
	}
	else
	{
		::MF_Log("::DoD_PatchAutoScope(FG42) signature not found!");
	}

	if (::findInMemory((unsigned char*)memInfo.AllocationBase, pImgNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::SIG_PATCHENFIELD].Signature, &Addr, true))
	{
		::g_pAutoScopeEnfieldAddr = (unsigned char*)Addr;
	}
	else
	{
		::MF_Log("::DoD_PatchAutoScope(Enfield) signature not found!");
	}
#else
	char Buffer[256];
	::g_fn_BuildPathnameR(Buffer, sizeof Buffer, "%s/dod_hacks_signatures.ini", ::g_fn_GetLocalInfo("amxx_configsdir", "addons/amxmodx/configs"));
	::FILE* pConfig = ::fopen(Buffer, "r");
	if (!pConfig)
	{
		::MF_Log("Unable to open '%s'!", Buffer);
		::g_fn_AddNatives(::DoDHacks_Natives);
		return;
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
		if (false == Linux)
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

	::g_fn_BuildPathnameR(Buffer, sizeof Buffer, "dlls/dod.so");
	void* pDoD = ::dlopen(Buffer, RTLD_LAZY | RTLD_NODELETE | RTLD_NOLOAD);
	if (!pDoD)
	{
		pDoD = ::dlopen(Buffer, RTLD_NOW);
		if (!pDoD)
		{
			::MF_Log("::dlopen failed!");
			::g_fn_AddNatives(::DoDHacks_Natives);
			return;
		}
	}

	void* pSym = ::dlsym(pDoD, "GiveFnptrsToDll");
	if (!pSym)
	{
		::MF_Log("::dlsym (::GiveFnptrsToDll) failed!");
		::dlclose(pDoD);
		::g_fn_AddNatives(::DoDHacks_Natives);
		return;
	}

	::Dl_info memInfo;
	if (!::dladdr(pSym, &memInfo))
	{
		::MF_Log("::dladdr failed!");
		::dlclose(pDoD);
		::g_fn_AddNatives(::DoDHacks_Natives);
		return;
	}

	struct ::stat memData;
	if (::stat(memInfo.dli_fname, &memData))
	{
		::MF_Log("::stat failed!");
		::dlclose(pDoD);
		::g_fn_AddNatives(::DoDHacks_Natives);
		return;
	}

	::size_t Addr = false;
	void* pAddr = NULL;

	if (::g_Sigs[::SIG_PLAYERSPAWN].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_PLAYERSPAWN].Symbol.c_str()))
		{
			::g_pDoDPlayerSpawn = ::subhook_new(pAddr, ::DoD_PlayerSpawn_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDPlayerSpawn);
			::DoD_PlayerSpawn = (::DoD_PlayerSpawn_Type) ::subhook_get_trampoline(::g_pDoDPlayerSpawn);
		}
		else
		{
			::MF_Log("::DoD_PlayerSpawn symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_PLAYERSPAWN].Signature, &Addr, true))
		{
			::g_pDoDPlayerSpawn = ::subhook_new((void*)Addr, ::DoD_PlayerSpawn_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDPlayerSpawn);
			::DoD_PlayerSpawn = (::DoD_PlayerSpawn_Type) ::subhook_get_trampoline(::g_pDoDPlayerSpawn);
		}
		else
		{
			::MF_Log("::DoD_PlayerSpawn signature not found!");
		}
	}

	if (::g_Sigs[::SIG_GIVENAMEDITEM].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_GIVENAMEDITEM].Symbol.c_str()))
		{
			::g_pDoDGiveNamedItem = ::subhook_new(pAddr, ::DoD_GiveNamedItem_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDGiveNamedItem);
			::DoD_GiveNamedItem = (::DoD_GiveNamedItem_Type) ::subhook_get_trampoline(::g_pDoDGiveNamedItem);
		}
		else
		{
			::MF_Log("::DoD_GiveNamedItem symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_GIVENAMEDITEM].Signature, &Addr, true))
		{
			::g_pDoDGiveNamedItem = ::subhook_new((void*)Addr, ::DoD_GiveNamedItem_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDGiveNamedItem);
			::DoD_GiveNamedItem = (::DoD_GiveNamedItem_Type) ::subhook_get_trampoline(::g_pDoDGiveNamedItem);
		}
		else
		{
			::MF_Log("::DoD_GiveNamedItem signature not found!");
		}
	}

	if (::g_Sigs[::SIG_DROPPLAYERITEM].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_DROPPLAYERITEM].Symbol.c_str()))
		{
			::g_pDoDDropPlayerItem = ::subhook_new(pAddr, ::DoD_DropPlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDDropPlayerItem);
			::DoD_DropPlayerItem = (::DoD_DropPlayerItem_Type) ::subhook_get_trampoline(::g_pDoDDropPlayerItem);
		}
		else
		{
			::MF_Log("::DoD_DropPlayerItem symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_DROPPLAYERITEM].Signature, &Addr, true))
		{
			::g_pDoDDropPlayerItem = ::subhook_new((void*)Addr, ::DoD_DropPlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDDropPlayerItem);
			::DoD_DropPlayerItem = (::DoD_DropPlayerItem_Type) ::subhook_get_trampoline(::g_pDoDDropPlayerItem);
		}
		else
		{
			::MF_Log("::DoD_DropPlayerItem signature not found!");
		}
	}

	if (::g_Sigs[::SIG_GIVEAMMO].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_GIVEAMMO].Symbol.c_str()))
		{
			::g_pDoDGiveAmmo = ::subhook_new(pAddr, ::DoD_GiveAmmo_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDGiveAmmo);
			::DoD_GiveAmmo = (::DoD_GiveAmmo_Type) ::subhook_get_trampoline(::g_pDoDGiveAmmo);
		}
		else
		{
			::MF_Log("::DoD_GiveAmmo symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_GIVEAMMO].Signature, &Addr, true))
		{
			::g_pDoDGiveAmmo = ::subhook_new((void*)Addr, ::DoD_GiveAmmo_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDGiveAmmo);
			::DoD_GiveAmmo = (::DoD_GiveAmmo_Type) ::subhook_get_trampoline(::g_pDoDGiveAmmo);
		}
		else
		{
			::MF_Log("::DoD_GiveAmmo signature not found!");
		}
	}

	if (::g_Sigs[::SIG_SETWAVETIME].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_SETWAVETIME].Symbol.c_str()))
		{
			::g_pDoDSetWaveTime = ::subhook_new(pAddr, ::DoD_SetWaveTime_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDSetWaveTime);
			::DoD_SetWaveTime = (::DoD_SetWaveTime_Type) ::subhook_get_trampoline(::g_pDoDSetWaveTime);
		}
		else
		{
			::MF_Log("::DoD_SetWaveTime symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_SETWAVETIME].Signature, &Addr, true))
		{
			::g_pDoDSetWaveTime = ::subhook_new((void*)Addr, ::DoD_SetWaveTime_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDSetWaveTime);
			::DoD_SetWaveTime = (::DoD_SetWaveTime_Type) ::subhook_get_trampoline(::g_pDoDSetWaveTime);
		}
		else
		{
			::MF_Log("::DoD_SetWaveTime signature not found!");
		}
	}

	if (::g_Sigs[::SIG_GETWAVETIME].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_GETWAVETIME].Symbol.c_str()))
		{
			::g_pDoDGetWaveTime = ::subhook_new(pAddr, ::DoD_GetWaveTime_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDGetWaveTime);
			::DoD_GetWaveTime = (::DoD_GetWaveTime_Type) ::subhook_get_trampoline(::g_pDoDGetWaveTime);
		}
		else
		{
			::MF_Log("::DoD_GetWaveTime symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_GETWAVETIME].Signature, &Addr, true))
		{
			::g_pDoDGetWaveTime = ::subhook_new((void*)Addr, ::DoD_GetWaveTime_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDGetWaveTime);
			::DoD_GetWaveTime = (::DoD_GetWaveTime_Type) ::subhook_get_trampoline(::g_pDoDGetWaveTime);
		}
		else
		{
			::MF_Log("::DoD_GetWaveTime signature not found!");
		}
	}

	if (::g_Sigs[::SIG_REMOVEPLAYERITEM].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_REMOVEPLAYERITEM].Symbol.c_str()))
		{
			::g_pDoDRemovePlayerItem = ::subhook_new(pAddr, ::DoD_RemovePlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDRemovePlayerItem);
			::DoD_RemovePlayerItem = (::DoD_RemovePlayerItem_Type) ::subhook_get_trampoline(::g_pDoDRemovePlayerItem);
		}
		else
		{
			::MF_Log("::DoD_RemovePlayerItem symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_REMOVEPLAYERITEM].Signature, &Addr, true))
		{
			::g_pDoDRemovePlayerItem = ::subhook_new((void*)Addr, ::DoD_RemovePlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDRemovePlayerItem);
			::DoD_RemovePlayerItem = (::DoD_RemovePlayerItem_Type) ::subhook_get_trampoline(::g_pDoDRemovePlayerItem);
		}
		else
		{
			::MF_Log("::DoD_RemovePlayerItem signature not found!");
		}
	}

	if (::g_Sigs[::SIG_ADDPLAYERITEM].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_ADDPLAYERITEM].Symbol.c_str()))
		{
			::g_pDoDAddPlayerItem = ::subhook_new(pAddr, ::DoD_AddPlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDAddPlayerItem);
			::DoD_AddPlayerItem = (::DoD_AddPlayerItem_Type) ::subhook_get_trampoline(::g_pDoDAddPlayerItem);
		}
		else
		{
			::MF_Log("::DoD_AddPlayerItem symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_ADDPLAYERITEM].Signature, &Addr, true))
		{
			::g_pDoDAddPlayerItem = ::subhook_new((void*)Addr, ::DoD_AddPlayerItem_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDAddPlayerItem);
			::DoD_AddPlayerItem = (::DoD_AddPlayerItem_Type) ::subhook_get_trampoline(::g_pDoDAddPlayerItem);
		}
		else
		{
			::MF_Log("::DoD_AddPlayerItem signature not found!");
		}
	}

	if (::g_Sigs[::SIG_REMOVEALLITEMS].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_REMOVEALLITEMS].Symbol.c_str()))
		{
			::g_pDoDRemoveAllItems = ::subhook_new(pAddr, ::DoD_RemoveAllItems_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDRemoveAllItems);
			::DoD_RemoveAllItems = (::DoD_RemoveAllItems_Type) ::subhook_get_trampoline(::g_pDoDRemoveAllItems);
		}
		else
		{
			::MF_Log("::DoD_RemoveAllItems symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_REMOVEALLITEMS].Signature, &Addr, true))
		{
			::g_pDoDRemoveAllItems = ::subhook_new((void*)Addr, ::DoD_RemoveAllItems_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDRemoveAllItems);
			::DoD_RemoveAllItems = (::DoD_RemoveAllItems_Type) ::subhook_get_trampoline(::g_pDoDRemoveAllItems);
		}
		else
		{
			::MF_Log("::DoD_RemoveAllItems signature not found!");
		}
	}

	if (::g_Sigs[::SIG_SETBODYGROUP].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_SETBODYGROUP].Symbol.c_str()))
		{
			::g_pDoDSetBodygroup = ::subhook_new(pAddr, ::DoD_SetBodygroup_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDSetBodygroup);
			::DoD_SetBodygroup = (::DoD_SetBodygroup_Type) ::subhook_get_trampoline(::g_pDoDSetBodygroup);
		}
		else
		{
			::MF_Log("::DoD_SetBodygroup symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_SETBODYGROUP].Signature, &Addr, true))
		{
			::g_pDoDSetBodygroup = ::subhook_new((void*)Addr, ::DoD_SetBodygroup_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDSetBodygroup);
			::DoD_SetBodygroup = (::DoD_SetBodygroup_Type) ::subhook_get_trampoline(::g_pDoDSetBodygroup);
		}
		else
		{
			::MF_Log("::DoD_SetBodygroup signature not found!");
		}
	}

	if (::g_Sigs[::SIG_INSTALLGAMERULES].IsSymbol)
	{
		if (pAddr = ::dlsym(pDoD, ::g_Sigs[::SIG_INSTALLGAMERULES].Symbol.c_str()))
		{
			::g_pDoDInstallGameRules = ::subhook_new(pAddr, ::DoD_InstallGameRules_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDInstallGameRules);
			::DoD_InstallGameRules = (::DoD_InstallGameRules_Type) ::subhook_get_trampoline(::g_pDoDInstallGameRules);
		}
		else
		{
			::MF_Log("::DoD_InstallGameRules symbol not found!");
		}
	}
	else
	{
		if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_INSTALLGAMERULES].Signature, &Addr, true))
		{
			::g_pDoDInstallGameRules = ::subhook_new((void*)Addr, ::DoD_InstallGameRules_Hook, ::SUBHOOK_TRAMPOLINE);
			::subhook_install(::g_pDoDInstallGameRules);
			::DoD_InstallGameRules = (::DoD_InstallGameRules_Type) ::subhook_get_trampoline(::g_pDoDInstallGameRules);
		}
		else
		{
			::MF_Log("::DoD_InstallGameRules signature not found!");
		}
	}

	if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_PATCHFG42].Signature, &Addr, true))
	{
		::g_pAutoScopeFG42Addr = (unsigned char*)Addr;
	}
	else
	{
		::MF_Log("::DoD_PatchAutoScope(FG42) signature not found!");
	}

	if (::findInMemory((unsigned char*)memInfo.dli_fbase, memData.st_size, ::g_Sigs[::SIG_PATCHENFIELD].Signature, &Addr, true))
	{
		::g_pAutoScopeEnfieldAddr = (unsigned char*)Addr;
	}
	else
	{
		::MF_Log("::DoD_PatchAutoScope(Enfield) signature not found!");
	}

	::dlclose(pDoD);
#endif

	::g_fn_AddNatives(::DoDHacks_Natives);
}

void OnAmxxDetach()
{
#ifndef __linux__
	if (::DoD_PlayerSpawn)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_PlayerSpawn, ::DoD_PlayerSpawn_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_GiveNamedItem)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_GiveNamedItem, ::DoD_GiveNamedItem_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_DropPlayerItem)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_DropPlayerItem, ::DoD_DropPlayerItem_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_RemovePlayerItem)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_RemovePlayerItem, ::DoD_RemovePlayerItem_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_AddPlayerItem)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_AddPlayerItem, ::DoD_AddPlayerItem_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_RemoveAllItems)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_RemoveAllItems, ::DoD_RemoveAllItems_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_SetWaveTime)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_SetWaveTime, ::DoD_SetWaveTime_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_GetWaveTime)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_GetWaveTime, ::DoD_GetWaveTime_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_GiveAmmo)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_GiveAmmo, ::DoD_GiveAmmo_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_SetBodygroup)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_SetBodygroup, ::DoD_SetBodygroup_Hook);
		::DetourTransactionCommit();
	}
	if (::DoD_InstallGameRules)
	{
		::DetourTransactionBegin();
		::DetourUpdateThread(::GetCurrentThread());
		::DetourDetach(&(void*&) ::DoD_InstallGameRules, ::DoD_InstallGameRules_Hook);
		::DetourTransactionCommit();
	}
#else
	if (::DoD_PlayerSpawn)
	{
		::subhook_remove(::g_pDoDPlayerSpawn);
		::subhook_free(::g_pDoDPlayerSpawn);
	}
	if (::DoD_GiveNamedItem)
	{
		::subhook_remove(::g_pDoDGiveNamedItem);
		::subhook_free(::g_pDoDGiveNamedItem);
	}
	if (::DoD_DropPlayerItem)
	{
		::subhook_remove(::g_pDoDDropPlayerItem);
		::subhook_free(::g_pDoDDropPlayerItem);
	}
	if (::DoD_RemovePlayerItem)
	{
		::subhook_remove(::g_pDoDRemovePlayerItem);
		::subhook_free(::g_pDoDRemovePlayerItem);
	}
	if (::DoD_AddPlayerItem)
	{
		::subhook_remove(::g_pDoDAddPlayerItem);
		::subhook_free(::g_pDoDAddPlayerItem);
	}
	if (::DoD_RemoveAllItems)
	{
		::subhook_remove(::g_pDoDRemoveAllItems);
		::subhook_free(::g_pDoDRemoveAllItems);
	}
	if (::DoD_SetWaveTime)
	{
		::subhook_remove(::g_pDoDSetWaveTime);
		::subhook_free(::g_pDoDSetWaveTime);
	}
	if (::DoD_GetWaveTime)
	{
		::subhook_remove(::g_pDoDGetWaveTime);
		::subhook_free(::g_pDoDGetWaveTime);
	}
	if (::DoD_GiveAmmo)
	{
		::subhook_remove(::g_pDoDGiveAmmo);
		::subhook_free(::g_pDoDGiveAmmo);
	}
	if (::DoD_SetBodygroup)
	{
		::subhook_remove(::g_pDoDSetBodygroup);
		::subhook_free(::g_pDoDSetBodygroup);
	}
	if (::DoD_InstallGameRules)
	{
		::subhook_remove(::g_pDoDInstallGameRules);
		::subhook_free(::g_pDoDInstallGameRules);
	}
#endif

	::g_Sigs.clear();
}

void OnPluginsLoaded()
{
	::g_fwPlayerSpawn = ::g_fn_RegisterForward("DoD_OnPlayerSpawn", ::ET_STOP, ::FP_CELL, ::FP_CELL, ::FP_DONE);
	::g_fwGiveNamedItem = ::g_fn_RegisterForward("DoD_OnGiveNamedItem", ::ET_STOP, ::FP_CELL, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_DONE);
	::g_fwDropPlayerItem = ::g_fn_RegisterForward("DoD_OnDropPlayerItem", ::ET_STOP, ::FP_CELL, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
	::g_fwAddPlayerItem = ::g_fn_RegisterForward("DoD_OnAddPlayerItem", ::ET_STOP, ::FP_CELL, ::FP_CELL, ::FP_DONE);
	::g_fwRemovePlayerItem = ::g_fn_RegisterForward("DoD_OnRemovePlayerItem", ::ET_STOP, ::FP_CELL, ::FP_CELL, ::FP_DONE);
	::g_fwRemoveAllItems = ::g_fn_RegisterForward("DoD_OnRemoveAllItems", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
	::g_fwGiveAmmo = ::g_fn_RegisterForward("DoD_OnGiveAmmo", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_STRINGEX /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
	::g_fwGetWaveTime = ::g_fn_RegisterForward("DoD_OnGetWaveTime", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_FLOAT_BYREF /** can be altered during exec */, ::FP_DONE);
	::g_fwSetWaveTime = ::g_fn_RegisterForward("DoD_OnSetWaveTime", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_FLOAT_BYREF /** can be altered during exec */, ::FP_DONE);
	::g_fwSetBodygroup = ::g_fn_RegisterForward("DoD_OnSetBodygroup", ::ET_STOP, ::FP_CELL, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_CELL_BYREF /** can be altered during exec */, ::FP_DONE);
	::g_fwInstallGameRules = ::g_fn_RegisterForward("DoD_OnInstallGameRules", ::ET_STOP, ::FP_DONE);

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
}

int AmxxCheckGame(const char* pGame)
{
	if (false == ::_stricmp("DoD", pGame))
	{
		return false; /// OK
	}
	return true; /// ERROR
}
