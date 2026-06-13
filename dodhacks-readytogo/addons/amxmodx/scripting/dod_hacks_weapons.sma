
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <sqlx>
#include <dodconst>
#include <dodhacks>
#tryinclude <dod_hacks_knives> /// Works without this as well.

///
/// Either "mysql", "sqlite" or both should be enabled in /dod/addons/amxmodx/configs/modules.ini
/// (by removing the ; symbol from its/ their front).
///
/// Choose mysql if you have a host, user, password and remote sql database name.
/// Choose sqlite otherwise.
///
/// Both are threaded and won't cause any lag spikes at all.
///
/// mysql is better than sqlite with up to 10%.
///

#if !defined MPROP_PAGE_CALLBACK
#error AMX Mod X version too old to handle dod_hacks_weapons plugin! Consider upgrading! ('MPROP_PAGE_CALLBACK' is needed ...)
#endif

#define INT_CBasePlayer_m_rgAmmo_ANade     290 /// Allies/ British (weapon_handgrenade)     player ammo count (int CBasePlayer::m_rgAmmo[32]). Cell  #9.
// #define INT_CBasePlayer_m_rgAmmo_AANade 291 /// Allies/ British (weapon_handgrenade_ex)  player ammo count (int CBasePlayer::m_rgAmmo[32]). Cell #10.
#define INT_CBasePlayer_m_rgAmmo_GNade     292 /// Axis            (weapon_stickgrenade)    player ammo count (int CBasePlayer::m_rgAmmo[32]). Cell #11.
// #define INT_CBasePlayer_m_rgAmmo_AGNade 293 /// Axis            (weapon_stickgrenade_ex) player ammo count (int CBasePlayer::m_rgAmmo[32]). Cell #12.

#if !defined _dod_hacks_knives_included /// Works without this too.
native bool: DoD_IsUserWaitingThrowingKnife(Player);
#endif

new Handle: g_tmpSqlConnectionTuple = Empty_Handle; /// Non-threaded temporary database storage.
new Handle: g_threadedSqlConnectionTuple = Empty_Handle; /// Threaded database storage.
new DoD_Address: g_WpnBoxKill; /// CWeaponBox::Kill() function address.
new Array: g_Items[33]; /// Player item entity names.
new Array: g_Entities[33]; /// Player item entities.
new Array: g_Names; /// Weapon names.
new Array: g_Weapons; /// Weapon entity names.
new Array: g_Ammos; /// Weapon ammo names.
new Array: g_Counts; /// Weapon ammo counts.
new Array: g_Flags; /// Weapon admin flags.
new Array: g_Dropped; /// Dropped weapons to be deleted from map. << Only valid if dropped weapon deletion (when selecting a new one via the menu) is enabled >>.
new Array: g_Times; /// Timestamps of when the guns are being dropped. << Only valid if dropped weapon deletion (when selecting a new one via the menu) is enabled >>.
new bool: g_InSpawn[33]; /// Player is currently spawning.
new bool: g_InServer[33]; /// Player has fully joined.
new bool: g_IsAuth[33]; /// Player is authorized.
new bool: g_Fake[33]; /// Player is fake (a BOT).
new bool: g_UseFake; /// Whether to include fake players when team-limiting certain guns.
new bool: g_Local; /// Local storage in use.
new bool: g_HideMsg; /// Whether or not to hide the 'guns' chat command typed by the players.
new bool: g_British; /// Map is optimized for British against Axis.
new bool: g_Delete; /// Delete the dropped gun when selecting a new one via the menu.
new bool: g_HudStyle; /// Whether or not to use a visual effect on the HUD message(s).
new bool: g_ForceMaxOneNadePerSpawn; /// Whether or not to force max. 1 explo. grenade per player spawn (to be given).
new bool: g_badIsUserWaitingThrowingKnife = false; /// 'DoD_IsUserWaitingThrowingKnife' function has not been found on the server (if true).
new Float: g_SpawnTime[33]; /// Time when the player spawned.
new Float: g_EquipTime[33]; /// Time when the player got equipped.
new Float: g_TimeLimit; /// Seconds to be able to change the gun after being spawned.
new Float: g_Delay; /// Seconds between two menu selections.
new Float: g_LimitRocket; /// Rocket launcher team-limiting option.
new Float: g_LimitSniper; /// Sniper rifle team-limiting option.
new Float: g_LimitMachine; /// Machine gun team-limiting option.
new Float: g_DeleteTime; /// Time in seconds to delete the dropped gun when selecting a new one.
new Float: g_VerPos; /// HUD message(s) vertical position.
new Float: g_HorPos; /// HUD message(s) horizontal position.
new g_Buffer[256]; /// A large buffer.
new g_Steam[33][32]; /// Player Steam accounts.
new g_Gun[33]; /// Player gun index selection.
new g_Team[33]; /// Player team index.
new g_Page[33]; /// Player menu page.
new g_AlliesGrenade[32]; /// Allies custom-given grenade entity name.
new g_AxisGrenade[32]; /// Axis custom-given grenade entity name.
new g_BritishGrenade[32]; /// British custom-given grenade entity name.
new g_ColtAmmo[32]; /// Ammo name used for weapon_colt.
new g_LugerAmmo[32]; /// Ammo name used for weapon_luger.
new g_WebleyAmmo[32]; /// Ammo name used for weapon_webley.
new g_AlliesGrenades; /// Allies custom-given grenade(s) count.
new g_AxisGrenades; /// Axis custom-given grenade(s) count.
new g_BritishGrenades; /// British custom-given grenade(s) count.
new g_ColtCount; /// Colt custom-given ammo count.
new g_LugerCount; /// Luger custom-given ammo count.
new g_WebleyCount; /// Webley custom-given ammo count.
new g_WeaponsCount; /// Weapons into the current menu.
new g_HudMsg; /// HUD message(s) sync handle.
new g_Menu; /// Guns menu handle.
new g_MaxPlayers; /// Maximum players server can handle.
new g_Flag; /// Admin access required for using this feature ('guns' command).
new g_FlagNades; /// Admin access required for receiving explosive grenade(s) during spawn.
new g_FlagHandGuns; /// Admin access required for receiving hand gun ammo during spawn.
new Handle: g_tmpSqlConnection = Empty_Handle;
new bool: g_initiallyConnected = false;

public plugin_init()
{
    register_plugin("DoD Hacks: Weapons", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    if (!F_SafeIsSqlModuleRunning("sqlite") && !F_SafeIsSqlModuleRunning("mysql"))
    {
        set_fail_state("'../amxmodx/configs/modules.ini' needs either sqlite or mysql enabled!");
        return PLUGIN_HANDLED;
    }
    get_configsdir(g_Buffer, charsmax(g_Buffer));
    add(g_Buffer, charsmax(g_Buffer), "/dod_hacks_weapons.ini");
    new Config = fopen(g_Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", g_Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }
    g_Names = ArrayCreate(32);
    g_Weapons = ArrayCreate(32);
    g_Ammos = ArrayCreate(32);
    g_Flags = ArrayCreate();
    g_Counts = ArrayCreate();
    g_British = DoD_AreAlliesBritish();
    new bool: AdminAsterisk, bool: allowAll, FlagsNum, Driver[16], User[32], Pass[32],
        Db[32], Host[32], Weapon[32], Name[32], Team[32], Flags[32], Ammo[32], Count[8],
        errCode, Err[4];
    while (fgets(Config, g_Buffer, charsmax(g_Buffer)) > 0)
    {
        trim(g_Buffer);
        if (!g_Buffer[0] || g_Buffer[0] == ';' || g_Buffer[0] == '/')
            continue;
        if (parse(g_Buffer, Weapon, charsmax(Weapon), Name, charsmax(Name), Team, charsmax(Team),
            Flags, charsmax(Flags), Ammo, charsmax(Ammo), Count, charsmax(Count)) < 6)
        {
            if (g_British && equali(Weapon, "@menu_title_british"))
                g_Menu = menu_create(Name, "OnWeaponMenuItem");
            else if (false == g_British && equali(Weapon, "@menu_title_allies"))
                g_Menu = menu_create(Name, "OnWeaponMenuItem");
            else if (equali(Weapon, "@db_driver"))
            {
                copy(Driver, charsmax(Driver), Name);
                g_Local = bool: equali(Driver, "sqlite");
            }
            else if (equali(Weapon, "@db_user"))
                copy(User, charsmax(User), Name);
            else if (equali(Weapon, "@db_pass"))
                copy(Pass, charsmax(Pass), Name);
            else if (equali(Weapon, "@db_host"))
                copy(Host, charsmax(Host), Name);
            else if (equali(Weapon, "@db_name"))
                copy(Db, charsmax(Db), Name);
            else if (equali(Weapon, "@weapon_colt"))
            {
                copy(g_ColtAmmo, charsmax(g_ColtAmmo), Team);
                g_ColtCount = str_to_num(Name);
            }
            else if (equali(Weapon, "@weapon_luger"))
            {
                copy(g_LugerAmmo, charsmax(g_LugerAmmo), Team);
                g_LugerCount = str_to_num(Name);
            }
            else if (equali(Weapon, "@weapon_webley"))
            {
                copy(g_WebleyAmmo, charsmax(g_WebleyAmmo), Team);
                g_WebleyCount = str_to_num(Name);
            }
            else if (equali(Weapon, "@grenade_allies"))
            {
                copy(g_AlliesGrenade, charsmax(g_AlliesGrenade), Team);
                g_AlliesGrenades = str_to_num(Name);
            }
            else if (equali(Weapon, "@grenade_axis"))
            {
                copy(g_AxisGrenade, charsmax(g_AxisGrenade), Team);
                g_AxisGrenades = str_to_num(Name);
            }
            else if (equali(Weapon, "@grenade_british"))
            {
                copy(g_BritishGrenade, charsmax(g_BritishGrenade), Team);
                g_BritishGrenades = str_to_num(Name);
            }
            else if (equali(Weapon, "@max_seconds"))
                g_TimeLimit = str_to_float(Name);
            else if (equali(Weapon, "@hide_chat_cmd"))
                g_HideMsg = bool: str_to_num(Name);
            else if (equali(Weapon, "@selection_delay"))
                g_Delay = str_to_float(Name);
            else if (equali(Weapon, "@admin_asterisk"))
                AdminAsterisk = bool: str_to_num(Name);
            else if (equali(Weapon, "@include_bots"))
                g_UseFake = bool: str_to_num(Name);
            else if (equali(Weapon, "@drop_delete"))
                g_Delete = bool: str_to_num(Name);
            else if (equali(Weapon, "@delete_time"))
                g_DeleteTime = str_to_float(Name);
            else if (equali(Weapon, "@limit_machine"))
                g_LimitMachine = str_to_float(Name);
            else if (equali(Weapon, "@limit_sniper"))
                g_LimitSniper = str_to_float(Name);
            else if (equali(Weapon, "@limit_rocket"))
                g_LimitRocket = str_to_float(Name);
            else if (equali(Weapon, "@hud_effects"))
                g_HudStyle = bool: str_to_num(Name);
            else if (equali(Weapon, "@ver_pos"))
                g_VerPos = str_to_float(Name);
            else if (equali(Weapon, "@hor_pos"))
                g_HorPos = str_to_float(Name);
            else if (equali(Weapon, "@allow_all"))
                allowAll = bool: str_to_num(Name);
            else if (equali(Weapon, "@admin_access"))
                g_Flag = read_flags(Name);
            else if (equali(Weapon, "@nades_access"))
                g_FlagNades = read_flags(Name);
            else if (equali(Weapon, "@handguns_access"))
                g_FlagHandGuns = read_flags(Name);
            else if (equali(Weapon, "@g_ForceMaxOneNadePerSpawn"))
                g_ForceMaxOneNadePerSpawn = bool: str_to_num(Name);
            continue;
        }
        if (false == allowAll &&
            ((false == g_British && 'B' == Team[0]) || (g_British && 'U' == Team[0])))
            continue;
        FlagsNum = read_flags(Flags);
        ArrayPushCell(g_Flags, read_flags(Flags));
        if (AdminAsterisk && FlagsNum > 0)
            add(Name, charsmax(Name), " *");
        ArrayPushString(g_Names, Name);
        ArrayPushString(g_Weapons, Weapon);
        ArrayPushString(g_Ammos, Ammo);
        ArrayPushCell(g_Counts, str_to_num(Count));
        menu_additem(g_Menu, Name);
    }
    fclose(Config);

    g_MaxPlayers = get_maxplayers();
    for (new Iter = 1; Iter <= g_MaxPlayers; Iter++)
    {
        g_Items[Iter] = ArrayCreate(32);
        g_Entities[Iter] = ArrayCreate();
    }
    g_WeaponsCount = ArraySize(g_Names);
    g_HudMsg = CreateHudSyncObj();
    menu_setprop(g_Menu, MPROP_PAGE_CALLBACK, "OnWeaponMenuPage");
    register_clcmd("guns", "OnPlayerClConCmd_Guns", ADMIN_ALL, "- displays the guns menu");
    register_clcmd("weapons", "OnPlayerClConCmd_Guns", ADMIN_ALL, "- displays the guns menu");
#if defined amxclient_cmd && defined RegisterHamPlayer
    RegisterHamPlayer(Ham_Killed, "OnPlayerKilled_Post", true);
#else
    RegisterHam(Ham_Killed, "player", "OnPlayerKilled_Post", true);
#endif
    /**
     * If a player selects the Marksman class (scoped Enfield gun), for example, do not automatically
     * add a scope to it during player spawn function by default.
     */
    if (DoD_IsAutoScopingEnabled())
        DoD_DisableAutoScoping();
    g_WpnBoxKill = DoD_GetFunctionAddress(DoD_FI_WpnBoxKill, false);
    if (DoD_Address_Null == g_WpnBoxKill)
        g_Delete = false; /// Can't delete, no arrays.
    else if (g_Delete)
    { /// Can delete, arrays.
        g_Dropped = ArrayCreate();
        g_Times = ArrayCreate();
        register_forward(FM_RemoveEntity, "OnRemoveEntity_Post", true);
    }
    SQL_GetAffinity(g_Buffer, charsmax(g_Buffer));
    if (!equali(g_Buffer, Driver))
        if (!SQL_SetAffinity(Driver))
            log_amx("SQL_SetAffinity('%s') call failed. Ensure the module is enabled in '../amxmodx/configs/modules.ini'.",
                Driver);
    g_threadedSqlConnectionTuple = SQL_MakeDbTuple(Host, User, Pass, Db);
    g_tmpSqlConnectionTuple = SQL_MakeDbTuple(Host, User, Pass, Db, 3);
    g_tmpSqlConnection = SQL_Connect(g_tmpSqlConnectionTuple, errCode, Err, charsmax(Err));
    if (Empty_Handle != g_tmpSqlConnection)
    {
        g_initiallyConnected = true;
        SQL_FreeHandle(g_tmpSqlConnection);
        g_tmpSqlConnection = Empty_Handle;
        SQL_FreeHandle(g_tmpSqlConnectionTuple);
        g_tmpSqlConnectionTuple = Empty_Handle;
        if (g_British)
        {
            if (g_Local)
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "EmptySqlHandler",
                    "CREATE TABLE IF NOT EXISTS guns_british (gun TINYINT NOT NULL, steam CHARACTER (32) NOT NULL UNIQUE COLLATE NOCASE, PRIMARY KEY (steam), UNIQUE (steam))");
            else
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "EmptySqlHandler",
                    "CREATE TABLE IF NOT EXISTS guns_british (gun TINYINT NOT NULL, steam CHAR (32) NOT NULL COLLATE utf8mb4_unicode_520_ci, PRIMARY KEY (steam), UNIQUE (steam)) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_520_ci");
        }
        else
        {
            if (g_Local)
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "EmptySqlHandler",
                    "CREATE TABLE IF NOT EXISTS guns_allies (gun TINYINT NOT NULL, steam CHARACTER (32) NOT NULL UNIQUE COLLATE NOCASE, PRIMARY KEY (steam), UNIQUE (steam))");
            else
                SQL_ThreadQuery(g_threadedSqlConnectionTuple, "EmptySqlHandler",
                    "CREATE TABLE IF NOT EXISTS guns_allies (gun TINYINT NOT NULL, steam CHAR (32) NOT NULL COLLATE utf8mb4_unicode_520_ci, PRIMARY KEY (steam), UNIQUE (steam)) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_520_ci");
        }
    }
    else
    {
        log_amx("Weapons plugin loaded with MySQL server being offline.");
        log_amx("As soon as the connection establishes, map will restart automatically.");
        log_amx("If you are using SQLite, 'sqlite' module needs to be enabled in '../amxmodx/configs/modules.ini'.");
        set_task(1.0, "Task_VerifyConnection");
    }
    return PLUGIN_CONTINUE;
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

public client_putinserver(Player)
{
    g_InServer[Player] = true;
    g_Fake[Player] = bool: is_user_bot(Player);

    ArrayClear(g_Items[Player]);
    ArrayClear(g_Entities[Player]);
}

#if defined client_connectex
public client_authorized(Player, const Steam[])
#else
public client_authorized(Player)
#endif
{
#if !defined client_connectex
    static Steam[32], Info[32];
    get_user_authid(Player, Steam, charsmax(Steam));
#else
    static Info[32];
#endif
    g_IsAuth[Player] = true;
    if (copy(g_Steam[Player], charsmax(g_Steam[]), Steam) > 4 && g_initiallyConnected)
    { /// Skipping fake players (long enough Steam string).
        if (g_British)
            formatex(g_Buffer, charsmax(g_Buffer),
                "SELECT gun FROM guns_british WHERE steam = '%s'", Steam);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "SELECT gun FROM guns_allies WHERE steam = '%s'", Steam);

        num_to_str(get_user_userid(Player), Info, charsmax(Info));
        SQL_ThreadQuery(g_threadedSqlConnectionTuple, "OnSqlSelection", g_Buffer, Info, sizeof Info);
    }
}

public client_connect(Player)
    F_ZeroPlayerVars(Player);

public DoD_OnPlayerSpawn(DoD_Address: CDoDTeamPlay, &Player)
    if (g_InServer[Player])
        g_InSpawn[Player] = true;

public DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player)
{
    static Weapon[32], Name[32], Ammo[32], Count, Item, Res, Iter, Flags, Access, Players, Float: Percent;
    if (g_InServer[Player])
    {
        g_InSpawn[Player] = false;
        g_Team[Player] = get_user_team(Player);
        g_SpawnTime[Player] = get_gametime();
        Access = get_user_flags(Player);
        if (g_Flag != (Access & g_Flag))
            goto Skip;
        if (g_Gun[Player] < 0)
        {
            if (false == g_Fake[Player])
            {
                if (false == F_IsViewingAMenu(Player))
                    F_ShowMenu(Player, true);
                else
                    client_print(Player, print_chat, "* Type 'guns' to choose a gun.");
            }
        }
        else
        {
            ArrayGetString(g_Names, g_Gun[Player], Name, charsmax(Name));
            Flags = ArrayGetCell(g_Flags, g_Gun[Player]);
            if (Flags != (get_user_flags(Player) & Flags))
            {
                if (false == F_IsViewingAMenu(Player))
                {
                    F_ShowMenu(Player, true);
                    client_print(Player, print_chat,
                        "* No more access for %s.", Name);
                }
                else
                    client_print(Player, print_chat,
                        "* No access for %s. Type 'guns' for more.", Name);
            }
            else
            {
                ArrayGetString(g_Weapons, g_Gun[Player], Weapon, charsmax(Weapon));
                if (F_IsSniper(Weapon))
                {
                    if (F_SniperLimitReached(Player, g_Team[Player], Players, Percent))
                    {
                        if (false == F_IsViewingAMenu(Player))
                        {
                            F_ShowMenu(Player, true);
                            if (1.0 != g_LimitSniper)
                                client_print(Player, print_chat, "* Sniper gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitSniper, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Sniper gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                        }
                        else
                        {
                            if (1.0 != g_LimitSniper)
                                client_print(Player, print_chat, "* Sniper gun limit of %0.2f%% reached (%d or %0.2f%%), type 'guns' for more.", g_LimitSniper, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Sniper gun limit of 1 reached (%d or %0.2f%%), type 'guns' for more.", Players, Percent);
                        }
                        goto Skip;
                    }
                }
                else if (F_IsMachine(Weapon))
                {
                    if (F_MachineLimitReached(Player, g_Team[Player], Players, Percent))
                    {
                        if (false == F_IsViewingAMenu(Player))
                        {
                            F_ShowMenu(Player, true);
                            if (1.0 != g_LimitMachine)
                                client_print(Player, print_chat, "* Machine gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitMachine, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Machine gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                        }
                        else
                        {
                            if (1.0 != g_LimitMachine)
                                client_print(Player, print_chat, "* Machine gun limit of %0.2f%% reached (%d or %0.2f%%), type 'guns' for more.", g_LimitMachine, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Machine gun limit of 1 reached (%d or %0.2f%%), type 'guns' for more.", Players, Percent);
                        }
                        goto Skip;
                    }
                }
                else if (F_IsRocket(Weapon))
                {
                    if (F_RocketLimitReached(Player, g_Team[Player], Players, Percent))
                    {
                        if (false == F_IsViewingAMenu(Player))
                        {
                            F_ShowMenu(Player, true);
                            if (1.0 != g_LimitRocket)
                                client_print(Player, print_chat, "* Rocket gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitRocket, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Rocket gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                        }
                        else
                        {
                            if (1.0 != g_LimitRocket)
                                client_print(Player, print_chat, "* Rocket gun limit of %0.2f%% reached (%d or %0.2f%%), type 'guns' for more.", g_LimitRocket, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Rocket gun limit of 1 reached (%d or %0.2f%%), type 'guns' for more.", Players, Percent);
                        }
                        goto Skip;
                    }
                }
                ArrayGetString(g_Ammos, g_Gun[Player], Ammo, charsmax(Ammo));
                if (DoD_GiveNamedItem(Player, Weapon, Item) && Item > g_MaxPlayers)
                {
                    Count = ArrayGetCell(g_Counts, g_Gun[Player]);
                    if (Count > 0)
                        DoD_GiveAmmo(Player, Count, Ammo, Count, Res);
                }
            }
        }
Skip:
        if (g_FlagNades != (Access & g_FlagNades))
            goto SkipNades;
        if (false == F_HasGrenade(Player, Weapon, charsmax(Weapon), Item))
        {
            switch (g_Team[Player])
            {
                case 1:
                {
                    if (g_British)
                    {
                        for (Iter = 0; Iter < g_BritishGrenades; Iter++)
                            DoD_GiveNamedItem(Player, g_BritishGrenade, Item);
                    }
                    else
                    {
                        for (Iter = 0; Iter < g_AlliesGrenades; Iter++)
                            DoD_GiveNamedItem(Player, g_AlliesGrenade, Item);
                    }
                }
                default:
                    for (Iter = 0; Iter < g_AxisGrenades; Iter++)
                        DoD_GiveNamedItem(Player, g_AxisGrenade, Item);
            }
        }
SkipNades:
        if (g_FlagHandGuns == (Access & g_FlagHandGuns) &&
            F_HasSecondaryWeapon(Player, Weapon, charsmax(Weapon), Item))
        {
            if (g_WebleyCount > 0 && equali(Weapon[7], "webley"))
                DoD_GiveAmmo(Player, g_WebleyCount, g_WebleyAmmo, g_WebleyCount, Res);
            else if (g_ColtCount > 0 && equali(Weapon[7], "colt"))
                DoD_GiveAmmo(Player, g_ColtCount, g_ColtAmmo, g_ColtCount, Res);
            else if (g_LugerCount > 0)
                DoD_GiveAmmo(Player, g_LugerCount, g_LugerAmmo, g_LugerCount, Res);
        }
    }
}

public OnWeaponMenuPage(Player, Status)
{
    switch (Status)
    {
        case MENU_BACK:
            if (g_Page[Player] > 0)
                --g_Page[Player];
        case MENU_MORE:
            ++g_Page[Player];
        case MENU_EXIT:
            g_Page[Player] = 0;
    }
}

public Task_VerifyConnection()
{
    static Map[64], errCode, Err[4];
    g_tmpSqlConnection = SQL_Connect(g_tmpSqlConnectionTuple, errCode, Err, charsmax(Err));
    if (Empty_Handle != g_tmpSqlConnection)
    {
        SQL_FreeHandle(g_tmpSqlConnection);
        if (g_tmpSqlConnectionTuple != Empty_Handle)
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

public OnWeaponMenuItem(Player, Menu, Item)
{
    static Weapon[32], Name[32], Buffer[32], Access, Count, Flags, Entity, Res, Float: Time, Float: Wait, Players, Float: Percent;
    if (Item > -1)
    {
        Access = get_user_flags(Player);
        if (g_Flag != (Access & g_Flag))
            goto NoAccess;
        Flags = ArrayGetCell(g_Flags, Item);
        if (Flags != (Access & Flags))
        {
            F_ShowMenu(Player, false, g_Page[Player]);
NoAccess:
            set_hudmessage(20 /** Red. */, 60 /** Green. */, 180 /** Blue. */,
                g_HorPos /** Horizontal position. */, g_VerPos /** Vertical position. */,
                g_HudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                2.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
            ShowSyncHudMsg(Player, g_HudMsg, "NO ACCESS!");
            return PLUGIN_HANDLED;
        }
        ArrayGetString(g_Weapons, Item, Weapon, charsmax(Weapon));
        if (F_IsSniper(Weapon))
        {
            if (F_SniperLimitReached(Player, g_Team[Player], Players, Percent))
            {
                F_ShowMenu(Player, false, g_Page[Player]);
                set_hudmessage(180 /** Red. */, 20 /** Green. */, 100 /** Blue. */,
                    g_HorPos /** Horizontal position. */, g_VerPos /** Vertical position. */,
                    g_HudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    3.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                if (1.0 != g_LimitSniper)
                    ShowSyncHudMsg(Player, g_HudMsg, "SNIPER LIMIT OF %0.2f%% REACHED: %d OR %0.2f%%!", g_LimitSniper, Players, Percent);
                else
                    ShowSyncHudMsg(Player, g_HudMsg, "SNIPER LIMIT OF 1 REACHED: %d OR %0.2f%%!", Players, Percent);
                return PLUGIN_HANDLED;
            }
        }
        else if (F_IsMachine(Weapon))
        {
            if (F_MachineLimitReached(Player, g_Team[Player], Players, Percent))
            {
                F_ShowMenu(Player, false, g_Page[Player]);
                set_hudmessage(180 /** Red. */, 20 /** Green. */, 100 /** Blue. */,
                    g_HorPos /** Horizontal position. */, g_VerPos /** Vertical position. */,
                    g_HudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    3.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                if (1.0 != g_LimitMachine)
                    ShowSyncHudMsg(Player, g_HudMsg, "MACHINE LIMIT OF %0.2f%% REACHED: %d OR %0.2f%%!", g_LimitMachine, Players, Percent);
                else
                    ShowSyncHudMsg(Player, g_HudMsg, "MACHINE LIMIT OF 1 REACHED: %d OR %0.2f%%!", Players, Percent);
                return PLUGIN_HANDLED;
            }
        }
        else if (F_IsRocket(Weapon))
        {
            if (F_RocketLimitReached(Player, g_Team[Player], Players, Percent))
            {
                F_ShowMenu(Player, false, g_Page[Player]);
                set_hudmessage(180 /** Red. */, 20 /** Green. */, 100 /** Blue. */,
                    g_HorPos /** Horizontal position. */, g_VerPos /** Vertical position. */,
                    g_HudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    3.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                if (1.0 != g_LimitRocket)
                    ShowSyncHudMsg(Player, g_HudMsg, "ROCKET LIMIT OF %0.2f%% REACHED: %d OR %0.2f%%!", g_LimitRocket, Players, Percent);
                else
                    ShowSyncHudMsg(Player, g_HudMsg, "ROCKET LIMIT OF 1 REACHED: %d OR %0.2f%%!", Players, Percent);
                return PLUGIN_HANDLED;
            }
        }
        g_Gun[Player] = Item;
        Time = get_gametime();
        ArrayGetString(g_Names, Item, Name, charsmax(Name));
        if (is_user_alive(Player) && (!g_TimeLimit || Time - g_SpawnTime[Player] < g_TimeLimit))
        {
            if (Time - g_EquipTime[Player] < g_Delay)
            {
                F_ShowMenu(Player, false, g_Page[Player]);
                Wait = g_Delay - (Time - g_EquipTime[Player]);
                set_hudmessage(200 /** Red. */, 20 /** Green. */, 20 /** Blue. */,
                    g_HorPos /** Horizontal position. */, g_VerPos /** Vertical position. */,
                    g_HudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    Wait /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                ShowSyncHudMsg(Player, g_HudMsg, "WAIT %.1f SEC!", Wait);
                return PLUGIN_HANDLED;
            }
            if (false == g_badIsUserWaitingThrowingKnife && DoD_IsUserWaitingThrowingKnife(Player))
            {
                F_ShowMenu(Player, false, g_Page[Player]);
                set_hudmessage(200 /** Red. */, 160 /** Green. */, 20 /** Blue. */,
                    g_HorPos /** Horizontal position. */, g_VerPos /** Vertical position. */,
                    g_HudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    Wait /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                ShowSyncHudMsg(Player, g_HudMsg, "WAIT FOR THE KNIFE!");
                return PLUGIN_HANDLED;
            }
            if (F_HasPrimaryWeapon(Player, Buffer, charsmax(Buffer), Entity))
            {
                if (g_Delete)
                {
                    ArrayPushCell(g_Dropped, Entity);
                    ArrayPushCell(g_Times, Time);
                }
                DoD_DropPlayerItem(Player, Buffer, true);
                Res = ArrayFindString(g_Items[Player], Buffer);
                if (Res > -1)
                {
                    ArrayDeleteItem(g_Items[Player], Res);
                    ArrayDeleteItem(g_Entities[Player], Res);
                }
            }
            if (DoD_GiveNamedItem(Player, Weapon, Entity) && Entity > g_MaxPlayers)
            {
                client_print(Player, print_chat, "* %s equipped. Type 'guns' for more.", Name);
                g_EquipTime[Player] = Time;
                Count = ArrayGetCell(g_Counts, Item);
                if (Count > 0)
                {
                    ArrayGetString(g_Ammos, Item, Buffer, charsmax(Buffer));
                    DoD_GiveAmmo(Player, Count, Buffer, Count, Res);
                }
            }
        }
        else
            client_print(Player, print_chat, "* %s selected. Type 'guns' for more.", Name);
        if (g_initiallyConnected && g_IsAuth[Player])
            F_StoreSelection(Player); /// Store the selection.
    }
    return PLUGIN_CONTINUE;
}

public OnPlayerKilled_Post(Player)
{
    ArrayClear(g_Items[Player]);
    ArrayClear(g_Entities[Player]);
}

public DoD_OnPackWeapon(Entity, &Weapon, &OverrideRes)
{ /// Getting the weaponbox entity index by weapon entity index.
    static Item, Float: Time;
    if (false == g_Delete) /// Continue dropping the gun.
        return PLUGIN_CONTINUE;
    Item = ArrayFindValue(g_Dropped, Weapon);
    if (Item > -1)
    { /// Remove the dropped weapon.
        Time = ArrayGetCell(g_Times, Item);
        if (get_gametime() - Time < 0.01)
            set_task(0.000001, "Task_RemoveWeaponBox", Entity); /// Remove it only if recently dropped.
        ArrayDeleteItem(g_Dropped, Item);
        ArrayDeleteItem(g_Times, Item);
    } /// Continue dropping the gun.
    return PLUGIN_CONTINUE;
}

public OnRemoveEntity_Post(Entity)
    F_EraseEntityFromDroppedGuns(Entity);

public DoD_OnSubRemove_Post(Entity)
    if (g_Delete)
        F_EraseEntityFromDroppedGuns(Entity);

public DoD_OnUtilRemove_Post(Entity)
    if (g_Delete)
        F_EraseEntityFromDroppedGuns(Entity);

public Task_RemoveWeaponBox(Entity)
{
    static Class[16], Delta;
    if (pev_valid(Entity) < 1 ||
        pev(Entity, pev_classname, Class, charsmax(Class)) < 1 || !equali("weaponbox", Class))
        return PLUGIN_HANDLED; /// Skip.

    if (g_WpnBoxKill != DoD_GetEntityThinkFunc(Entity, Delta)) /// Entity not yet prepared to get destroyed.
        set_task(0.000001, "Task_RemoveWeaponBox", Entity); /// Retry.
    else
        set_pev(Entity, pev_nextthink, get_gametime() + g_DeleteTime); /// Set it auto. remove at a desired time.
    return PLUGIN_HANDLED;
}

public DOD_ON_PLAYER_DISCONNECTED
{
    if (g_initiallyConnected && g_IsAuth[Player] && g_Gun[Player] > -1)
        F_StoreSelection(Player); /// Store the selection.
    ArrayClear(g_Items[Player]);
    ArrayClear(g_Entities[Player]);
    F_ZeroPlayerVars(Player);
}

public EmptySqlHandler(FailState, Handle: Query, const Error[], ErrorNum)
    if (TQUERY_QUERY_FAILED == FailState)
        log_amx("SQL Error (#%d): %s", ErrorNum, Error);

#if defined FindPlayerFlags
public OnSqlSelection(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static Player, Gun, Players, Float: Percent, Name[32];
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
    {
        Player = find_player_ex(FindPlayer_IncludeConnecting | FindPlayer_MatchUserId, str_to_num(Data));
        if (Player > 0)
        {
            Gun = SQL_ReadResult(Query, 0);
            if (Gun < 0 || Gun >= g_WeaponsCount)
            {
                g_Gun[Player] = -1;
                if (g_InServer[Player])
                        client_print(Player, print_chat, "* Operator altered gun menu entries, so your gun selection expired.");
            }
            else
            {
                g_Gun[Player] = Gun;
                if (g_InServer[Player])
                {
                    ArrayGetString(g_Weapons, Gun, Name, charsmax(Name));
                    if (F_IsSniper(Name))
                    {
                        if (F_SniperLimitReached(Player, g_Team[Player], Players, Percent))
                        {
                            if (1.0 == g_LimitSniper)
                                client_print(Player, print_chat, "* Sniper gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitSniper, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Sniper gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                        }
                        else
                        {
                            ArrayGetString(g_Names, Gun, Name, charsmax(Name));
                            client_print(Player, print_chat, "* Your gun selection updated: %s.", Name);
                        }
                    }
                    else if (F_IsMachine(Name))
                    {
                        if (F_MachineLimitReached(Player, g_Team[Player], Players, Percent))
                        {
                            if (1.0 == g_LimitMachine)
                                client_print(Player, print_chat, "* Machine gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitMachine, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Machine gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                        }
                        else
                        {
                            ArrayGetString(g_Names, Gun, Name, charsmax(Name));
                            client_print(Player, print_chat, "* Your gun selection updated: %s.", Name);
                        }
                    }
                    else if (F_IsRocket(Name))
                    {
                        if (F_RocketLimitReached(Player, g_Team[Player], Players, Percent))
                        {
                            if (1.0 == g_LimitRocket)
                                client_print(Player, print_chat, "* Rocket gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitRocket, Players, Percent);
                            else
                                client_print(Player, print_chat, "* Rocket gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                        }
                        else
                        {
                            ArrayGetString(g_Names, Gun, Name, charsmax(Name));
                            client_print(Player, print_chat, "* Your gun selection updated: %s.", Name);
                        }
                    }
                    else
                    {
                        ArrayGetString(g_Names, Gun, Name, charsmax(Name));
                        client_print(Player, print_chat, "* Your gun selection updated: %s.", Name);
                    }
                }
            }
        }
    }
}
#else
public OnSqlSelection(FailState, Handle: Query, const Error[], ErrorNum, const Data[])
{
    static UniqueIndex, Player, Gun, Players, Float: Percent, Name[32];
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
    {
        UniqueIndex = str_to_num(Data);
        for (Player = 1; Player <= g_MaxPlayers; Player++)
        {
            if (UniqueIndex == get_user_userid(Player))
            {
                Gun = SQL_ReadResult(Query, 0);
                if (Gun < 0 || Gun >= g_WeaponsCount)
                {
                    g_Gun[Player] = -1;
                    if (g_InServer[Player])
                        client_print(Player, print_chat, "* Operator altered gun menu entries, so your gun selection expired.");
                }
                else
                {
                    g_Gun[Player] = Gun;
                    if (g_InServer[Player])
                    {
                        ArrayGetString(g_Weapons, Gun, Name, charsmax(Name));
                        if (F_IsSniper(Name))
                        {
                            if (F_SniperLimitReached(Player, g_Team[Player], Players, Percent))
                            {
                                if (1.0 == g_LimitSniper)
                                    client_print(Player, print_chat, "* Sniper gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitSniper, Players, Percent);
                                else
                                    client_print(Player, print_chat, "* Sniper gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                            }
                            else
                            {
                                ArrayGetString(g_Names, Gun, Name, charsmax(Name));
                                client_print(Player, print_chat, "* Your gun selection updated: %s.", Name);
                            }
                        }
                        else if (F_IsMachine(Name))
                        {
                            if (F_MachineLimitReached(Player, g_Team[Player], Players, Percent))
                            {
                                if (1.0 == g_LimitMachine)
                                    client_print(Player, print_chat, "* Machine gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitMachine, Players, Percent);
                                else
                                    client_print(Player, print_chat, "* Machine gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                            }
                            else
                            {
                                ArrayGetString(g_Names, Gun, Name, charsmax(Name));
                                client_print(Player, print_chat, "* Your gun selection updated: %s.", Name);
                            }
                        }
                        else if (F_IsRocket(Name))
                        {
                            if (F_RocketLimitReached(Player, g_Team[Player], Players, Percent))
                            {
                                if (1.0 == g_LimitRocket)
                                    client_print(Player, print_chat, "* Rocket gun limit of %0.2f%% reached (%d or %0.2f%%), select a new gun.", g_LimitRocket, Players, Percent);
                                else
                                    client_print(Player, print_chat, "* Rocket gun limit of 1 reached (%d or %0.2f%%), select a new gun.", Players, Percent);
                            }
                            else
                            {
                                ArrayGetString(g_Names, Gun, Name, charsmax(Name));
                                client_print(Player, print_chat, "* Your gun selection updated: %s.", Name);
                            }
                        }
                        else
                        {
                            ArrayGetString(g_Names, Gun, Name, charsmax(Name));
                            client_print(Player, print_chat, "* Your gun selection updated: %s.", Name);
                        }
                    }
                }
                break;
            }
        }
    }
}
#endif

public DoD_OnGiveNamedItem(Player, Item[], ItemSize /** = 64 */, &OverrideRes)
{ /// Item may be altered during execution.
    if (!g_InSpawn[Player])
        return PLUGIN_CONTINUE; /// Skip.
    if (g_ForceMaxOneNadePerSpawn &&
        (get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_ANade) ||
        get_pdata_int(Player, INT_CBasePlayer_m_rgAmmo_GNade)) &&
        DoD_IsWeaponGrenade(Item))
        return PLUGIN_HANDLED;
    if (g_Fake[Player])
        return PLUGIN_CONTINUE; /// Skip.
    if (DoD_IsWeaponPrimary(Item)) /// Do not auto. give a prim. weap. by default (excl. fake players).
        return PLUGIN_HANDLED;
    return PLUGIN_CONTINUE;
}

public DoD_OnGiveNamedItem_Post(Player, const Item[], Entity)
{ /// Perform weapon scoping if needed (fake players only).
    static Class;
    if (!g_InSpawn[Player] || !g_Fake[Player] || Entity <= g_MaxPlayers)
        return PLUGIN_CONTINUE; /// Skip.
    Class = pev(Player, pev_playerclass);
    switch (Class)
    { /// Add a scope if needed (fake players).
        case DODC_SCOPED_FG42: /// Scoped FG42 (Axis).
            if (equali(Item[7], "fg42"))
                DoD_AddScope(Entity, true);
        case DODC_MARKSMAN: /// Scoped Enfield (British).
            if (equali(Item[7], "enfield"))
                DoD_AddScope(Entity, true);
    }
    return PLUGIN_CONTINUE;
}

public client_command(Player)
{
    static Cmd[32], Arg[32];
    if (g_InServer[Player] && false == g_Fake[Player] && read_argv(0, Cmd, charsmax(Cmd)) > 2 &&
        (equali(Cmd, "say") || equali(Cmd, "say_team")))
    {
        if (read_argv(1, Arg, charsmax(Arg)) > 3 &&
            (equali(Arg, "guns") || equali(Arg, "weapons") ||
            equali(Arg, "!guns") || equali(Arg, "!weapons") ||
            equali(Arg, "/guns") || equali(Arg, "/weapons") ||
            equali(Arg, ".guns") || equali(Arg, ".weapons") ||
            equali(Arg, ",guns") || equali(Arg, ",weapons")))
        {
            if (g_Flag != (get_user_flags(Player) & g_Flag))
            {
                client_print(Player, print_chat, "* No access to use this feature.");
                return g_HideMsg ? PLUGIN_HANDLED /** Block showing the typed msg. */ : PLUGIN_CONTINUE;
            }
            if (false == F_IsViewingAMenu(Player))
                F_ShowMenu(Player, true);
            else
                client_print(Player, print_chat, "* You're already viewing a menu.");
            if (g_HideMsg) /// Block showing the typed message.
                return PLUGIN_HANDLED;
        }
    }
    return PLUGIN_CONTINUE;
}

public OnPlayerClConCmd_Guns(Player)
{
    if (false == g_InServer[Player] || g_Fake[Player])
        return PLUGIN_HANDLED; /// Block.
    if (g_Flag != (get_user_flags(Player) & g_Flag))
    {
        client_print(Player, print_chat, "* No access to use this feature.");
        return PLUGIN_HANDLED;
    }
    if (false == F_IsViewingAMenu(Player))
        F_ShowMenu(Player, true);
    else
        client_print(Player, print_chat, "* You're already viewing a menu.");
    return PLUGIN_HANDLED;
}

public DoD_OnAddPlayerItem_Post(Player, Item, Res)
{
    static Class[32];
    if (Res > 0 && pev(Item, pev_classname, Class, charsmax(Class)) > 0 &&
        ArrayFindString(g_Items[Player], Class) < 0)
    {
        ArrayPushString(g_Items[Player], Class);
        ArrayPushCell(g_Entities[Player], Item);
    }
}

public DoD_OnRemovePlayerItem_Post(Player, Item, Res)
{
    static Index, Class[32];
    if (Res > 0 && pev(Item, pev_classname, Class, charsmax(Class)) > 0)
    {
        Index = ArrayFindString(g_Items[Player], Class);
        if (Index > -1)
        {
            ArrayDeleteItem(g_Items[Player], Index);
            ArrayDeleteItem(g_Entities[Player], Index);
        }
    }
}

public DoD_OnRemoveAllItems_Post(Player, RemoveSuit)
{
    ArrayClear(g_Items[Player]);
    ArrayClear(g_Entities[Player]);
}

F_ShowMenu(Player, bool: PageZero, Page = 0)
{
    if (PageZero)
        g_Page[Player] = 0;
    menu_display(Player, g_Menu, Page);
}

bool: F_IsViewingAMenu(Player)
{
    static Old, New;
    player_menu_info(Player, Old, New);
    return Old > 0 || New > -1;
}

bool: F_HasGrenade(Player, Weapon[], Size, &Entity)
{
    static Item[32], Iter, Num;
    Entity = -1;
    Num = ArraySize(g_Items[Player]);
    for (Iter = 0; Iter < Num; Iter++)
    {
        ArrayGetString(g_Items[Player], Iter, Item, charsmax(Item));
        if (DoD_IsWeaponGrenade(Item))
        {
            copy(Weapon, Size, Item);
            Entity = ArrayGetCell(g_Entities[Player], Iter);
            return true;
        }
    }
    return false;
}

bool: F_HasSecondaryWeapon(Player, Weapon[], Size, &Entity)
{
    static Item[32], Iter, Num;
    Entity = -1;
    Num = ArraySize(g_Items[Player]);
    for (Iter = 0; Iter < Num; Iter++)
    {
        ArrayGetString(g_Items[Player], Iter, Item, charsmax(Item));
        if (DoD_IsWeaponSecondary(Item))
        {
            copy(Weapon, Size, Item);
            Entity = ArrayGetCell(g_Entities[Player], Iter);
            return true;
        }
    }
    return false;
}

bool: F_HasPrimaryWeapon(Player, Weapon[], Size, &Entity)
{
    static Item[32], Iter, Num;
    Entity = -1;
    Num = ArraySize(g_Items[Player]);
    for (Iter = 0; Iter < Num; Iter++)
    {
        ArrayGetString(g_Items[Player], Iter, Item, charsmax(Item));
        if (DoD_IsWeaponPrimary(Item))
        {
            copy(Weapon, Size, Item);
            Entity = ArrayGetCell(g_Entities[Player], Iter);
            return true;
        }
    }
    return false;
}

F_ZeroPlayerVars(Player)
{
    g_Gun[Player] = -1;
    g_Team[Player] = 0;
    g_Page[Player] = 0;
    g_Fake[Player] = false;
    g_IsAuth[Player] = false;
    g_InSpawn[Player] = false;
    g_InServer[Player] = false;
    g_SpawnTime[Player] = 0.0;
    g_EquipTime[Player] = 0.0;
}

F_StoreSelection(Player)
{
    if (g_British)
    {
        if (g_Local)
            formatex(g_Buffer, charsmax(g_Buffer),
                "INSERT INTO guns_british VALUES (%d, '%s') ON CONFLICT (steam) DO UPDATE SET gun = %d",
                g_Gun[Player], g_Steam[Player], g_Gun[Player]);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "INSERT INTO guns_british VALUES (%d, '%s') ON DUPLICATE KEY UPDATE gun = %d",
                g_Gun[Player], g_Steam[Player], g_Gun[Player]);
    }
    else
    {
        if (g_Local)
            formatex(g_Buffer, charsmax(g_Buffer),
                "INSERT INTO guns_allies VALUES (%d, '%s') ON CONFLICT (steam) DO UPDATE SET gun = %d",
                g_Gun[Player], g_Steam[Player], g_Gun[Player]);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "INSERT INTO guns_allies VALUES (%d, '%s') ON DUPLICATE KEY UPDATE gun = %d",
                g_Gun[Player], g_Steam[Player], g_Gun[Player]);
    }
    SQL_ThreadQuery(g_threadedSqlConnectionTuple, "EmptySqlHandler", g_Buffer);
}

F_PlayingPlayers(Team)
{
    static Player, Playing;
    Playing = 0;
    for (Player = 1; Player <= g_MaxPlayers; Player++)
    {
        if (Team != g_Team[Player])
            continue;
        Playing++;
    }
    return Playing;
}

bool: F_IsSniper(const Name[])
    return equali(Name[7], "spring") || equali(Name[7], "scoped", 6);

bool: F_IsMachine(const Name[])
    return equali(Name[7], "30cal") || equali(Name[7], "mg", 2);

bool: F_IsRocket(const Name[])
    return equali(Name[7], "piat") || equali(Name[7], "bazooka") || equali(Name[7], "pschreck");

F_PlayersOwningSniper(playerToSkip, Team)
{
    static Player, Players, Name[32], Class;
    Players = 0;
    for (Player = 1; Player <= g_MaxPlayers; Player++)
    {
        if (playerToSkip == Player || g_Team[Player] != Team)
            continue;
        if (g_Gun[Player] < 0)
        {
            if (g_UseFake && g_Fake[Player])
            {
                Class = pev(Player, pev_playerclass);
                if (Class == DODC_SNIPER || DODC_MARKSMAN == Class || DODC_SCHARFSCHUTZE == Class)
                    ++Players;
            }
            continue;
        }
        ArrayGetString(g_Weapons, g_Gun[Player], Name, charsmax(Name));
        if (false == F_IsSniper(Name))
            continue;
        Players++;
    }
    return Players;
}

F_PlayersOwningMachine(playerToSkip, Team)
{
    static Player, Players, Name[32], Class;
    Players = 0;
    for (Player = 1; Player <= g_MaxPlayers; Player++)
    {
        if (playerToSkip == Player || g_Team[Player] != Team)
            continue;
        if (g_Gun[Player] < 0)
        {
            if (g_UseFake && g_Fake[Player])
            {
                Class = pev(Player, pev_playerclass);
                if (Class == DODC_30CAL || DODC_MG34 == Class || DODC_MG42 == Class)
                    ++Players;
            }
            continue;
        }
        ArrayGetString(g_Weapons, g_Gun[Player], Name, charsmax(Name));
        if (false == F_IsMachine(Name))
            continue;
        Players++;
    }
    return Players;
}

F_PlayersOwningRocket(playerToSkip, Team)
{
    static Player, Players, Name[32], Class;
    Players = 0;
    for (Player = 1; Player <= g_MaxPlayers; Player++)
    {
        if (playerToSkip == Player || g_Team[Player] != Team)
            continue;
        if (g_Gun[Player] < 0)
        {
            if (g_UseFake && g_Fake[Player])
            {
                Class = pev(Player, pev_playerclass);
                if (Class == DODC_PIAT || DODC_PANZERJAGER == Class || DODC_BAZOOKA == Class)
                    ++Players;
            }
            continue;
        }
        ArrayGetString(g_Weapons, g_Gun[Player], Name, charsmax(Name));
        if (false == F_IsRocket(Name))
            continue;
        Players++;
    }
    return Players;
}

bool: F_SniperLimitReached(PlayerToSkip, Team, &Players, &Float: Percent)
{
    Players = F_PlayersOwningSniper(PlayerToSkip, Team);
    Percent = 100.0 * float(Players) / float(F_PlayingPlayers(Team));
    return Percent >= g_LimitSniper;
}

bool: F_MachineLimitReached(PlayerToSkip, Team, &Players, &Float: Percent)
{
    Players = F_PlayersOwningMachine(PlayerToSkip, Team);
    Percent = 100.0 * float(Players) / float(F_PlayingPlayers(Team));
    return Percent >= g_LimitMachine;
}

bool: F_RocketLimitReached(PlayerToSkip, Team, &Players, &Float: Percent)
{
    Players = F_PlayersOwningRocket(PlayerToSkip, Team);
    Percent = 100.0 * float(Players) / float(F_PlayingPlayers(Team));
    return Percent >= g_LimitRocket;
}

F_EraseEntityFromDroppedGuns(Entity)
{
    static Entry;
    Entry = ArrayFindValue(g_Dropped, Entity);
    if (Entry > -1)
    {
        ArrayDeleteItem(g_Dropped, Entry);
        ArrayDeleteItem(g_Times, Entry);
    }
}

bool: F_SafeIsSqlModuleRunning(const Name[])
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
ArrayFindString(Array: Which, const String[])
{
    static Iter, Size, Buffer[256];
    Size = ArraySize(Which);
    for (Iter = 0; Iter < Size; Iter++)
    {
        ArrayGetString(Which, Iter, Buffer, charsmax(Buffer));
        if (equal(Buffer, String))
            return Iter;
    }
    return -1;
}

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
