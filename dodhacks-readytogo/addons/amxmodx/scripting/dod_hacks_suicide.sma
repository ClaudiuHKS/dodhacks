
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>

new bool: g_alwaysBlock;
new bool: g_isBetweenRounds = true;

public plugin_init()
{
    register_plugin("DoD Hacks: Suicide", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_suicide.ini");
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
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key, "@always_block"))
        {
            g_alwaysBlock = bool: str_to_num(Val);
            break;
        }
    }
    fclose(Config);

    register_forward(FM_ClientKill, "OnClient_Kill_Pre");

    register_event("RoundState", "OnRoundBeginFreezeEnd", "a", "1=1"       );
    register_event("RoundState", "OnRoundEnd",            "a", "1=3", "1=4");
    return PLUGIN_CONTINUE;
}

public plugin_natives()
    register_native("DoD_AreUsersBlockedFromSuicide", "Func_AreUsersBlockedFromSuicide");

public Func_AreUsersBlockedFromSuicide()
    return g_alwaysBlock || g_isBetweenRounds;

public OnClient_Kill_Pre(Player)
    return g_alwaysBlock || g_isBetweenRounds ? FMRES_SUPERCEDE : FMRES_IGNORED;

public client_command(Player)
{
    static Cmd[8];
    return (g_alwaysBlock || g_isBetweenRounds) && read_argv(0, Cmd, charsmax(Cmd)) > 3 &&
        equali(Cmd, "kill") ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
}

public OnRoundBeginFreezeEnd()
    g_isBetweenRounds = false;

public OnRoundEnd()
    g_isBetweenRounds = true;
