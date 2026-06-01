
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <dodhacks>

new g_Sync;
new g_MaxPlayers;
new g_Team[33];
new bool: g_Effects;
new bool: g_TeamCheck;
new bool: g_Fake[33];
new bool: g_InSrv[33];
new Float: g_VerPos;
new Float: g_HorPos;

public plugin_init()
{
    register_plugin("DoD Hacks: Head Shot", "1.0.0.6", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_headshot.ini");
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
            parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;
        if (equali(Key, "@hud_effects"))
            g_Effects = bool: str_to_num(Val);
        else if (equali(Key, "@hud_vertical_pos"))
            g_VerPos = str_to_float(Val);
        else if (equali(Key, "@hud_horizontal_pos"))
            g_HorPos = str_to_float(Val);
        else if (equali(Key, "@team_check"))
            g_TeamCheck = bool: str_to_num(Val);
    }
    fclose(Config);

#if defined amxclient_cmd && defined RegisterHamPlayer
    RegisterHamPlayer(Ham_TraceAttack, "OnPlayerTraceAttack_Post", true);
#else
    RegisterHam(Ham_TraceAttack, "player", "OnPlayerTraceAttack_Post", true);
#endif
    g_MaxPlayers = get_maxplayers();
    g_Sync = CreateHudSyncObj();
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player)
    if (g_TeamCheck && g_InSrv[Player])
        g_Team[Player] = get_user_team(Player);

public OnPlayerTraceAttack_Post(Player, Attacker, Float: Damage, Float: Direction[3], TraceRes)
    if (TraceRes && Attacker > 0 && Attacker <= g_MaxPlayers && g_InSrv[Attacker] &&
        false == g_Fake[Attacker] && (false == g_TeamCheck || g_Team[Attacker] != g_Team[Player]) &&
        get_tr2(TraceRes, TR_iHitgroup) == HIT_HEAD)
    {
        set_hudmessage(200 /** red */, 100 /** green */, 20 /** blue */,
            g_HorPos /** horizontal pos */, g_VerPos /** vertical pos */,
            g_Effects ? 1 : 0 /** effect type */, 0.5 /** effect time */,
            1.0 /** duration */, 0.1 /** fade in time */, 0.1 /** fade out time */);
        ShowSyncHudMsg(Attacker, g_Sync, "HEAD!");
    }

public client_putinserver(Player)
{
    g_InSrv[Player] = true;
    g_Fake[Player] = bool: is_user_bot(Player);
}

public DOD_ON_PLAYER_DISCONNECTED
{
    g_InSrv[Player] = false;
    g_Fake[Player] = false;
}
