
#include <amxmodx>
#include <amxmisc>
#include <dodhacks>
#include <fakemeta>
#include <sqlx>

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
new Array: g_Items[33]; /// Player item entity names.
new Array: g_Entities[33]; /// Player item entities.
new Array: g_Names; /// Weapon names.
new Array: g_Weapons; /// Weapon entity names.
new Array: g_Ammos; /// Weapon ammo names.
new Array: g_Counts; /// Weapon ammo counts.
new Array: g_Flags; /// Weapon admin flags.
new bool: g_InSpawn[33]; /// Player is currently spawning.
new bool: g_InServer[33]; /// Player has fully joined.
new bool: g_IsAuth[33]; /// Player is authorized.
new bool: g_Local; /// Local storage in use.
new bool: g_HideMsg; /// Whether or not to hide the 'guns' chat command typed by the players.
new bool: g_British; /// Map is optimized for British against Axis.
new Float: g_SpawnTime[33]; /// Time when the player spawned.
new Float: g_EquipTime[33]; /// Time when the player got equipped.
new Float: g_TimeLimit; /// Seconds to be able to change the gun after being spawned.
new Float: g_Delay; /// Seconds between two menu selections.
new g_Buffer[256]; /// A large buffer.
new g_Steam[33][32]; /// Player Steam accounts.
new g_Gun[33]; /// Player gun index selection.
new g_Page[33]; /// Player menu page.
new g_Menu; /// Guns menu handle.
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
new g_HudMsg; /// HUD message sync handle.
new g_MaxPlayers; /// Maximum players server can handle.

public plugin_init()
{
	register_plugin("DoD Hacks: Weapons", "1.0", "Hattrick HKS (claudiuhks)");

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
	new bool: AdminAsterisk, FlagsNum, CurDriver[16], Driver[16], User[32], Pass[32],
		Db[32], Host[32], Weapon[32], Name[32], Team[32], Flags[32], Ammo[32], Count[8];
	while (fgets(Config, g_Buffer, charsmax(g_Buffer)) > 0)
	{
		trim(g_Buffer);
		if (!g_Buffer[0] || g_Buffer[0] == ';' || g_Buffer[0] == '/')
		{
			continue;
		}
		if (parse(g_Buffer, Weapon, charsmax(Weapon), Name, charsmax(Name), Team, charsmax(Team),
			Flags, charsmax(Flags), Ammo, charsmax(Ammo), Count, charsmax(Count)) < 6)
		{
			if (g_British && equali(Weapon, "@menu_title_british"))
			{
				g_Menu = menu_create(Name, "OnWeaponMenuItem");
			}
			else if (false == g_British && equali(Weapon, "@menu_title_allies"))
			{
				g_Menu = menu_create(Name, "OnWeaponMenuItem");
			}
			else if (equali(Weapon, "@db_driver"))
			{
				copy(Driver, charsmax(Driver), Name);
				g_Local = bool: equali(Driver, "sqlite");
			}
			else if (equali(Weapon, "@db_user"))
			{
				copy(User, charsmax(User), Name);
			}
			else if (equali(Weapon, "@db_pass"))
			{
				copy(Pass, charsmax(Pass), Name);
			}
			else if (equali(Weapon, "@db_host"))
			{
				copy(Host, charsmax(Host), Name);
			}
			else if (equali(Weapon, "@db_name"))
			{
				copy(Db, charsmax(Db), Name);
			}
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
			{
				g_TimeLimit = str_to_float(Name);
			}
			else if (equali(Weapon, "@hide_chat_cmd"))
			{
				g_HideMsg = bool: str_to_num(Name);
			}
			else if (equali(Weapon, "@selection_delay"))
			{
				g_Delay = str_to_float(Name);
			}
			else if (equali(Weapon, "@admin_asterisk"))
			{
				AdminAsterisk = bool: str_to_num(Name);
			}
			continue;
		}
		if ((false == g_British && 'B' == Team[0]) || (g_British && 'U' == Team[0]))
		{
			continue;
		}
		FlagsNum = read_flags(Flags);
		ArrayPushCell(g_Flags, read_flags(Flags));
		if (FlagsNum && AdminAsterisk)
		{
			add(Name, charsmax(Name), " *");
		}
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
	register_clcmd("guns", "OnPlayerConCmd_Guns", ADMIN_ALL, "- displays the guns menu");
	register_clcmd("weapons", "OnPlayerConCmd_Guns", ADMIN_ALL, "- displays the guns menu");
	DoD_DisableAutoScoping(); /// If a player selects the Marksman class (scoped Enfield gun), for example, do not automatically add a scope to it during player spawn function by default.
	SQL_GetAffinity(CurDriver, charsmax(CurDriver));
	if (!equali(CurDriver, Driver))
	{
		SQL_SetAffinity(Driver);
	}
	g_Sql = SQL_MakeDbTuple(Host, User, Pass, Db);
	if (Empty_Handle != g_Sql)
	{
		if (g_British)
		{
			if (g_Local)
			{
				SQL_ThreadQuery(g_Sql, "EmptySqlHandler", "create table if not exists guns_british (gun tinyint, steam character(32) unique)");
			}
			else
			{
				SQL_ThreadQuery(g_Sql, "EmptySqlHandler", "create table if not exists guns_british (gun int(4), steam varchar(32) unique)");
			}
		}
		else
		{
			if (g_Local)
			{
				SQL_ThreadQuery(g_Sql, "EmptySqlHandler", "create table if not exists guns_allies (gun tinyint, steam character(32) unique)");
			}
			else
			{
				SQL_ThreadQuery(g_Sql, "EmptySqlHandler", "create table if not exists guns_allies (gun int(4), steam varchar(32) unique)");
			}
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
}

#if defined client_connectex
public client_authorized(Player, const Steam[])
#else
public client_authorized(Player)
#endif
{
	g_IsAuth[Player] = true;
#if !defined client_connectex
	static Steam[32];
	get_user_authid(nPlayer, Steam, charsmax(Steam));
#endif
	copy(g_Steam[Player], charsmax(g_Steam[]), Steam);
	if (Empty_Handle != g_Sql)
	{
		if (g_British)
		{
			formatex(g_Buffer, charsmax(g_Buffer), "select gun from guns_british where steam = '%s'", Steam);
		}
		else
		{
			formatex(g_Buffer, charsmax(g_Buffer), "select gun from guns_allies where steam = '%s'", Steam);
		}
		static Info[32];
		num_to_str(get_user_userid(Player), Info, charsmax(Info));
		SQL_ThreadQuery(g_Sql, "OnSqlSelection", g_Buffer, Info, sizeof Info);
	}
}

public client_connect(Player)
{
	g_Gun[Player] = -1;
	g_Page[Player] = 0;
	g_IsAuth[Player] = false;
	g_InSpawn[Player] = false;
	g_InServer[Player] = false;
	g_SpawnTime[Player] = 0.0;
	g_EquipTime[Player] = 0.0;
}

public DoD_OnPlayerSpawn(DoDTeamPlay, Player)
{
	g_InSpawn[Player] = true;
}

public DoD_OnPlayerSpawn_Post(DoDTeamPlay, Player)
{
	g_InSpawn[Player] = false;
	if (g_InServer[Player])
	{
		static Weapon[32], Name[32], Ammo[32], Count, Item, Res, Iter, Old, New, Flags;
		g_SpawnTime[Player] = get_gametime();
		if (g_Gun[Player] < 0)
		{
			player_menu_info(Player, Old, New);
			if (Old < 1 && New < 0)
			{
				g_Page[Player] = 0;
				menu_display(Player, g_Menu);
			}
			else
			{
				client_print(Player, print_chat, "* Type 'guns' to choose a gun.");
			}
		}
		else
		{
			ArrayGetString(g_Names, g_Gun[Player], Name, charsmax(Name));
			Flags = ArrayGetCell(g_Flags, g_Gun[Player]);
			if (Flags != (get_user_flags(Player) & Flags))
			{
				player_menu_info(Player, Old, New);
				if (Old < 1 && New < 0)
				{
					g_Page[Player] = 0;
					menu_display(Player, g_Menu);
					client_print(Player, print_chat, "* No more access for %s.", Name);
				}
				else
				{
					client_print(Player, print_chat, "* No access for %s. Type 'guns' for more.", Name);
				}
			}
			else
			{
				ArrayGetString(g_Weapons, g_Gun[Player], Weapon, charsmax(Weapon));
				ArrayGetString(g_Ammos, g_Gun[Player], Ammo, charsmax(Ammo));
				DoD_GiveNamedItem(Player, Weapon, Item);
				if (Item)
				{
					Count = ArrayGetCell(g_Counts, g_Gun[Player]);
					if (Count > 0)
					{
						DoD_GiveAmmo(Player, Count, Ammo, Count, Res);
					}
				}
			}
		}
		if (false == F_HasGrenade(Player, Weapon, charsmax(Weapon), Item))
		{
			switch (get_user_team(Player))
			{
				case 1:
				{
					if (g_British)
					{
						for (Iter = 0; Iter < g_BritishGrenades; Iter++)
						{
							DoD_GiveNamedItem(Player, g_BritishGrenade, Item);
						}
					}
					else
					{
						for (Iter = 0; Iter < g_AlliesGrenades; Iter++)
						{
							DoD_GiveNamedItem(Player, g_AlliesGrenade, Item);
						}
					}
				}
				default:
				{
					for (Iter = 0; Iter < g_AxisGrenades; Iter++)
					{
						DoD_GiveNamedItem(Player, g_AxisGrenade, Item);
					}
				}
			}
		}
		if (F_HasSecondaryWeapon(Player, Weapon, charsmax(Weapon), Item))
		{
			if (g_WebleyCount > 0 && equali(Weapon, "weapon_webley"))
			{
				DoD_GiveAmmo(Player, g_WebleyCount, g_WebleyAmmo, g_WebleyCount, Res);
			}
			else if (g_ColtCount > 0 && equali(Weapon, "weapon_colt"))
			{
				DoD_GiveAmmo(Player, g_ColtCount, g_ColtAmmo, g_ColtCount, Res);
			}
			else if (g_LugerCount > 0)
			{
				DoD_GiveAmmo(Player, g_LugerCount, g_LugerAmmo, g_LugerCount, Res);
			}
		}
	}
}

bool: F_HasGrenade(Player, Weapon[], Size, &Entity)
{
	static Item[32], Iter;
	for (Iter = 0; Iter < ArraySize(g_Items[Player]); Iter++)
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
	static Item[32], Iter;
	for (Iter = 0; Iter < ArraySize(g_Items[Player]); Iter++)
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
	static Item[32], Iter;
	for (Iter = 0; Iter < ArraySize(g_Items[Player]); Iter++)
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

public OnWeaponMenuPage(Player, Status, Menu)
{
	switch (Status)
	{
		case MENU_BACK:
		{
			if (g_Page[Player] > 0)
			{
				--g_Page[Player];
			}
		}
		case MENU_MORE:
		{
			++g_Page[Player];
		}
		case MENU_EXIT:
		{
			g_Page[Player] = 0;
		}
	}
}

public OnWeaponMenuItem(Player, Menu, Item)
{
	if (Item > -1)
	{
		static Weapon[32], Name[32], Ammo[32], Count, Flags, Entity, Res, Float: Time, Float: Wait;
		Flags = ArrayGetCell(g_Flags, Item);
		if (Flags != (get_user_flags(Player) & Flags))
		{
			menu_display(Player, g_Menu, g_Page[Player]);
			set_hudmessage(20 /** red */, 40 /** green */, 160 /** blue */,
				0.0 /** horizontal pos */, 0.3 /** vertical pos */,
				1 /** effect type */, 0.5 /** effect time */,
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
				menu_display(Player, g_Menu, g_Page[Player]);
				Wait = g_Delay - (Time - g_EquipTime[Player]);
				set_hudmessage(180 /** red */, 40 /** green */, 20 /** blue */,
					0.0 /** horizontal pos */, 0.3 /** vertical pos */,
					1 /** effect type */, 0.5 /** effect time */,
					Wait /** duration */, 0.1 /** fade in time */, 0.1 /** fade out time */);
				ShowSyncHudMsg(Player, g_HudMsg, "WAIT %.1f SEC!", Wait);
				return PLUGIN_HANDLED;
			}
			if (F_HasPrimaryWeapon(Player, Weapon, charsmax(Weapon), Entity))
			{
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
			DoD_GiveNamedItem(Player, Weapon, Entity);
			if (Entity)
			{
				client_print(Player, print_chat, "* %s equipped. Type 'guns' for more.", Name);
				g_EquipTime[Player] = Time;
				Count = ArrayGetCell(g_Counts, Item);
				if (Count > 0)
				{
					DoD_GiveAmmo(Player, Count, Ammo, Count, Res);
				}
			}
		}
		else
		{
			client_print(Player, print_chat, "* %s selected. Type 'guns' for more.", Name);
		}
		if (Empty_Handle != g_Sql && g_IsAuth[Player])
		{
			if (g_British)
			{
				if (g_Local)
				{
					formatex(g_Buffer, charsmax(g_Buffer),
						"insert into guns_british values (%d, '%s') on conflict (steam) do update set gun = %d",
						Item, g_Steam[Player], Item);
				}
				else
				{
					formatex(g_Buffer, charsmax(g_Buffer),
						"insert into guns_british values (%d, '%s') on duplicate key update gun = %d",
						Item, g_Steam[Player], Item);
				}
			}
			else
			{
				if (g_Local)
				{
					formatex(g_Buffer, charsmax(g_Buffer),
						"insert into guns_allies values (%d, '%s') on conflict (steam) do update set gun = %d",
						Item, g_Steam[Player], Item);
				}
				else
				{
					formatex(g_Buffer, charsmax(g_Buffer),
						"insert into guns_allies values (%d, '%s') on duplicate key update gun = %d",
						Item, g_Steam[Player], Item);
				}
			}
			SQL_ThreadQuery(g_Sql, "EmptySqlHandler", g_Buffer);
		}
	}
	return PLUGIN_CONTINUE;
}

public DOD_ON_PLAYER_DISCONNECTED
{
	if (Empty_Handle != g_Sql && g_IsAuth[Player] && g_Gun[Player] > -1)
	{
		if (g_British)
		{
			if (g_Local)
			{
				formatex(g_Buffer, charsmax(g_Buffer),
					"insert into guns_british values (%d, '%s') on conflict (steam) do update set gun = %d",
					g_Gun[Player], g_Steam[Player], g_Gun[Player]);
			}
			else
			{
				formatex(g_Buffer, charsmax(g_Buffer),
					"insert into guns_british values (%d, '%s') on duplicate key update gun = %d",
					g_Gun[Player], g_Steam[Player], g_Gun[Player]);
			}
		}
		else
		{
			if (g_Local)
			{
				formatex(g_Buffer, charsmax(g_Buffer),
					"insert into guns_allies values (%d, '%s') on conflict (steam) do update set gun = %d",
					g_Gun[Player], g_Steam[Player], g_Gun[Player]);
			}
			else
			{
				formatex(g_Buffer, charsmax(g_Buffer),
					"insert into guns_allies values (%d, '%s') on duplicate key update gun = %d",
					g_Gun[Player], g_Steam[Player], g_Gun[Player]);
			}
		}
		SQL_ThreadQuery(g_Sql, "EmptySqlHandler", g_Buffer);
	}
	ArrayClear(g_Items[Player]);
	ArrayClear(g_Entities[Player]);

	g_Gun[Player] = -1;
	g_Page[Player] = 0;
	g_IsAuth[Player] = false;
	g_InSpawn[Player] = false;
	g_InServer[Player] = false;
	g_SpawnTime[Player] = 0.0;
	g_EquipTime[Player] = 0.0;
}

public EmptySqlHandler(FailState, Handle: Query, Error[], ErrorNum, Data[], Size, Float: Time)
{
}

#if defined FindPlayerFlags
public OnSqlSelection(FailState, Handle: Query, Error[], ErrorNum, Data[], Size, Float: Time)
{
	if (Query != Empty_Handle)
	{
		new Player = find_player_ex(FindPlayer_IncludeConnecting | FindPlayer_MatchUserId, str_to_num(Data));
		if (Player > 0 && SQL_NumResults(Query) > 0)
		{
			new Gun = SQL_ReadResult(Query, 0);
			if (Gun < 0 || Gun >= g_WeaponsCount)
			{
				g_Gun[Player] = -1;
			}
			else
			{
				g_Gun[Player] = Gun;
			}
		}
	}
}
#else
public OnSqlSelection(FailState, Handle: Query, Error[], ErrorNum, Data[], Size, Float: Time)
{
	if (Query != Empty_Handle)
	{
		new UniqueIndex = str_to_num(Data);
		for (new Player = 1; Player <= g_MaxPlayers; Player++)
		{
			if (UniqueIndex == get_user_userid(Player) && SQL_NumResults(Query) > 0)
			{
				new Gun = SQL_ReadResult(Query, 0);
				if (Gun < 0 || Gun >= g_WeaponsCount)
				{
					g_Gun[Player] = -1;
				}
				else
				{
					g_Gun[Player] = Gun;
				}
				break;
			}
		}
	}
}
#endif

public DoD_OnGiveNamedItem(Player, Item[], ItemSize /** = 128 */)
{ /// Item may be altered during execution.
	if (!g_InSpawn[Player])
	{
		return PLUGIN_CONTINUE;
	}
	if (DoD_IsWeaponPrimary(Item))
	{ /// Do not automatically give a primary weapon by default.
		return PLUGIN_HANDLED;
	}
	return PLUGIN_CONTINUE;
}

public client_command(Player)
{
	static Cmd[32], Arg[32], Old, New;
	read_argv(0, Cmd, charsmax(Cmd));
	if (equali(Cmd, "Say") || equali(Cmd, "Say_Team"))
	{
		read_argv(1, Arg, charsmax(Arg));
		if (equali(Arg, "Guns") || equali(Arg, "Weapons") ||
			equali(Arg, "!Guns") || equali(Arg, "!Weapons") ||
			equali(Arg, "/Guns") || equali(Arg, "/Weapons") ||
			equali(Arg, ".Guns") || equali(Arg, ".Weapons") ||
			equali(Arg, ",Guns") || equali(Arg, ",Weapons"))
		{
			player_menu_info(Player, Old, New);
			if (Old < 1 && New < 0)
			{
				g_Page[Player] = 0;
				menu_display(Player, g_Menu);
			}
			else
			{
				client_print(Player, print_chat, "* You're already viewing a menu.");
			}
			if (g_HideMsg)
			{
				return PLUGIN_HANDLED;
			}
		}
	}
	return PLUGIN_CONTINUE;
}

public OnPlayerConCmd_Guns(Player)
{
	static Old, New;
	player_menu_info(Player, Old, New);
	if (Old < 1 && New < 0)
	{
		g_Page[Player] = 0;
		menu_display(Player, g_Menu);
	}
	else
	{
		client_print(Player, print_chat, "* You're already viewing a menu.");
	}
	return PLUGIN_HANDLED;
}

public DoD_OnAddPlayerItem_Post(Player, Item, Res)
{
	if (Res > 0)
	{
		static Class[32];
		pev(Item, pev_classname, Class, charsmax(Class));
		ArrayPushString(g_Items[Player], Class);
		ArrayPushCell(g_Entities[Player], Item);
	}
}

public DoD_OnRemovePlayerItem_Post(Player, Item, Res)
{
	if (Res > 0)
	{
		static Index, Class[32];
		pev(Item, pev_classname, Class, charsmax(Class));
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
