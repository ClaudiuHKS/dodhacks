
#include <amxmodx>
#include <amxmisc>
#include <dodconst>
#include <dodhacks>

new Float: g_AlliesTime; /// Interval in seconds between two Allies or British respawn waves.
new Float: g_AxisTime; /// Interval in seconds between two Axis respawn waves.

public plugin_init()
{
    register_plugin("DoD Hacks: Respawn", "1.0.0.3", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_respawn.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        set_fail_state("Error opening '%s'!", Buffer);
        return PLUGIN_HANDLED;
    }

    new Float: TaskInterval, Map[64], CfgMap[64], Allies[32], Axis[32];
    get_mapname(Map, charsmax(Map));
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/')
            continue;
        if (parse(Buffer, CfgMap, charsmax(CfgMap),
            Allies, charsmax(Allies), Axis, charsmax(Axis)) < 3)
        {
            if (equali(CfgMap, "@task_interval"))
                TaskInterval = str_to_float(Allies);
            else if (equali(CfgMap, "@def_allies_time"))
                g_AlliesTime = str_to_float(Allies);
            else if (equali(CfgMap, "@def_axis_time"))
                g_AxisTime = str_to_float(Allies);
            continue;
        }
        if (equali(Map, CfgMap))
        {
            g_AlliesTime = str_to_float(Allies);
            g_AxisTime = str_to_float(Axis);
            break;
        }
    }
    fclose(Config);
    set_task(TaskInterval, "Alter_RespawnTimes", .flags = "b");
    return PLUGIN_CONTINUE;
}

public Alter_RespawnTimes()
{
    if (DoD_GetWaveTime(ALLIES) > g_AlliesTime) /// Team #1 (Allies)
        DoD_SetWaveTime(ALLIES  /** 1 */, g_AlliesTime);
    if (DoD_GetWaveTime(AXIS)   > g_AxisTime)   /// Team #2 (Axis)
        DoD_SetWaveTime(AXIS    /** 2 */, g_AxisTime);
}
