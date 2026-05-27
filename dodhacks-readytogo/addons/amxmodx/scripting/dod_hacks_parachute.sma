
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <dodhacks>

new bool: g_Alive[33];
new bool: g_inGame[33];
new Float: g_fallSpeed;

public plugin_init()
{
    register_plugin("DoD Hacks: Parachute", "1.0.0.5", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_parachute.ini");
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
        if (equali(Key, "@fall_speed"))
        {
            g_fallSpeed = str_to_float(Val);
            break;
        }
    }
    fclose(Config);

    register_forward(FM_CmdStart, "OnCmdStart_Post", true);

#if defined amxclient_cmd && defined RegisterHamPlayer
    RegisterHamPlayer(Ham_Killed, "OnPlayerKilled_Post", true);
#else
    RegisterHam(Ham_Killed, "player", "OnPlayerKilled_Post", true);
#endif
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
    g_inGame[Player] = true;

public DOD_ON_PLAYER_DISCONNECTED
{
    g_Alive[Player] = false;
    g_inGame[Player] = false;
}

public OnPlayerKilled_Post(Player)
    g_Alive[Player] = false;

public DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player)
    if (g_inGame[Player])
        g_Alive[Player] = true;

public OnCmdStart_Post(Player, Pack)
{
    if (!g_Alive[Player] || !(get_uc(Pack, UC_Buttons) & IN_USE))
        return FMRES_IGNORED;

    static Float: Velocity[3];
    if (pev(Player, pev_velocity, Velocity) && Velocity[2] < g_fallSpeed)
    {
        Velocity[2] = g_fallSpeed;
        set_pev(Player, pev_velocity, Velocity);
    }
    return FMRES_IGNORED;
}
