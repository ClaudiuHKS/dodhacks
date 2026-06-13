
#include <amxmodx>
#include <amxmisc>
#include <dodhacks>

public plugin_init()
{
    register_plugin(
        "DoD Hacks: Death", "1.0.1.0", "Hattrick HKS (claudiuhks)"
    );

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_death.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[32], Float: pevFrameRate, Float: cbaseFrameRate,
        Float: respawnFramesSub, bool: alterCBaseFrameRate,
        bool: enableInterp;
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key,         "@PevFrameRate"))
            pevFrameRate          = str_to_float(Val);
        else if (equali(Key,    "@CBaseFrameRate"))
            cbaseFrameRate        = str_to_float(Val);
        else if (equali(Key,    "@RespawnFramesSub"))
            respawnFramesSub      = str_to_float(Val);
        else if (equali(Key,    "@AlterCBaseFrameRate"))
            alterCBaseFrameRate   = bool: str_to_num(Val);
        else if (equali(Key,    "@EnableInterp"))
            enableInterp          = bool: str_to_num(Val);
    }
    fclose(Config);

    DoD_AddPlayerCorpseManager(
        pevFrameRate, cbaseFrameRate, respawnFramesSub,
        alterCBaseFrameRate, enableInterp
    );
    return PLUGIN_CONTINUE;
}
