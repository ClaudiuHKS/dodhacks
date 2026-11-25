
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>

new bool: g_isPlayerInServer[33];
new bool: g_isPlayerInTeamTalk[33];

public plugin_init()
{
    register_plugin("Team Talk", "1.0.0.1", "Hattrick HKS (claudiuhks)");

    register_forward(FM_Voice_SetClientListening, "On_FM_Voice_SetClientListening");

    register_clcmd("+teamtalk",   "OnPlayerClConCmd_TeamTalk_Begin", ADMIN_ALL, "- begins team talking");
    register_clcmd("-teamtalk",   "OnPlayerClConCmd_TeamTalk_End",   ADMIN_ALL, "- ends team talking");

    register_clcmd("+team_talk",  "OnPlayerClConCmd_TeamTalk_Begin", ADMIN_ALL, "- begins team talking");
    register_clcmd("-team_talk",  "OnPlayerClConCmd_TeamTalk_End",   ADMIN_ALL, "- ends team talking");

    register_clcmd("+teamvoice",  "OnPlayerClConCmd_TeamTalk_Begin", ADMIN_ALL, "- begins team talking");
    register_clcmd("-teamvoice",  "OnPlayerClConCmd_TeamTalk_End",   ADMIN_ALL, "- ends team talking");

    register_clcmd("+team_voice", "OnPlayerClConCmd_TeamTalk_Begin", ADMIN_ALL, "- begins team talking");
    register_clcmd("-team_voice", "OnPlayerClConCmd_TeamTalk_End",   ADMIN_ALL, "- ends team talking");
}

public plugin_cfg()
{
    set_cvar_string("sv_voiceenable", "1");
    set_cvar_string("sv_alltalk",     "1");
}

public client_putinserver(Player)
{
    g_isPlayerInServer[Player]   = true;
    g_isPlayerInTeamTalk[Player] = false;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    g_isPlayerInServer[Player]   = false;
    g_isPlayerInTeamTalk[Player] = false;
}

public On_FM_Voice_SetClientListening(To, Src)
{
    if (To == Src || !g_isPlayerInServer[To] || !g_isPlayerInServer[Src])
        return FMRES_IGNORED;

    if (g_isPlayerInTeamTalk[Src])
        engfunc(EngFunc_SetClientListening, To, Src, get_user_team(Src) == get_user_team(To) ? 2 : 0);

    else
        engfunc(EngFunc_SetClientListening, To, Src, 2);

    return FMRES_SUPERCEDE;
}

public OnPlayerClConCmd_TeamTalk_Begin(Player)
{
    if (false == g_isPlayerInServer[Player])
        return PLUGIN_HANDLED;

    g_isPlayerInTeamTalk[Player] = true;

    client_cmd(Player, "+voicerecord");

    return PLUGIN_HANDLED;
}

public OnPlayerClConCmd_TeamTalk_End(Player)
{
    if (false == g_isPlayerInServer[Player])
        return PLUGIN_HANDLED;

    client_cmd(Player, "-voicerecord");

    g_isPlayerInTeamTalk[Player] = false;

    return PLUGIN_HANDLED;
}
