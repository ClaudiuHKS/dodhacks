
#include <amxmodx>
#include <fakemeta>

#define New_ScoreInfoLong "ScrInfoLong" /** swds.dll/ engine_i486.so allows max. 11 characters message names. */

public plugin_init()
    register_plugin("ScoreInfoLong Msg. Fix", "1.0.0.0", "Hattrick HKS (claudiuhks)");

public plugin_precache()
    register_forward(FM_RegUserMsg, "OnRegUserMsg_Pre");

public OnRegUserMsg_Pre(const messageName[], messageSize)
{
    if (equali("ScoreInfoLong", messageName))
    {
        forward_return(FMV_CELL, engfunc(EngFunc_RegUserMsg, New_ScoreInfoLong, messageSize));
        return FMRES_SUPERCEDE;
    }
    return FMRES_IGNORED;
}
