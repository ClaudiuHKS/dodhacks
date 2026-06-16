
#include <amxmodx>
#include <amxmisc>
#include <dodconst>

new g_Flag;
new g_maxPlayers;
new Float: g_maxSeconds;
new Float: g_playerSpecTime[33];
new bool: g_isPlayerFake[33];
new bool: g_isPlayerWarned[33];
new bool: g_isPlayerInServer[33];

public plugin_init()
{
    register_plugin("DoD Hacks: Spec", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_spec.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[32];
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;
        if (equali(Key, "@spec_seconds"))
            g_maxSeconds = floatmax(15.0, str_to_float(Val));
        else if (equali(Key, "@admin_flag"))
            g_Flag = read_flags(Val);
    }
    fclose(Config);

    g_maxPlayers = get_maxplayers();
    set_task(5.0, "Task_Spec", .flags = "b");
    return PLUGIN_CONTINUE;
}

public client_putinserver(Player)
{
    g_isPlayerInServer[Player] = true;
    g_isPlayerFake[Player] = bool: is_user_bot(Player);
    g_isPlayerWarned[Player] = false;
    g_playerSpecTime[Player] = 0.0;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    g_isPlayerInServer[Player] = false;
    g_isPlayerWarned[Player] = false;
    g_isPlayerFake[Player] = false;
    g_playerSpecTime[Player] = 0.0;
}

public client_command(Player)
    if (g_isPlayerInServer[Player])
    {
        g_playerSpecTime[Player] = 0.0;
        g_isPlayerWarned[Player] = false;
    }

public Task_Spec()
{
    static Player, Team;
    if (get_playersnum() == g_maxPlayers)
        for (Player = 1; Player <= g_maxPlayers; Player++)
        {
            if (!g_isPlayerInServer[Player])
                continue;
            Team = get_user_team(Player);
            if (Team == ALLIES || Team == AXIS)
            {
                g_playerSpecTime[Player] = 0.0;
                g_isPlayerWarned[Player] = false;
                continue;
            }
            g_playerSpecTime[Player] += 5.0;
            if (g_playerSpecTime[Player] >= g_maxSeconds &&
                (!g_Flag || g_Flag != (get_user_flags(Player) & g_Flag)))
                server_cmd("kick #%d You were Spec+AFK on full server.",
                    get_user_userid(Player));
            else if (!g_isPlayerWarned[Player] && !g_isPlayerFake[Player] &&
                10.0 + g_playerSpecTime[Player] >= g_maxSeconds &&
                (!g_Flag || g_Flag != (get_user_flags(Player) & g_Flag)))
            {
                client_print(Player, print_chat,
                    "* AFK + Spectator on full server may get you kicked.");
                g_isPlayerWarned[Player] = true;
            }
        }
    else
        for (Player = 1; Player <= g_maxPlayers; Player++)
        {
            if (!g_isPlayerInServer[Player])
                continue;
            Team = get_user_team(Player);
            if (Team == ALLIES || Team == AXIS)
            {
                g_playerSpecTime[Player] = 0.0;
                g_isPlayerWarned[Player] = false;
                continue;
            }
            g_playerSpecTime[Player] += 5.0;
        }
}
