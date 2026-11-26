
#include <amxmodx>
#include <fakemeta>
#include <dodx>

#define PLUGIN  "DoD Medic"
#define VERSION "1.3"
#define AUTHOR  "AlliedModders LLC"

#define TASK_HEAL_BASE 16672

new HEAL_TIMES           =   2;

new Float: HEAL_STEP     = 9.0;
new Float: HEAL_INTERVAL = 0.3;

new g_Calls[33];

new bool: g_isHealing[33];
new bool: g_inServer[33];

new bool: g_isAuto = false;

public plugin_init()
{
    register_plugin(PLUGIN, VERSION, AUTHOR);

    register_clcmd("say /medic",      "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");
    register_clcmd("say_team /medic", "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");

    register_clcmd("say medic",       "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");
    register_clcmd("say_team medic",  "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");

    register_clcmd("medic",           "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");
}

public plugin_cfg() /// Do not allow two plugins at a time. Only one can be enabled.
    g_isAuto = -1 != is_plugin_loaded("DoD Hacks: Auto Medic");

public client_putinserver(Player)
{
    g_inServer[Player] = true;

    zeroPlayer(Player);
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    zeroPlayer(Player);

    g_inServer[Player] = false;
}

public dod_client_spawn(Player)
{
    if (g_inServer[Player])
        zeroPlayer(Player);
}

zeroPlayer(Player)
{
    static taskIndex;

    g_Calls[Player]     = 0;
    g_isHealing[Player] = false;

    taskIndex           = TASK_HEAL_BASE + Player;

    if (task_exists(taskIndex))
        remove_task(taskIndex);
}

public OnPlayerClConCmd_Medic(Player)
{
    static Float: Health, Float: maxHealth, Calls;

    if (g_isAuto || !g_inServer[Player])
        return PLUGIN_HANDLED;

    if (!is_user_alive(Player))
    {
        client_print(Player, print_chat, "[Medic] You're dead and can't call a medic.");
        client_print(Player, print_console, "* You're dead and can't call a medic.");
        return PLUGIN_HANDLED;
    }

    if (g_isHealing[Player])
    {
        client_print(Player, print_chat, "[Medic] The medic is already treating you.");
        client_print(Player, print_console, "* The medic is already treating you.");
        return PLUGIN_HANDLED;
    }

    if (g_Calls[Player] >= HEAL_TIMES)
    {
        client_print(Player, print_chat, "[Medic] Can only call a medic %d %s every life.",
            HEAL_TIMES, 1 == HEAL_TIMES ? "time" : "times");
        client_print(Player, print_console, "* Can only call a medic %d %s every life.",
            HEAL_TIMES, 1 == HEAL_TIMES ? "time" : "times");
        return PLUGIN_HANDLED;
    }

    pev(Player, pev_health, Health);
    pev(Player, pev_max_health, maxHealth);
    if (Health >= maxHealth)
    {
        client_print(Player, print_chat, "[Medic] You're already healthy (%.0f HP).", Health);
        client_print(Player, print_console, "* You're already healthy (%.0f HP).", Health);
        return PLUGIN_HANDLED;
    }

    g_Calls[Player]++;
    g_isHealing[Player] = true;
    Calls               = HEAL_TIMES - g_Calls[Player];

    client_print(Player, print_chat, "[Medic] Healing started! %d %s remaining.",
        Calls, 1 == Calls ? "call" : "calls");
    client_print(Player, print_console, "* Healing started! %d %s remaining.",
        Calls, 1 == Calls ? "call" : "calls");

    set_task(HEAL_INTERVAL, "On_Timer_Progressive_Healing", TASK_HEAL_BASE + Player, "", 0, "b");
    return PLUGIN_HANDLED;
}

public On_Timer_Progressive_Healing(taskIndex)
{
    static Player, Float: Health, Float: maxHealth;
    Player = taskIndex - TASK_HEAL_BASE;

    if (!g_inServer[Player] || !g_isHealing[Player] || !is_user_alive(Player))
    {
        g_isHealing[Player] = false;

        if (task_exists(taskIndex))
            remove_task(taskIndex);

        return PLUGIN_HANDLED;
    }

    pev(Player, pev_health, Health);
    pev(Player, pev_max_health, maxHealth);
    if (Health >= maxHealth)
    {
        g_isHealing[Player] = false;

        if (task_exists(taskIndex))
            remove_task(taskIndex);

        return PLUGIN_HANDLED;
    }

    Health = floatmin(maxHealth, Health + HEAL_STEP);

    set_pev(Player, pev_health, Health);

    if (Health >= maxHealth)
    {
        g_isHealing[Player] = false;

        if (task_exists(taskIndex))
            remove_task(taskIndex);
    }

    return PLUGIN_HANDLED;
}
