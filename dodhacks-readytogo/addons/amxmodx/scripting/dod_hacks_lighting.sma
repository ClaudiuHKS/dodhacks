
#pragma ctrlchar '\' /// Replaces '^' with '\'.

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>

#define TaskOffs_Lighting 2064384 /** -32768+2^21 value. A value other plugins do not use. */

new g_lightingHook;
new g_Lighting[2] = { '?', EOS };

public plugin_init()
{
    register_plugin("DoD Hacks: Lighting", "1.0.1.0", "HKS Hattrick (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_lighting.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[64], Val[8], Map[64],
        genLighting[2] = { '?', EOS }, Lighting[2] = { '?', EOS };
    get_mapname(Map, charsmax(Map));
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key, "@general"))
        {
            strtolower(Val);
            copy(genLighting, charsmax(genLighting), Val);
        }
        else if (equali(Key, Map))
        {
            strtolower(Val);
            copy(Lighting, charsmax(Lighting), Val);
            break;
        }
    }
    fclose(Config);
    if (isalpha(Lighting[0]))         /// Custom map lighting desired.
        set_task(0.000001, "Task_Lighting", TaskOffs_Lighting, Lighting,
            sizeof Lighting);
    else if (isalpha(genLighting[0])) /// Custom general lighting desired.
        set_task(0.000001, "Task_Lighting", TaskOffs_Lighting, genLighting,
            sizeof genLighting);
    register_concmd("amx_lighting", "adminConCmd_Lighting", ADMIN_CVAR,
        "<a (darkest) ... z (lightest)> - sets map lighting");
    return PLUGIN_CONTINUE;
}

public plugin_precache()
    g_lightingHook = register_forward(FM_LightStyle, "OnLightStyle_Post", true);

public OnLightStyle_Post(Style, const Lighting[])
    if (false == bool: Style)
        copy(g_Lighting, charsmax(g_Lighting), Lighting);

public Task_Lighting(const Lighting[])
{
    unregister_forward(FM_LightStyle, g_lightingHook, true);
    engfunc(EngFunc_LightStyle, false, Lighting);
    copy(g_Lighting, charsmax(g_Lighting), Lighting);
}

public adminConCmd_Lighting(Player, Level, Command)
{
    if (!cmd_access(Player, Level, Command, 2))
    {
        console_print(Player, "* Actual map lighting is set to \"%s\".",
            g_Lighting);
        return PLUGIN_HANDLED;
    }
    new Lighting[2];
    if (read_argv(1, Lighting, charsmax(Lighting)) < 1 || !isalpha(Lighting[0]))
    {
        console_print(Player, "* Map lighting may be from a to z.");
        return PLUGIN_HANDLED;
    }
    strtolower(Lighting);
    engfunc(EngFunc_LightStyle, false, Lighting);
    new Steam[32], Name[32];
    get_user_authid(Player, Steam, charsmax(Steam));
    get_user_name(Player, Name, charsmax(Name));
    log_amx("Cmd: \"%s<%d><%s><>\" sets map lighting (from \"%s\" to \"%s\")",
        Name, get_user_userid(Player), Steam, g_Lighting, Lighting);
    console_print(Player, "* Replaced map lighting from \"%s\" to \"%s\".",
        g_Lighting, Lighting);
    show_activity(Player, Name, "replaces map lighting from \"%s\" to \"%s\"",
        g_Lighting, Lighting);
    copy(g_Lighting, charsmax(g_Lighting), Lighting);
    return PLUGIN_HANDLED;
}
