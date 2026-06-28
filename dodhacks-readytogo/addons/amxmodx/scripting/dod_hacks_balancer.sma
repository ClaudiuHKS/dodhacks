
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <dodconst>
#include <dodhacks>

#define ofs_playerUsingRandomClass   1472 /// 'bool ::CBasePlayer::m_bIsRandomClass' variable.
#define ofs_playerRecentlyChangedTeam 356 /// 'int  ::CBasePlayer::m_ilastteam'      variable.
#define ofs_playerNextClass           367 /// 'int  ::CBasePlayer::m_iNextClass'     variable.

new bool: g_doBeep;
new bool: g_exclUnclassed;
new bool: g_showScreenFade;
new bool: g_areAlliesBritish;
new bool: g_showCustomMessage;
new bool: g_isPlayerFake[33];
new bool: g_isPlayerInServer[33];
new g_Flag;
new g_maxPlayers;
new g_msgScreenFade;
new g_multiPlayerTeamLimit;

public plugin_init()
{
    register_plugin("DoD Hacks: Balancer", "1.0.1.1", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_balancer.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
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
        else if (equali(Key, "@show_screen_fade"))
            g_showScreenFade = bool: str_to_num(Val);
        else if (equali(Key, "@excl_unclassed"))
            g_exclUnclassed = bool: str_to_num(Val);
        else if (equali(Key, "@audio_beep"))
            g_doBeep = bool: str_to_num(Val);
        else if (equali(Key, "@excl_flags"))
            g_Flag = read_flags(Val);
    }
    fclose(Config);

    g_maxPlayers = get_maxplayers();
    g_areAlliesBritish = DoD_AreAlliesBritish();
    if (g_showScreenFade)
        g_msgScreenFade = get_user_msgid("ScreenFade");
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
{
    g_isPlayerInServer[Player] = false;
    g_isPlayerFake[Player] = false;
}

public client_putinserver(Player)
{
    g_isPlayerInServer[Player] = true;
    g_isPlayerFake[Player] = bool: is_user_bot(Player);
}

public Task_BalanceTeams()
{
    static Allies, Axis, maxAllowedDifference, Players[32], Name[32], Player, Class, bool: Random;
    maxAllowedDifference = get_pcvar_num(g_multiPlayerTeamLimit);
    if (!maxAllowedDifference)
        return PLUGIN_HANDLED;
    Allies = get_players_dod(Players, false, false, ALLIES, false);
    Axis = get_players_dod(Players, false, false, AXIS, false);
    if (Allies - Axis > maxAllowedDifference)
    {
        Allies = get_players_dod(Players, true, true, ALLIES, g_exclUnclassed);
        if (Allies > 0)
        {
            Player = Players[random_num(0, Allies - 1)];
            Random = DoD_GetPvDataBool(Player, ofs_playerUsingRandomClass);
            if (g_showCustomMessage)
            {
                DoD_ChangePlayerTeam(Player, AXIS, false, false, true, true, false, true);
                get_user_name(Player, Name, charsmax(Name));
                client_print(0, print_chat, "* %s joins Axis to balance teams.", Name);
            }
            else
                DoD_ChangePlayerTeam(Player, AXIS, false, false, true, true, true, true);
            if (g_isPlayerFake[Player])
            {
                if (Random)
                    DoD_ChooseRandomClass(Player, Class, true, true, false, false, DoD_RCA_Add);
                else
                    DoD_ChooseRandomClass(Player, Class, true, true, false, false, DoD_RCA_Remove);
            }
            else
            {
                if (Random)
                    DoD_ChooseRandomClass(Player, Class, true, true, true, true, DoD_RCA_Add);
                else
                    DoD_ChooseRandomClass(Player, Class, true, true, true, false, DoD_RCA_Remove);
                if (g_showScreenFade)
                    doScreenFade(Player, AXIS, 0.15, 0.20);
                if (g_doBeep)
                    client_cmd(Player, "spk fvox/beep");
            }
        }
    }
    else if (Axis - Allies > maxAllowedDifference)
    {
        Axis = get_players_dod(Players, true, true, AXIS, g_exclUnclassed);
        if (Axis > 0)
        {
            Player = Players[random_num(0, Axis - 1)];
            Random = DoD_GetPvDataBool(Player, ofs_playerUsingRandomClass);
            if (g_showCustomMessage)
            {
                DoD_ChangePlayerTeam(Player, ALLIES, false, false, true, true, false, true);
                get_user_name(Player, Name, charsmax(Name));
                client_print(0, print_chat, "* %s joins %s to balance teams.",
                    Name, g_areAlliesBritish ? "British" : "Allies");
            }
            else
                DoD_ChangePlayerTeam(Player, ALLIES, false, false, true, true, true, true);
            if (g_isPlayerFake[Player])
            {
                if (Random)
                    DoD_ChooseRandomClass(Player, Class, true, true, false, false, DoD_RCA_Add);
                else
                    DoD_ChooseRandomClass(Player, Class, true, true, false, false, DoD_RCA_Remove);
            }
            else
            {
                if (Random)
                    DoD_ChooseRandomClass(Player, Class, true, true, true, true, DoD_RCA_Add);
                else
                    DoD_ChooseRandomClass(Player, Class, true, true, true, false, DoD_RCA_Remove);
                if (g_showScreenFade)
                    doScreenFade(Player, ALLIES, 0.15, 0.20);
                if (g_doBeep)
                    client_cmd(Player, "spk fvox/beep");
            }
        }
    }
    return PLUGIN_HANDLED;
}

get_players_dod
(Players[32], bool: exclAlive, bool: exclRecentlyChangedTeam, Team, bool: exclUnclassed)
{
    static Count, Player;
    for (Player = 1, Count = 0; Player <= g_maxPlayers; Player++)
        if (g_isPlayerInServer[Player] && Team == get_user_team(Player) &&
            (!exclAlive || !is_user_alive(Player)) &&
            (!exclRecentlyChangedTeam || !get_pdata_int(Player, ofs_playerRecentlyChangedTeam)) &&
            (!exclUnclassed || false == isPlayerUnclassed(Player)) &&
            (!g_Flag || g_Flag != (get_user_flags(Player) & g_Flag)))
            Players[Count++] = Player;
    return Count;
}

bool: isPlayerUnclassed(const &Player)
    return pev(Player, pev_playerclass) < DODC_GARAND &&
        !DoD_GetPvDataBool(Player, ofs_playerUsingRandomClass) &&
        get_pdata_int(Player, ofs_playerNextClass) < DODC_GARAND;

doScreenFade(const &Player, Team, Float: Duration, Float: holdTime)
{
    message_begin(MSG_ONE_UNRELIABLE, g_msgScreenFade,
        { 0, 0, 0 } /** Message origin. */, Player);
    write_short(floatround(4096.0 /** UNIT_SECOND = (1 << 12). */
        * Duration, floatround_round)); /// Duration.
    write_short(floatround(4096.0 /** UNIT_SECOND = (1 << 12). */
        * holdTime, floatround_round)); /// Hold time.
    write_short(0 /** FFADE_IN = 0x0000. */); /// Fade type.
    if (Team == AXIS)
    { /// Red.
        write_byte(200); /// Red.
        write_byte( 20); /// Green.
        write_byte( 20); /// Blue.
        write_byte(200); /// Alpha.
    }
    else
    { /// Teal.
        write_byte( 20); /// Red.
        write_byte(180); /// Green.
        write_byte(120); /// Blue.
        write_byte(200); /// Alpha.
    }
    message_end();
}
