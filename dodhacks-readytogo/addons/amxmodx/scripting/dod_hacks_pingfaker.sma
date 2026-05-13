
#include <amxmodx>
#include <amxmisc>
#include <dodhacks>

new g_maxPlayers;
new g_minFakePlayerBasePing;
new g_maxFakePlayerBasePing;
new g_maxFakePlayerBasePingDeletion;
new g_maxFakePlayerBasePingAddition;
new g_maxRealPlayerPing;
new g_maxRealPlayerPingDeletion;
new g_fakePlayerPing[33];
new g_fakePlayerBasePing[33];
new g_realPlayerPing[33];
new bool: g_isPlayerFake[33];
new bool: g_isPlayerInServer[33];
new Float: g_pingUpdateInterval;
new Float: g_realPlayerPingTime[33];
new Float: g_fakePlayerPingTime[33];

public plugin_init()
{
    register_plugin("DoD Hacks: Ping Faker", "1.0.0.4", "claudiuhks (Hattrick HKS)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_pingfaker.ini");
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
        else if (equali(Key, "@update_interval"))
            g_pingUpdateInterval = str_to_float(Val);
        else if (equali(Key, "@max_real_ping"))
            g_maxRealPlayerPing = str_to_num(Val);
        else if (equali(Key, "@max_real_ping_del"))
            g_maxRealPlayerPingDeletion = str_to_num(Val);
        else if (equali(Key, "@min_fake_base_ping"))
            g_minFakePlayerBasePing = str_to_num(Val);
        else if (equali(Key, "@max_fake_base_ping"))
            g_maxFakePlayerBasePing = str_to_num(Val);
        else if (equali(Key, "@max_fake_base_ping_del"))
            g_maxFakePlayerBasePingDeletion = str_to_num(Val);
        else if (equali(Key, "@max_fake_base_ping_add"))
            g_maxFakePlayerBasePingAddition = str_to_num(Val);
    }
    fclose(Config);
    g_maxPlayers = get_maxplayers();
    set_task(taskInterval, "Task_UpdateFakePings", .flags = "b");
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    g_isPlayerFake[Player] = false;
    g_isPlayerInServer[Player] = false;
    g_fakePlayerPing[Player] = 0;
    g_realPlayerPing[Player] = 0;
    g_fakePlayerBasePing[Player] = 0;
    g_realPlayerPingTime[Player] = 0.0;
    g_fakePlayerPingTime[Player] = 0.0;
}

public client_putinserver(Player)
{
    g_isPlayerInServer[Player] = true;
    g_isPlayerFake[Player]     = bool: is_user_bot(Player);
    if (g_isPlayerFake[Player])
    {
        g_fakePlayerPingTime[Player] = get_gametime();
        g_fakePlayerBasePing[Player] = random_num(g_minFakePlayerBasePing, g_maxFakePlayerBasePing);
        g_fakePlayerPing[Player]     = random_num(g_fakePlayerBasePing[Player] - g_maxFakePlayerBasePingDeletion,
                                                  g_fakePlayerBasePing[Player] + g_maxFakePlayerBasePingAddition);
    }
    else
    {
        g_realPlayerPingTime[Player] = get_gametime();
        g_realPlayerPing[Player]     = random_num(g_maxRealPlayerPing - g_maxRealPlayerPingDeletion, g_maxRealPlayerPing);
    }
}

public Task_UpdateFakePings()
{
    static Player, Float: Time;
    Time = get_gametime();
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (g_isPlayerInServer[Player])
            switch (g_isPlayerFake[Player])
            {
                case false:
                    if (Time - g_realPlayerPingTime[Player] >= g_pingUpdateInterval)
                    {
                        g_realPlayerPingTime[Player] = Time;
                        g_realPlayerPing[Player]     = random_num(g_maxRealPlayerPing - g_maxRealPlayerPingDeletion, g_maxRealPlayerPing);
                    }
                default:
                    if (Time - g_fakePlayerPingTime[Player] >= g_pingUpdateInterval)
                    {
                        g_fakePlayerPingTime[Player] = Time;
                        g_fakePlayerPing[Player]     = random_num(g_fakePlayerBasePing[Player] - g_maxFakePlayerBasePingDeletion,
                                                                  g_fakePlayerBasePing[Player] + g_maxFakePlayerBasePingAddition);
                    }
            }
}

public DoD_OnEngine_CalcPing(Player, &Ping)
{
    if (!g_isPlayerInServer[Player])
        return PLUGIN_CONTINUE;
    if (g_isPlayerFake[Player])
        Ping = g_fakePlayerPing[Player];  /// Skips original game engine call.
    else
    {
        DoD_Engine_CalcPing(Player, Ping); /// Calls original game engine call.
        if (Ping > g_maxRealPlayerPing)
            Ping = g_realPlayerPing[Player];
    }
    return PLUGIN_HANDLED;
}
