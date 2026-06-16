
#pragma ctrlchar '\' /// Replace all '^' with '\'.

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>

new g_adminFlag;
new g_maxPlayers;
new Float: g_maxSeconds;
new Float: g_playerSeconds[33];
new Float: g_playerOrigin[33][3];
new Float: g_playerViewAng[33][3];
new bool: g_includeFakePlayers;
new bool: g_isPlayerFake[33];
new bool: g_isPlayerAlive[33];
new bool: g_isPlayerWarned[33];
new bool: g_isPlayerInServer[33];

public plugin_init()
{
    register_plugin("DoD Hacks: AFK", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_afk.ini");
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
        if (equali(Key, "@include_fake_players"))
            g_includeFakePlayers = bool: str_to_num(Val);
        else if (equali(Key, "@afk_seconds"))
            g_maxSeconds = floatmax(15.0, str_to_float(Val));
        else if (equali(Key, "@admin_flag"))
            g_adminFlag = read_flags(Val);
    }
    fclose(Config);

    g_maxPlayers = get_maxplayers();
    set_task(5.0, "Task_AFK", .flags = "b");
#if defined amxclient_cmd && defined RegisterHamPlayer
    RegisterHamPlayer(Ham_Killed, "On_Player_Killed_Post", true);
    RegisterHamPlayer(Ham_Spawn, "On_Player_Spawn_Post", true);
#else
    RegisterHam(Ham_Killed, "player", "On_Player_Killed_Post", true);
    RegisterHam(Ham_Spawn, "player", "On_Player_Spawn_Post", true);
#endif
    return PLUGIN_CONTINUE;
}

public client_connect(Player)
    g_isPlayerAlive[Player] = false;

public client_putinserver(Player)
{
    g_isPlayerFake[Player] = bool: is_user_bot(Player);
    g_isPlayerInServer[Player] = true;
    g_isPlayerWarned[Player] = false;
    g_playerSeconds[Player] = 0.0;
    rmVec3(g_playerOrigin[Player]);
    rmVec3(g_playerViewAng[Player]);
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    g_isPlayerFake[Player] = false;
    g_isPlayerAlive[Player] = false;
    g_isPlayerInServer[Player] = false;
    g_isPlayerWarned[Player] = false;
    g_playerSeconds[Player] = 0.0;
    rmVec3(g_playerOrigin[Player]);
    rmVec3(g_playerViewAng[Player]);
}

public On_Player_Killed_Post(Player)
{
    g_isPlayerAlive[Player] = false;
    g_isPlayerWarned[Player] = false;
    g_playerSeconds[Player] = 0.0;
    rmVec3(g_playerOrigin[Player]);
    rmVec3(g_playerViewAng[Player]);
}

public On_Player_Spawn_Post(Player)
    if (g_isPlayerInServer[Player])
    {
        g_isPlayerAlive[Player] = true;
        g_isPlayerWarned[Player] = false;
        g_playerSeconds[Player] = 0.0;
        rmVec3(g_playerOrigin[Player]);
        rmVec3(g_playerViewAng[Player]);
    }

public client_command(Player)
    if (g_isPlayerInServer[Player])
    {
        g_isPlayerWarned[Player] = false;
        g_playerSeconds[Player] = 0.0;
        rmVec3(g_playerOrigin[Player]);
        rmVec3(g_playerViewAng[Player]);
    }

public Task_AFK()
{
    static Player, Float: Origin[3], Float: viewAng[3];
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (g_isPlayerInServer[Player] &&
            (g_includeFakePlayers || !g_isPlayerFake[Player]) &&
            (!g_adminFlag || g_adminFlag != (get_user_flags(Player) & g_adminFlag)))
            switch (g_isPlayerAlive[Player])
            {
                case false:
                {
                    g_isPlayerWarned[Player] = false;
                    g_playerSeconds[Player] = 0.0;
                    rmVec3(g_playerOrigin[Player]);
                    rmVec3(g_playerViewAng[Player]);
                }
                default:
                {
                    pev(Player, pev_v_angle, viewAng);
                    pev(Player, pev_origin, Origin);
                    if (eqVec3(viewAng, g_playerViewAng[Player], false) &&
                        eqVec3(Origin, g_playerOrigin[Player], true))
                    {
                        g_playerSeconds[Player] += 5.0;
                        if (g_playerSeconds[Player] >= g_maxSeconds)
                        {
                            engclient_cmd(Player, "jointeam", "3");
                            g_isPlayerWarned[Player] = false;
                            g_playerSeconds[Player] = 0.0;
                            rmVec3(g_playerOrigin[Player]);
                            rmVec3(g_playerViewAng[Player]);
                        }
                        else if (!g_isPlayerFake[Player] &&
                            !g_isPlayerWarned[Player] &&
                            10.0 + g_playerSeconds[Player] >= g_maxSeconds)
                        {
                            client_print(Player, print_chat,
                                "* Move, or you'll auto. join Spectators (AFK).");
                            client_cmd(Player,
                                "spk \"vox/deeoo _comma may you move now please\"");
                            g_isPlayerWarned[Player] = true;
                        }
                    }
                    else
                    {
                        g_isPlayerWarned[Player] = false;
                        g_playerSeconds[Player] = 0.0;
                        cpVec3(g_playerViewAng[Player], viewAng);
                        cpVec3(g_playerOrigin[Player], Origin);
                    }
                }
            }
}

bool: eqVec3
(const Float: Which[3], const Float: Other[3], bool: validateY)
    return validateY ? Which[0] == Other[0] &&
                       Which[1] == Other[1] &&
                       Which[2] == Other[2] :
                       Which[0] == Other[0] &&
                       Which[2] == Other[2];

rmVec3(Float: Which[3])
    Which[0] = Which[1] = Which[2] = 0.0;

cpVec3(Float: vecTo[3], const Float: vecFrom[3])
{
    vecTo[0] = vecFrom[0];
    vecTo[1] = vecFrom[1];
    vecTo[2] = vecFrom[2];
}
