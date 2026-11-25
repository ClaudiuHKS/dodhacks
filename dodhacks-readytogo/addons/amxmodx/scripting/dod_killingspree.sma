
#pragma semicolon 1
#pragma ctrlchar  '\'

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <dodx>
#include <dodfun>
#include <dodhacks>
#tryinclude <dod_hacks_knives> /// Works without this too.

#if !defined _dod_hacks_knives_included /// Works without this too.
native bool: DoD_AllowAllPlayersThrowKnives();
#endif

enum ksStatus
{
    Idle = 0,
    Prep = 1,
    Done = 2,
};

const Float: g_killingSpreeLength = 1.9; /// misc/killingspree.wav length.

new g_ksMsg[256];
new g_allyWav[256];
new g_axisWav[256];
new Float: g_axisLength;
new Float: g_allyLength;
new Float: g_customTime;
new Float: g_Timer = 0.0;
new Float: g_axisTime = 0.0;
new Float: g_allyTime = 0.0;
new Float: g_winStamp = 0.0;
new ksStatus: g_Status = ksStatus: Idle;
new g_Team = 0;
new g_Life = 100;
new g_maxPlayers;
new g_syncMsg = -1;
new g_syncTmr = -1;
new g_tchHook = -1;
new g_sndHook = -1;
new g_stripEntity = -1;
new g_scoreEntity = -1;
new g_controlPointMasterEntity = -1;
new g_thrknValue = 16744448; /// Can be changed via .ini file (no need to change here).
new g_origWinSnd[256];
new g_msgEnts[16];
new g_msgTrgs[16][256];
new g_msgEntsNum = 0;
new bool: g_throwingKnivesCalls = true;
new bool: g_origPlayed = false;
new bool: g_customPlayed = false;
new bool: g_areAxisParas = false;
new bool: g_areOpSpawnAreasSafe = false;
new bool: g_isPlayerFake[33];
new bool: g_isPlayerInServer[33];

public plugin_natives()
{
    set_native_filter("nativesFilter");

    register_native("DoD_IsInKillingSpree", "Func_IsInKillingSpree");
}

public nativesFilter(const Name[], Index, bool: Found)
{
    if (('D' == Name[0] || Name[0] == 'd') && equali(Name, "DoD_AllowAllPlayersThrowKnives"))
        switch (Found)
        {
            case true:
                g_throwingKnivesCalls = true;
            default:
            {
                g_throwingKnivesCalls = false;
                return PLUGIN_HANDLED;
            }
        }
    return PLUGIN_CONTINUE;
}

public plugin_precache()
{
    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_killingspree.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[256];
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;
        if (equali(Key, "@ally_wav"))
            g_allyWav = Val;
        else if (equali(Key, "@axis_wav"))
            g_axisWav = Val;
        else if (equali(Key, "@custom_msg"))
            g_ksMsg = Val;
        else if (equali(Key, "@ally_length"))
            g_allyLength = str_to_float(Val);
        else if (equali(Key, "@axis_length"))
            g_axisLength = str_to_float(Val);
        else if (equali(Key, "@custom_time"))
            g_customTime = str_to_float(Val);
        else if (equali(Key, "@thrkn_value"))
            g_thrknValue = str_to_num(Val);
    }
    fclose(Config);

    if (!is_plugin_loaded("DoD Sounds Precache"))
        precache_sound("misc/killingspree.wav");

    precache_sound(g_allyWav);
    precache_sound(g_axisWav);

    return PLUGIN_CONTINUE;
}

public plugin_init()
{
    register_plugin("DoD Killing Spree", "1.0.0.1", "Vet(3TT3V)&claudiuhks");

    g_maxPlayers = get_maxplayers();

    g_controlPointMasterEntity = engfunc(EngFunc_FindEntityByString,
        g_controlPointMasterEntity, "classname", "dod_control_point_master");
    if (g_controlPointMasterEntity <= g_maxPlayers)
        return PLUGIN_CONTINUE;

    new Entity;
    for (Entity = 0; Entity < 2; Entity++) {
        g_scoreEntity = engfunc(EngFunc_FindEntityByString,
            g_scoreEntity, "classname", "dod_score_ent");
        if (g_scoreEntity <= g_maxPlayers)
            return PLUGIN_CONTINUE;
    }

    g_stripEntity = engfunc(EngFunc_FindEntityByString,
        g_stripEntity, "classname", "player_weaponstrip");
    if (g_stripEntity <= g_maxPlayers)
    {
        if (false == DoD_CreateNamedEntity("player_weaponstrip", g_stripEntity) ||
            g_stripEntity <= g_maxPlayers)
            return PLUGIN_CONTINUE;
        dllfunc(DLLFunc_Spawn, g_stripEntity);
    }

    RegisterHam(Ham_Think, "dod_control_point_master", "On_Ham_CtrlPntMaster_Think_Pre");
    RegisterHam(Ham_Use,   "dod_score_ent",            "On_Ham_ScoreEntity_Use_Pre");
    RegisterHam(Ham_Use,   "dod_score_ent",            "On_Ham_ScoreEntity_Use_Post", true);
    RegisterHam(Ham_Touch, "dod_control_point",        "On_Ham_MapObjective_Touch_Pre");
    RegisterHam(Ham_Touch, "dod_capture_area",         "On_Ham_MapObjective_Touch_Pre");
    RegisterHam(Ham_Touch, "trigger_hurt",             "On_Ham_TriggerHurt_Touch_Pre");
    RegisterHam(Ham_Spawn, "player",                   "On_Ham_Player_Spawn_Post", true);

    new Name[256];
    if (get_mapname(Name, charsmax(Name)) && equali(Name, "dod_escape"))
        RegisterHam(Ham_Touch, "func_breakable", "On_Ham_EscapeBreak_Touch_Pre");

    register_event("RoundState", "On_RoundState_Begin_FreezeEnd", "a", "1=1");

    g_areAxisParas = bool: dod_get_map_info(MI_AXIS_PARAS);

    g_syncMsg      = CreateHudSyncObj();
    g_syncTmr      = CreateHudSyncObj();

    g_axisTime     = g_customTime + g_killingSpreeLength + g_axisLength;
    g_allyTime     = g_customTime + g_killingSpreeLength + g_allyLength;

    Entity         = -1;
    while ((Entity = engfunc(EngFunc_FindEntityByString, Entity, "classname", "env_message")))
    {
        pev(Entity, pev_message, Name, charsmax(Name));
        if (!equali(Name, "MAP_SPAWN_WARNING"))
            continue;

        g_msgEnts[g_msgEntsNum] = Entity;
        pev(Entity, pev_targetname, Name, charsmax(Name));
        g_msgTrgs[g_msgEntsNum++] = Name;
    }
    return PLUGIN_CONTINUE;
}

public Func_IsInKillingSpree()
    return ksStatus: Idle != g_Status;

public client_putinserver(Player)
{
    g_isPlayerInServer[Player] = true;
    g_isPlayerFake[Player]     = bool: is_user_bot(Player);
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
    g_isPlayerInServer[Player] = false;

public On_RoundState_Begin_FreezeEnd()
{
    static Entity;
    Entity = -1;
    while ((Entity = engfunc(EngFunc_FindEntityByString, Entity, "classname", "func_tank")))
        ExecuteHamB(Ham_Think, Entity);
    g_origPlayed = false;
    g_customPlayed = false;
    g_areOpSpawnAreasSafe = false;
    for (Entity = 0; Entity < g_msgEntsNum; Entity++)
        set_pev(g_msgEnts[Entity], pev_targetname, g_msgTrgs[Entity]);
}

public On_FM_EmitAmbientSound_Pre(Entity, Float: Pos[3], Snd[], Float: Vol, Float: Attn, Flags, Pitch)
{ /// Enabled only on demand.
    static Trie: Bundle, Buffer[4];
    if (get_gametime() - g_winStamp < 0.01)
    {
        Bundle = TrieCreate();
        copy(g_origWinSnd, charsmax(g_origWinSnd), Snd);
        TrieSetCell (Bundle, "entity",      Entity);
        TrieSetCell (Bundle, "volume",      Vol);
        TrieSetCell (Bundle, "attenuation", Attn);
        TrieSetCell (Bundle, "flags",       Flags);
        TrieSetCell (Bundle, "pitch",       Pitch);
        TrieSetArray(Bundle, "origin",      Pos, sizeof Pos);
        Buffer[0] = normalizeTrie: Bundle;
        set_task(g_killingSpreeLength, "On_Timer_Custom_Music", 696101252, Buffer, sizeof Buffer);
        return FMRES_SUPERCEDE;
    }
    return FMRES_IGNORED;
}

public On_FM_Touch_Pre(Entity, Other)
{ /// Enabled only on demand.
    static Class[8];
    if (g_Status != ksStatus: Idle)
    {
        if (Entity > 0 && Entity <= g_maxPlayers && g_isPlayerInServer[Entity])
        {
            if (pev(Other, pev_classname, Class, charsmax(Class)) &&
                g_thrknValue != pev(Other, pev_flSwimTime) && equali(Class, "weapon", 6))
                return FMRES_SUPERCEDE; /// Let throwing knives work (16744448 [by default]).
        }
        else if (Other > 0 && Other <= g_maxPlayers && g_isPlayerInServer[Other])
        {
            if (pev(Entity, pev_classname, Class, charsmax(Class)) &&
                g_thrknValue != pev(Entity, pev_flSwimTime) && equali(Class, "weapon", 6))
                return FMRES_SUPERCEDE; /// Let throwing knives work (16744448 [by default]).
        }
    }
    return FMRES_IGNORED;
}

public On_Ham_ScoreEntity_Use_Pre(Entity)
{
    if (g_Status == ksStatus: Idle)
    { /// Preparation phase #1.
        g_origPlayed = false;
        g_customPlayed = false;
        g_areOpSpawnAreasSafe = true;
        g_Status = ksStatus: Prep;
        g_scoreEntity = Entity;
        g_Team = pev(Entity, pev_team);
        set_task(g_Team == AXIS ? g_axisTime : g_allyTime,
            "On_Timer_End_The_Killing_Spree", 272606959);
        g_tchHook = register_forward(FM_Touch,            "On_FM_Touch_Pre");
        g_sndHook = register_forward(FM_EmitAmbientSound, "On_FM_EmitAmbientSound_Pre");
        return HAM_SUPERCEDE; /// The task above will call Use() at a later time.
    }
    return HAM_IGNORED;
}

public On_Ham_ScoreEntity_Use_Post(Entity)
{
    static Player, Entity;
    switch (g_Status)
    {
        case (ksStatus: Prep):
        { /// Preparation phase #2.
            g_winStamp = get_gametime();
            for (Player = 1; Player <= g_maxPlayers; Player++)
                if (g_isPlayerInServer[Player] && is_user_alive(Player))
                {
                    stripEquip(Player, -1);
                    dod_set_stamina(Player, STAMINA_SET, 100, 100);
                }
            if (g_throwingKnivesCalls)
                DoD_AllowAllPlayersThrowKnives();
            Entity = -1;
            while ((Entity = engfunc(EngFunc_FindEntityByString, Entity, "classname", "func_tank")))
                set_pev(Entity, pev_nextthink, 0.0);
            for (Entity = 0; Entity < g_msgEntsNum; Entity++)
                set_pev(g_msgEnts[Entity], pev_targetname, "");
            set_hudmessage(20, 200, 40, -1.0, 0.0, 0, 0.5, 6.0, 0.1, 0.1);
            ShowSyncHudMsg(0, g_syncMsg, g_ksMsg);
            client_cmd(0, "spk misc/killingspree");
            g_Life = 100;
            g_Timer = (g_Team == AXIS) ? (g_axisTime / 100.0) : (g_allyTime / 100.0);
            On_Timer_Update_Visual_Message();
            set_task(g_Timer, "On_Timer_Update_Visual_Message", 404565191, "", 0, "a", 99);
            g_Status = ksStatus: Done;
        }
        case (ksStatus: Done):
        {
            g_Status = ksStatus: Idle;
            ExecuteHamB(Ham_Think, g_controlPointMasterEntity);
            for (Player = 1; Player <= g_maxPlayers; Player++)
                if (g_isPlayerInServer[Player])
                    dod_set_stamina(Player, STAMINA_RESET);
            unregister_forward(FM_Touch,            g_tchHook);
            unregister_forward(FM_EmitAmbientSound, g_sndHook);
        }
    }
}

public On_Ham_CtrlPntMaster_Think_Pre(Entity)
    return g_Status == ksStatus: Done ? HAM_SUPERCEDE : HAM_IGNORED;

public On_Ham_MapObjective_Touch_Pre(Entity)
    return g_Status != ksStatus: Idle ? HAM_SUPERCEDE : HAM_IGNORED;

public On_Ham_TriggerHurt_Touch_Pre(Entity)
    return g_areOpSpawnAreasSafe || g_Status != ksStatus: Idle ? HAM_SUPERCEDE : HAM_IGNORED;

public On_Ham_EscapeBreak_Touch_Pre(Entity)
{
    static Name[8];
    return g_Status != ksStatus: Idle && pev(Entity, pev_model, Name, charsmax(Name)) &&
        equal(Name, "*178") ? HAM_SUPERCEDE : HAM_IGNORED;
}

public On_Ham_Player_Spawn_Post(Player)
{
    static Team;
    if (g_Status != ksStatus: Idle && g_isPlayerInServer[Player])
    {
        Team = pev(Player, pev_team);
        switch (Team)
        {
            case AXIS, ALLIES:
                if (!g_isPlayerFake[Player])
                    set_task(0.000001, "On_Timer_Player_Spawn_Strip",
                        8320 + get_user_userid(Player));
                else
                    stripEquip(Player, Team);
        }
    }
}

public On_Timer_Player_Spawn_Strip(taskIndex)
{
    static Player;
    Player = find_player("fk", taskIndex - 8320);
    if (Player > 0) /// Skip dead, match by unique user index.
        stripEquip(Player, -1);
}

public On_Timer_Custom_Music(Buffer[4], taskIndex)
{
    static Trie: Bundle, Entity, Float: Pos[3], Float: Vol, Float: Attn, Flags, Pitch, Other[4];
    Bundle = Trie: Buffer[0];
    TrieGetCell (Bundle, "entity",      Entity);
    TrieGetCell (Bundle, "volume",      Vol);
    TrieGetCell (Bundle, "attenuation", Attn);
    TrieGetCell (Bundle, "flags",       Flags);
    TrieGetCell (Bundle, "pitch",       Pitch);
    TrieGetArray(Bundle, "origin",      Pos, sizeof Pos);
    if (Vol > 0.99 && !g_customPlayed)
    {
        Other[0] = normalizeTrie: Bundle;
        set_task(0.000001, "On_Timer_Custom_Music", 696101252, Other, sizeof Other);
    }
    else
    {
        engfunc(EngFunc_EmitAmbientSound, Entity, Pos,
            AXIS == g_Team ? g_axisWav : g_allyWav,
            Vol, Attn, Flags, Pitch);
        Other[0] = normalizeTrie: Bundle;
        set_task(g_Team == AXIS ? g_axisLength : g_allyLength,
            "On_Timer_Original_Music", 515090747, Other, sizeof Other);
        g_customPlayed = true;
    }
}

public On_Timer_Original_Music(Buffer[4], taskIndex)
{
    static Trie: Bundle, Entity, Float: Pos[3], Float: Vol, Float: Attn, Flags, Pitch, Other[4];
    Bundle = Trie: Buffer[0];
    TrieGetCell (Bundle, "entity",      Entity);
    TrieGetCell (Bundle, "volume",      Vol);
    TrieGetCell (Bundle, "attenuation", Attn);
    TrieGetCell (Bundle, "flags",       Flags);
    TrieGetCell (Bundle, "pitch",       Pitch);
    TrieGetArray(Bundle, "origin",      Pos, sizeof Pos);
    if (Vol > 0.99 && !g_origPlayed)
    {
        Other[0] = normalizeTrie: Bundle;
        set_task(0.000001, "On_Timer_Original_Music", 515090747, Other, sizeof Other);
    }
    else
    {
        engfunc(EngFunc_EmitAmbientSound, Entity, Pos, g_origWinSnd, Vol, Attn, Flags, Pitch);
        TrieDestroy(Bundle);
        g_origPlayed = true;
    }
}

public On_Timer_Update_Visual_Message()
{
    static R, G, B;
    switch (g_Life)
    {
        case 19:
        {
            R = 200;
            G =  20;
            B =  20;
        }
        case 49:
        {
            R = 200;
            G = 100;
            B =  20;
        }
        case 100:
        {
            R =  20;
            G = 200;
            B =  40;
        }
    }
    set_hudmessage(R, G, B, -1.0, 0.90, 0, 0.5, g_Timer, 0.05, 0.05);
    ShowSyncHudMsg(0, g_syncTmr, "%d%%", g_Life);
    --g_Life;
}

public On_Timer_End_The_Killing_Spree()
    ExecuteHamB(Ham_Use, g_scoreEntity, g_controlPointMasterEntity,
        g_controlPointMasterEntity, USE_TOGGLE, 0.0);

bool: stripEquip(Player, Team)
{
    static Res;
    dllfunc(DLLFunc_Use, g_stripEntity, Player);
    if (Team < 0)
    {
        Team = pev(Player, pev_team);
        if (Team == AXIS && g_areAxisParas)
            Team = 0;
    }
    switch (Team)
    {
        case 0:      DoD_GiveNamedItem(Player, "weapon_gerknife",  Res);
        case ALLIES: DoD_GiveNamedItem(Player, "weapon_amerknife", Res);
        default:     DoD_GiveNamedItem(Player, "weapon_spade",     Res);
    }
    return Res > g_maxPlayers;
}
