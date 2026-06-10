
#pragma ctrlchar '\' /// Replace '^' with '\'.

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <sqlx>
#include <dodx>

#if !defined NULL_STRING && !defined engine_changelevel
#include <dodhacks> /// For 'DoD_ChangeMap(Map)' function. May be replaced with 'engine_changelevel(Map)' or 'engfunc(EngFunc_ChangeLevel, Map, NULL_STRING)' for removing DoD Hacks dependency (if your AMXX version has either of these 2).
#endif

///
/// Either "mysql", "sqlite" or both should be enabled in /dod/addons/amxmodx/configs/modules.ini
/// (by removing the ; symbol from its/ their front).
///
/// Choose mysql if you have a host, user, password and remote sql database name.
/// Choose sqlite otherwise.
///
/// Both are threaded and won't cause any lag spikes at all.
///
/// mysql is better than sqlite with up to 10%.
///

#if !defined SVC_DIRECTOR
#define      SVC_DIRECTOR    51
#endif

#if !defined DRC_CMD_MESSAGE
#define      DRC_CMD_MESSAGE  6
#endif

#if !defined SPECTATORS
#define      SPECTATORS       3
#endif

#define m_CBasePlayer_iDeaths          477
#define TaskOfs_Rank_UniqueUserId 33619967 /// 65535+2^25. Some offset other plugins don't use.

new g_maxPlayers;
new g_totalRanks = -1;
new g_minPlayersForRanking;
new g_secondsToDeleteEntries;
new g_playerRankEntry[33];
new g_playerJoinUnixTimeStamp[33];
new Handle: g_tmpSqlConnectionTuple = Empty_Handle;
new Handle: g_threadedSqlConnectionTuple = Empty_Handle;
new bool: g_isSpecial;
new bool: g_isClassic;
new bool: g_onlyHumans;
new bool: g_hideChatCmds;
new bool: g_showWelcomeMsg;
new bool: g_isStorageLocal;
new bool: g_removeOldEntries;
new bool: g_shouldDropTables = false;
new bool: g_doRankFakePlayers;
new bool: g_areRanksSteamBased;
new bool: g_doAlterPlayerDeaths;
new bool: g_requireMinPlayersForRanking;
new bool: g_isPlayerFake[33];
new bool: g_isPlayerAuth[33];
new bool: g_isPlayerInServer[33];
new Float: g_welcomeTaskDelay;
new g_Buffer[512];
new g_playerName[33][32];
new g_playerSteam[33][96];
new g_playerCleanName[33][64];
new Handle: g_tmpSqlConnection = Empty_Handle;
new bool: g_initiallyConnected = false;

/**
 * AMX Mod X forwards.
 */

public plugin_init()
{
    register_plugin("DoD Hacks: Rank", "1.0.0.8", "Hattrick HKS (claudiuhks)");

    if (!safeIsSqlModuleRunning("sqlite") && !safeIsSqlModuleRunning("mysql"))
    {
        set_fail_state("'../amxmodx/configs/modules.ini' needs either sqlite or mysql enabled!");
        return PLUGIN_HANDLED;
    }
    get_configsdir(g_Buffer, charsmax(g_Buffer));
    add(g_Buffer, charsmax(g_Buffer), "/dod_hacks_rank.ini");
    new Config = fopen(g_Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", g_Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }
    new errCode, Driver[16], User[32], Pass[32], Db[32], Host[32], Key[32], Val[32], Err[4];
    while (fgets(Config, g_Buffer, charsmax(g_Buffer)) > 0)
    {
        trim(g_Buffer);
        if (!g_Buffer[0] || g_Buffer[0] == ';' || g_Buffer[0] == '/' ||
            parse(g_Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;
        if (equali(Key, "@db_driver"))
        {
            copy(Driver, charsmax(Driver), Val);
            g_isStorageLocal = bool: equali(Driver, "sqlite");
        }
        else if (equali(Key, "@db_user"))
            copy(User, charsmax(User), Val);
        else if (equali(Key, "@db_pass"))
            copy(Pass, charsmax(Pass), Val);
        else if (equali(Key, "@db_host"))
            copy(Host, charsmax(Host), Val);
        else if (equali(Key, "@db_name"))
            copy(Db, charsmax(Db), Val);
        else if (equali(Key, "@rank_bots"))
            g_doRankFakePlayers = bool: str_to_num(Val);
        else if (equali(Key, "@use_special"))
            g_isSpecial = bool: str_to_num(Val);
        else if (equali(Key, "@use_classic"))
            g_isClassic = bool: str_to_num(Val);
        else if (equali(Key, "@alter_deaths"))
            g_doAlterPlayerDeaths = bool: str_to_num(Val);
        else if (equali(Key, "@rank_steam"))
            g_areRanksSteamBased = bool: str_to_num(Val);
        else if (equali(Key, "@rank_min_players"))
            g_requireMinPlayersForRanking = bool: str_to_num(Val);
        else if (equali(Key, "@hide_chat_cmds"))
            g_hideChatCmds = bool: str_to_num(Val);
        else if (equali(Key, "@rank_players_num"))
            g_minPlayersForRanking = str_to_num(Val);
        else if (equali(Key, "@show_welcome"))
            g_showWelcomeMsg = bool: str_to_num(Val);
        else if (equali(Key, "@erase_old"))
            g_removeOldEntries = bool: str_to_num(Val);
        else if (equali(Key, "@only_humans"))
            g_onlyHumans = bool: str_to_num(Val);
        else if (equali(Key, "@erase_old_secs"))
            g_secondsToDeleteEntries = str_to_num(Val);
        else if (equali(Key, "@task_delay"))
            g_welcomeTaskDelay = str_to_float(Val);
    }
    fclose(Config);

    SQL_GetAffinity(g_Buffer, charsmax(g_Buffer));
    if (!equali(g_Buffer, Driver))
        if (!SQL_SetAffinity(Driver))
            log_amx("SQL_SetAffinity('%s') call failed. Ensure the module is enabled in '../amxmodx/configs/modules.ini'.",
                Driver);
    g_threadedSqlConnectionTuple = SQL_MakeDbTuple(Host, User, Pass, Db);
    g_tmpSqlConnectionTuple = SQL_MakeDbTuple(Host, User, Pass, Db, 3);
    g_tmpSqlConnection = SQL_Connect(g_tmpSqlConnectionTuple, errCode, Err, charsmax(Err));
    if (Empty_Handle != g_tmpSqlConnection)
    {
        g_initiallyConnected = true;
        SQL_FreeHandle(g_tmpSqlConnection);
        g_tmpSqlConnection = Empty_Handle;
        SQL_FreeHandle(g_tmpSqlConnectionTuple);
        g_tmpSqlConnectionTuple = Empty_Handle;
        register_concmd("amx_rank_drop", "OnConCmd_Drop", ADMIN_RCON, "- erases all rank entries (after map change)");
        register_concmd("amx_rank_zero", "OnConCmd_Zero", ADMIN_RCON, "- zeroes all rank entries (now)");
        register_message(get_user_msgid("ObjScore"), "On_ObjScore_Message");
        if (g_areRanksSteamBased)
        {
            if (g_isStorageLocal)
            {
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql",
                    "CREATE TABLE IF NOT EXISTS pSteamStats (pEntry INTEGER NOT NULL, pName CHARACTER (32) NOT NULL COLLATE NOCASE, pSteam CHARACTER (64) NOT NULL UNIQUE COLLATE NOCASE, pKills INTEGER NOT NULL, pDeaths INTEGER NOT NULL, pHeads INTEGER NOT NULL, pScore INTEGER NOT NULL, pSecs INTEGER NOT NULL, pPerf FLOAT NOT NULL, pKdr FLOAT NOT NULL, pJoin INTEGER NOT NULL, PRIMARY KEY (pEntry AUTOINCREMENT), UNIQUE (pSteam))");
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "CREATE INDEX IF NOT EXISTS idxPlayerPerf ON pSteamStats(pPerf)");
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "CREATE INDEX IF NOT EXISTS idxPlayerKdr ON pSteamStats(pKdr)");
            }
            else
            { /// Workaround for older AMX Mod X versions.
                formatex(g_Buffer, charsmax(g_Buffer),
                    "CREATE TABLE IF NOT EXISTS pSteamStats (pEntry INTEGER NOT NULL AUTO_INCREMENT, pName CHAR (32) NOT NULL COLLATE utf8mb4_unicode_520_ci, pSteam CHAR (64) NOT NULL COLLATE utf8mb4_unicode_520_ci, pKills INTEGER NOT NULL, pDeaths INTEGER NOT NULL, pHeads INTEGER NOT NULL, pScore INTEGER NOT NULL, pSecs INTEGER NOT NULL, pPerf FLOAT NOT NULL, pKdr FLOAT NOT NULL, pJoin INTEGER NOT NULL, PRIMARY KEY (pEntry), UNIQUE (pSteam))");
                add(g_Buffer, charsmax(g_Buffer),
                    " ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_520_ci");
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "CREATE INDEX IF NOT EXISTS idxPlayerPerf ON pSteamStats(pPerf)");
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "CREATE INDEX IF NOT EXISTS idxPlayerKdr ON pSteamStats(pKdr)");
            }
            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Total_Ranks_Revealing", "SELECT COUNT(*) FROM pSteamStats");
        }
        else
        {
            if (g_isStorageLocal)
            {
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql",
                    "CREATE TABLE IF NOT EXISTS pNameStats (pEntry INTEGER NOT NULL, pName CHARACTER (32) NOT NULL UNIQUE COLLATE NOCASE, pKills INTEGER NOT NULL, pDeaths INTEGER NOT NULL, pHeads INTEGER NOT NULL, pScore INTEGER NOT NULL, pSecs INTEGER NOT NULL, pPerf FLOAT NOT NULL, pKdr FLOAT NOT NULL, pJoin INTEGER NOT NULL, PRIMARY KEY (pEntry AUTOINCREMENT), UNIQUE (pName))");
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "CREATE INDEX IF NOT EXISTS idxPlayerPerf ON pNameStats(pPerf)");
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "CREATE INDEX IF NOT EXISTS idxPlayerKdr ON pNameStats(pKdr)");
            }
            else
            {
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql",
                    "CREATE TABLE IF NOT EXISTS pNameStats (pEntry INTEGER NOT NULL AUTO_INCREMENT, pName CHAR (32) NOT NULL COLLATE utf8mb4_unicode_520_ci, pKills INTEGER NOT NULL, pDeaths INTEGER NOT NULL, pHeads INTEGER NOT NULL, pScore INTEGER NOT NULL, pSecs INTEGER NOT NULL, pPerf FLOAT NOT NULL, pKdr FLOAT NOT NULL, pJoin INTEGER NOT NULL, PRIMARY KEY (pEntry), UNIQUE (pName)) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_520_ci");
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "CREATE INDEX IF NOT EXISTS idxPlayerPerf ON pNameStats(pPerf)");
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "CREATE INDEX IF NOT EXISTS idxPlayerKdr ON pNameStats(pKdr)");
            }
            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Total_Ranks_Revealing", "SELECT COUNT(*) FROM pNameStats");
        }
    }
    else
    {
        log_amx("Rank plugin loaded with MySQL server being offline.");
        log_amx("As soon as the connection establishes, map will restart automatically.");
        log_amx("If you are using SQLite, 'sqlite' module needs to be enabled in '../amxmodx/configs/modules.ini'.");
        set_task(1.0, "Task_VerifyConnection");
    }

    if (g_doAlterPlayerDeaths)
#if defined amxclient_cmd && defined RegisterHamPlayer
        RegisterHamPlayer(Ham_Killed, "On_Player_Killed_Pre");
#else
        RegisterHam(Ham_Killed, "player", "On_Player_Killed_Pre");
#endif
    g_maxPlayers = get_maxplayers();
    return PLUGIN_CONTINUE;
}

public Task_VerifyConnection()
{
    static Map[64], errCode, Err[4];
    g_tmpSqlConnection = SQL_Connect(g_tmpSqlConnectionTuple, errCode, Err, charsmax(Err));
    if (Empty_Handle != g_tmpSqlConnection)
    {
        SQL_FreeHandle(g_tmpSqlConnection);
        if (Empty_Handle != g_tmpSqlConnectionTuple)
            SQL_FreeHandle(g_tmpSqlConnectionTuple);
        get_mapname(Map, charsmax(Map));
#if defined engine_changelevel
        engine_changelevel(Map);
#else
#if defined NULL_STRING
        engfunc(EngFunc_ChangeLevel, Map, NULL_STRING);
#else
        DoD_ChangeMap(Map);
#endif
#endif
    }
    else
        set_task(1.0, "Task_VerifyConnection");
}

public plugin_end()
    if (g_shouldDropTables)
    { /// At this point, `g_initiallyConnected' is true.
        SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "DROP TABLE IF EXISTS pNameStats");
        SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", "DROP TABLE IF EXISTS pSteamStats");
    }
    else if (g_removeOldEntries && g_initiallyConnected)
    {
        if (g_areRanksSteamBased)
            formatex(g_Buffer, charsmax(g_Buffer),
                "DELETE FROM pSteamStats WHERE pJoin < %d",
                get_systime() - g_secondsToDeleteEntries);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "DELETE FROM pNameStats WHERE pJoin < %d",
                get_systime() - g_secondsToDeleteEntries);
        SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
    }

public client_connect(Player)
{
    g_isPlayerAuth[Player]     = false;
    g_isPlayerFake[Player]     = false;
    g_isPlayerInServer[Player] = false;
    g_playerJoinUnixTimeStamp[Player] =  get_systime();
    g_playerRankEntry[Player]         = -1;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    if (g_playerRankEntry[Player] > -1)
    {
        if (g_areRanksSteamBased)
            formatex(g_Buffer, charsmax(g_Buffer),
                "UPDATE pSteamStats SET pSecs = pSecs + %d WHERE pEntry = %d",
                get_systime() - g_playerJoinUnixTimeStamp[Player],
                g_playerRankEntry[Player]);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "UPDATE pNameStats SET pSecs = pSecs + %d WHERE pEntry = %d",
                get_systime() - g_playerJoinUnixTimeStamp[Player],
                g_playerRankEntry[Player]);
        SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
    }
    g_isPlayerAuth[Player]     = false;
    g_isPlayerFake[Player]     = false;
    g_isPlayerInServer[Player] = false;
    g_playerJoinUnixTimeStamp[Player] =  0;
    g_playerRankEntry[Player]         = -1;
}

#if defined client_connectex
public client_authorized(Player, const Steam[])
#else
public client_authorized(Player)
#endif
{
#if !defined client_connectex
    static Steam[32], Info[32];
    get_user_authid(Player, Steam, charsmax(Steam));
#else
    static Info[32];
#endif
    g_isPlayerAuth[Player] = true;
    g_isPlayerFake[Player] = bool: is_user_bot(Player);
    get_user_name(Player, g_playerName[Player], charsmax(g_playerName[]));
    SQL_QuoteString(Empty_Handle,
        g_playerCleanName[Player], charsmax(g_playerCleanName[]),
        g_playerName[Player]);
    if (g_initiallyConnected)
    {
        if (g_areRanksSteamBased)
        {
            if (g_doRankFakePlayers)
            {
                if (copy(g_playerSteam[Player], charsmax(g_playerSteam[]), Steam) < 5)
                {
                    add(g_playerSteam[Player], charsmax(g_playerSteam[]), "@");
                    add(g_playerSteam[Player], charsmax(g_playerSteam[]),
                        g_playerCleanName[Player]);
                }
                formatex(g_Buffer, charsmax(g_Buffer),
                    "SELECT pEntry FROM pSteamStats WHERE pSteam = '%s'",
                    g_playerSteam[Player]);
                num_to_str(get_user_userid(Player), Info, charsmax(Info));
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection", g_Buffer, Info, sizeof Info);
            }
            else if (copy(g_playerSteam[Player], charsmax(g_playerSteam[]), Steam) > 4)
            {
                formatex(g_Buffer, charsmax(g_Buffer),
                    "SELECT pEntry FROM pSteamStats WHERE pSteam = '%s'",
                    Steam);
                num_to_str(get_user_userid(Player), Info, charsmax(Info));
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection", g_Buffer, Info, sizeof Info);
            }
        }
        else
        {
            if (g_doRankFakePlayers)
            {
                if (copy(g_playerSteam[Player], charsmax(g_playerSteam[]), Steam) < 5)
                {
                    add(g_playerSteam[Player], charsmax(g_playerSteam[]), "@");
                    add(g_playerSteam[Player], charsmax(g_playerSteam[]),
                        g_playerCleanName[Player]);
                }
                formatex(g_Buffer, charsmax(g_Buffer),
                    "SELECT pEntry FROM pNameStats WHERE pName = '%s'",
                    g_playerCleanName[Player]);
                num_to_str(get_user_userid(Player), Info, charsmax(Info));
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection", g_Buffer, Info, sizeof Info);
            }
            else if (copy(g_playerSteam[Player], charsmax(g_playerSteam[]), Steam) > 4)
            {
                formatex(g_Buffer, charsmax(g_Buffer),
                    "SELECT pEntry FROM pNameStats WHERE pName = '%s'",
                    g_playerCleanName[Player]);
                num_to_str(get_user_userid(Player), Info, charsmax(Info));
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection", g_Buffer, Info, sizeof Info);
            }
        }
    }
}

public client_putinserver(Player)
{
    g_isPlayerInServer[Player] = true;
    g_isPlayerFake[Player]     = bool: is_user_bot(Player);
}

public client_infochanged(Player)
{
    static Name[32], Info[32], Time;
    get_user_info(Player, "name", Name, charsmax(Name));
    if (!equal(Name, g_playerName[Player]))
    {
        if (g_playerRankEntry[Player] > -1)
        {
            if (g_areRanksSteamBased)
            {
                g_playerName[Player] = Name;
                SQL_QuoteString(Empty_Handle,
                    g_playerCleanName[Player], charsmax(g_playerCleanName[]),
                    g_playerName[Player]);
                formatex(g_Buffer, charsmax(g_Buffer),
                    "UPDATE pSteamStats SET pName = '%s' WHERE pEntry = %d",
                    g_playerCleanName[Player], g_playerRankEntry[Player]);
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
            }
            else if (!equali(Name, g_playerName[Player]))
            {
                g_playerName[Player] = Name;
                SQL_QuoteString(Empty_Handle,
                    g_playerCleanName[Player], charsmax(g_playerCleanName[]),
                    g_playerName[Player]);
                Time = get_systime();
                formatex(g_Buffer, charsmax(g_Buffer),
                    "UPDATE pNameStats SET pSecs = pSecs + %d WHERE pEntry = %d",
                    Time - g_playerJoinUnixTimeStamp[Player], g_playerRankEntry[Player]);
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
                g_playerJoinUnixTimeStamp[Player] = Time;
                formatex(g_Buffer, charsmax(g_Buffer),
                    "SELECT pEntry FROM pNameStats WHERE pName = '%s'",
                    g_playerCleanName[Player]);
                num_to_str(get_user_userid(Player), Info, charsmax(Info));
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection", g_Buffer,
                    Info, sizeof Info);
            }
            else
            {
                g_playerName[Player] = Name;
                SQL_QuoteString(Empty_Handle,
                    g_playerCleanName[Player], charsmax(g_playerCleanName[]),
                    g_playerName[Player]);
                formatex(g_Buffer, charsmax(g_Buffer),
                    "UPDATE pNameStats SET pName = '%s' WHERE pEntry = %d",
                    g_playerCleanName[Player], g_playerRankEntry[Player]);
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
            }
        }
        else
        {
            g_playerName[Player] = Name;
            SQL_QuoteString(Empty_Handle,
                g_playerCleanName[Player], charsmax(g_playerCleanName[]),
                g_playerName[Player]);
        }
    }
}

public client_command(Player)
{
    static Cmd[16], Arg[16], Info[32], Len;
    if (g_isPlayerInServer[Player] && g_playerRankEntry[Player] > -1 && !g_isPlayerFake[Player] &&
        read_argv(0, Cmd, charsmax(Cmd)) > 2 && (equali(Cmd, "say") || equali(Cmd, "say_team")) &&
        (Len = read_argv(1, Arg, charsmax(Arg))) > 2)
    {
        if (Len > 3 && (equali(Arg, "rank") ||
            ((Arg[0] == '/' || Arg[0] == '!' || Arg[0] == ',' || Arg[0] == '.') &&
            equali(Arg[1], "rank"))))
        {
            num_to_str(get_user_userid(Player), Info, charsmax(Info));
            if (g_areRanksSteamBased)
            {
                if (g_isSpecial)
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "SELECT DISTINCT pPerf FROM pSteamStats WHERE pPerf >= (SELECT pPerf FROM pSteamStats WHERE pEntry = %d) ORDER BY pPerf ASC",
                        g_playerRankEntry[Player]);
                else
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "SELECT DISTINCT pKdr FROM pSteamStats WHERE pKdr >= (SELECT pKdr FROM pSteamStats WHERE pEntry = %d) ORDER BY pKdr ASC",
                        g_playerRankEntry[Player]);
            }
            else
            {
                if (g_isSpecial)
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "SELECT DISTINCT pPerf FROM pNameStats WHERE pPerf >= (SELECT pPerf FROM pNameStats WHERE pEntry = %d) ORDER BY pPerf ASC",
                        g_playerRankEntry[Player]);
                else
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "SELECT DISTINCT pKdr FROM pNameStats WHERE pKdr >= (SELECT pKdr FROM pNameStats WHERE pEntry = %d) ORDER BY pKdr ASC",
                        g_playerRankEntry[Player]);
            }
            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Rank_Cmd", g_Buffer, Info, sizeof Info);
            return g_hideChatCmds ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
        }
        else if (equali(Arg, "top") || equali(Arg, "top10") || equali(Arg, "top15") ||
            ((Arg[0] == '/' || Arg[0] == '!' || Arg[0] == ',' || Arg[0] == '.') &&
            (equali(Arg[1], "top") || equali(Arg[1], "top10") || equali(Arg[1], "top15"))))
        {
            switch (isPlayerViewingAMenu(Player))
            {
                case false:
                {
                    num_to_str(get_user_userid(Player), Info, charsmax(Info));
                    if (g_areRanksSteamBased)
                    {
                        if (g_isSpecial)
                            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Top_Cmd",
                                "SELECT pName, pPerf FROM pSteamStats ORDER BY pPerf DESC LIMIT 63",
                                Info, sizeof Info);
                        else
                            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Top_Cmd",
                                "SELECT pName, pKdr FROM pSteamStats ORDER BY pKdr DESC LIMIT 63",
                                Info, sizeof Info);
                    }
                    else
                    {
                        if (g_isSpecial)
                            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Top_Cmd",
                                "SELECT pName, pPerf FROM pNameStats ORDER BY pPerf DESC LIMIT 63",
                                Info, sizeof Info);
                        else
                            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Top_Cmd",
                                "SELECT pName, pKdr FROM pNameStats ORDER BY pKdr DESC LIMIT 63",
                                Info, sizeof Info);
                    }
                }
                default:
                    client_print(Player, print_chat, "* You're already viewing a menu.");
            }
            return g_hideChatCmds ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
        }
    }
    return PLUGIN_CONTINUE;
}

/**
 * Custom Hamsandwitch forwards.
 */

public On_Player_Killed_Pre(Victim, Killer)
    if (Victim == Killer || Killer < 1 || Killer > g_maxPlayers ||
        !g_isPlayerInServer[Killer])
        set_pdata_int(Victim, m_CBasePlayer_iDeaths,
            get_pdata_int(Victim, m_CBasePlayer_iDeaths) - 1);

/**
 * Custom AMX Mod X forwards.
 */

public OnConCmd_Drop(Player, Level, Command)
{
    if (!cmd_access(Player, Level, Command, 1))
        return PLUGIN_HANDLED;
    if (g_shouldDropTables)
    {
        console_print(Player,
            "* pNameStats and pSteamStats tables are already set to drop after map change.");
        return PLUGIN_HANDLED;
    }
    g_shouldDropTables = true;
    console_print(Player,
        "* pNameStats and pSteamStats tables will drop after map change.");
    if (Player < 1)
    {
        get_user_name  (Player, g_playerName [Player], charsmax(g_playerName []));
        get_user_authid(Player, g_playerSteam[Player], charsmax(g_playerSteam[]));
    }
    log_amx("Cmd: \"%s<%d><%s><>\" sets server drop pSteamStats and pNameStats tables after map change",
        g_playerName[Player], get_user_userid(Player), g_playerSteam[Player]);
    show_activity(Player, g_playerName[Player],
        "sets server drop pSteamStats and pNameStats tables after map change");
    return PLUGIN_HANDLED;
}

public OnConCmd_Zero(Player, Level, Command)
{
    if (!cmd_access(Player, Level, Command, 1))
        return PLUGIN_HANDLED;
    console_print(Player,
        "* pNameStats and pSteamStats tables are being set to zero.");
    SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql",
        "UPDATE pSteamStats SET pKills = 0, pDeaths = 0, pHeads = 0, pScore = 0, pKdr = 0.0, pPerf = 0.0");
    SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql",
        "UPDATE pNameStats SET pKills = 0, pDeaths = 0, pHeads = 0, pScore = 0, pKdr = 0.0, pPerf = 0.0");
    if (Player < 1)
    {
        get_user_name  (Player, g_playerName [Player], charsmax(g_playerName []));
        get_user_authid(Player, g_playerSteam[Player], charsmax(g_playerSteam[]));
    }
    log_amx("Cmd: \"%s<%d><%s><>\" sets pSteamStats and pNameStats tables to zero",
        g_playerName[Player], get_user_userid(Player), g_playerSteam[Player]);
    show_activity(Player, g_playerName[Player],
        "sets pSteamStats and pNameStats tables to zero");
    return PLUGIN_HANDLED;
}

public On_Menu_Top_Players(Player, Menu, Item)
    if (Item != MENU_BACK && Item != MENU_MORE)
        menu_destroy(Menu);

public On_Task_Welcome_Player(taskIdx)
{
    static Player, uniqueUserIndex, Team, Info[32];
    uniqueUserIndex = taskIdx - TaskOfs_Rank_UniqueUserId;
#if defined FindPlayerFlags
    Player = find_player_ex(FindPlayer_IncludeConnecting | FindPlayer_MatchUserId,
        uniqueUserIndex);
#else
    Player = playerByUniqueUserIndex(uniqueUserIndex, false);
#endif
    if (Player < 1)
        return PLUGIN_HANDLED;
    if (!g_isPlayerInServer[Player])
    {
        set_task(2.0, "On_Task_Welcome_Player", taskIdx);
        return PLUGIN_HANDLED;
    }
    Team = get_user_team(Player);
    if (Team < ALLIES || Team > SPECTATORS)
    {
        set_task(2.0, "On_Task_Welcome_Player", taskIdx);
        return PLUGIN_HANDLED;
    }
    if (g_isSpecial)
    {
        if (g_areRanksSteamBased)
            formatex(g_Buffer, charsmax(g_Buffer),
                "SELECT DISTINCT pPerf FROM pSteamStats WHERE pPerf >= (SELECT pPerf FROM pSteamStats WHERE pEntry = %d) ORDER BY pPerf ASC",
                g_playerRankEntry[Player]);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "SELECT DISTINCT pPerf FROM pNameStats WHERE pPerf >= (SELECT pPerf FROM pNameStats WHERE pEntry = %d) ORDER BY pPerf ASC",
                g_playerRankEntry[Player]);
    }
    else
    {
        if (g_areRanksSteamBased)
            formatex(g_Buffer, charsmax(g_Buffer),
                "SELECT DISTINCT pKdr FROM pSteamStats WHERE pKdr >= (SELECT pKdr FROM pSteamStats WHERE pEntry = %d) ORDER BY pKdr ASC",
                g_playerRankEntry[Player]);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "SELECT DISTINCT pKdr FROM pNameStats WHERE pKdr >= (SELECT pKdr FROM pNameStats WHERE pEntry = %d) ORDER BY pKdr ASC",
                g_playerRankEntry[Player]);
    }
    num_to_str(uniqueUserIndex, Info, charsmax(Info));
    SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Rank_Revealing", g_Buffer, Info, sizeof Info);
    return PLUGIN_HANDLED;
}

public On_ObjScore_Message(Index, Destination)
{
    static Player;
    if ((MSG_ALL == Destination || Destination == MSG_BROADCAST) &&
        (!g_requireMinPlayersForRanking || playingPlayers() >= g_minPlayersForRanking))
    {
        Player = get_msg_arg_int(1);
        if (g_playerRankEntry[Player] > -1)
        {
            if (g_isStorageLocal)
            { /// Using multiple "+1" on SQLITE local storage only.
                if (g_areRanksSteamBased)
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pSteamStats SET pScore = pScore + 1, pPerf = (CAST(pKills AS REAL) + CAST(pScore AS REAL) + 1.0) / CAST(MAX(1, pDeaths) AS REAL) WHERE pEntry = %d",
                        g_playerRankEntry[Player]);
                else
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pNameStats SET pScore = pScore + 1, pPerf = (CAST(pKills AS REAL) + CAST(pScore AS REAL) + 1.0) / CAST(MAX(1, pDeaths) AS REAL) WHERE pEntry = %d",
                        g_playerRankEntry[Player]);
            }
            else
            {
                if (g_areRanksSteamBased)
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pSteamStats SET pScore = pScore + 1, pPerf = (CAST(pKills AS DOUBLE) + CAST(pScore AS DOUBLE)) / CAST(GREATEST(1, pDeaths) AS DOUBLE) WHERE pEntry = %d",
                        g_playerRankEntry[Player]);
                else
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pNameStats SET pScore = pScore + 1, pPerf = (CAST(pKills AS DOUBLE) + CAST(pScore AS DOUBLE)) / CAST(GREATEST(1, pDeaths) AS DOUBLE) WHERE pEntry = %d",
                        g_playerRankEntry[Player]);
            }
            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
        }
    }
}

/**
 * Custom SQL forwards.
 */

public dummySql(FailState, Handle: Query, const Error[], ErrorNum)
    if (TQUERY_QUERY_FAILED == FailState)
        log_amx("SQL Error (#%d): %s", ErrorNum, Error);

public On_Sql_Total_Ranks_Revealing(FailState, Handle: Query)
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
        g_totalRanks = SQL_ReadResult(Query, 0);
    else
    { /// If MySQL server down or restarting, retry very fast.
        if (g_areRanksSteamBased)
            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Total_Ranks_Revealing",
                "SELECT COUNT(*) FROM pSteamStats");
        else
            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Total_Ranks_Revealing",
                "SELECT COUNT(*) FROM pNameStats");
    }

public On_Sql_Entry_Selection(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static Player, uniqueUserIndex, taskIdx, Info[32];
    if (Query != Empty_Handle)
    {
        uniqueUserIndex = str_to_num(Data);
#if defined FindPlayerFlags
        Player = find_player_ex(FindPlayer_IncludeConnecting | FindPlayer_MatchUserId,
            uniqueUserIndex);
#else
        Player = playerByUniqueUserIndex(uniqueUserIndex, false);
#endif
        if (Player > 0)
        {
            if (SQL_NumResults(Query) > 0)
            {
                g_playerRankEntry[Player] = SQL_ReadResult(Query, 0);
                if (g_areRanksSteamBased)
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pSteamStats SET pName = '%s', pJoin = %d WHERE pEntry = %d",
                        g_playerCleanName[Player], g_playerJoinUnixTimeStamp[Player],
                        g_playerRankEntry[Player]);
                else
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pNameStats SET pName = '%s', pJoin = %d WHERE pEntry = %d",
                        g_playerCleanName[Player], g_playerJoinUnixTimeStamp[Player],
                        g_playerRankEntry[Player]);
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
                if (g_showWelcomeMsg && !g_isPlayerFake[Player])
                {
                    taskIdx = uniqueUserIndex + TaskOfs_Rank_UniqueUserId;
                    if (task_exists(taskIdx))
                        remove_task(taskIdx);
                    set_task(g_welcomeTaskDelay, "On_Task_Welcome_Player", taskIdx);
                }
            }
            else
            {
                if (g_areRanksSteamBased)
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "INSERT INTO pSteamStats (pName, pSteam, pKills, pDeaths, pHeads, pScore, pSecs, pPerf, pKdr, pJoin) VALUES ('%s', '%s', 0, 0, 0, 0, 0, 0.0, 0.0, %d)",
                        g_playerCleanName[Player], g_playerSteam[Player],
                        g_playerJoinUnixTimeStamp[Player]);
                else
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "INSERT INTO pNameStats (pName, pKills, pDeaths, pHeads, pScore, pSecs, pPerf, pKdr, pJoin) VALUES ('%s', 0, 0, 0, 0, 0, 0.0, 0.0, %d)",
                        g_playerCleanName[Player], g_playerJoinUnixTimeStamp[Player]);
                num_to_str(uniqueUserIndex, Info, charsmax(Info));
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Insertion", g_Buffer,
                    Info, sizeof Info);
            }
        }
    }
    else
    { /// MySQL currently restarting or is offline, try again!
        uniqueUserIndex = str_to_num(Data);
#if defined FindPlayerFlags
        Player = find_player_ex(FindPlayer_IncludeConnecting | FindPlayer_MatchUserId,
            uniqueUserIndex);
#else
        Player = playerByUniqueUserIndex(uniqueUserIndex, false);
#endif
        if (Player > 0)
        {
            if (g_areRanksSteamBased)
            {
                if (g_doRankFakePlayers)
                {
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "SELECT pEntry FROM pSteamStats WHERE pSteam = '%s'",
                        g_playerSteam[Player]);
                    num_to_str(uniqueUserIndex, Info, charsmax(Info));
                    SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection",
                        g_Buffer, Info, sizeof Info);
                }
                else if (!g_isPlayerFake[Player])
                {
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "SELECT pEntry FROM pSteamStats WHERE pSteam = '%s'",
                        g_playerSteam[Player]);
                    num_to_str(uniqueUserIndex, Info, charsmax(Info));
                    SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection",
                        g_Buffer, Info, sizeof Info);
                }
            }
            else
            {
                if (g_doRankFakePlayers)
                {
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "SELECT pEntry FROM pNameStats WHERE pName = '%s'",
                        g_playerCleanName[Player]);
                    num_to_str(uniqueUserIndex, Info, charsmax(Info));
                    SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection",
                        g_Buffer, Info, sizeof Info);
                }
                else if (!g_isPlayerFake[Player])
                {
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "SELECT pEntry FROM pNameStats WHERE pName = '%s'",
                        g_playerCleanName[Player]);
                    num_to_str(uniqueUserIndex, Info, charsmax(Info));
                    SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Entry_Selection",
                        g_Buffer, Info, sizeof Info);
                }
            }
        }
    }
}

public On_Sql_Entry_Insertion(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static Player, uniqueUserIndex, taskIdx;
    if (Query != Empty_Handle && SQL_AffectedRows(Query) > 0)
    {
        if (g_totalRanks > -1)
            ++g_totalRanks;
        uniqueUserIndex = str_to_num(Data);
#if defined FindPlayerFlags
        Player = find_player_ex(FindPlayer_IncludeConnecting | FindPlayer_MatchUserId,
            uniqueUserIndex);
#else
        Player = playerByUniqueUserIndex(uniqueUserIndex, false);
#endif
        if (Player > 0)
        {
            g_playerRankEntry[Player] = SQL_GetInsertId(Query);
            if (g_showWelcomeMsg && !g_isPlayerFake[Player])
            {
                taskIdx = uniqueUserIndex + TaskOfs_Rank_UniqueUserId;
                if (task_exists(taskIdx))
                    remove_task(taskIdx);
                set_task(g_welcomeTaskDelay, "On_Task_Welcome_Player", taskIdx);
            }
        }
    }
}

public On_Sql_Rank_Revealing(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static Player, Msg[192], Float: Perf, perfStr[24], rankStr[24], ranksStr[24];
    if (Query != Empty_Handle)
    {
#if defined FindPlayerFlags
        Player = find_player_ex(FindPlayer_MatchUserId, str_to_num(Data));
#else
        Player = playerByUniqueUserIndex(str_to_num(Data), true);
#endif
        if (Player > 0)
        {
            SQL_ReadResult(Query, 0, Perf);
            addCommasFlt(Perf, 2, perfStr, charsmax(perfStr));
            addCommasInt(SQL_NumResults(Query), rankStr, charsmax(rankStr));
            if (g_totalRanks > -1)
                addCommasInt(g_totalRanks, ranksStr, charsmax(ranksStr));
            else
                copy(ranksStr, charsmax(ranksStr), "?");
            message_begin(MSG_ONE_UNRELIABLE, SVC_DIRECTOR, { 0, 0, 0 }, Player);
            write_byte(
                formatex(
                    Msg, sizeof Msg,
                    "Welcome, %s!\n\nYour rank is %s of %s (%s %s)\nEnjoy our server!",
                    g_playerName[Player], rankStr, ranksStr,
                    g_isSpecial ? "PRF" : "KDR", perfStr
                ) + 31
            );
            fillAndEndDirectorMsg(Msg, 0.2, 0.55);
        }
    }
}

public On_Sql_Top_Cmd(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static Player, uniqueUserIndex, Menu, Name[32], Float: Perf, Entry[96], Rank,
        perfStr[24];
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
    {
        uniqueUserIndex = str_to_num(Data);
#if defined FindPlayerFlags
        Player = find_player_ex(FindPlayer_MatchUserId, uniqueUserIndex);
#else
        Player = playerByUniqueUserIndex(uniqueUserIndex, true);
#endif
        if (Player > 0)
        {
            switch (isPlayerViewingAMenu(Player))
            {
                case false:
                {
                    Rank = 0;
                    if (g_isSpecial)
                        Menu = menu_create("Top Players\\r (by PRF)\\w", "On_Menu_Top_Players");
                    else
                        Menu = menu_create("Top Players\\r (by KDR)\\w", "On_Menu_Top_Players");
                    while (SQL_MoreResults(Query))
                    {
                        SQL_ReadResult(Query, 0, Name, charsmax(Name));
                        SQL_ReadResult(Query, 1, Perf);
                        addCommasFlt(Perf, 2, perfStr, charsmax(perfStr));
                        formatex(Entry, charsmax(Entry), "%02d)\\y %s\\r (%s)",
                            ++Rank, Name, perfStr);
                        menu_additem(Menu, Entry);
                        SQL_NextRow(Query);
                    }
                    menu_display(Player, Menu);
                }
                default:
                    client_print(Player, print_chat, "* You're already viewing a menu.");
            }
        }
    }
}

public On_Sql_Rank_Cmd(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static Player, uniqueUserIndex, Float: Perf, Info[32], perfStr[24], rankStr[24],
        ranksStr[24];
    if (Query != Empty_Handle)
    {
        uniqueUserIndex = str_to_num(Data);
#if defined FindPlayerFlags
        Player = find_player_ex(FindPlayer_MatchUserId, uniqueUserIndex);
#else
        Player = playerByUniqueUserIndex(uniqueUserIndex, true);
#endif
        if (Player > 0)
        {
            SQL_ReadResult(Query, 0, Perf);
            addCommasFlt(Perf, 2, perfStr, charsmax(perfStr));
            addCommasInt(SQL_NumResults(Query), rankStr, charsmax(rankStr));
            if (g_totalRanks > -1)
                addCommasInt(g_totalRanks, ranksStr, charsmax(ranksStr));
            else
                copy(ranksStr, charsmax(ranksStr), "?");
            client_print(Player, print_chat,
                "* Your rank is %s of %s (%s %s)",
                rankStr, ranksStr, g_isSpecial ? "PRF" : "KDR", perfStr);
            if (g_areRanksSteamBased)
                formatex(g_Buffer, charsmax(g_Buffer),
                    "SELECT pKills, pDeaths, pHeads, pScore, pSecs, pPerf, pKdr FROM pSteamStats WHERE pEntry = %d",
                    g_playerRankEntry[Player]);
            else
                formatex(g_Buffer, charsmax(g_Buffer),
                    "SELECT pKills, pDeaths, pHeads, pScore, pSecs, pPerf, pKdr FROM pNameStats WHERE pEntry = %d",
                    g_playerRankEntry[Player]);
            num_to_str(uniqueUserIndex, Info, charsmax(Info));
            SQL_ThreadQuery(g_threadedSqlConnectionTuple, "On_Sql_Rank_Full_Info", g_Buffer, Info, sizeof Info);
        }
    }
}

public On_Sql_Rank_Full_Info(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static Player, uniqueUserIndex, Float: Perf, Float: Kdr, Msg[192], killsStr[24],
        deathsStr[24], headsStr[24], scoreStr[24], perfStr[24], kdrStr[24], timeStr[24];
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
    {
        uniqueUserIndex = str_to_num(Data);
#if defined FindPlayerFlags
        Player = find_player_ex(FindPlayer_MatchUserId, uniqueUserIndex);
#else
        Player = playerByUniqueUserIndex(uniqueUserIndex, true);
#endif
        if (Player > 0)
        {
            SQL_ReadResult(Query, 5, Perf);
            addCommasFlt(Perf, 2, perfStr, charsmax(perfStr));
            SQL_ReadResult(Query, 6, Kdr);
            addCommasFlt(Kdr, 2, kdrStr, charsmax(kdrStr));
            addCommasInt(SQL_ReadResult(Query, 0), killsStr, charsmax(killsStr));
            addCommasInt(SQL_ReadResult(Query, 1), deathsStr, charsmax(deathsStr));
            addCommasInt(SQL_ReadResult(Query, 2), headsStr, charsmax(headsStr));
            addCommasInt(SQL_ReadResult(Query, 3), scoreStr, charsmax(scoreStr));
            addCommasFlt(
                float(SQL_ReadResult(Query, 4)) / 3600.0,
                1, timeStr, charsmax(timeStr)
            );
            message_begin(MSG_ONE_UNRELIABLE, SVC_DIRECTOR, { 0, 0, 0 }, Player);
            write_byte(
                formatex(
                    Msg, sizeof Msg,
                    "K/ D: %s/ %s\nHS: %s\nScore: %s\nPRF: %s\nKDR: %s\nTime: %s h",
                    killsStr, deathsStr, headsStr, scoreStr, perfStr, kdrStr, timeStr
                ) + 31
            );
            fillAndEndDirectorMsg(Msg, 0.55, 0.55);
        }
    }
}

/**
 * DOD forwards.
 */

public client_death(Killer, Victim, Weapon, Place, isTeamKill)
{
    static bool: superKiller;
    if (!g_requireMinPlayersForRanking || playingPlayers() >= g_minPlayersForRanking)
    {
        superKiller =
            Killer != Victim && Killer > 0 && Killer <= g_maxPlayers && g_playerRankEntry[Killer] > -1;
        if (g_areRanksSteamBased)
        {
            if (superKiller)
            {
                if (g_isStorageLocal)
                { /// Using multiple "+1" on SQLITE local storage only.
                    if (HIT_HEAD == Place)
                        formatex(g_Buffer, charsmax(g_Buffer),
                            "UPDATE pSteamStats SET pHeads = pHeads + 1, pKills = pKills + 1, pPerf = (CAST(pKills AS REAL) + 1.0 + CAST(pScore AS REAL)) / CAST(MAX(pDeaths, 1) AS REAL), pKdr = (CAST(pKills AS REAL) + 1.0) / CAST(MAX(pDeaths, 1) AS REAL) WHERE pEntry = %d",
                            g_playerRankEntry[Killer]);
                    else
                        formatex(g_Buffer, charsmax(g_Buffer),
                            "UPDATE pSteamStats SET pKills = pKills + 1, pPerf = (CAST(pKills AS REAL) + 1.0 + CAST(pScore AS REAL)) / CAST(MAX(pDeaths, 1) AS REAL), pKdr = (CAST(pKills AS REAL) + 1.0) / CAST(MAX(pDeaths, 1) AS REAL) WHERE pEntry = %d",
                            g_playerRankEntry[Killer]);
                }
                else
                {
                    if (HIT_HEAD == Place)
                        formatex(g_Buffer, charsmax(g_Buffer),
                            "UPDATE pSteamStats SET pHeads = pHeads + 1, pKills = pKills + 1, pPerf = (CAST(pKills AS DOUBLE) + CAST(pScore AS DOUBLE)) / CAST(GREATEST(pDeaths, 1) AS DOUBLE), pKdr = CAST(pKills AS DOUBLE) / CAST(GREATEST(pDeaths, 1) AS DOUBLE) WHERE pEntry = %d",
                            g_playerRankEntry[Killer]);
                    else
                        formatex(g_Buffer, charsmax(g_Buffer),
                            "UPDATE pSteamStats SET pKills = pKills + 1, pPerf = (CAST(pKills AS DOUBLE) + CAST(pScore AS DOUBLE)) / CAST(GREATEST(pDeaths, 1) AS DOUBLE), pKdr = CAST(pKills AS DOUBLE) / CAST(GREATEST(pDeaths, 1) AS DOUBLE) WHERE pEntry = %d",
                            g_playerRankEntry[Killer]);
                }
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
            }
            if (g_playerRankEntry[Victim] > -1 && (g_isClassic || superKiller))
            { /// Using multiple "+1" on SQLITE local storage only.
                if (g_isStorageLocal)
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pSteamStats SET pDeaths = pDeaths + 1, pPerf = (CAST(pKills AS REAL) + CAST(pScore AS REAL)) / (CAST(pDeaths AS REAL) + 1.0), pKdr = CAST(pKills AS REAL) / (CAST(pDeaths AS REAL) + 1.0) WHERE pEntry = %d",
                        g_playerRankEntry[Victim]);
                else
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pSteamStats SET pDeaths = pDeaths + 1, pPerf = (CAST(pKills AS DOUBLE) + CAST(pScore AS DOUBLE)) / CAST(pDeaths AS DOUBLE), pKdr = CAST(pKills AS DOUBLE) / CAST(pDeaths AS DOUBLE) WHERE pEntry = %d",
                        g_playerRankEntry[Victim]);
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
            }
        }
        else
        {
            if (superKiller)
            {
                if (g_isStorageLocal)
                { /// Using multiple "+1" on SQLITE local storage only.
                    if (HIT_HEAD == Place)
                        formatex(g_Buffer, charsmax(g_Buffer),
                            "UPDATE pNameStats SET pHeads = pHeads + 1, pKills = pKills + 1, pPerf = (CAST(pKills AS REAL) + 1.0 + CAST(pScore AS REAL)) / CAST(MAX(pDeaths, 1) AS REAL), pKdr = (CAST(pKills AS REAL) + 1.0) / CAST(MAX(pDeaths, 1) AS REAL) WHERE pEntry = %d",
                            g_playerRankEntry[Killer]);
                    else
                        formatex(g_Buffer, charsmax(g_Buffer),
                            "UPDATE pNameStats SET pKills = pKills + 1, pPerf = (CAST(pKills AS REAL) + 1.0 + CAST(pScore AS REAL)) / CAST(MAX(pDeaths, 1) AS REAL), pKdr = (CAST(pKills AS REAL) + 1.0) / CAST(MAX(pDeaths, 1) AS REAL) WHERE pEntry = %d",
                            g_playerRankEntry[Killer]);
                }
                else
                {
                    if (HIT_HEAD == Place)
                        formatex(g_Buffer, charsmax(g_Buffer),
                            "UPDATE pNameStats SET pHeads = pHeads + 1, pKills = pKills + 1, pPerf = (CAST(pKills AS DOUBLE) + CAST(pScore AS DOUBLE)) / CAST(GREATEST(pDeaths, 1) AS DOUBLE), pKdr = CAST(pKills AS DOUBLE) / CAST(GREATEST(pDeaths, 1) AS DOUBLE) WHERE pEntry = %d",
                            g_playerRankEntry[Killer]);
                    else
                        formatex(g_Buffer, charsmax(g_Buffer),
                            "UPDATE pNameStats SET pKills = pKills + 1, pPerf = (CAST(pKills AS DOUBLE) + CAST(pScore AS DOUBLE)) / CAST(GREATEST(pDeaths, 1) AS DOUBLE), pKdr = CAST(pKills AS DOUBLE) / CAST(GREATEST(pDeaths, 1) AS DOUBLE) WHERE pEntry = %d",
                            g_playerRankEntry[Killer]);
                }
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
            }
            if (g_playerRankEntry[Victim] > -1 && (g_isClassic || superKiller))
            { /// Using multiple "+1" on SQLITE local storage only.
                if (g_isStorageLocal)
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pNameStats SET pDeaths = pDeaths + 1, pPerf = (CAST(pKills AS REAL) + CAST(pScore AS REAL)) / (CAST(pDeaths AS REAL) + 1.0), pKdr = CAST(pKills AS REAL) / (CAST(pDeaths AS REAL) + 1.0) WHERE pEntry = %d",
                        g_playerRankEntry[Victim]);
                else
                    formatex(g_Buffer, charsmax(g_Buffer),
                        "UPDATE pNameStats SET pDeaths = pDeaths + 1, pPerf = (CAST(pKills AS DOUBLE) + CAST(pScore AS DOUBLE)) / CAST(pDeaths AS DOUBLE), pKdr = CAST(pKills AS DOUBLE) / CAST(pDeaths AS DOUBLE) WHERE pEntry = %d",
                        g_playerRankEntry[Victim]);
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "dummySql", g_Buffer);
            }
        }
    }
}

/**
 * Custom helpers.
 */

playingPlayers()
{
    static Player, Playing, Team;
    for (Playing = 0, Player = 1; Player <= g_maxPlayers; Player++)
    {
        if (g_playerRankEntry[Player] < 0)
            continue;
        if (g_onlyHumans && g_isPlayerFake[Player])
            continue;
        Team = get_user_team(Player);
        if (Team < ALLIES || Team > AXIS)
            continue;
        Playing++;
    }
    return Playing;
}

#if !defined FindPlayerFlags
playerByUniqueUserIndex(uniqueUserIndex, bool: onlyPlayersInServer)
{
    static Player;
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if ((!onlyPlayersInServer || g_isPlayerInServer[Player]) &&
            get_user_userid(Player) == uniqueUserIndex)
            return Player;
    return 0;
}
#endif

fillAndEndDirectorMsg(const Msg[], Float: Horizontal, Float: Vertical)
{
    write_byte(DRC_CMD_MESSAGE);
    write_byte(2  /** Effect type. */);
    write_long(40 /** Blue. */ + (200 /** Green. */ << 8) + (20 /** Red. */ << 16) /** Color. */);
    write_long(unsigned: Horizontal /** Horizontal position. */);
    write_long(unsigned: Vertical   /** Vertical position */   );
    write_long(unsigned: 0.01       /** Fade in time. */       );
    write_long(unsigned: 0.7        /** Fade out time. */      );
    write_long(unsigned: 8.0        /** Hold time. */          );
    write_long(unsigned: 0.07       /** Effect time. */        );
    write_string(Msg);
    message_end();
}

addCommasInt(From, To[], Size)
{
    static absFrom;
    if (From >= 1000000000 || From <= -1000000000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d,%03d,%03d",
            From / 1000000000,
            (absFrom / 1000000) % 1000,
            (absFrom / 1000) % 1000,
            absFrom % 1000);
    }
    else if (From >= 1000000 || From <= -1000000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d,%03d",
            From / 1000000,
            (absFrom / 1000) % 1000,
            absFrom % 1000);
    }
    else if (From >= 1000 || From <= -1000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d",
            From / 1000,
            absFrom % 1000);
    }
    else
        num_to_str(From, To, Size);
}

addCommasFlt(Float: fromFlt, decChrs, To[], Size)
{
    static From, absFrom, Tmp[24], tmpLen, Cpy[8], cpyLen, Chr, bool: doCpy;
    if (decChrs)
    {
        tmpLen = float_to_str(fromFlt, Tmp, charsmax(Tmp));
        for (Chr = 0, cpyLen = 0, doCpy = false; Chr < tmpLen; Chr++)
            switch (doCpy)
            {
                case false:
                    if ('.' == Tmp[Chr])
                    {
                        doCpy = true;
                        Cpy[cpyLen++] = '.';
                    }
                default:
                {
                    Cpy[cpyLen] = Tmp[Chr];
                    if (decChrs == cpyLen++)
                        break;
                }
            }
        Cpy[cpyLen] = 0;
    }
    else
        float_to_str(fromFlt, Tmp, charsmax(Tmp));
    From = str_to_num(Tmp);
    if (From >= 1000000000 || From <= -1000000000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d,%03d,%03d",
            From / 1000000000,
            (absFrom / 1000000) % 1000,
            (absFrom / 1000) % 1000,
            absFrom % 1000);
    }
    else if (From >= 1000000 || From <= -1000000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d,%03d",
            From / 1000000,
            (absFrom / 1000) % 1000,
            absFrom % 1000);
    }
    else if (From >= 1000 || From <= -1000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d",
            From / 1000,
            absFrom % 1000);
    }
    else
        num_to_str(From, To, Size);
    if (decChrs)
        add(To, Size, Cpy);
}

bool: safeIsSqlModuleRunning(const Name[])
{
    new Buffer[256];
    get_localinfo("amxx_modules", Buffer, charsmax(Buffer));
    new File = fopen(Buffer, "r");
    if (!File)
        return false;
    new Len = strlen(Name);
    while (fgets(File, Buffer, charsmax(Buffer)))
    {
        trim(Buffer);
        if (equali(Buffer, Name, Len))
        {
            fclose(File);
            return true;
        }
    }
    fclose(File);
    return false;
}

bool: isPlayerViewingAMenu(Player)
{
    static Old, New;
    player_menu_info(Player, Old, New);
    return Old > 0 || New > -1;
}
