
#include <amxmodx>
#include <amxmisc>
#include <dodhacks>

new m_flRestartTime;
new Float: g_roundEndTimeAdjustmentSeconds;

public plugin_init()
{
    register_plugin("DoD Hacks: Round", "1.0.1.1", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_round.ini");
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
        if (equali(Key, "@roundEndTimeAdjusSeconds"))
            g_roundEndTimeAdjustmentSeconds = str_to_float(Val);
        else if (equali(Key, "@m_flRestartTime"))
            m_flRestartTime = str_to_num(Val);
    }
    fclose(Config);

    register_event("RoundState", "roundEnded", "a", "1=3", "1=4");
    return PLUGIN_CONTINUE;
}

public roundEnded()
    set_task(0.000001, "adjustRoundEndTimeSeconds");

public adjustRoundEndTimeSeconds()
{
    static Float: Time;
    Time = DoD_ReadGameRulesFloat(m_flRestartTime);
    if (Time >= get_gametime())
        DoD_StoreGameRulesFloat(m_flRestartTime, Time + g_roundEndTimeAdjustmentSeconds);
}
