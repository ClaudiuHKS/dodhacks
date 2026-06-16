
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <sqlx>
#include <dodconst>
#include <dodhacks>
#tryinclude <dod_hacks_knives> /// Works without this as well.

///
/// Either "mysql", "sqlite" or both should be enabled in
/// /dod/addons/amxmodx/configs/modules.ini
/// (by removing the ; symbol from its/ their front)
/// if you want to use the "on-impact explo. nade proj. deton." feature.
///
/// Choose mysql if you have a host, user, password and remote sql database name.
/// Choose sqlite otherwise.
///
/// Both are threaded and won't cause any lag spikes at all.
///
/// mysql is better than sqlite with up to 10%.
///

#if !defined DMG_GRENADE
#define      DMG_GRENADE (1 << 24)
#endif

#if !defined _dod_hacks_knives_included /// Works without this too.
native bool: DoD_IsUserWaitingThrowingKnife(Player);
#endif

/** Nade proj. ents. */
#define ON_IMPACT_EXPLO_NADE_PROJ_VAL      557055              /** 32767+2^19 */         /// Something different from other plugins in-use.
#define ON_IMPACT_EXPLO_NADE_PROJ_KEY      pev_flTimeStepSound /** pev_flSwimTime, .. */ /// Something unrelated to an explo. nade proj.

#define INT_CBasePlayer_m_rgAmmo_ANade     290 /// Allies/ British (weapon_handgrenade)     player ammo count (int CBasePlayer::m_rgAmmo[32]). Cell  #9.
// #define INT_CBasePlayer_m_rgAmmo_AANade 291 /// Allies/ British (weapon_handgrenade_ex)  player ammo count (int CBasePlayer::m_rgAmmo[32]). Cell #10.
#define INT_CBasePlayer_m_rgAmmo_GNade     292 /// Axis            (weapon_stickgrenade)    player ammo count (int CBasePlayer::m_rgAmmo[32]). Cell #11.
// #define INT_CBasePlayer_m_rgAmmo_AGNade 293 /// Axis            (weapon_stickgrenade_ex) player ammo count (int CBasePlayer::m_rgAmmo[32]). Cell #12.

#define PV_CBasePlayer_m_pActiveItem       278 /// '::CBasePlayerItem* ::CBasePlayer::m_pActiveItem' variable.
#define REAL_CDoDGrenade_m_flStartThrow    117 /// 'float ::CDoDGrenade::m_flStartThrow' variable.
#define Linux_Difference_Offset_Weapons      4 /// +4 only on Linux. For 'get_pdata_*' funcs. Weapon entities only.

/**
 * ***  - Updated only if on-death explo. nades dropping is enabled.
 * ***# - Updated only if on-death explo. nades dropping and related HUD msg. are enabled.
 * **   - Updated only if on-impact explo. nade proj. deton. is enabled.
 */

new g_maxPlayers; /// Max. players the game server can handle.
new g_maxNades; /// Max. explo. nades a player can carry at a time.
new g_maxDrop; /// Max. explo. nades a dead player will drop.
new g_Flags; /// Admin flag(s) required for on-impact explo. nade proj. deton. client cmd.
new g_syncImpa; /// HUD msg. sync handle used for on-impact explo. nade proj. deton. msg. (**)
new g_cmdImpaLen; /// On-impact explo. nade proj. deton. client cmd. length.
new g_alliesGrenades; /// Allies custom-given grenade(s) count.
new g_axisGrenades; /// Axis custom-given grenade(s) count.
new g_britishGrenades; /// British custom-given grenade(s) count.
new g_flagNades; /// Admin access required for receiving explosive grenade(s) during spawn.
new g_hudSync[3]; /// HUD msg. sync handles required for on-death dropped explo. nades msg. (***#)
new g_alliesGrenade[32]; /// Allies custom-given grenade entity name.
new g_axisGrenade[32]; /// Axis custom-given grenade entity name.
new g_britishGrenade[32]; /// British custom-given grenade entity name.
new g_cmdImpa[32]; /// Client cmd. for on-impact explo. nade proj. deton.
new g_adjusStep[33]; /// Player's on-death dropped explo. nade entity origin and velocity adjus. step. (***)
new g_Team[33]; /// Player's team index.
new g_dmgType[33]; /// Player's taken dmg. type. (***)
new g_hudChan[33]; /// Player's HUD msg. channel index used for on-death dropped explo. nade msg. (***#)
new g_playerNade[33]; /// Player's unowned nade entity index.
new g_Steam[33][32]; /// Player Steam accounts. (**)
new g_Buffer[256]; /// A large buffer.
new bool: g_isInServer[33]; /// Whether or not the player is in server.
new bool: g_inSpawn[33]; /// Whether we are inside DoD_OnPlayerSpawn() function at the moment.
new bool: g_isAuthenticated[33]; /// Player is authorized. (**)
new bool: g_isFakePlayer[33]; /// Whether or not the player is a BOT (a fake player).
new bool: g_isDead[33]; /// Whether or not the player has died and can't pick up explo. nades from floor anymore. (***)
new bool: g_usingImpa[33]; /// Player's on-impact explo. nade proj. deton. status. (**)
new bool: g_impaNades; /// Whether or not on-impact explo. nade proj. deton. is enabled.
new bool: g_Accid; /// Whether or not to drop explo. nades on accid. deaths (i.e. fall dmg.).
new bool: g_Changing; /// Whether or not to drop explo. nades when changing the team while alive.
new bool: g_Ents; /// Whether or not to drop explo. nades on deaths caused by map ents. (i.e. ai-driven enemy territory machine guns).
new bool: g_Suicide; /// Whether or not to drop explo. nades on suicides ('kill' console command, slay and slap).
new bool: g_suicideGun; /// Whether or not to drop explo. nades on suicides (with a gun or a grenade).
new bool: g_Friendly; /// Whether or not to drop explo. nades on friendly fire incidents.
new bool: g_Explo; /// Whether or not to drop explo. nades on explo. related deaths.
new bool: g_isInDrop; /// Whether or not an on-death explo. nade is being dropped right now.
new bool: g_hudMsg; /// Whether or not to show HUD msg. if the player has picked up an on-death dropped explo. nade.
new bool: g_hudStyle; /// Whether or not to use a visual effect on the HUD msg. used for picking up an on-death dropped explo. nade.
new bool: g_hudMsgImpa; /// Whether or not to show a HUD msg. when an explo. nade is being thrown and the player's on-impact deton. setting is turned on.
new bool: g_hudStyleImpa; /// Whether or not to use a visual effect on the HUD msg. managed by 'g_hudMsgImpa'.
new bool: g_hudStyleImpaCmd; /// Whether or not to use a visual effect on the HUD msg. related to on-impact explo. nade proj. deton. client cmd.
new bool: g_hideChatCmd; /// Hide the on-impact explo. nade proj. deton. client cmd. if it's been activated from chat.
new bool: g_areAlliesBritish; /// Whether Allies are British this map.
new bool: g_isStorageLocal; /// Local storage in use.
new bool: g_gameForceMaxOneNadePerSpawn; /// Whether or not to force max. 1 explo. grenade per player spawn (to be given).
new bool: g_forceKnife; /// Whether to force players use their knife if they're already using a knife and picking up explo. nades.
new bool: g_badIsUserWaitingThrowingKnife = false; /// "DoD_IsUserWaitingThrowingKnife" function has not been found on the server (if true).
new Array: g_arrayNades = Invalid_Array; /// On-death dropped explo. nade ents. (***)
new Array: g_arrayNadeRemovalTimes = Invalid_Array; /// On-death dropped explo. nade removal timestamps. (***)
new Float: g_spawnTime[33]; /// Player's spawn timestamp. (***)
new Float: g_dmgTime[33]; /// Player's taken dmg. timestamp. (***)
new Float: g_pickTime[33]; /// Player's on-death dropped explo. nade pick up timestamp. (***#)
new Float: g_nadesLife; /// Second(s) required to erase from map an on-death dropped explo. nade, if not already picked up by a player.
new Float: g_minTime; /// Second(s) required to be elapsed from player spawn at the moment of player death, to drop explo. nades.
new Float: g_hudVerPos; /// Vertical HUD msg. position for the msg. when a player picks up an on-death dropped explo. nade.
new Float: g_hudHorPos; /// Horizontal HUD msg. position for the msg. when a player picks up an on-death dropped explo. nade.
new Float: g_hudVerPosImpa; /// Vertical position of the HUD msg. managed by 'g_hudMsgImpa'.
new Float: g_hudHorPosImpa; /// Horizontal position of the HUD msg. managed by 'g_hudMsgImpa'.
new Float: g_hudVerPosImpaCmd; /// Vertical position of the HUD msg. related to on-impact explo. nade proj. deton. client cmd.
new Float: g_hudHorPosImpaCmd; /// Horizontal position of the HUD msg. related to on-impact explo. nade proj. deton. client cmd.
new Handle: g_threadedSqlConnectionTuple = Empty_Handle; /// Threaded database storage. (**)
new Handle: g_tmpSqlConnectionTuple = Empty_Handle; /// Non-threaded temporary database storage. (**)
new Handle: g_tmpSqlConnection = Empty_Handle; /// A temporary non-threaded database connection handle. (**)
new bool: g_initiallyConnected = false; /// Whether SQL server was online during plugin's plugin_init() call (SQLite is always online). (**)

public plugin_init()
{
    register_plugin("DoD Hacks: Nade Manager", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    get_configsdir(g_Buffer, charsmax(g_Buffer));
    add(g_Buffer, charsmax(g_Buffer), "/dod_hacks_nademanager.ini");
    new Config = fopen(g_Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", g_Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[32], Drv[32], Host[32], User[32], Pass[32], Db[32], bool: eraseOnNewRound,
        bool: allowSelfGlow, bool: allowTeamGlow, bool: allowDroppedGlow, Tmp[32], errCode;
    while (fgets(Config, g_Buffer, charsmax(g_Buffer)) > 0)
    {
        trim(g_Buffer);
        if (!g_Buffer[0] || g_Buffer[0] == ';' || g_Buffer[0] == '/' ||
            parse(g_Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;
        if (equali(Key, "@max_nades"))
            g_maxNades = clamp(str_to_num(Val), 2, 6);
        else if (equali(Key, "@sec_removal"))
            g_nadesLife = str_to_float(Val);
        else if (equali(Key, "@drop_suicide"))
            g_Suicide = bool: str_to_num(Val);
        else if (equali(Key, "@drop_suicidegun"))
            g_suicideGun = bool: str_to_num(Val);
        else if (equali(Key, "@max_drop"))
            g_maxDrop = clamp(str_to_num(Val), 0, 6);
        else if (equali(Key, "@del_new_round"))
            eraseOnNewRound = bool: str_to_num(Val);
        else if (equali(Key, "@drop_accidental"))
            g_Accid = bool: str_to_num(Val);
        else if (equali(Key, "@drop_tchange"))
            g_Changing = bool: str_to_num(Val);
        else if (equali(Key, "@drop_friendly_fire"))
            g_Friendly = bool: str_to_num(Val);
        else if (equali(Key, "@drop_explo"))
            g_Explo = bool: str_to_num(Val);
        else if (equali(Key, "@drop_ents"))
            g_Ents = bool: str_to_num(Val);
        else if (equali(Key, "@hud_msg"))
            g_hudMsg = bool: str_to_num(Val);
        else if (equali(Key, "@hud_effects"))
            g_hudStyle = bool: str_to_num(Val);
        else if (equali(Key, "@ver_pos"))
            g_hudVerPos = str_to_float(Val);
        else if (equali(Key, "@hor_pos"))
            g_hudHorPos = str_to_float(Val);
        else if (equali(Key, "@drop_seconds"))
            g_minTime = str_to_float(Val);
        else if (equali(Key, "@on_impact"))
            g_impaNades = bool: str_to_num(Val);
        else if (equali(Key, "@ver_pos_impact"))
            g_hudVerPosImpa = str_to_float(Val);
        else if (equali(Key, "@hor_pos_impact"))
            g_hudHorPosImpa = str_to_float(Val);
        else if (equali(Key, "@msg_impact"))
            g_hudMsgImpa = bool: str_to_num(Val);
        else if (equali(Key, "@hud_effects_impact"))
            g_hudStyleImpa = bool: str_to_num(Val);
        else if (equali(Key, "@hud_effects_impact_cmd"))
            g_hudStyleImpaCmd = bool: str_to_num(Val);
        else if (equali(Key, "@ver_pos_impact_cmd"))
            g_hudVerPosImpaCmd = str_to_float(Val);
        else if (equali(Key, "@hor_pos_impact_cmd"))
            g_hudHorPosImpaCmd = str_to_float(Val);
        else if (equali(Key, "@cmd_flag"))
            g_Flags = read_flags(Val);
        else if (equali(Key, "@cmd_name"))
            g_cmdImpaLen = copy(g_cmdImpa, charsmax(g_cmdImpa), Val);
        else if (equali(Key, "@hide_chat_cmd"))
            g_hideChatCmd = bool: str_to_num(Val);
        else if (equali(Key, "@force_knife"))
            g_forceKnife = bool: str_to_num(Val);
        else if (equali(Key, "@allow_self_glow"))
            allowSelfGlow = bool: str_to_num(Val);
        else if (equali(Key, "@allow_team_glow"))
            allowTeamGlow = bool: str_to_num(Val);
        else if (equali(Key, "@nades_access"))
            g_flagNades = read_flags(Val);
        else if (equali(Key, "@allow_dropped_glow"))
            allowDroppedGlow = bool: str_to_num(Val);
        else if (equali(Key, "@game_max_1_nade_at_spawn"))
            g_gameForceMaxOneNadePerSpawn = bool: str_to_num(Val);
        else if (equali(Key, "@db_driver"))
        {
            copy(Drv, charsmax(Drv), Val);
            g_isStorageLocal = bool: equali(Drv, "sqlite");
        }
        else if (equali(Key, "@db_user"))
            copy(User, charsmax(User), Val);
        else if (equali(Key, "@db_pass"))
            copy(Pass, charsmax(Pass), Val);
        else if (equali(Key, "@db_host"))
            copy(Host, charsmax(Host), Val);
        else if (equali(Key, "@db_name"))
            copy(Db, charsmax(Db), Val);
        else if (equali(Key, "@grenade_allies"))
        {
            if (parse(g_Buffer, Key, charsmax(Key), Val, charsmax(Val),
                Tmp, charsmax(Tmp)) > 2)
            {
                copy(g_alliesGrenade, charsmax(g_alliesGrenade), Tmp);
                g_alliesGrenades = str_to_num(Val);
            }
        }
        else if (equali(Key, "@grenade_axis"))
        {
            if (parse(g_Buffer, Key, charsmax(Key), Val, charsmax(Val),
                Tmp, charsmax(Tmp)) > 2)
            {
                copy(g_axisGrenade, charsmax(g_axisGrenade), Tmp);
                g_axisGrenades = str_to_num(Val);
            }
        }
        else if (equali(Key, "@grenade_british"))
        {
            if (parse(g_Buffer, Key, charsmax(Key), Val, charsmax(Val),
                Tmp, charsmax(Tmp)) > 2)
            {
                copy(g_britishGrenade, charsmax(g_britishGrenade), Tmp);
                g_britishGrenades = str_to_num(Val);
            }
        }
    }
    fclose(Config);

#if defined amxclient_cmd && defined RegisterHamPlayer
    RegisterHamPlayer(Ham_Touch,  "OnPlayerTouch_Pre"        );
    RegisterHamPlayer(Ham_Killed, "OnPlayerKilled_Post", true);
    if (g_maxDrop)
    {
        RegisterHamPlayer(Ham_Killed,     "OnPlayerKilled_Pre"           );
        RegisterHamPlayer(Ham_TakeDamage, "OnPlayerTakeDamage_Post", true);
    }
#else
    RegisterHam(Ham_Touch,  "player", "OnPlayerTouch_Pre"        );
    RegisterHam(Ham_Killed, "player", "OnPlayerKilled_Post", true);
    if (g_maxDrop)
    {
        RegisterHam(Ham_Killed,     "player", "OnPlayerKilled_Pre"           );
        RegisterHam(Ham_TakeDamage, "player", "OnPlayerTakeDamage_Post", true);
    }
#endif
    RegisterHam(Ham_Touch, "weapon_handgrenade",  "OnAlliesNadeTouch_Pre");
    RegisterHam(Ham_Touch, "weapon_stickgrenade", "OnAxisNadeTouch_Pre"  );
    if (g_forceKnife)
    {
        RegisterHam(Ham_Item_CanDeploy, "weapon_handgrenade",  "OnExploNadeCanDeploy_Pre");
        RegisterHam(Ham_Item_CanDeploy, "weapon_stickgrenade", "OnExploNadeCanDeploy_Pre");
    }
    if (g_impaNades)
    {
        /** Just "grenade" is enough. */
        RegisterHam(Ham_Touch, "grenade", "OnExploNadeProjTouch_Post", true);
        register_forward(FM_SetModel, "OnSetModel_Post", true);
        /**
         * Let the game server show everyone that it has this feature.
         */
        register_clcmd(g_cmdImpa, "OnPlayerClConCmd_InstaNades", ADMIN_ALL,
            "- enables or disables insta nade detonations");
        g_syncImpa = CreateHudSyncObj();
        SQL_GetAffinity(g_Buffer, charsmax(g_Buffer));
        if (!equali(g_Buffer, Drv))
            if (!SQL_SetAffinity(Drv))
            {
                log_amx("SQL_SetAffinity('%s') call failed. Ensure the module is enabled in '../amxmodx/configs/modules.ini'.",
                    Drv);
                log_amx("Nade Manager plugin may work without SQL stuff if 'on-impact explo. nade proj. deton.' feature is off.");
            }
        g_threadedSqlConnectionTuple = SQL_MakeDbTuple(Host, User, Pass, Db);
        g_tmpSqlConnectionTuple = SQL_MakeDbTuple(Host, User, Pass, Db, 3);
        g_tmpSqlConnection = SQL_Connect(g_tmpSqlConnectionTuple, errCode, Tmp, charsmax(Tmp)); /// Connect just to see whether server is up or down.
        if (Empty_Handle != g_tmpSqlConnection)
        {
            g_initiallyConnected = true;
            SQL_FreeHandle(g_tmpSqlConnection); /// If server is up, immediately shut down the non-threaded SQL database connection.
            g_tmpSqlConnection = Empty_Handle;
            SQL_FreeHandle(g_tmpSqlConnectionTuple);
            g_tmpSqlConnectionTuple = Empty_Handle;
            if (g_isStorageLocal)
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "EmptySqlHandler",
                    "CREATE TABLE IF NOT EXISTS insta_nades (setting TINYINT NOT NULL, steam CHARACTER (32) NOT NULL UNIQUE COLLATE NOCASE, PRIMARY KEY (steam), UNIQUE (steam))");
            else
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "EmptySqlHandler",
                    "CREATE TABLE IF NOT EXISTS insta_nades (setting TINYINT NOT NULL, steam CHAR (32) NOT NULL COLLATE utf8mb4_unicode_520_ci, PRIMARY KEY (steam), UNIQUE (steam)) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_520_ci");
        }
        else
        {
            if (!safeIsSqlModuleRunning("sqlite") && !safeIsSqlModuleRunning("mysql"))
            {
                log_amx("'../amxmodx/configs/modules.ini' needs either sqlite or mysql enabled!");
                log_amx("Nade Manager plugin may work without SQL stuff if 'on-impact explo. nade proj. deton.' feature is off.");
            }
            else
            {
                log_amx("Nade Manager plugin loaded with MySQL server being offline.");
                log_amx("As soon as the connection establishes, map will restart automatically.");
                log_amx("If you are using SQLite, 'sqlite' module needs to be enabled in '../amxmodx/configs/modules.ini'.");
                log_amx("Nade Manager plugin may work without SQL stuff if 'on-impact explo. nade proj. deton.' feature is off.");
                set_task(1.0, "Task_VerifyConnection");
            }
        }
    }
    if (g_maxDrop)
    { /// Only if dropping explo. nades on death is enabled.
        g_arrayNades            = ArrayCreate();
        g_arrayNadeRemovalTimes = ArrayCreate();
        if (g_hudMsg)
        { /// Only if picking up on-death dropped explo. nades HUD msg. are enabled.
            g_hudSync[0] = CreateHudSyncObj();
            g_hudSync[1] = CreateHudSyncObj();
            g_hudSync[2] = CreateHudSyncObj();
        }
        register_forward(FM_RemoveEntity,      "OnRemoveEntity_Post",      true);
        register_forward(FM_CreateNamedEntity, "OnCreateNamedEntity_Post", true);
        RegisterHam(Ham_Think, "weapon_handgrenade",  "OnNadeThink_Post", true);
        RegisterHam(Ham_Think, "weapon_stickgrenade", "OnNadeThink_Post", true);
        if (eraseOnNewRound)
            register_event("HLTV", "OnRoundInitialization", "a", "1=0", "2=0");
        set_task(1.0, "Task_RemoveDroppedExploNades", .flags = "b");
    }
    g_maxPlayers = get_maxplayers();
    g_areAlliesBritish = DoD_AreAlliesBritish();
    if (allowSelfGlow)
        DoD_AddSelfExploNadeProjGlow(      kRenderNormal, kRenderFxGlowShell,
            {  20, 180, 200 } /** L. blue. */, 5, SOLID_NOT, true);
    if (allowTeamGlow)
        DoD_AddTeamExploNadeProjGlow(      kRenderNormal, kRenderFxGlowShell,
            { 200, 160,  20 }  /** Yellow. */, 5, SOLID_NOT, true);
    if (allowDroppedGlow)
        DoD_AddDroppedExploNadeGlow (true, kRenderNormal, kRenderFxGlowShell,
            {  20, 200,  40 }   /** Green. */, 5, SOLID_NOT, true);
    return PLUGIN_CONTINUE;
}

public Task_VerifyConnection()
{
    static Map[64], errCode, Err[4];
    g_tmpSqlConnection = SQL_Connect(g_tmpSqlConnectionTuple, errCode, Err, charsmax(Err));
    if (Empty_Handle != g_tmpSqlConnection)
    {
        SQL_FreeHandle(g_tmpSqlConnection);
        if (Empty_Handle != g_tmpSqlConnectionTuple)
            SQL_FreeHandle(g_tmpSqlConnectionTuple);
        get_mapname(Map, charsmax(Map));
#if defined engine_changelevel
        engine_changelevel(Map);
#else
#if defined NULL_STRING
        engfunc(EngFunc_ChangeLevel, Map, NULL_STRING);
#else
        DoD_ChangeMap(Map);
#endif
#endif
    }
    else
        set_task(1.0, "Task_VerifyConnection");
}

public OnExploNadeCanDeploy_Pre(Entity)
{
    if (findAliveNadeOwnerByUnownedNade(Entity) && false == isPlayer(pev(Entity, pev_owner)))
    { /// Do not auto. switch the knife with a grenade.
        SetHamReturnInteger(false);
        return HAM_SUPERCEDE;
    }
    return HAM_IGNORED;
}

public plugin_natives()
    set_native_filter("nativesFilter");

public nativesFilter(const Name[], Index, bool: Found)
{
    if (('D' == Name[0] || Name[0] == 'd') && equali(Name, "DoD_IsUserWaitingThrowingKnife"))
        switch (Found)
        {
            case true:
                g_badIsUserWaitingThrowingKnife = false;
            default:
            {
                g_badIsUserWaitingThrowingKnife = true;
                return PLUGIN_HANDLED;
            }
        }
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public OnRemoveEntity_Post(Entity)
    deleteExploNade(Entity);

public DoD_OnSubRemove_Post(Entity)
    deleteExploNade(Entity);

public DoD_OnUtilRemove_Post(Entity)
    deleteExploNade(Entity);

#if defined client_connectex
public client_authorized(Player, const Steam[])
#else
public client_authorized(Player)
#endif
{
#if !defined client_connectex
    static Steam[32], Info[32];
    if (!g_impaNades)
        return PLUGIN_CONTINUE; /// Skip if feature disabled.
    get_user_authid(Player, Steam, charsmax(Steam));
#else
    static Info[32];
    if (!g_impaNades)
        return PLUGIN_CONTINUE; /// Skip if feature disabled.
#endif
    g_isAuthenticated[Player] = true;
    if (copy(g_Steam[Player], charsmax(g_Steam[]), Steam) > 4 && g_initiallyConnected)
    { /// Skipping fake players (long enough Steam string).
        formatex(g_Buffer, charsmax(g_Buffer), "SELECT setting FROM insta_nades WHERE steam = '%s'", Steam);
        num_to_str(get_user_userid(Player), Info, charsmax(Info));
        SQL_ThreadQuery(g_threadedSqlConnectionTuple, "OnSqlSelection", g_Buffer, Info, sizeof Info);
    }
    return PLUGIN_CONTINUE;
}

public EmptySqlHandler(FailState, Handle: Query, const Error[], ErrorNum) /// Not expecting an answer.
    if (TQUERY_QUERY_FAILED == FailState)
        log_amx("SQL Error (#%d): %s", ErrorNum, Error);

public OnNadeThink_Post(Nade) /// To not block carrier's death animation.
    if (ArrayFindValue(g_arrayNades, Nade) > -1)
        set_pev(Nade, pev_solid, SOLID_TRIGGER);

public OnCreateNamedEntity_Post(allocatedString)
{ /// DoD_Create() calls engine's CreateNamedEntity() before calling DispatchSpawn().
    static Entity;
    if (g_isInDrop)
    { /// Ensure the item (the on-death dropped explo. nade) is not set to respawn before DoD_Create() returns.
        Entity = get_orig_retval();
        if (Entity > g_maxPlayers)
            set_pev(Entity, pev_spawnflags, pev(Entity, pev_spawnflags) | SF_NORESPAWN);
        g_isInDrop = false;
    }
}

public OnSetModel_Post(Entity)
{ /// Catch "weapon_handgrenade" and "weapon_stickgrenade" to "grenade" (proj.) transformation.
    static Owner, Class[8];
    if (pev(Entity, pev_classname, Class, charsmax(Class)) > 0 && equali(Class, "grenade"))
    { /// "grenade" (Allies/ British) & "grenade2" (Axis)
        Owner = pev(Entity, pev_owner);
        if (isPlayer(Owner) && g_isInServer[Owner] && g_usingImpa[Owner] && g_Flags == (get_user_flags(Owner) & g_Flags))
        { /// Make sure the player is still an admin, if admin access required.
            set_pev(Entity, ON_IMPACT_EXPLO_NADE_PROJ_KEY, ON_IMPACT_EXPLO_NADE_PROJ_VAL);
            if (g_hudMsgImpa && false == g_isFakePlayer[Owner])
            {
                set_hudmessage(20 /** Red. */, 180 /** Green. */, 200 /** Blue. */,
                    g_hudHorPosImpa /** Horizontal position. */, g_hudVerPosImpa /** Vertical position. */,
                    g_hudStyleImpa ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    1.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                ShowSyncHudMsg(Owner, g_syncImpa, "IMPACT DETONATION!");
            }
        }
    }
}

public client_command(Player)
{
    static Cmd[16], Arg[32];
    if (g_impaNades && g_isInServer[Player] && read_argv(0, Cmd, charsmax(Cmd)) > 2 &&
        (equali(Cmd, "say") || equali(Cmd, "say_team")))
    {
        if (read_argv(1, Arg, charsmax(Arg)) >= g_cmdImpaLen && (equali(Arg, g_cmdImpa) ||
            ((Arg[0] == ',' || Arg[0] == '.' || Arg[0] == '/' || Arg[0] == '!') && equali(Arg[1], g_cmdImpa))))
        {
            if (g_Flags != (get_user_flags(Player) & g_Flags))
            { /// No access.
                if (false == g_isFakePlayer[Player])
                {
                    set_hudmessage(200 /** Red. */, 20 /** Green. */, 20 /** Blue. */,
                        g_hudHorPosImpaCmd /** Horizontal position. */, g_hudVerPosImpaCmd /** Vertical position. */,
                        g_hudStyleImpaCmd ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                        1.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                    ShowSyncHudMsg(Player, g_syncImpa, "NO ACCESS!");
                }
                return g_hideChatCmd ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
            }
            g_usingImpa[Player] = !g_usingImpa[Player];
            if (false == g_isFakePlayer[Player])
                showImpaStatus(Player, false);
            if (g_hideChatCmd)
                return PLUGIN_HANDLED;
        }
    }
    return PLUGIN_CONTINUE;
}

public OnPlayerClConCmd_InstaNades(Player)
{
    if (!g_isInServer[Player])
        return PLUGIN_HANDLED;
    if (g_Flags != (get_user_flags(Player) & g_Flags))
    { /// No access.
        if (false == g_isFakePlayer[Player])
        {
            console_print(Player, ">> NO ACCESS!");
            set_hudmessage(200 /** Red. */, 20 /** Green. */, 20 /** Blue. */,
                g_hudHorPosImpaCmd /** Horizontal position. */, g_hudVerPosImpaCmd /** Vertical position. */,
                g_hudStyleImpaCmd ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                1.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
            ShowSyncHudMsg(Player, g_syncImpa, "NO ACCESS!");
        }
        return PLUGIN_HANDLED;
    }
    g_usingImpa[Player] = !g_usingImpa[Player];
    if (false == g_isFakePlayer[Player])
        showImpaStatus(Player, true);
    return PLUGIN_HANDLED;
}

public OnExploNadeProjTouch_Post(Proj)
{ /// Nade proj. did hit something.
    if (pev(Proj, ON_IMPACT_EXPLO_NADE_PROJ_KEY) == ON_IMPACT_EXPLO_NADE_PROJ_VAL)
    { /// Detonate ASAP.
        set_pev(Proj, pev_dmgtime, 0.0);
        set_pev(Proj, ON_IMPACT_EXPLO_NADE_PROJ_KEY, 0);
        set_pev(Proj, pev_nextthink, get_gametime());
    }
}

public OnPlayerTakeDamage_Post(Player, Inflictor, Attacker, Float: Damage, Type)
{
    g_dmgType[Player] = Type;
    g_dmgTime[Player] = get_gametime();
}

public OnPlayerKilled_Post(Player)
    g_playerNade[Player] = 0;

public OnPlayerKilled_Pre(Player, Attacker)
{
    static Float: Time, Float: Damage;
    Time = get_gametime();
    if (Attacker == Player)
    {
        if (pev(Player, pev_dmg_take, Damage) > 0 && Damage)
            switch (g_suicideGun)
            {
                case false:
                    return HAM_IGNORED; /// Filter dropping on suicides (gun/ grenade related), if disabled.
                default:
                    if (false == g_Explo && Time - g_dmgTime[Player] < 0.01)
                    {
                        if ((g_dmgType[Player] & DMG_BLAST) || (g_dmgType[Player] & DMG_MORTAR) || (g_dmgType[Player] & DMG_GRENADE))
                            return HAM_IGNORED; /// Filter dropping on explo. related deaths, if disabled.
                    }
            }
        else
            switch (g_Suicide)
            {
                case false:
                    return HAM_IGNORED; /// Filter dropping on suicides ('kill' console command, slay or slap related), if disabled.
                default:
                    if (false == g_Explo && Time - g_dmgTime[Player] < 0.01)
                    {
                        if ((g_dmgType[Player] & DMG_BLAST) || (g_dmgType[Player] & DMG_MORTAR) || (g_dmgType[Player] & DMG_GRENADE))
                            return HAM_IGNORED; /// Filter dropping on explo. related deaths, if disabled.
                    }
            }
        goto End;
    }
    switch (isPlayer(Attacker))
    {
        case true:
            if (false == g_Friendly && g_Team[Player] == g_Team[Attacker])
                return HAM_IGNORED; /// Filter dropping on friendly fire incidents, if disabled.
        default:
            switch (Attacker < 1)
            {
                case true:
                    if (pev(Player, pev_dmg_take, Damage) > 0 && Damage == 900.0)
                    {
                        if (false == g_Changing)
                            return HAM_IGNORED; /// Filter dropping when changing the team while alive, if disabled.
                    }
                    else
                    {
                        if (false == g_Accid)
                            return HAM_IGNORED; /// Filter dropping on accid. deaths (i.e. fall damage), if disabled.
                    }
                default:
                    if (false == g_Ents)
                        return HAM_IGNORED; /// Filter dropping on deaths caused by map ents. (i.e. ai-driven enemy territory machine guns), if disabled.
            }
    }
    if (false == g_Explo && Time - g_dmgTime[Player] < 0.01)
    {
        if ((g_dmgType[Player] & DMG_BLAST) || (g_dmgType[Player] & DMG_MORTAR) || (g_dmgType[Player] & DMG_GRENADE))
            return HAM_IGNORED; /// Filter dropping on explo. related deaths, if disabled.
    }
End:
    if (Time - g_spawnTime[Player] >= g_minTime)
        dropExploNades(Player);
    return HAM_IGNORED;
}

public OnPlayerTouch_Pre(Player, Other)
{
    static ANades, GNades, Class[32];
    if (pev(Other, pev_classname, Class, charsmax(Class)) > 0 &&
        (equali(Class[7], "handgrenade") || equali(Class[7], "stickgrenade")))
        return g_isDead[Player] || (false == g_badIsUserWaitingThrowingKnife && DoD_IsUserWaitingThrowingKnife(Player)) ||
            calcExploNades(Player, ANades, GNades) >= g_maxNades ||
            giveExploNadeAsAmmo(Player, Other, ANades, GNades, Class[7] == 'h' || Class[7] == 'H' ? ALLIES : AXIS) ?
            HAM_SUPERCEDE : HAM_IGNORED;
    return HAM_IGNORED;
}

public OnAlliesNadeTouch_Pre(Nade, Other)
{
    static ANades, GNades;
    if (!isPlayer(Other))
        return HAM_IGNORED;
    return g_isDead[Other] || (false == g_badIsUserWaitingThrowingKnife && DoD_IsUserWaitingThrowingKnife(Other)) ||
        calcExploNades(Other, ANades, GNades) >= g_maxNades ||
        giveExploNadeAsAmmo(Other, Nade, ANades, GNades, ALLIES) ? HAM_SUPERCEDE : HAM_IGNORED;
}

public OnAxisNadeTouch_Pre(Nade, Other)
{
    static ANades, GNades;
    if (!isPlayer(Other))
        return HAM_IGNORED;
    return g_isDead[Other] || (false == g_badIsUserWaitingThrowingKnife && DoD_IsUserWaitingThrowingKnife(Other)) ||
        calcExploNades(Other, ANades, GNades) >= g_maxNades ||
        giveExploNadeAsAmmo(Other, Nade, ANades, GNades, AXIS) ? HAM_SUPERCEDE : HAM_IGNORED;
}

public DoD_OnPlayerSpawn(DoD_Address: CDoDTeamPlay, &Player)
    if (g_isInServer[Player])
    {
        g_inSpawn[Player] = true;
        g_isDead[Player] = false;
        g_playerNade[Player] = 0;
        g_Team[Player] = get_user_team(Player);
        if (g_maxDrop)
            g_spawnTime[Player] = get_gametime();
    }

public DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player)
{
    static Iter, Item;
    if (g_isInServer[Player])
    {
        g_inSpawn[Player] = false;
        if (g_flagNades == (get_user_flags(Player) & g_flagNades) &&
            calcExploNades(Player, Iter, Item) < 1)
        {
            switch (g_Team[Player])
            {
                case ALLIES:
                    if (g_areAlliesBritish)
                        for (Iter = 0; Iter < g_britishGrenades; Iter++)
                            DoD_GiveNamedItem(Player, g_britishGrenade, Item);
                    else
                        for (Iter = 0; Iter < g_alliesGrenades; Iter++)
                            DoD_GiveNamedItem(Player, g_alliesGrenade, Item);
                default:
                    for (Iter = 0; Iter < g_axisGrenades; Iter++)
                        DoD_GiveNamedItem(Player, g_axisGrenade, Item);
            }
        }
    }
}

public DoD_OnGiveNamedItem(Player, Item[], ItemSize /** = 64 */, &OverrideRes)
{ /// Item may be altered during execution.
    if (!g_inSpawn[Player])
        return PLUGIN_CONTINUE; /// Skip.
    if (g_gameForceMaxOneNadePerSpawn &&
        (get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_ANade) ||
        get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_GNade)) &&
        DoD_IsWeaponGrenade(Item))
        return PLUGIN_HANDLED;
    return PLUGIN_CONTINUE;
}

public client_putinserver(Player)
{
    g_isInServer[Player] = true;
    g_isFakePlayer[Player] = bool: is_user_bot(Player);
}

public DOD_ON_PLAYER_DISCONNECTED
{
    if (g_impaNades && g_initiallyConnected && false == g_isFakePlayer[Player] && g_isAuthenticated[Player])
    { /// Store the selection.
        if (g_isStorageLocal)
            formatex(g_Buffer, charsmax(g_Buffer),
                "INSERT INTO insta_nades VALUES (%d, '%s') ON CONFLICT (steam) DO UPDATE SET setting = %d",
                g_usingImpa[Player], g_Steam[Player], g_usingImpa[Player]);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "INSERT INTO insta_nades VALUES (%d, '%s') ON DUPLICATE KEY UPDATE setting = %d",
                g_usingImpa[Player], g_Steam[Player], g_usingImpa[Player]);

        SQL_ThreadQuery(g_threadedSqlConnectionTuple, "EmptySqlHandler", g_Buffer);
    }
    zeroPlayerGlobals(Player);
}

public client_connect(Player)
    zeroPlayerGlobals(Player);

public OnRoundInitialization()
{
    static Iter, Nades;
    Nades = ArraySize(g_arrayNades);
    for (Iter = 0; Iter < Nades; Iter++) /// Erase all entries.
        DoD_DestroyItem(ArrayGetCell(g_arrayNades, Iter)); /// Removes the entity as well.
    ArrayClear(g_arrayNades);
    ArrayClear(g_arrayNadeRemovalTimes);
}

public Task_RemoveDroppedExploNades()
{
    static Iter, Nades, Float: Time, Float: Removal;
    Nades = ArraySize(g_arrayNades);
    Time = get_gametime();
    for (Iter = Nades - 1; Iter > -1; --Iter)
    { /// Erase entries where needed, starting from tail.
        Removal = ArrayGetCell(g_arrayNadeRemovalTimes, Iter);
        if (Removal > Time)
            continue;
        DoD_DestroyItem(ArrayGetCell(g_arrayNades, Iter)); /// Removes the entity as well.
        clearExploNadeByPos(Iter);
    }
}

public DoD_OnAddPlayerItem(Player, &Item, &OverrideRes)
{
    g_playerNade[Player] = Item;
    if (g_maxDrop)
        manageExploNadePick(Player, Item);
}

public DoD_OnRemovePlayerItem(Player, &Item, &OverrideRes)
    if (Item == g_playerNade[Player])
        g_playerNade[Player] = 0;

public DoD_OnRemoveAllItems(Player, &RemoveSuit)
    g_playerNade[Player] = 0;

#if defined FindPlayerFlags
public OnSqlSelection(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static Player;
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
    {
        Player = find_player_ex(FindPlayer_IncludeConnecting | FindPlayer_MatchUserId, str_to_num(Data));
        if (Player > 0)
            g_usingImpa[Player] = bool: SQL_ReadResult(Query, 0);
    }
}
#else
public OnSqlSelection(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static UniqueIndex, Player;
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
    {
        UniqueIndex = str_to_num(Data);
        for (Player = 1; Player <= g_maxPlayers; Player++)
        {
            if (UniqueIndex == get_user_userid(Player))
            {
                g_usingImpa[Player] = bool: SQL_ReadResult(Query, 0);
                break;
            }
        }
    }
}
#endif

calcExploNades(Player, &ANades, &GNades)
{
    ANades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_ANade);
    GNades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_GNade);
    return ANades + GNades;
}

Float: verMsgPosByChan(Player)
{
    switch (g_hudChan[Player])
    {
        case 0: return g_hudVerPos;
        case 1: return g_hudVerPos + 0.05;
    }
    return g_hudVerPos + 0.1;
}

bool: giveExploNadeAsAmmo(Player, Nade, ANades, GNades, Team)
{
    static Res;
    switch (Team)
    {
        case ALLIES:
            if (ANades > 2) /// DoD limits to 3 Allies/ British (weapon_handgrenade) grenades by default.
            { /// Bypass this limit by giving explo. nades as they were ammo instead.
                DoD_GiveAmmo(Player, 1, "ammo_agrens", g_maxNades, Res);
                emit_sound(Player, CHAN_ITEM, "items/weaponpickup.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
                if (g_maxDrop)
                    manageExploNadePick(Player, Nade);
                DoD_DestroyItem(Nade); /// Removes the entity as well.
                return true; /// Block Touch() function, as the entity gets removed.
            }
        default:
            if (GNades > 4) /// DoD limits to 5 Axis (weapon_stickgrenade) grenades by default.
            { /// Bypass this limit by giving explo. nades as they were ammo instead.
                DoD_GiveAmmo(Player, 1, "ammo_ggrens", g_maxNades, Res);
                emit_sound(Player, CHAN_ITEM, "items/weaponpickup.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
                if (g_maxDrop)
                    manageExploNadePick(Player, Nade);
                DoD_DestroyItem(Nade); /// Removes the entity as well.
                return true; /// Block Touch() function, as the entity gets removed.
            }
    }
    return false;
}

manageExploNadePick(Player, Nade)
{ /// On-death explo. nades dropping feature is enabled now.
    static Item, Float: Time;
    Item = ArrayFindValue(g_arrayNades, Nade); /// Nothing to do if the dropped explo. nade isn't a custom one (an on-death one).
    if (Item > -1)
    {
        if (g_hudMsg && false == g_isFakePlayer[Player])
        { /// Notice the player on their screen, if enabled and they're not fake.
            Time = get_gametime();
            if (Time - g_pickTime[Player] >= 2.0)
                g_hudChan[Player] = 0; /// Avoid a big vertical distance between two HUD msg.
            set_hudmessage(20 /** Red. */, 200 /** Green. */, 40 /** Blue. */,
                g_hudHorPos /** Horizontal position. */, verMsgPosByChan(Player) /** Vertical position. */,
                g_hudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                2.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
            ShowSyncHudMsg(Player, g_hudSync[g_hudChan[Player]], "+1 EXPLO NADE!");
            g_pickTime[Player] = Time;
            if (++g_hudChan[Player] > 2)
                g_hudChan[Player] = 0; /// Reset player's chan. index if needed.
        }
        clearExploNadeByPos(Item);
    } /// Erase the item from list, as it's been picked up.
}

showImpaStatus(Player, bool: Console)
{
    switch (g_usingImpa[Player])
    {
        case true:
        {
            set_hudmessage(20 /** Red. */, 200 /** Green. */, 40 /** Blue. */,
                g_hudHorPosImpaCmd /** Horizontal position. */, g_hudVerPosImpaCmd /** Vertical position. */,
                g_hudStyleImpaCmd ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                1.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
            ShowSyncHudMsg(Player, g_syncImpa, "IMPACT DETONATION ON!");
            if (Console)
                console_print(Player, ">> IMPACT DETONATION ON!");
        }
        default:
        {
            set_hudmessage(200 /** Red. */, 20 /** Green. */, 20 /** Blue. */,
                g_hudHorPosImpaCmd /** Horizontal position. */, g_hudVerPosImpaCmd /** Vertical position. */,
                g_hudStyleImpaCmd ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                1.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
            ShowSyncHudMsg(Player, g_syncImpa, "IMPACT DETONATION OFF!");
            if (Console)
                console_print(Player, ">> IMPACT DETONATION OFF!");
        }
    }
}

clearExploNadeByPos(Pos)
{
    ArrayDeleteItem(g_arrayNades,            Pos);
    ArrayDeleteItem(g_arrayNadeRemovalTimes, Pos);
}

zeroPlayerGlobals(Player)
{
    g_isDead[Player] = false;
    g_usingImpa[Player] = false;
    g_isInServer[Player] = false;
    g_isFakePlayer[Player] = false;
    g_isAuthenticated[Player] = false;
    g_inSpawn[Player] = false;
    g_dmgTime[Player] = 0.0;
    g_pickTime[Player] = 0.0;
    g_spawnTime[Player] = 0.0;
    g_Team[Player] = 0;
    g_hudChan[Player] = 0;
    g_dmgType[Player] = 0;
    g_adjusStep[Player] = 0;
    g_playerNade[Player] = 0;
}

bool: holdsPlayerArmedNonExExploNade(Player, &armedNadeTeam)
{
    static Item, Class[32];
    armedNadeTeam = 0;
    Item = get_pdata_cbase(Player, PV_CBasePlayer_m_pActiveItem);
    if (Item <= g_maxPlayers || pev(Item, pev_classname, Class, charsmax(Class)) < 1 || containi(Class, "ex") > -1 ||
        !get_pdata_float(Item, REAL_CDoDGrenade_m_flStartThrow, Linux_Difference_Offset_Weapons))
        return false;
    armedNadeTeam = Class[7] == 'h' || 'H' == Class[7] ? ALLIES : AXIS;
    return true;
}

dropExploNades(Player)
{ /// Game already drops an explo. grenade on death if it's armed, even if no "_ex" type.
    static ANades, GNades, Iter, Dropped, armedNadeTeam;
    g_isDead[Player] = true;
    Dropped = 0;
    holdsPlayerArmedNonExExploNade(Player, armedNadeTeam);
    switch (g_Team[Player])
    {
        case ALLIES /** 1 */:
        { /// Ensure Allies explo. nade(s) drop first.
            if (ALLIES == armedNadeTeam)
                ANades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_ANade) - 1; /// Don't drop the same grenade twice.
            else
                ANades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_ANade);
            for (Iter = 0; Iter < ANades; Iter++)
            {
                dropExploNade(Player, "weapon_handgrenade");
                if (++Dropped == g_maxDrop)
                    return;
            }
            if (AXIS == armedNadeTeam)
                GNades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_GNade) - 1; /// Don't drop the same grenade twice.
            else
                GNades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_GNade);
            for (Iter = 0; Iter < GNades; Iter++)
            {
                dropExploNade(Player, "weapon_stickgrenade");
                if (++Dropped == g_maxDrop)
                    return;
            }
        }
        default: /** AXIS (2) */
        { /// Ensure Axis explo. nade(s) drop first.
            if (AXIS == armedNadeTeam)
                GNades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_GNade) - 1; /// Don't drop the same grenade twice.
            else
                GNades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_GNade);
            for (Iter = 0; Iter < GNades; Iter++)
            {
                dropExploNade(Player, "weapon_stickgrenade");
                if (++Dropped == g_maxDrop)
                    return;
            }
            if (ALLIES == armedNadeTeam)
                ANades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_ANade) - 1; /// Don't drop the same grenade twice.
            else
                ANades = get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_ANade);
            for (Iter = 0; Iter < ANades; Iter++)
            {
                dropExploNade(Player, "weapon_handgrenade");
                if (++Dropped == g_maxDrop)
                    return;
            }
        }
    }
}

dropExploNade(Player, const Class[])
{
    static Float: Origin[3], Float: Angles[3], Float: Velocity[3], Nade, bool: Flag;
    pev(Player, pev_origin, Origin);
    switch (++g_adjusStep[Player])
    { /// Ensure proper distance between on-death dropped explo. nades if dropping multiple nades at a time.
        case 1:
        {
            Origin[0] -= 5.0;
            Origin[1] += 5.0;
        }
        case 2:
        {
            Origin[0] += 5.0;
            Origin[1] -= 5.0;
        }
        case 3:
            Origin[1] -= 5.0;
        case 4:
            Origin[0] += 5.0;
        case 5:
            Origin[0] -= 5.0;
        default:
            Origin[1] += 5.0;
    }
    pev(Player, pev_angles, Angles);
    Angles[0] = 0.0;
    Angles[2] = 0.0;
    g_isInDrop = true; /// Catch engine's CreateNamedEntity() before AddEntityHashValue() and DispatchSpawn(), to ensure the dropped explo. nade won't respawn later.
    if (DoD_Create(Class, Origin, Angles, -1 /** no owner, as it might play an audio sound during drop otherwise */, Nade) &&
        Nade > g_maxPlayers)
    { /// DoD_Create() calls engine's CreateNamedEntity().
        Flag = !(g_adjusStep[Player] % 2);
        pev(Player, pev_velocity, Velocity);
        Velocity[0] *= (Flag ? 1.2 : 1.4);
        Velocity[1] *= (Flag ? 1.2 : 1.4); /// Ensure proper distance between on-death dropped explo. nades if dropping multiple nades at a time.
        Velocity[2] *= (Flag ? 1.2 : 1.4);
        set_pev(Nade, pev_velocity, Velocity);
        set_pev(Nade, pev_solid, SOLID_NOT); /// Strongly required, as the dying player is still solid for a while and will cause blocking.
        ArrayPushCell(g_arrayNades, Nade);
        ArrayPushCell(g_arrayNadeRemovalTimes, get_gametime() + g_nadesLife);
    } /// Reset player's on-death dropped explo. nade entity origin and velocity adjus. step if needed.
    if (g_adjusStep[Player] > 5)
        g_adjusStep[Player] = 0;
}

bool: isPlayer(Entity)
{
    if (Entity < 1)
        return false;
    return Entity <= g_maxPlayers;
}

deleteExploNade(Nade)
{
    static Item;
    if (Invalid_Array != g_arrayNades)
    {
        Item = ArrayFindValue(g_arrayNades, Nade);
        if (Item > -1)
            clearExploNadeByPos(Item);
    }
}

bool: findAliveNadeOwnerByUnownedNade(Nade)
{
    static Player;
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (Nade == g_playerNade[Player] && is_user_alive(Player))
            return true;
    return false;
}

bool: safeIsSqlModuleRunning(const Name[])
{
    new Buffer[256];
    get_localinfo("amxx_modules", Buffer, charsmax(Buffer));
    new File = fopen(Buffer, "r");
    if (!File)
        return false;
    new Len = strlen(Name);
    while (fgets(File, Buffer, charsmax(Buffer)))
    {
        trim(Buffer);
        if (equali(Buffer, Name, Len))
        {
            fclose(File);
            return true;
        }
    }
    fclose(File);
    return false;
}

#if !defined ArrayResize
ArrayFindValue(Array: Which, Value)
{
    static Iter, Size;
    Size = ArraySize(Which);
    for (Iter = 0; Iter < Size; Iter++)
        if (Value == ArrayGetCell(Which, Iter))
            return Iter;
    return -1;
}
#endif
