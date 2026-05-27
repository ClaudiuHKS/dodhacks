
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <dodconst>
#include <dodhacks>

#define offs_playerUsingRandomClass     1472 /// 'bool ::CBasePlayer::m_bIsRandomClass' variable.
#define offs_playerRecentlyChangedTeam   356 /// 'int  ::CBasePlayer::m_ilastteam'      variable.

new bool: g_showCustomMessage;
new bool: g_isPlayerInServer[33];
new g_maxPlayers;
new g_multiPlayerTeamLimit;

public plugin_init()
{
    register_plugin("DoD Hacks: Balancer", "1.0.0.5", "claudiuhks (Hattrick HKS)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_balancer.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        set_fail_state("Error opening '%s'!", Buffer);
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[32], Float: taskInterval;
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key, "@task_interval"))
            taskInterval = str_to_float(Val);
        else if (equali(Key, "@show_custom_msg"))
            g_showCustomMessage = bool: str_to_num(Val);
    }
    fclose(Config);
    g_maxPlayers = get_maxplayers();
    g_multiPlayerTeamLimit = get_cvar_pointer("mp_teamlimit");
    set_task(taskInterval, "Task_BalanceTeams", .flags = "b");
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
    g_isPlayerInServer[Player] = false;

public client_putinserver(Player)
    g_isPlayerInServer[Player] = true;

public Task_BalanceTeams()
{
    static Allies, Axis, maxAllowedDifference, Players[32], Name[32], Player, Class, bool: Random;
    maxAllowedDifference = get_pcvar_num(g_multiPlayerTeamLimit);
    if (!maxAllowedDifference)
        return PLUGIN_HANDLED;
    Allies = get_players_dod(Players, false, false, ALLIES);
    Axis = get_players_dod(Players, false, false, AXIS);
    if (Allies - Axis > maxAllowedDifference)
    {
        Allies = get_players_dod(Players, true, true, ALLIES);
        if (Allies > 0)
        {
            Player = Players[random_num(0, Allies - 1)];
            Random = get_pdata_bool(Player, offs_playerUsingRandomClass);
            if (g_showCustomMessage)
            {
                DoD_ChangePlayerTeam(Player, AXIS, false, false, true, true, false, true);
                get_user_name(Player, Name, charsmax(Name));
                client_print(0, print_chat, "* %s joined Axis to balance teams.", Name);
            }
            else
                DoD_ChangePlayerTeam(Player, AXIS, false, false, true, true, true, true);
            if (Random)
                DoD_ChooseRandomClass(Player, Class, true, true, true, true, DoD_RCA_Add);
            else
                DoD_ChooseRandomClass(Player, Class, true, true, true, false, DoD_RCA_Remove);
        }
    }
    else if (Axis - Allies > maxAllowedDifference)
    {
        Axis = get_players_dod(Players, true, true, AXIS);
        if (Axis > 0)
        {
            Player = Players[random_num(0, Axis - 1)];
            Random = get_pdata_bool(Player, offs_playerUsingRandomClass);
            if (g_showCustomMessage)
            {
                DoD_ChangePlayerTeam(Player, ALLIES, false, false, true, true, false, true);
                get_user_name(Player, Name, charsmax(Name));
                client_print(0, print_chat, "* %s joined Allies to balance teams.", Name);
            }
            else
                DoD_ChangePlayerTeam(Player, ALLIES, false, false, true, true, true, true);
            if (Random)
                DoD_ChooseRandomClass(Player, Class, true, true, true, true, DoD_RCA_Add);
            else
                DoD_ChooseRandomClass(Player, Class, true, true, true, false, DoD_RCA_Remove);
        }
    }
    return PLUGIN_HANDLED;
}

get_players_dod(Players[32], bool: excludeAlive, bool: excludeRecentlyChangedTeam, Team)
{
    static Count, Player;
    Count = 0;
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (g_isPlayerInServer[Player] && Team == get_user_team(Player) && (!excludeAlive || !is_user_alive(Player)) &&
            (!excludeRecentlyChangedTeam || !get_pdata_int(Player, offs_playerRecentlyChangedTeam)))
            Players[Count++] = Player;
    return Count;
}
