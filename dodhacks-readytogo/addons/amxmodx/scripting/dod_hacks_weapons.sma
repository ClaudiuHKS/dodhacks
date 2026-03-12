
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>
#include <sqlx>
#include <dodconst>
#include <dodhacks>

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

new Handle: g_Sql; /// Threaded database storage.
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
new bool: g_Local; /// Local storage in use.
new bool: g_HideMsg; /// Whether or not to hide the 'guns' chat command typed by the players.
new bool: g_British; /// Map is optimized for British against Axis.
new bool: g_Delete; /// Delete the dropped gun when selecting a new one via the menu.
new bool: g_HudStyle; /// Whether or not to use a visual effect on the HUD message(s).
new Float: g_SpawnTime[33]; /// Time when the player spawned.
new Float: g_EquipTime[33]; /// Time when the player got equipped.
new Float: g_TimeLimit; /// Seconds to be able to change the gun after being spawned.
new Float: g_Delay; /// Seconds between two menu selections.
new Float: g_DeleteTime; /// Time in seconds to delete the dropped gun when selecting a new one.
new Float: g_VerPos; /// HUD message(s) vertical position.
new Float: g_HorPos; /// HUD message(s) horizontal position.
new g_Buffer[256]; /// A large buffer.
new g_Steam[33][32]; /// Player Steam accounts.
new g_Gun[33]; /// Player gun index selection.
new g_Page[33]; /// Player menu page.
new g_Class[33]; /// Player class. << Used only for fake players at the moment >>.
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

public plugin_init()
{
    register_plugin("DoD Hacks: Weapons", "1.0.0.3", "Hattrick HKS (claudiuhks)");

    get_configsdir(g_Buffer, charsmax(g_Buffer));
    add(g_Buffer, charsmax(g_Buffer), "/dod_hacks_weapons.ini");
    new Config = fopen(g_Buffer, "r");
    if (!Config)
    {
        set_fail_state("Error opening '%s'!", g_Buffer);
        return PLUGIN_HANDLED;
    }
    g_Names = ArrayCreate(32);
    g_Weapons = ArrayCreate(32);
    g_Ammos = ArrayCreate(32);
    g_Flags = ArrayCreate();
    g_Counts = ArrayCreate();
    g_British = DoD_AreAlliesBritish();
    new bool: AdminAsterisk, bool: allowAll, FlagsNum, Driver[16], User[32], Pass[32],
        Db[32], Host[32], Weapon[32], Name[32], Team[32], Flags[32], Ammo[32], Count[8];
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
            else if (equali(Weapon, "@drop_delete"))
                g_Delete = bool: str_to_num(Name);
            else if (equali(Weapon, "@delete_time"))
                g_DeleteTime = str_to_float(Name);
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
    }
    SQL_GetAffinity(g_Buffer, charsmax(g_Buffer));
    if (!equali(g_Buffer, Driver))
        SQL_SetAffinity(Driver);
    g_Sql = SQL_MakeDbTuple(Host, User, Pass, Db);
    if (Empty_Handle != g_Sql)
    {
        if (g_British)
        {
            if (g_Local)
                SQL_ThreadQuery(g_Sql, "EmptySqlHandler",
                    "create table if not exists guns_british (gun tinyint, steam character(32) unique)");
            else
                SQL_ThreadQuery(g_Sql, "EmptySqlHandler",
                    "create table if not exists guns_british (gun int(4), steam varchar(32) unique)");
        }
        else
        {
            if (g_Local)
                SQL_ThreadQuery(g_Sql, "EmptySqlHandler",
                    "create table if not exists guns_allies (gun tinyint, steam character(32) unique)");
            else
                SQL_ThreadQuery(g_Sql, "EmptySqlHandler",
                    "create table if not exists guns_allies (gun int(4), steam varchar(32) unique)");
        }
    }
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** OLD AMX MOD X */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** NEW AMX MOD X */
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
    if (copy(g_Steam[Player], charsmax(g_Steam[]), Steam) > 4 && Empty_Handle != g_Sql)
    { /// Skipping fake players (long enough Steam string).
        if (g_British)
            formatex(g_Buffer, charsmax(g_Buffer),
                "select gun from guns_british where steam = '%s'", Steam);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "select gun from guns_allies where steam = '%s'", Steam);

        num_to_str(get_user_userid(Player), Info, charsmax(Info));
        SQL_ThreadQuery(g_Sql, "OnSqlSelection", g_Buffer, Info, sizeof Info);
    }
}

public client_connect(Player)
    F_ZeroPlayerVars(Player);

public DoD_OnPlayerSpawn(DoD_Address: CDoDTeamPlay, &Player)
{
    if (g_InServer[Player])
    {
        g_InSpawn[Player] = true;
        if (g_Fake[Player])
            g_Class[Player] = pev(Player, pev_playerclass);
    }
}

public DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player)
{
    static Weapon[32], Name[32], Ammo[32], Count, Item, Res, Iter, Flags, Access;
    if (g_InServer[Player])
    {
        g_InSpawn[Player] = false;
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
            switch (get_user_team(Player))
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
            if (g_WebleyCount > 0 && equali(Weapon, "weapon_webley"))
                DoD_GiveAmmo(Player, g_WebleyCount, g_WebleyAmmo, g_WebleyCount, Res);
            else if (g_ColtCount > 0 && equali(Weapon, "weapon_colt"))
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

public OnWeaponMenuItem(Player, Menu, Item)
{
    static Weapon[32], Name[32], Ammo[32], Access, Count, Flags, Entity, Res, Float: Time, Float: Wait;
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
            set_hudmessage(20 /** red */, 60 /** green */, 180 /** blue */,
                g_HorPos /** horizontal pos */, g_VerPos /** vertical pos */,
                g_HudStyle ? 1 : 0 /** effect type */, 0.5 /** effect time */,
                2.0 /** duration */, 0.1 /** fade in time */, 0.1 /** fade out time */);
            ShowSyncHudMsg(Player, g_HudMsg, "NO ACCESS!");
            return PLUGIN_HANDLED;
        }
        g_Gun[Player] = Item;
        ArrayGetString(g_Names, Item, Name, charsmax(Name));
        Time = get_gametime();
        if (is_user_alive(Player) && (!g_TimeLimit || Time - g_SpawnTime[Player] < g_TimeLimit))
        {
            if (Time - g_EquipTime[Player] < g_Delay)
            {
                F_ShowMenu(Player, false, g_Page[Player]);
                Wait = g_Delay - (Time - g_EquipTime[Player]);
                set_hudmessage(200 /** red */, 20 /** green */, 20 /** blue */,
                    g_HorPos /** horizontal pos */, g_VerPos /** vertical pos */,
                    g_HudStyle ? 1 : 0 /** effect type */, 0.5 /** effect time */,
                    Wait /** duration */, 0.1 /** fade in time */, 0.1 /** fade out time */);
                ShowSyncHudMsg(Player, g_HudMsg, "WAIT %.1f SEC!", Wait);
                return PLUGIN_HANDLED;
            }
            if (F_HasPrimaryWeapon(Player, Weapon, charsmax(Weapon), Entity))
            {
                if (g_Delete)
                {
                    ArrayPushCell(g_Dropped, Entity);
                    ArrayPushCell(g_Times, Time);
                }
                DoD_DropPlayerItem(Player, Weapon, true);
                Res = ArrayFindString(g_Items[Player], Weapon);
                if (Res > -1)
                {
                    ArrayDeleteItem(g_Items[Player], Res);
                    ArrayDeleteItem(g_Entities[Player], Res);
                }
            }
            ArrayGetString(g_Weapons, Item, Weapon, charsmax(Weapon));
            ArrayGetString(g_Ammos, Item, Ammo, charsmax(Ammo));
            if (DoD_GiveNamedItem(Player, Weapon, Entity) && Entity > g_MaxPlayers)
            {
                client_print(Player, print_chat, "* %s equipped. Type 'guns' for more.", Name);
                g_EquipTime[Player] = Time;
                Count = ArrayGetCell(g_Counts, Item);
                if (Count > 0)
                    DoD_GiveAmmo(Player, Count, Ammo, Count, Res);
            }
        }
        else
            client_print(Player, print_chat, "* %s selected. Type 'guns' for more.", Name);
        if (Empty_Handle != g_Sql && g_IsAuth[Player])
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
    if (Empty_Handle != g_Sql && g_IsAuth[Player] && g_Gun[Player] > -1)
        F_StoreSelection(Player); /// Store the selection.
    ArrayClear(g_Items[Player]);
    ArrayClear(g_Entities[Player]);
    F_ZeroPlayerVars(Player);
}

public EmptySqlHandler()
{
}

#if defined FindPlayerFlags
public OnSqlSelection(FailState, Handle: Query, Error[], ErrorNum, Data[])
{
    static Player, Gun;
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
    {
        Player = find_player_ex(FindPlayer_IncludeConnecting | FindPlayer_MatchUserId, str_to_num(Data));
        if (Player > 0)
        {
            Gun = SQL_ReadResult(Query, 0);
            if (Gun < 0 || Gun >= g_WeaponsCount)
                g_Gun[Player] = -1;
            else
                g_Gun[Player] = Gun;
        }
    }
}
#else
public OnSqlSelection(FailState, Handle: Query, Error[], ErrorNum, Data[])
{
    static UniqueIndex, Player, Gun;
    if (Query != Empty_Handle && SQL_NumResults(Query) > 0)
    {
        UniqueIndex = str_to_num(Data);
        for (Player = 1; Player <= g_MaxPlayers; Player++)
        {
            if (UniqueIndex == get_user_userid(Player))
            {
                Gun = SQL_ReadResult(Query, 0);
                if (Gun < 0 || Gun >= g_WeaponsCount)
                    g_Gun[Player] = -1;
                else
                    g_Gun[Player] = Gun;
                break;
            }
        }
    }
}
#endif

public DoD_OnGiveNamedItem(Player, Item[], ItemSize /** = 64 */, &OverrideRes)
{ /// Item may be altered during execution.
    if (!g_InSpawn[Player] || g_Fake[Player])
        return PLUGIN_CONTINUE; /// Skip.
    if (DoD_IsWeaponPrimary(Item)) /// Do not auto. give a prim. weap. by default (excl. fake players).
        return PLUGIN_HANDLED;
    return PLUGIN_CONTINUE;
}

public DoD_OnGiveNamedItem_Post(Player, const Item[], Entity)
{ /// Perform weapon scoping if needed (fake players only).
    if (!g_InSpawn[Player] || !g_Fake[Player] || Entity <= g_MaxPlayers)
        return PLUGIN_CONTINUE; /// Skip.
    switch (g_Class[Player])
    { /// Add a scope if needed (fake players).
        case DODC_SCOPED_FG42: /// Scoped FG42 (Axis).
            if (equali(Item, "weapon_fg42"))
                DoD_AddScope(Entity, true);
        case DODC_MARKSMAN: /// Scoped Enfield (British).
            if (equali(Item, "weapon_enfield"))
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
    g_Page[Player] = 0;
    g_Class[Player] = 0;
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
                "insert into guns_british values (%d, '%s') on conflict (steam) do update set gun = %d",
                g_Gun[Player], g_Steam[Player], g_Gun[Player]);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "insert into guns_british values (%d, '%s') on duplicate key update gun = %d",
                g_Gun[Player], g_Steam[Player], g_Gun[Player]);
    }
    else
    {
        if (g_Local)
            formatex(g_Buffer, charsmax(g_Buffer),
                "insert into guns_allies values (%d, '%s') on conflict (steam) do update set gun = %d",
                g_Gun[Player], g_Steam[Player], g_Gun[Player]);
        else
            formatex(g_Buffer, charsmax(g_Buffer),
                "insert into guns_allies values (%d, '%s') on duplicate key update gun = %d",
                g_Gun[Player], g_Steam[Player], g_Gun[Player]);
    }
    SQL_ThreadQuery(g_Sql, "EmptySqlHandler", g_Buffer);
}
