
#include <amxmodx>
#include <dodhacks>

public plugin_init()
{
    register_plugin("DoD Hacks: Deploy", "1.0.1.0", "Hattrick HKS (claudiuhks)");
    DoD_AddAdvancedDeploy();
}
