
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>

#if !defined SVC_DIRECTOR
#define      SVC_DIRECTOR   51
#endif

#if !defined DRC_CMD_MESSAGE
#define      DRC_CMD_MESSAGE 6
#endif

#define m_CBasePlayerWeapon_iClip  108 /// "int ::CBasePlayerWeapon::m_iClip".
#define Linux_Difference_Ofs_Weapons 4 /// +4 only on Linux. For 'get_pdata_*' funcs. Weapon entities only.

new g_Sync;
new bool: g_Effects;
new bool: g_Director;
new bool: g_isPlayerFake[33];
new bool: g_isPlayerInServer[33];
new Float: g_verPos;
new Float: g_horPos;

public plugin_init()
{
    register_plugin("DoD Hacks: Ammo", "1.0.0.8", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_ammo.ini");
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
            parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;
        if (equali(Key, "@hud_effects"))
            g_Effects = bool: str_to_num(Val);
        else if (equali(Key, "@hud_vertical_pos"))
            g_verPos = str_to_float(Val);
        else if (equali(Key, "@hud_horizontal_pos"))
            g_horPos = str_to_float(Val);
        else if (equali(Key, "@hud_director"))
            g_Director = bool: str_to_num(Val);
    }
    fclose(Config);

    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_kar",           "On_Weapon_PrimaryAttack_Post", true);
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_enfield",       "On_Weapon_PrimaryAttack_Post", true);
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_garand",        "On_Weapon_PrimaryAttack_Post", true);
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_k43",           "On_Weapon_PrimaryAttack_Post", true);
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_spring",        "On_Weapon_PrimaryAttack_Post", true);
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_scopedkar",     "On_Weapon_PrimaryAttack_Post", true);
    RegisterHam(Ham_Weapon_PrimaryAttack, "weapon_scopedenfield", "On_Weapon_PrimaryAttack_Post", true);

    if (!g_Director)
        g_Sync = CreateHudSyncObj();
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
{
    g_isPlayerFake[Player] = bool: is_user_bot(Player);
    g_isPlayerInServer[Player] = true;
}

public DOD_ON_PLAYER_DISCONNECTED
{
    g_isPlayerFake[Player] = false;
    g_isPlayerInServer[Player] = false;
}

public On_Weapon_PrimaryAttack_Post(Weapon)
{
    static Owner;
    static const Msg[] = "LAST BULLET!";
    Owner = pev(Weapon, pev_owner);
    if (Owner > 0 &&
        g_isPlayerInServer[Owner] &&
        !g_isPlayerFake[Owner] &&
        is_user_alive(Owner) &&
        1 == get_pdata_int(Weapon, m_CBasePlayerWeapon_iClip, Linux_Difference_Ofs_Weapons))
    {
        switch (g_Director)
        {
            case false:
            {
                set_hudmessage(180 /** Red. */, 20 /** Green. */, 100 /** Blue. */,
                    g_horPos /** Horizontal position. */, g_verPos /** Vertical position. */,
                    g_Effects ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    0.75 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                ShowSyncHudMsg(Owner, g_Sync, Msg);
            }
            default:
            {
                message_begin(MSG_ONE_UNRELIABLE, SVC_DIRECTOR, { 0, 0, 0 }, Owner);
                write_byte(sizeof(Msg) + 30);
                write_byte(DRC_CMD_MESSAGE);
                write_byte(g_Effects ? 1 : 0 /** Effect type. */); /// 0 = none, 1 = glitter & 2 = text out.
                write_long( /** Color. */
                     100    /** Blue.  */        +
                    ( 20    /** Green. */ <<  8) +
                    (180    /** Red.   */ << 16)
                );
                write_long(unsigned: g_horPos /** Horizontal position. */);
                write_long(unsigned: g_verPos /** Vertical position */   );
                write_long(unsigned: 0.1      /** Fade in time. */       );
                write_long(unsigned: 0.1      /** Fade out time. */      );
                write_long(unsigned: 0.75     /** Hold time. */          );
                write_long(unsigned: 0.5      /** Effect time. */        );
                write_string(Msg);
                message_end();
            }
        }
    }
}
