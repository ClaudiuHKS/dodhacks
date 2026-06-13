
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <dodx>

#if !defined TE_BEAMPOINTS
    #define  TE_BEAMPOINTS 0
#endif

new g_Life;
new g_Width;
new g_Noise;
new g_Scroll;
new g_Sprite;
new g_Red_Allies;
new g_Red_Axis;
new g_Green_Allies;
new g_Green_Axis;
new g_Blue_Allies;
new g_Blue_Axis;
new g_Brightness_Allies;
new g_Brightness_Axis;
new bool: g_isPlayerFake[33];
new bool: g_isPlayerInServer[33];

public plugin_init()
{
    register_plugin("DoD Hacks: Tracers", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_tracers.ini");
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
        if (equali(Key, "@life"))
            g_Life = str_to_num(Val);
        else if (equali(Key, "@width"))
            g_Width = str_to_num(Val);
        else if (equali(Key, "@noise"))
            g_Noise = str_to_num(Val);
        else if (equali(Key, "@scroll"))
            g_Scroll = str_to_num(Val);
        else if (equali(Key, "@r_allies"))
            g_Red_Allies = str_to_num(Val);
        else if (equali(Key, "@g_allies"))
            g_Green_Allies = str_to_num(Val);
        else if (equali(Key, "@b_allies"))
            g_Blue_Allies = str_to_num(Val);
        else if (equali(Key, "@a_allies"))
            g_Brightness_Allies = str_to_num(Val);
        else if (equali(Key, "@r_axis"))
            g_Red_Axis = str_to_num(Val);
        else if (equali(Key, "@g_axis"))
            g_Green_Axis = str_to_num(Val);
        else if (equali(Key, "@b_axis"))
            g_Blue_Axis = str_to_num(Val);
        else if (equali(Key, "@a_axis"))
            g_Brightness_Axis = str_to_num(Val);
    }
    fclose(Config);
    return PLUGIN_CONTINUE;
}

public plugin_precache()
    g_Sprite = precache_model("sprites/laserbeam.spr");

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
{
    g_isPlayerInServer[Player] = true;
    g_isPlayerFake[Player] = bool: is_user_bot(Player);
}

public DOD_ON_PLAYER_DISCONNECTED
{
    g_isPlayerFake[Player] = false;
    g_isPlayerInServer[Player] = false;
}

public client_death(Killer, Victim, Weapon, Place, isTeamKill)
{
    static Float: killerPos[3], Float: victimPos[3], Float: killerOfs[3], Float: victimOfs[3];
    if (Killer != Victim && !g_isPlayerFake[Victim] && g_isPlayerInServer[Killer] &&
        is_user_alive(Killer) && pev(Killer, pev_origin, killerPos) > 0 &&
        pev(Victim, pev_origin, victimPos) > 0 && pev(Killer, pev_view_ofs, killerOfs) > 0)
    {
        engfunc(EngFunc_MessageBegin, MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, victimPos, Victim);
        write_byte(TE_BEAMPOINTS /** 0 by default. */);
        killerPos[2] += killerOfs[2];
        engfunc(EngFunc_WriteCoord, killerPos[0]);
        engfunc(EngFunc_WriteCoord, killerPos[1]);
        engfunc(EngFunc_WriteCoord, killerPos[2]);
        if (HIT_HEAD == Place && pev(Victim, pev_view_ofs, victimOfs) > 0)
            victimPos[2] += victimOfs[2];
        engfunc(EngFunc_WriteCoord, victimPos[0]);
        engfunc(EngFunc_WriteCoord, victimPos[1]);
        engfunc(EngFunc_WriteCoord, victimPos[2]);
        write_short(g_Sprite);
        write_byte(0);       /// Starting frame.
        write_byte(10);      /// Frame rate.
        write_byte(g_Life);  /// Life (10 means 1 second).
        write_byte(g_Width); /// Width.
        write_byte(g_Noise); /// Noise amplitude.
        if (get_user_team(Killer) == ALLIES)
        { /// Teal.
            write_byte(g_Red_Allies);        /// Red.
            write_byte(g_Green_Allies);      /// Green.
            write_byte(g_Blue_Allies);       /// Blue.
            write_byte(g_Brightness_Allies); /// Brightness.
        }
        else
        { /// Red.
            write_byte(g_Red_Axis);        /// Red.
            write_byte(g_Green_Axis);      /// Green.
            write_byte(g_Blue_Axis);       /// Blue.
            write_byte(g_Brightness_Axis); /// Brightness.
        }
        write_byte(g_Scroll); /// Scroll speed.
        message_end();
    }
}
