
#include <amxmodx>
#include <amxmisc>

new g_maxBrightness;

public plugin_init()
{
    register_plugin("DoD Hacks: Screen", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_screen.ini");
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
        if (equali(Key, "@max_brightness"))
        {
            g_maxBrightness = str_to_num(Val);
            break;
        }
    }
    fclose(Config);
    register_message(get_user_msgid("ScreenFade"), "On_ScreenFade_Message");
    return PLUGIN_CONTINUE;
}

public On_ScreenFade_Message()
{
    if (7 != get_msg_args() || ARG_SHORT != get_msg_argtype(1) ||
        ARG_SHORT != get_msg_argtype(2) || ARG_SHORT != get_msg_argtype(3) ||
        ARG_BYTE != get_msg_argtype(4) || ARG_BYTE != get_msg_argtype(5) ||
        ARG_BYTE != get_msg_argtype(6) || ARG_BYTE != get_msg_argtype(7))
        return PLUGIN_CONTINUE;
    if (get_msg_arg_int(7) > g_maxBrightness)
        set_msg_arg_int(7, ARG_BYTE, g_maxBrightness);
    return PLUGIN_CONTINUE;
}
