
#include <amxmodx>
#include <amxmisc>
#include <dodfun>

#if !defined SPECTATORS
#define      SPECTATORS 3
#endif

new g_Smoke;
new g_maxPlayers;
new bool: g_onlySelf;
new bool: g_isPlayerFake[33];
new bool: g_isPlayerInServer[33];

public plugin_init()
{
    register_plugin("DoD Hacks: Trails", "1.0.1.1", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_trails.ini");
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
        if (equali("@only_self", Key))
        {
            g_onlySelf = bool: str_to_num(Val);
            break;
        }
    }
    fclose(Config);

    g_maxPlayers = get_maxplayers();
    return PLUGIN_CONTINUE;
}

public plugin_precache()
    g_Smoke = precache_model("sprites/smoke.spr");

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

public grenade_throw(Player, nadeEntity, nadeIndex)
{
    static Other, Team, otherTeam;
    if (g_onlySelf)
    {
        if (!g_isPlayerFake[Player])
            addBeamFollow(Player, get_user_team(Player), nadeEntity);
    }
    else
        for (Other = 1, Team = get_user_team(Player); Other <= g_maxPlayers; Other++)
            if (Player == Other)
            {
                if (!g_isPlayerFake[Player])
                    addBeamFollow(Player, Team, nadeEntity);
            }
            else if (g_isPlayerInServer[Other] && !g_isPlayerFake[Other])
            {
                otherTeam = get_user_team(Other);
                if (otherTeam == Team || otherTeam == SPECTATORS)
                    addBeamFollow(Other, Team, nadeEntity);
            }
}

addBeamFollow(Player, Team, Entity)
{
    message_begin(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, { 0, 0, 0 }, Player);
    write_byte(TE_BEAMFOLLOW);
    write_short(Entity);
    write_short(g_Smoke);
    write_byte(10); /// Life.
    write_byte( 1); /// Width.
    switch (Team)
    {
        case AXIS:
        { /// Red.
            write_byte(200); /// Red.
            write_byte( 20); /// Green.
            write_byte( 20); /// Blue.
        }
        default:
        { /// Teal.
            write_byte( 20); /// Red.
            write_byte(180); /// Green.
            write_byte(120); /// Blue.
        }
    }
    write_byte(200); /// Brightness.
    message_end();
}
