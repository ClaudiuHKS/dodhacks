
#include <amxmodx>

public plugin_init()
    register_plugin("DoD Hacks: Meta", "1.0.1.1", "Hattrick HKS (claudiuhks)");

public client_command(Player)
{
    static Cmd[8];
    return read_argv(0, Cmd, charsmax(Cmd)) > 3 && equali("meta", Cmd) ?
        PLUGIN_HANDLED : PLUGIN_CONTINUE; /// Block "Unknown cmd: 'meta'." con. line.
}
