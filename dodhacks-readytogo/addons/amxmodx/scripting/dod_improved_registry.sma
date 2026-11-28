
#include <amxmodx>
#include <fakemeta>
#include <dodx>

new g_maxPlayers;

new bool: g_inServer[33];
new bool: g_isFake[33];

public plugin_init()
{
    register_plugin("DoD Improved Registry", "1.0", "AlliedModders LLC");

    register_forward(FM_TraceLine, "On_FM_TraceLine_Post", true);

    g_maxPlayers = get_maxplayers();
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
{
    g_inServer[Player] = true;

    g_isFake[Player]   = bool: is_user_bot(Player);
}

public DOD_ON_PLAYER_DISCONNECTED
    g_inServer[Player] = false;

public On_FM_TraceLine_Post(Float: Fr[3], Float: To[3], noMonsters, Idx, Trace)
{
    static Float: Pos[3], Entity, Body, bool: Good;

    if (Idx < 1 || Idx > g_maxPlayers || !g_inServer[Idx] || g_isFake[Idx] ||
        !is_user_alive(Idx) || !(pev(Idx, pev_button) & IN_ATTACK))
        return FMRES_IGNORED;

    Good = get_user_origin_f_3(Idx, Pos);

    if (Good)
        set_tr2(Trace, TR_vecEndPos, Pos);

    get_user_aiming(Idx, Entity, Body);

    if (Entity > 0 && Entity <= g_maxPlayers &&
        g_inServer[Entity] && is_user_alive(Entity))
    {
        set_tr2(Trace, TR_flFraction, 0.1);
        set_tr2(Trace, TR_pHit,       Entity);
        set_tr2(Trace, TR_iHitgroup,  Body);

        if (!Good)
        {
            pev(Entity, pev_origin, Pos);

            set_tr2(Trace, TR_vecEndPos, Pos);
        }
    }

    return FMRES_IGNORED;
}

bool: get_user_origin_f_3(Player, Float: Pos[3])
{
    static Float: Ofs[3], Float: Ang[3], Float: Vec[3], Float: Des[3], Float: Frc;

    pev(Player, pev_origin,   Pos);
    pev(Player, pev_view_ofs, Ofs);
    pev(Player, pev_v_angle,  Ang);

    angle_vector(Ang, ANGLEVECTOR_FORWARD, Vec);

    Pos[0] += Ofs[0];
    Pos[1] += Ofs[1];
    Pos[2] += Ofs[2];

    Des[0]  = Pos[0] + Vec[0] * 9999.0;
    Des[1]  = Pos[1] + Vec[1] * 9999.0;
    Des[2]  = Pos[2] + Vec[2] * 9999.0;

    engfunc(EngFunc_TraceLine, Pos, Des, DONT_IGNORE_MONSTERS, Player, 0);

    get_tr2(0, TR_flFraction, Frc);

    if (Frc < 1.0)
    {
        get_tr2(0, TR_vecEndPos, Pos);

        return true;
    }

    return false;
}
