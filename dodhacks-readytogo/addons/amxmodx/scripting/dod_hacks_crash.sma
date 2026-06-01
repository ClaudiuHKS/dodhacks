
#include <amxmodx>
#include <amxmisc>

#define TaskOffs_QueryClientCvar    67108736 /// -128+2^26 value will be used as player unique user index task offset.
#define TaskOffs_ClientPutInServer   8388735 ///  127+2^23 value will be used as player unique user index task offset.

new g_playerUniqueUserIndex[33] = { -1, ... };
new bool: g_isPlayerInServer[33] = { false, ... };
new Array: g_playerNotifications[33] = { Invalid_Array, ... };
new Float: g_minNotifySeconds;
new Float: g_maxNotifySeconds;

public plugin_init()
{
    register_plugin("DoD Hacks: Crash", "1.0.0.6", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_crash.ini");
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
        if (equali(Key, "@min_notify_sec"))
            g_minNotifySeconds = str_to_float(Val);
        else if (equali(Key, "@max_notify_sec"))
            g_maxNotifySeconds = str_to_float(Val);
    }
    fclose(Config);
    return PLUGIN_CONTINUE;
}

public client_putinserver(Player)
{
    g_isPlayerInServer[Player] = true;
    g_playerNotifications[Player] = Invalid_Array;
    g_playerUniqueUserIndex[Player] = get_user_userid(Player);
    if (!is_user_bot(Player))
        set_task(0.000001, "Task_QueryPlayerConVars_Delayed",
            TaskOffs_ClientPutInServer + g_playerUniqueUserIndex[Player]);
}

public Task_QueryPlayerConVars_Delayed(taskIndex)
{
    static Player, Arg[1];
    Arg[0] = taskIndex - TaskOffs_ClientPutInServer;
#if defined FindPlayerFlags
    Player = find_player_ex(FindPlayer_MatchUserId, Arg[0]);
#else
    Player = readPlayerFromUniqueUserIndex(Arg[0]);
#endif
    if (g_isPlayerInServer[Player])
    {
        query_client_cvar(Player, "mp_decals",     "onPlayerConVarCheck", sizeof Arg, Arg);
        query_client_cvar(Player, "sp_decals",     "onPlayerConVarCheck", sizeof Arg, Arg);
        query_client_cvar(Player, "cl_particlefx", "onPlayerConVarCheck", sizeof Arg, Arg);
    }
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    static taskIndex;
    taskIndex = g_playerUniqueUserIndex[Player] + TaskOffs_QueryClientCvar;
    if (task_exists(taskIndex))
        remove_task(taskIndex);
    taskIndex = g_playerUniqueUserIndex[Player] + TaskOffs_ClientPutInServer;
    if (task_exists(taskIndex))
        remove_task(taskIndex);
    g_isPlayerInServer[Player] = false;
    g_playerUniqueUserIndex[Player] = -1;
    if (Invalid_Array != g_playerNotifications[Player])
    {
        ArrayDestroy(g_playerNotifications[Player]);
        g_playerNotifications[Player] = Invalid_Array;
    }
}

public Task_NotifyPlayer_Delayed(taskIndex)
{
    static Player, Msg[128];
#if defined FindPlayerFlags
    Player = find_player_ex(FindPlayer_MatchUserId, taskIndex - TaskOffs_QueryClientCvar);
#else
    Player = readPlayerFromUniqueUserIndex(taskIndex - TaskOffs_QueryClientCvar);
#endif
    if (g_isPlayerInServer[Player])
    {
        ArrayGetString(g_playerNotifications[Player],  0, Msg, charsmax(Msg));
        ArrayDeleteItem(g_playerNotifications[Player], 0);
        client_print(Player, print_chat, Msg);
    }
    else if (Invalid_Array != g_playerNotifications[Player])
    {
        ArrayDestroy(g_playerNotifications[Player]);
        g_playerNotifications[Player] = Invalid_Array;
    }
}

public onPlayerConVarCheck(Player, const Key[], const Val[], const Arg[])
{
    static Value, Msg[128];
#if defined FindPlayerFlags
    Player = find_player_ex(FindPlayer_MatchUserId, Arg[0]);
#else
    Player = readPlayerFromUniqueUserIndex(Arg[0]);
#endif
    if (g_isPlayerInServer[Player])
        switch (Key[0])
        {
            case 's', 'S':
            {
                Value = str_to_num(Val);
                if (Value < 1024)
                {
                    formatex(Msg, charsmax(Msg),
                        "* Your '%s' cvar is a bit low (%d/ 4096). If you crash, consider increasing.",
                        Key, Value);
                    createPlayerMsgArrayIfNeeded(Player, sizeof Msg);
                    ArrayPushString(g_playerNotifications[Player], Msg);
                    set_task(random_float(g_minNotifySeconds, g_maxNotifySeconds), "Task_NotifyPlayer_Delayed",
                        g_playerUniqueUserIndex[Player] + TaskOffs_QueryClientCvar);
                }
            }
            case 'm', 'M':
            {
                Value = str_to_num(Val);
                if (Value < 1024)
                {
                    formatex(Msg, charsmax(Msg),
                        "* Your '%s' cvar is a bit low (%d/ 4096). If you crash, consider increasing.",
                        Key, Value);
                    createPlayerMsgArrayIfNeeded(Player, sizeof Msg);
                    ArrayPushString(g_playerNotifications[Player], Msg);
                    set_task(random_float(g_minNotifySeconds, g_maxNotifySeconds), "Task_NotifyPlayer_Delayed",
                        g_playerUniqueUserIndex[Player] + TaskOffs_QueryClientCvar);
                }
            }
            case 'c', 'C':
            {
                Value = str_to_num(Val);
                if (Value > 0)
                {
                    formatex(Msg, charsmax(Msg),
                        "* Your '%s' cvar is turned on (%d/ 2). If you crash, consider disabling.",
                        Key, Value);
                    createPlayerMsgArrayIfNeeded(Player, sizeof Msg);
                    ArrayPushString(g_playerNotifications[Player], Msg);
                    set_task(random_float(g_minNotifySeconds, g_maxNotifySeconds), "Task_NotifyPlayer_Delayed",
                        g_playerUniqueUserIndex[Player] + TaskOffs_QueryClientCvar);
                }
            }
        }
}

createPlayerMsgArrayIfNeeded(Player, Size)
    if (Invalid_Array == g_playerNotifications[Player])
        g_playerNotifications[Player] = ArrayCreate(Size);

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
