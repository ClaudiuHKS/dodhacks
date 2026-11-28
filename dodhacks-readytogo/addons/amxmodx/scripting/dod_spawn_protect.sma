
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>

#define TASK_BASE 16960

new Float: g_Time;
new g_Idx[33];
new bool: g_inServer[33];

public plugin_init()
{
    register_plugin("DoD Spawn Protection", "1.0", "AlliedModders LLC");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_spawn_protect.ini");
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
        if (equali(Key, "@protection_time"))
        {
            g_Time = floatmax(0.0, str_to_float(Val));
            break;
        }
    }
    fclose(Config);

    RegisterHam(Ham_Spawn, "player", "On_Ham_Player_Spawn_Post", true);
    return PLUGIN_CONTINUE;
}

public client_putinserver(Player)
{
    g_inServer[Player] = true;
    g_Idx[Player] = get_user_userid(Player);
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    static taskIndex;
    g_inServer[Player] = false;
    taskIndex = TASK_BASE + g_Idx[Player];
    if (task_exists(taskIndex))
        remove_task(taskIndex);
}

public On_Ham_Player_Spawn_Post(Player)
{
    static taskIndex;
    if (!g_inServer[Player])
        return HAM_IGNORED;

    taskIndex = g_Idx[Player] + TASK_BASE;
    if (task_exists(taskIndex))
        remove_task(taskIndex);

    set_pev(Player, pev_takedamage, DAMAGE_NO);
    set_task(g_Time, "Timer_Remove_Player_Protection", taskIndex);
    return HAM_IGNORED;
}

public Timer_Remove_Player_Protection(taskIndex)
{
    static Player;
    Player = find_player("k", taskIndex - TASK_BASE);
    if (Player > 0)
        set_pev(Player, pev_takedamage, DAMAGE_AIM);
}
