
#include <amxmodx>

enum ctrlChar
{
    NUL = false,
    SOH, /** Start of heading character. */
    STX,
    ETX,
    EOT,
    ENQ,
    ACK,
    BEL,
    BS,
    HT,  /** Horizontal tabulation character. */
    LF,
    VT,
    FF,
    CR,
    SO,
    SI,
    DLE,
    DC1,
    DC2,
    DC3,
    DC4,
    NAK,
    SYN,
    ETB,
    CAN,
    EM,
    SUB,
    ESC,
    FS,
    GS,
    RS,
    US,
};

new g_cvarHostName;
#if defined hook_cvar_change
new cvarhook: g_hookHostNameConVarChange = cvarhook: -1;
#endif

public plugin_init()
{
    register_plugin("DoD Hacks: Host Name", "1.0.1.1", "Hattrick HKS (claudiuhks)");
    g_cvarHostName = get_cvar_pointer("hostname");
    set_task(0.000001, "Task_PluginCfg");
}

public Task_PluginCfg()
{
    static Name[256];
    get_pcvar_string(g_cvarHostName, Name, charsmax(Name));
    if (ctrlChar: Name[0] > ctrlChar: SOH)
    {
        format(Name, charsmax(Name), "%c%c    %s", ctrlChar: SOH, ctrlChar: HT, Name);
        set_pcvar_string(g_cvarHostName, Name);
    }
#if defined hook_cvar_change
    g_hookHostNameConVarChange = hook_cvar_change(g_cvarHostName, "OnConVarChanged");
#endif
}

#if defined hook_cvar_change
public OnConVarChanged(conVarHandle, const oldValue[], const newValue[])
{
    static Name[256];
    if (ctrlChar: newValue[0] > ctrlChar: SOH)
    {
        formatex(Name, charsmax(Name), "%c%c    %s", ctrlChar: SOH, ctrlChar: HT, newValue);
        disable_cvar_hook(g_hookHostNameConVarChange);
        set_pcvar_string(g_cvarHostName, Name);
        enable_cvar_hook(g_hookHostNameConVarChange);
    }
}
#endif
