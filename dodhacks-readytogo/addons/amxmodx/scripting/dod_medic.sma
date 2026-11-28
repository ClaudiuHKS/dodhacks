
#pragma semicolon 1
#pragma ctrlchar  '\'

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <dodx>

#define TASK_HEAL_BASE 66048

new g_Times;
new Float: g_Step;
new Float: g_Interval;
new g_Idx[33];
new g_Calls[33];
new g_Sample[256];
new bool: g_Sound;
new bool: g_Speak;
new bool: g_isHealing[33];
new bool: g_inServer[33];

public plugin_init()
{
    register_plugin("DoD Medic", "1.3", "AlliedModders LLC");

    register_clcmd("say /medic",      "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");
    register_clcmd("say_team /medic", "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");

    register_clcmd("say medic",       "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");
    register_clcmd("say_team medic",  "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");

    register_clcmd("medic",           "OnPlayerClConCmd_Medic", ADMIN_ALL, "- call a medic");
}

public plugin_precache()
{
    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_medic.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[256];
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;
        if      (equali(Key, "@max_times"))
            g_Times    =       max(1,   str_to_num  (Val));
        else if (equali(Key, "@heal_step"))
            g_Step     = float(max(1,   str_to_num  (Val)));
        else if (equali(Key, "@task_interval"))
            g_Interval =  floatmax(0.1, str_to_float(Val));
        else if (equali(Key, "@wav_sound"))
            g_Sound    =          bool: str_to_num  (Val);
        else if (equali(Key, "@wav_speak"))
            g_Speak    =          bool: str_to_num  (Val);
        else if (equali(Key, "@wav_sample"))
            copy(g_Sample, charsmax(g_Sample),       Val);
    }
    fclose(Config);

    if (g_Sound)
        precache_sound(g_Sample);

    return PLUGIN_CONTINUE;
}

public client_putinserver(Player)
{
    g_Calls[Player]     = 0;
    g_inServer[Player]  = true;
    g_isHealing[Player] = false;
    g_Idx[Player]       = get_user_userid(Player);
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    static taskIndex;

    g_Calls[Player]     = 0;
    g_inServer[Player]  = false;
    g_isHealing[Player] = false;

    taskIndex           = TASK_HEAL_BASE + g_Idx[Player];

    if (task_exists(taskIndex))
        remove_task(taskIndex);
}

public dod_client_spawn(Player)
{
    static taskIndex;

    if (!g_inServer[Player])
        return PLUGIN_CONTINUE;

    g_Calls[Player]     = 0;
    g_isHealing[Player] = false;

    taskIndex           = TASK_HEAL_BASE + g_Idx[Player];

    if (task_exists(taskIndex))
        remove_task(taskIndex);

    return PLUGIN_CONTINUE;
}

public OnPlayerClConCmd_Medic(Player)
{
    static Float: Health, Float: maxHealth, Calls;

    if (!g_inServer[Player])
        return PLUGIN_HANDLED;

    if (!is_user_alive(Player))
    {
        switch (random_num(0, 5))
        {
            case 0:
            {
                client_print(Player, print_chat, "* Nothing we can do.");
                client_print(Player, print_console, "* Nothing we can do.");
            }
            case 1:
            {
                client_print(Player, print_chat, "* Sorry about it.");
                client_print(Player, print_console, "* Sorry about it.");
            }
            case 2:
            {
                client_print(Player, print_chat, "* We're sorry.");
                client_print(Player, print_console, "* We're sorry.");
            }
            case 3:
            {
                client_print(Player, print_chat, "* Doesn't breathe.");
                client_print(Player, print_console, "* Doesn't breathe.");
            }
            case 4:
            {
                client_print(Player, print_chat, "* Pulseless.");
                client_print(Player, print_console, "* Pulseless.");
            }
            default:
            {
                client_print(Player, print_chat, "* Sorry.");
                client_print(Player, print_console, "* Sorry.");
            }
        }
        return PLUGIN_HANDLED;
    }

    if (g_isHealing[Player])
    {
        switch (random_num(0, 4))
        {
            case 0:
            {
                client_print(Player, print_chat, "* Already treating.");
                client_print(Player, print_console, "* Already treating.");
            }
            case 1:
            {
                client_print(Player, print_chat, "* Already working on it.");
                client_print(Player, print_console, "* Already working on it.");
            }
            case 2:
            {
                client_print(Player, print_chat, "* Already taking care of it.");
                client_print(Player, print_console, "* Already taking care of it.");
            }
            case 3:
            {
                client_print(Player, print_chat, "* Already healing.");
                client_print(Player, print_console, "* Already healing.");
            }
            default:
            {
                client_print(Player, print_chat, "* Already bandaging.");
                client_print(Player, print_console, "* Already bandaging.");
            }
        }
        return PLUGIN_HANDLED;
    }

    if (g_Calls[Player] >= g_Times)
    {
        switch (random_num(0, 1))
        {
            case 0:
            {
                client_print(Player, print_chat, "* Medic available only %d %s each life.",
                    g_Times, 1 == g_Times ? "time" : "times");
                client_print(Player, print_console, "* Medic available only %d %s each life.",
                    g_Times, 1 == g_Times ? "time" : "times");
            }
            default:
            {
                client_print(Player, print_chat, "* Calls available only %d %s each life.",
                    g_Times, 1 == g_Times ? "time" : "times");
                client_print(Player, print_console, "* Calls available only %d %s each life.",
                    g_Times, 1 == g_Times ? "time" : "times");
            }
        }
        return PLUGIN_HANDLED;
    }

    pev(Player, pev_health, Health);
    pev(Player, pev_max_health, maxHealth);
    if (Health >= maxHealth)
    {
        switch (random_num(0, 3))
        {
            case 0:
            {
                client_print(Player, print_chat, "* Already healthy.");
                client_print(Player, print_console, "* Already healthy.");
            }
            case 1:
            {
                client_print(Player, print_chat, "* Already treating someone.");
                client_print(Player, print_console, "* Already treating someone.");
            }
            case 2:
            {
                client_print(Player, print_chat, "* Try again later.");
                client_print(Player, print_console, "* Try again later.");
            }
            default:
            {
                client_print(Player, print_chat, "* Busy.");
                client_print(Player, print_console, "* Busy.");
            }
        }
        return PLUGIN_HANDLED;
    }

    g_Calls[Player]++;
    g_isHealing[Player] = true;
    Calls               = g_Times - g_Calls[Player];

    switch (random_num(0, 2))
    {
        case 0:
        {
            client_print(Player, print_chat, "* Bandaging! %d %s remaining.",
                Calls, 1 == Calls ? "call" : "calls");
            client_print(Player, print_console, "* Bandaging! %d %s remaining.",
                Calls, 1 == Calls ? "call" : "calls");
        }
        case 1:
        {
            client_print(Player, print_chat, "* Healing! %d %s remaining.",
                Calls, 1 == Calls ? "call" : "calls");
            client_print(Player, print_console, "* Healing! %d %s remaining.",
                Calls, 1 == Calls ? "call" : "calls");
        }
        default:
        {
            client_print(Player, print_chat, "* Treating! %d %s remaining.",
                Calls, 1 == Calls ? "call" : "calls");
            client_print(Player, print_console, "* Treating! %d %s remaining.",
                Calls, 1 == Calls ? "call" : "calls");
        }
    }

    if (g_Sound)
    {
        if (g_Speak)
            client_cmd(Player, "spk \"%s\"", g_Sample);

        else
            emit_sound(Player, CHAN_ITEM, g_Sample, VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
    }

    set_task(g_Interval, "On_Timer_Progressive_Healing", TASK_HEAL_BASE + g_Idx[Player], "", 0, "b");
    return PLUGIN_HANDLED;
}

public On_Timer_Progressive_Healing(taskIndex)
{
    static Player, Float: Health, Float: maxHealth;
    Player = find_player("k", taskIndex - TASK_HEAL_BASE);

    if (Player < 1)
        return PLUGIN_HANDLED;

    if (!is_user_alive(Player))
    {
        g_Calls[Player]     = 0;
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

    Health = floatmin(maxHealth, Health + g_Step);

    set_pev(Player, pev_health, Health);

    if (Health >= maxHealth)
    {
        g_isHealing[Player] = false;

        if (task_exists(taskIndex))
            remove_task(taskIndex);
    }

    return PLUGIN_HANDLED;
}
