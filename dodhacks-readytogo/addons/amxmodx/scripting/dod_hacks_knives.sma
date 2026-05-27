
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <dodconst>
#include <dodhacks>

#define Linux_Difference_Offset_Weapons        4             /// +4 only on Linux. For 'get_pdata_*' funcs. Weapon entities only.
#define PV_CWeaponBox_m_rgpPlayerItems        82             /// For 'get_pdata_cbase'. '::CBasePlayerItem* ::CWeaponBox::m_rgpPlayerItems[6]' var.
#define INT_CBasePlayer_m_iClientHealth      267             /// 'int ::CBasePlayer::m_iClientHealth' variable.
#define PV_CBasePlayer_m_rgpPlayerItems      272             /// '::CBasePlayerItem* ::CBasePlayer::m_rgpPlayerItems[6]' variable.
#define PV_CBasePlayer_m_pActiveItem         278             /// '::CBasePlayerItem* ::CBasePlayer::m_pActiveItem' variable.
#define PV_CBasePlayer_m_pLastItem           280             /// '::CBasePlayerItem* ::CBasePlayer::m_pLastItem' variable.
#define ThrowingKnives_UserIdTaskOffset  1081343             /// 32767+2^20. Some offset other plugins don't use.
#define ThrowingKnives_RemoveTaskOffset 33619967             /// 65535+2^25.
#define ThrowingKnives_EntityFlagValue  16744448             /// -32768+2^24. This is the thrown knife weaponbox.
#define ThrowingKnives_EntityKeyForFlag pev_flSwimTime       /// Something unrelated to a "weaponbox" entity.
#define ThrowingKnives_EntityKeyForTime pev_pain_finished    /// get_gametime() at the moment of throwing knife launch/ throw time.
#define WeaponSpade_WorldModelFilePath  "models/w_spade.mdl" /// Spade requires pre-caching and re-modeling (world model only).

#if !defined SVC_INTERMISSION
    #define  SVC_INTERMISSION           30 /** Scores table is being displayed to players now (before map change). */
#endif

new g_msgBlood;
new g_msgHealth;
new g_maxPlayers;
new g_knifeSpeed;
new g_playerTeamIndex[33] = { 0, ... };
new g_playerUniqueUserIndex[33] = { -1, ... };
new bool: g_teamAttack;
new bool: g_infiniteAttacks;
new bool: g_isBetweenRounds = true;
new bool: g_infiniteAttempts;
new bool: g_shouldAlterCreateCall = false;
new bool: g_canPlayerThrow[33];
new bool: g_isPlayerInServer[33];
new bool: g_isPlayerAwaiting[33];
new Float: g_damageKnife;
new Float: g_damageKnifeLow;
new Float: g_knifeVerPosOffs;
new Float: g_playerSpawnTime[33];
new Float: g_pitchAngleOffs_Spade;
new Float: g_pitchAngleOffs_GerKnife;
new Float: g_pitchAngleOffs_AmerKnife;
new DoD_Address: g_wpnBoxKill = DoD_Address: DoD_Address_Null;

public plugin_init()
{
    register_plugin("DoD Hacks: Knives", "1.0.0.5", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_knives.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        set_fail_state("Error opening '%s'!", Buffer);
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[32];
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key, "@speed"))
            g_knifeSpeed = str_to_num(Val);
        else if (equali(Key, "@offs"))
            g_knifeVerPosOffs = str_to_float(Val);
        else if (equali(Key, "@spade_x_angle_offs"))
            g_pitchAngleOffs_Spade = str_to_float(Val);
        else if (equali(Key, "@gerknife_x_angle_offs"))
            g_pitchAngleOffs_GerKnife = str_to_float(Val);
        else if (equali(Key, "@amerknife_x_angle_offs"))
            g_pitchAngleOffs_AmerKnife = str_to_float(Val);
        else if (equali(Key, "@allow_team_attack"))
            g_teamAttack = bool: str_to_num(Val);
        else if (equali(Key, "@damage_knife"))
            g_damageKnife = str_to_float(Val);
        else if (equali(Key, "@damage_knife_low"))
            g_damageKnifeLow = str_to_float(Val);
        else if (equali(Key, "@infinite_attacks"))
            g_infiniteAttacks = bool: str_to_num(Val);
        else if (equali(Key, "@infinite_attempts"))
            g_infiniteAttempts = bool: str_to_num(Val);
    }
    fclose(Config);

#if defined amxclient_cmd && defined RegisterHamPlayer
    RegisterHamPlayer(Ham_Killed, "OnPlayer_Killed_Pre"       );
    RegisterHamPlayer(Ham_Killed, "OnPlayer_Killed_Post", true);
    RegisterHamPlayer(Ham_Touch,  "OnPlayer_Touch_Pre"        );
#else
    RegisterHam(Ham_Killed, "player", "OnPlayer_Killed_Pre"       );
    RegisterHam(Ham_Killed, "player", "OnPlayer_Killed_Post", true);
    RegisterHam(Ham_Touch,  "player", "OnPlayer_Touch_Pre"        );
#endif
    RegisterHam(Ham_Touch, "weaponbox", "OnWeaponBox_Touch_Pre");
    RegisterHam(Ham_Think, "weaponbox", "OnWeaponBox_Think_Pre");
    RegisterHam(Ham_Item_PreFrame, "weapon_spade",     "OnSpade_ItemPreFrame_Pre"    );
    RegisterHam(Ham_Item_PreFrame, "weapon_gerknife",  "OnGerKnife_ItemPreFrame_Pre" );
    RegisterHam(Ham_Item_PreFrame, "weapon_amerknife", "OnAmerKnife_ItemPreFrame_Pre");
    register_forward(FM_SetModel,     "OnSetModel_Pre"           );
    register_forward(FM_RemoveEntity, "OnRemoveEntity_Post", true);
    register_event("RoundState", "OnRoundBeginFreezeEnd", "a", "1=1"       );
    register_event("RoundState", "OnRoundEnd",            "a", "1=3", "1=4");
    register_message(SVC_INTERMISSION /** 30 */, "OnMsgIntermission");
    DoD_HookShouldCollide();
    g_msgBlood =   get_user_msgid("BloodPuff");
    g_msgHealth =  get_user_msgid("Health");
    g_maxPlayers = get_maxplayers();
    g_wpnBoxKill = DoD_GetFunctionAddress(DoD_FI_WpnBoxKill, false);
    return PLUGIN_CONTINUE;
}

public OnPlayer_Killed_Pre(Player)
{ /// Workaround for player dying with mangled 'pev_weapons'.
    static Class[32], taskIndex, Item, Iter, Ofs;
    taskIndex = g_playerUniqueUserIndex[Player] + ThrowingKnives_UserIdTaskOffset;
    if (task_exists(taskIndex))
        remove_task(taskIndex);
    Item = get_pdata_cbase(Player, PV_CBasePlayer_m_pActiveItem);
    if (Item > g_maxPlayers) /// Item is valid.
    { /// Drop held armed explo. grenade if needed and holster the current weapon.
        ExecuteHamB(Ham_DOD_Item_DropGren, Item       );
        ExecuteHamB(Ham_Item_Holster,      Item, false);
    }
    for (Iter = 0; Iter < 6; Iter++)
    { /// Drop/ delete all owned weapons.
        Ofs = Iter + PV_CBasePlayer_m_rgpPlayerItems;
        Item = get_pdata_cbase(Player, Ofs);
        if (Item > g_maxPlayers)
        { /// If valid item.
            if (pev(Item, pev_classname, Class, charsmax(Class)) > 0 && DoD_IsWeaponPrimary(Class))
                DoD_DropPlayerItem(Player, Class, true);
            else /// If weapon can't be dropped, gets erased.
                DoD_DestroyItem(Item);
            set_pdata_cbase(Player, Ofs, -1);
        }
    }
    set_pdata_cbase(Player, PV_CBasePlayer_m_pActiveItem, -1);
    set_pdata_cbase(Player, PV_CBasePlayer_m_pLastItem,   -1);
    return HAM_IGNORED;
}

public OnRoundEnd()
    g_isBetweenRounds = true;

public OnRoundBeginFreezeEnd()
    g_isBetweenRounds = false;

public OnMsgIntermission()
    g_isBetweenRounds = true;

public plugin_natives()
    register_native("DoD_IsUserWaitingThrowingKnife", "Func_IsUserWaitingThrowingKnife");

public Func_IsUserWaitingThrowingKnife()
{
    static Player;
    Player = get_param(1);
    return isPlayerEntity(Player) ? g_isPlayerAwaiting[Player] : false;
}

public OnPlayer_Touch_Pre(Player, Other)
{
    static Class[32];
    return g_isPlayerAwaiting[Player] && pev(Other, pev_classname, Class, charsmax(Class)) > 0 &&
        equali(Class, "weapon", 6) && !DoD_IsWeaponKnife(Class) ? HAM_SUPERCEDE : HAM_IGNORED;
}

public plugin_precache()
    precache_model(WeaponSpade_WorldModelFilePath);

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
{
    g_isPlayerInServer[Player] = true;
    g_playerUniqueUserIndex[Player] = get_user_userid(Player);
}

public DOD_ON_PLAYER_DISCONNECTED
{
    static taskIndex;
    taskIndex = g_playerUniqueUserIndex[Player] + ThrowingKnives_UserIdTaskOffset;
    if (task_exists(taskIndex))
        remove_task(taskIndex);
    zeroPlayerVars(Player);
}

public OnPlayer_Killed_Post(Player)
{
    static Iter;
    g_canPlayerThrow[Player] = false;
    g_isPlayerAwaiting[Player] = false;
    for (Iter = 0; Iter < 32; Iter++)
        if (get_pdata_int(Player, 281 + Iter) > 0)
            server_print("!!! Player ammo on death > 0 @ cell %d!", Iter);
}

public OnRemoveEntity_Post(Entity)
    DoD_UnblockFromPlayerCollision(Entity);

public DoD_OnSubRemove_Post(Entity)
    DoD_UnblockFromPlayerCollision(Entity);

public DoD_OnUtilRemove_Post(Entity)
    DoD_UnblockFromPlayerCollision(Entity);

public OnSpade_ItemPreFrame_Pre(Item)
{
    static Owner;
    Owner = pev(Item, pev_owner);
    if (!g_isBetweenRounds && isPlayerEntity(Owner) && g_canPlayerThrow[Owner] && !g_isPlayerAwaiting[Owner] &&
        is_user_alive(Owner) && (pev(Owner, pev_button) & IN_ATTACK2) && pev(Owner, pev_waterlevel) < 3)
    {
        g_shouldAlterCreateCall = true;
        DoD_DropPlayerItem(Owner, "weapon_spade", true);
    }
}

public OnAmerKnife_ItemPreFrame_Pre(Item)
{
    static Owner;
    Owner = pev(Item, pev_owner);
    if (!g_isBetweenRounds && isPlayerEntity(Owner) && g_canPlayerThrow[Owner] && !g_isPlayerAwaiting[Owner] &&
        is_user_alive(Owner) && (pev(Owner, pev_button) & IN_ATTACK2) && pev(Owner, pev_waterlevel) < 3)
    {
        g_shouldAlterCreateCall = true;
        DoD_DropPlayerItem(Owner, "weapon_amerknife", true);
    }
}

public OnGerKnife_ItemPreFrame_Pre(Item)
{
    static Owner;
    Owner = pev(Item, pev_owner);
    if (!g_isBetweenRounds && isPlayerEntity(Owner) && g_canPlayerThrow[Owner] && !g_isPlayerAwaiting[Owner] &&
        is_user_alive(Owner) && (pev(Owner, pev_button) & IN_ATTACK2) && pev(Owner, pev_waterlevel) < 3)
    {
        g_shouldAlterCreateCall = true;
        DoD_DropPlayerItem(Owner, "weapon_gerknife", true);
    }
}

public DoD_OnCreate(Name[], nameSize /** = 64 */, Float: Origin[3], Float: Angles[3], &Owner, &overrideRes)
    if (g_shouldAlterCreateCall)
    {
        g_shouldAlterCreateCall = false;
        Origin[2] += g_knifeVerPosOffs;
    }

public OnWeaponBox_Think_Pre(Entity)
{ /// Removes a thrown weaponbox entity if needed.
    static Owner;
    if (ThrowingKnives_EntityFlagValue != pev(Entity, ThrowingKnives_EntityKeyForFlag))
        return HAM_IGNORED; /// If this weaponbox is not a thrown weaponbox, skip.
    Owner = pev(Entity, pev_owner);
    if (!isPlayerEntity(Owner) || !g_isPlayerInServer[Owner]) /// Gets removed right away if it has no owner.
    { /// Insta-removing during Think() should work just fine.
        killWeaponBox(Entity);
        return HAM_SUPERCEDE;
    }
    return HAM_IGNORED; /// Keep thinking, if valid owner.
}

public OnWeaponBox_Touch_Pre(weaponBox, Entity)
{
    static Owner, Class[16], Float: Origin[3], Float: takeDamage, Float: playerOrigin[3], Float: Time;
    if (weaponBox == Entity || ThrowingKnives_EntityFlagValue != pev(weaponBox, ThrowingKnives_EntityKeyForFlag))
        return HAM_IGNORED;
    Owner = pev(weaponBox, pev_owner);
    if (g_isBetweenRounds || !isPlayerEntity(Owner) || !g_isPlayerInServer[Owner])
    { /// Insta-removing during Touch() should work just fine.
        killWeaponBox(weaponBox);
        return HAM_SUPERCEDE;
    }
    else if (Entity == Owner)
        return HAM_SUPERCEDE; /// Knife owner (dead or alive) doesn't touch or collide with the knife.
    else if (!isPlayerEntity(Entity))
    {
        if (pev(weaponBox, pev_waterlevel) < 2)
        {
            pev(weaponBox, pev_origin, Origin);
            makeSparks(Origin);
            emit_sound(weaponBox, CHAN_WEAPON, 1 == random_num(1, 2) ?
                "weapons/cbar_hit1.wav" : "weapons/cbar_hit2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
        }
        else
            emit_sound(weaponBox, CHAN_WEAPON, 1 == random_num(1, 2) ?
                "weapons/hit_water1.wav" : "weapons/hit_water2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
        if (Entity > g_maxPlayers && pev(Entity, pev_classname, Class, charsmax(Class)) > 0 &&
            equali(Class, "func_breakable") && pev(Entity, pev_takedamage, takeDamage) > 0 && takeDamage)
            ExecuteHamB(Ham_TakeDamage, Entity, Owner, Owner, g_damageKnife, DMG_SLASH);
        if (false == g_infiniteAttempts)
        {
            pev(weaponBox, ThrowingKnives_EntityKeyForTime, Time);
            if (Time > g_playerSpawnTime[Owner])
                g_canPlayerThrow[Owner] = false;
        }
        killWeaponBox(weaponBox);
        g_isPlayerAwaiting[Owner] = false;
        return HAM_SUPERCEDE;
    }
    else if (g_playerTeamIndex[Owner] == g_playerTeamIndex[Entity])
    {
        if (g_teamAttack)
            goto handlePlayerAttack;
        if (g_isPlayerInServer[Entity] && is_user_alive(Entity))
        {
            pev(Entity, pev_takedamage, takeDamage);
            if (takeDamage)
                emit_sound(weaponBox, CHAN_WEAPON, 1 == random_num(1, 2) ?
                    "weapons/hit_grass1.wav" : "weapons/hit_grass2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
            else
            {
                pev(weaponBox, pev_origin, Origin);
                makeSparks(Origin);
                emit_sound(weaponBox, CHAN_WEAPON, 1 == random_num(1, 2) ?
                    "weapons/cbar_hit1.wav" : "weapons/cbar_hit2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
            }
        }
        if (false == g_infiniteAttempts)
        {
            pev(weaponBox, ThrowingKnives_EntityKeyForTime, Time);
            if (Time > g_playerSpawnTime[Owner])
                g_canPlayerThrow[Owner] = false;
        }
        killWeaponBox(weaponBox);
        g_isPlayerAwaiting[Owner] = false;
        return HAM_SUPERCEDE;
    }
    else
    {
handlePlayerAttack:
        if (g_isPlayerInServer[Entity] && is_user_alive(Entity))
        {
            pev(Entity, pev_takedamage, takeDamage);
            pev(weaponBox, pev_origin, Origin);
            if (takeDamage)
            {
                emit_sound(weaponBox, CHAN_WEAPON, 1 == random_num(1, 2) ?
                    "weapons/hit_grass1.wav" : "weapons/hit_grass2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
                makePlayerBlood(Origin);
                pev(Entity, pev_origin, playerOrigin);
                if (false == bool: pev(Entity, pev_iuser3)) /// Non-prone (0).
                    takeDamage = Origin[2] >= playerOrigin[2] ? g_damageKnife : g_damageKnifeLow;
                else /// Prone.
                    takeDamage = g_damageKnife;
                DoD_TraceLine(Origin, playerOrigin, false, Owner); /// To further inform DoDX (XStats), for proper client_damage and client_death calls.
                ExecuteHamB(Ham_TakeDamage, Entity, Owner, Owner, takeDamage, DMG_SLASH);
                sendHealthMsgNotifyingDoDXStats(Entity); /// Save kill as knife kill and update player rank.
                pev(weaponBox, ThrowingKnives_EntityKeyForTime, Time);
                killWeaponBox(weaponBox);
                if ((false == g_infiniteAttacks || false == g_infiniteAttempts) && Time > g_playerSpawnTime[Owner])
                    g_canPlayerThrow[Owner] = false;
            }
            else
            {
                makeSparks(Origin);
                emit_sound(weaponBox, CHAN_WEAPON, 1 == random_num(1, 2) ?
                    "weapons/cbar_hit1.wav" : "weapons/cbar_hit2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
                if (false == g_infiniteAttempts)
                {
                    pev(weaponBox, ThrowingKnives_EntityKeyForTime, Time);
                    if (Time > g_playerSpawnTime[Owner])
                        g_canPlayerThrow[Owner] = false;
                }
                killWeaponBox(weaponBox);
            }
            g_isPlayerAwaiting[Owner] = false;
            return HAM_SUPERCEDE;
        }
        if (false == g_infiniteAttempts)
        {
            pev(weaponBox, ThrowingKnives_EntityKeyForTime, Time);
            if (Time > g_playerSpawnTime[Owner])
                g_canPlayerThrow[Owner] = false;
        }
        killWeaponBox(weaponBox);
        g_isPlayerAwaiting[Owner] = false;
        return HAM_SUPERCEDE;
    }
    /// All cases managed above.
}

public DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player)
{
    static taskIndex;
    if (g_isPlayerInServer[Player])
    {
        taskIndex = g_playerUniqueUserIndex[Player] + ThrowingKnives_UserIdTaskOffset;
        if (task_exists(taskIndex))
            remove_task(taskIndex);
        g_canPlayerThrow[Player] = true; /// Player may throw their knife.
        g_isPlayerAwaiting[Player] = false; /// Player can switch between guns.
        g_playerSpawnTime[Player] = get_gametime();
        g_playerTeamIndex[Player] = get_user_team(Player);
    }
}

public client_command(Player)
{ /// If player is waiting for the throwing knife to touch something, do not allow them switch between guns or drop items.
    static Cmd[8];
    return g_isPlayerAwaiting[Player] && read_argv(0, Cmd, charsmax(Cmd)) > 0 &&
        (equali(Cmd, "drop") || equali(Cmd, "lastinv") || equali(Cmd, "weapon_")) ?
        PLUGIN_HANDLED : PLUGIN_CONTINUE;
}

public DoD_OnDropPlayerItem(Player, Item[], ItemSize /** = 64 */, &bool: Force)
    return g_isPlayerAwaiting[Player] ? PLUGIN_HANDLED : PLUGIN_CONTINUE; /// If player can't switch between guns, they can't drop either.

public Task_RestorePlayerItems(Buffer[1], taskIndex)
{ /// Let the player switch between guns again.
    static Player;
#if defined FindPlayerFlags
    Player = find_player_ex(FindPlayer_MatchUserId, taskIndex - ThrowingKnives_UserIdTaskOffset);
#else
    Player = readPlayerFromUniqueUserIndex(taskIndex - ThrowingKnives_UserIdTaskOffset);
#endif
    if (!g_isPlayerInServer[Player] || !is_user_alive(Player))
        return PLUGIN_HANDLED; /// Can't restore anything.
    if (g_isPlayerAwaiting[Player])
    { /// Re-try.
        set_task(0.000001, "Task_RestorePlayerItems", taskIndex, Buffer, sizeof Buffer);
        return PLUGIN_HANDLED;
    } /// Player now able to switch between guns.
    set_pev(Player, pev_weapons, pev(Player, pev_weapons) | Buffer[0]);
    return PLUGIN_HANDLED;
}

public OnSetModel_Pre(Entity)
{ /// Buffer is automatically freed by AMXX once task ends.
    static Class[32], Weapon, Float: Angles[3], Float: Velocity[3], Owner, Res, Buffer[1];
    Res = FMRES_IGNORED; /// Ignore by default.
    if (pev(Entity, pev_classname, Class, charsmax(Class)) > 0 && equali("weaponbox", Class))
    { /// Only interested in "weaponbox" entities.
        Weapon = get_pdata_cbase(Entity, PV_CWeaponBox_m_rgpPlayerItems, Linux_Difference_Offset_Weapons);
        if (Weapon > g_maxPlayers && pev(Weapon, pev_classname, Class, charsmax(Class)) > 0 && DoD_IsWeaponKnife(Class))
        { /// Only interested in knife-related "weaponbox" entities.
            Owner = pev(Entity, pev_owner);
            if (isPlayerEntity(Owner))
            { /// Only interested in knife-related "weaponbox" entities that have a valid owner.
                if (!g_isBetweenRounds && g_canPlayerThrow[Owner] && !g_isPlayerAwaiting[Owner] && is_user_alive(Owner))
                { /// If the owner is in server and alive, throw the knife.
                    g_isPlayerAwaiting[Owner] = true; /// Player can't pick up items now.
                    set_pev(Entity, ThrowingKnives_EntityKeyForTime, get_gametime());
                    Buffer[0] = pev(Owner, pev_weapons); /// Store guns and grenades for later.
                    set_pev(Owner, pev_weapons, 0); /// Player can't switch between guns at the moment.
                    if ('s' == Class[7] || 'S' == Class[7])
                        DoD_GiveNamedItem(Owner, "weapon_spade", Weapon);
                    else if ('a' == Class[7] || 'A' == Class[7])
                        DoD_GiveNamedItem(Owner, "weapon_amerknife", Weapon);
                    else
                        DoD_GiveNamedItem(Owner, "weapon_gerknife", Weapon);
                    engclient_cmd(Owner, Class); /// Really required.
                    set_task(0.000001, "Task_RestorePlayerItems",
                        g_playerUniqueUserIndex[Owner] + ThrowingKnives_UserIdTaskOffset, Buffer, sizeof Buffer);
                    velocity_by_aim(Owner, g_knifeSpeed, Velocity);
                    vector_to_angle(Velocity, Angles);
                    if ('s' == Class[7] || 'S' == Class[7])
                    {
                        Angles[0] += g_pitchAngleOffs_Spade;
                        engfunc(EngFunc_SetModel, Entity, WeaponSpade_WorldModelFilePath);
                        Res = FMRES_SUPERCEDE;
                    }
                    else if ('a' == Class[7] || 'A' == Class[7])
                        Angles[0] += g_pitchAngleOffs_AmerKnife;
                    else
                        Angles[0] += g_pitchAngleOffs_GerKnife;
                    Angles[2] += 90.0; /// Really required.
                    DoD_BlockToPlayerCollision(Entity);
                    emit_sound(Owner, CHAN_WEAPON, 1 == random_num(1, 2) ?
                        "weapons/knifeswing.wav" : "weapons/knifeswing2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
                    set_pev(Entity, pev_angles, Angles);
                    set_pev(Entity, pev_movetype, MOVETYPE_TOSS);
                    set_pev(Entity, ThrowingKnives_EntityKeyForFlag, ThrowingKnives_EntityFlagValue);
                    set_pev(Entity, pev_solid, SOLID_BBOX);
                    set_pev(Entity, pev_velocity, Velocity);
                    set_pev(Entity, pev_mins, Float: { -0.000001, -0.000001, -0.000001 });
                    set_pev(Entity, pev_maxs, Float: {  0.000001,  0.000001,  0.000001 });
                    engfunc(EngFunc_SetSize, Entity,
                        Float: { -0.000001, -0.000001, -0.000001 }, Float: { 0.000001, 0.000001, 0.000001 });
                }
                else /// If the owner is not in server or is dead, safely erase the weaponbox entity.
                {
                    if (safeEraseWeaponBox(Entity))
                        Res = FMRES_SUPERCEDE; /// Block setting a model if successfully erased (less likely).
                    else if ('s' == Class[7] || 'S' == Class[7]) /// At this time, a task to remove the weaponbox is set.
                    { /// Before getting destroyed, wear a proper model.
                        engfunc(EngFunc_SetModel, Entity, WeaponSpade_WorldModelFilePath);
                        Res = FMRES_SUPERCEDE; /// Model already set.
                    }
                }
            }
        }
    }
    return Res;
}

public Task_RemoveWeaponBox_Delayed(taskIndex)
{ /// Erase weaponbox if possible or re-try indefinitely.
    static Class[16], Delta, Entity;
    Entity = taskIndex - ThrowingKnives_RemoveTaskOffset;
    if (pev_valid(Entity) < 1 || pev(Entity, pev_classname, Class, charsmax(Class)) < 1 || !equali("weaponbox", Class))
        return PLUGIN_HANDLED; /// Skip + stop re-trying.
    if (g_wpnBoxKill != DoD_GetEntityThinkFunc(Entity, Delta)) /// Entity not yet prepared to get destroyed.
        set_task(0.000001, "Task_RemoveWeaponBox_Delayed", Entity + ThrowingKnives_RemoveTaskOffset); /// Re-try.
    else
        killWeaponBox(Entity); /// Erase from map + stop re-trying.
    return PLUGIN_HANDLED;
}

bool: safeEraseWeaponBox(Entity)
{ /// Safely erase weaponbox from map ASAP.
    static Delta, taskIndex;
    if (g_wpnBoxKill == DoD_GetEntityThinkFunc(Entity, Delta))
    { /// Now, if possible.
        killWeaponBox(Entity);
        return true;
    } /// Later.
    taskIndex = Entity + ThrowingKnives_RemoveTaskOffset;
    if (false == bool: task_exists(taskIndex)) /// Allows only one task at a time.
        set_task(0.000001, "Task_RemoveWeaponBox_Delayed", taskIndex);
    return false;
}

makePlayerBlood(Float: Origin[3])
{ /// Using float variables.
    engfunc(EngFunc_MessageBegin, MSG_BROADCAST, g_msgBlood, Origin, -1 /** A null entity (an invalid entity index). */);
    writeOrigin(Origin);
    message_end();
}

makeSparks(Float: Origin[3])
{ /// Using float variables.
    engfunc(EngFunc_MessageBegin, MSG_BROADCAST, SVC_TEMPENTITY, Origin, -1 /** A null entity (an invalid entity index). */);
    write_byte(TE_SPARKS);
    writeOrigin(Origin);
    message_end();
}

writeOrigin(Float: Origin[3])
{ /// Using float variables.
    static Iter;
    for (Iter = 0; Iter < sizeof Origin; Iter++)
        engfunc(EngFunc_WriteCoord, Origin[Iter]);
}

bool: isPlayerEntity(Entity)
{
    if (Entity < 1)
        return false;
    return Entity <= g_maxPlayers;
}

zeroPlayerVars(Player)
{ /// Zero all player global variables.
    g_canPlayerThrow[Player] = false;
    g_isPlayerAwaiting[Player] = false;
    g_isPlayerInServer[Player] = false;
    g_playerTeamIndex[Player] = 0;
    g_playerUniqueUserIndex[Player] = -1;
    g_playerSpawnTime[Player] = 0.0;
}

#if !defined FindPlayerFlags
readPlayerFromUniqueUserIndex(playerUniqueUserIndex)
{ /// Older AMXX versions need something like this.
    static Player;
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (playerUniqueUserIndex == g_playerUniqueUserIndex[Player])
            return Player;
    return 0;
}
#endif

sendHealthMsgNotifyingDoDXStats(Player)
{ /// Allows XStats (DoDX) module see this custom damage/ kill.
    emessage_begin(MSG_ONE_UNRELIABLE, g_msgHealth, { 0, 0, 0 }, Player);
    ewrite_byte(get_pdata_int(Player, INT_CBasePlayer_m_iClientHealth));
    emessage_end();
}

killWeaponBox(Entity)
{
    DoD_WpnBoxKill(Entity);
    DoD_UnblockFromPlayerCollision(Entity);
}
