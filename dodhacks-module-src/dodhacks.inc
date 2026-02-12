
//
// DoD Hacks Functions
//

#if defined _dodhacks_included
	#endinput
#endif
#define _dodhacks_included

#pragma reqlib dodhacks
#if !defined AMXMODX_NOAUTOLOAD
	#pragma loadlib dodhacks
#endif

/**
 * Return PLUGIN_CONTINUE to continue
 * the execution of a Pre forward.
 * Game's original function will execute as well.
 *
 * Return PLUGIN_HANDLED to block
 * the execution of a Pre forward.
 * Game's original function will not execute.
 *
 * A Post forward will not execute
 * if its Pre forward returned PLUGIN_HANDLED.
 *
 * In Post forwards, do not return PLUGIN_HANDLED.
 */

/**
 * Player is being spawned. Ensure the player
 * is in server during this call.
 * pev_playerclass checks, DoD_GiveNamedItem()
 * calls and weapon_enfield/ weapon_fg42 scope
 * installations are made in this game's original
 * function (if not disabled by calling
 * DoD_DisableAutoScoping() already).
 *
 * CDoDTeamPlay (Game Rules) is an address.
 */
forward DoD_OnPlayerSpawn(CDoDTeamPlay, Player);
forward DoD_OnPlayerSpawn_Post(CDoDTeamPlay, Player);

/**
 * Player receives a named item.
 * Item's content may be altered during call.
 * In module's source code, Item's size is 128.
 * Strings like "weapon_scopedenfield" or
 * "weapon_scopedfg42" may be passed here.
 */
forward DoD_OnGiveNamedItem(Player, Item[], ItemSize /** = 128 */);

/**
 * Player has received a named item.
 * ItemEntity is the resulted item
 * entity index, if any. Zero otherwise.
 */
forward DoD_OnGiveNamedItem_Post(Player, const Item[], ItemEntity);

/**
 * Player drops an item.
 * Item's content may be altered during call.
 * In module's source code, Item's size is 128.
 * If Force is set to true, the player will
 * be able to drop their knife or spade
 * (as an example). Item's content can be empty.
 * If empty string, current weapon will drop.
 */
forward DoD_OnDropPlayerItem(Player, Item[], ItemSize /** = 128 */, &bool: Force);
forward DoD_OnDropPlayerItem_Post(Player, const Item[], Force);

/**
 * Item is removed from player's inventory.
 */
forward DoD_OnRemovePlayerItem(Player, Item);

/**
 * On success, Res is set to 1.
 */
forward DoD_OnRemovePlayerItem_Post(Player, Item, Res);

/**
 * Item is added to player's inventory.
 */
forward DoD_OnAddPlayerItem(Player, Item);

/**
 * On success, Res is set to 1.
 */
forward DoD_OnAddPlayerItem_Post(Player, Item, Res);

/**
 * Game is asking for respawn wave time.
 * Team can be altered during call.
 * If PLUGIN_HANDLED is returned,
 * game's original function will not be called
 * anymore and it will just return what Time
 * variable says. Time variable is ignored if
 * returning PLUGIN_CONTINUE.
 * Time variable is initially zero (0.0).
 * CDoDTeamPlay (Game Rules) is an address.
 */
forward DoD_OnGetWaveTime(CDoDTeamPlay, &Team, &Float: Time);

/**
 * Time variable is the result.
 * CDoDTeamPlay (Game Rules) is an address.
 */
forward DoD_OnGetWaveTime_Post(CDoDTeamPlay, Team, Float: Time);

/**
 * Game is setting a respawn wave time.
 * Team can be changed during execution.
 * Time can be changed during execution.
 * CDoDTeamPlay (Game Rules) is an address.
 */
forward DoD_OnSetWaveTime(CDoDTeamPlay, &Team, &Float: Time);
forward DoD_OnSetWaveTime_Post(CDoDTeamPlay, Team, Float: Time);

/**
 * Player is having all items removed
 * from their inventory.
 * RemoveSuit may be changed during call.
 */
forward DoD_OnRemoveAllItems(Player, &RemoveSuit);
forward DoD_OnRemoveAllItems_Post(Player, RemoveSuit);

/**
 * Player receives ammo.
 * Ammo, Name and Max may
 * be altered during call.
 * For more details,
 * see DoD_GiveAmmo() native.
 */
forward DoD_OnGiveAmmo(Player, &Ammo, Name[], NameSize /** = 128 */, &Max);

/**
 * Player has received ammo.
 * Res is the result returned
 * by the original game call.
 */
forward DoD_OnGiveAmmo_Post(Player, Ammo, const Name[], Max, Res);

/**
 * Entities/ players get
 * their bodygroup set.
 * Group and Value may
 * be altered.
 */
forward DoD_OnSetBodygroup(Entity, &Group, &Value);

/**
 * Entity/ player got
 * its bodygroup set.
 */
forward DoD_OnSetBodygroup_Post(Entity, Group, Value);

/**
 * CDoDTeamPlay (Game Rules)
 * is being installed.
 */
forward DoD_OnInstallGameRules();

/**
 * CDoDTeamPlay (Game Rules) installed.
 * CDoDTeamPlay is the resulted address.
 */
forward DoD_OnInstallGameRules_Post(CDoDTeamPlay);

/**
 * Returns true if the current
 * CDoDTeamPlay (Game Rules)
 * address for this map session
 * is valid.
 */
native bool: DoD_AreGameRulesReady();

/**
 * Calls game's original
 * InstallGameRules()
 * function. Returns
 * CDoDTeamPlay (Game Rules)
 * address.
 */
native DoD_InstallGameRules();

/**
 * Calls game's original
 * PlayerSpawn() function.
 * Returns true on success.
 */
native bool: DoD_PlayerSpawn(Player);

/**
 * Deploys an item.
 * If DoD_AddScope() is called
 * with Refresh parameter set
 * to true, this call is
 * executed within DoD_AddScope(),
 * to make the scope display
 * instantly on the screen.
 */
native bool: DoD_DeployItem(Item);

/**
 * Gives the player a named item.
 * Returns true on success.
 * ItemEntity will be the entity
 * index or a zero on failure.
 * Strings like
 * "weapon_scopedenfield" or
 * "weapon_scopedfg42" may be
 * passed here successfully.
 */
native bool: DoD_GiveNamedItem(Player, const Item[], &ItemEntity);

/**
 * Sets entity's or
 * player's bodygroup.
 * Returns true on success.
 */
native bool: DoD_SetBodygroup(Entity, Group, Value);

/**
 * Drops an item from player.
 * Returns true on success.
 * If empty string, current
 * item will be dropped. If
 * Force is true, spade/ knife
 * may be dropped as well.
 */
native bool: DoD_DropPlayerItem(Player, const Item[], Force);

/**
 * Gives ammo to a player.
 * Returns true on success.
 * Max can't be smaller than Ammo.
 * Zero Ammo/ Max values are not allowed.
 * Maximum allowed value is set to 32767
 * for both Ammo and Max.
 * Res is the result returned by game call.
 *
 * Ammo names are listed below.
 *
 * ammo_agrens   @ HANDGRENADE
 * ammo_agrensex @ HANDGRENADE_EX
 * ammo_ggrens   @ STICKGRENADE
 * ammo_ggrensex @ STICKGRENADE_EX
 *
 * ammo_66mm_ger @ MG42
 * ammo_66mm_us  @ 30CAL
 * ammo_22mm     @ SPRING,   SCOPEDKAR
 * ammo_12mm     @ WEBLEY,   LUGER,     COLT
 * ammo_16mm     @ KAR,      GARAND,    ENFIELD
 * ammo_66mm     @ MG34,     M1CARBINE, K43
 * ammo_55mm     @ MP44,     FG42,      BREN,   BAR
 * ammo_44mm     @ THOMPSON, STEN,      MP40,   GREASEGUN
 *
 * rockets       @ PIAT,     BAZOOKA,   PSCHRECK
 */
native bool: DoD_GiveAmmo(Player, Ammo, const Name[], Max, &Res);

/**
 * Removes all items from
 * player's inventory.
 * Returns true on success.
 */
native bool: DoD_RemoveAllItems(Player, RemoveSuit);

/**
 * Adds Health HP to player if needed.
 * If pev_health + Health >= pev_maxhealth,
 * sets pev_health to pev_maxhealth.
 * Adds Health HP to pev_health otherwise.
 * Returns true on success.
 * If the player is not alive, no error
 * logs will trigger. They will be skipped.
 * Added is the added HP, if any.
 * Zero otherwise.
 */
native bool: DoD_AddHealthIfWounded(Player, Health, &Added);

/**
 * Returns true if player's
 * pev_health equals to
 * pev_maxhealth.
 */
native bool: DoD_IsPlayerFullHealth(Player);

/**
 * Adds a scope to the given weapon
 * (either weapon_fg42 or weapon_enfield
 * entity). Returns true on success.
 * If Refresh is true, it's the default
 * behaviour. If Refresh is true, player's
 * screen will instantly show the scope
 * attached to the weapon, without having
 * to switch between weapons before,
 * in order to see it there. If Refresh is
 * true, DoD_DeployItem() is included in
 * this call.
 */
native bool: DoD_AddScope(Weapon, bool: Refresh);

/**
 * Returns true if the
 * weapon has a scope
 * attached to it.
 */
native bool: DoD_HasScope(Weapon);

/**
 * Returns true if
 * Allies are British.
 */
native bool: DoD_AreAlliesBritish();

/**
 * Returns true if
 * Allies are Paratroopers.
 */
native bool: DoD_AreAlliesParatroopers();

/**
 * Returns true if
 * Axis are Paratroopers.
 */
native bool: DoD_AreAxisParatroopers();

/**
 * Returns true if
 * Allies got infinite lives.
 */
native bool: DoD_HaveAlliesInfiniteLives();

/**
 * Returns true if
 * Axis got infinite lives.
 */
native bool: DoD_HaveAxisInfiniteLives();

/**
 * Returns Allies respawn factor.
 */
native Float: DoD_GetAlliesRespawnFactor();

/**
 * Returns Axis respawn factor.
 */
native Float: DoD_GetAxisRespawnFactor();

/**
 * Reads a bool from
 * CDoDTeamPlay (Game Rules)
 * address.
 */
native bool: DoD_ReadGameRulesBool(Offset);

/**
 * Reads an int from
 * CDoDTeamPlay (Game Rules)
 * address.
 */
native DoD_ReadGameRulesInt(Offset);

/**
 * Reads a float from
 * CDoDTeamPlay (Game Rules)
 * address.
 */
native Float: DoD_ReadGameRulesFloat(Offset);

/**
 * Reads a str from
 * CDoDTeamPlay (Game Rules)
 * address. Returns chars num.
 */
native DoD_ReadGameRulesStr(Offset, Bytes, Buffer[], Size, bool: AsUtf8);

/**
 * Removes an item from
 * player's inventory.
 * Returns -1 on error.
 * Returns what game
 * returns otherwise
 * (1 on success).
 */
native DoD_RemovePlayerItem(Player, Item);

/**
 * Adds an item to player's
 * inventory. Returns -1 on
 * error. Returns what game
 * returns otherwise
 * (1 on success).
 */
native DoD_AddPlayerItem(Player, Item);

/**
 * Sets the respawn wave time
 * for a specific team. Team 1
 * for Allies and 2 for Axis.
 * Returns true on success.
 */
native bool: DoD_SetWaveTime(Team, Float: Time);

/**
 * Gets the respawn wave time
 * for a specific team. Team 1
 * for Allies and 2 for Axis.
 */
native Float: DoD_GetWaveTime(Team);

/**
 * Use within plugin_precache().
 *
 * Adds and/ or alters (updates) a key
 * value to map. Map can be empty (to
 * apply on all maps). Val can be empty
 * (to remove the key value from the map
 * no matter whan map's value is - so
 * executing it with key only).
 *
 * An example of turning Allies
 * to British (on dod_kalt).
 *
 * "dod_kalt"
 *   "info_doddetect"
 *     "detect_allies_country"
 *       "1"
 */
native bool: DoD_AddKeyValAdd(Map[], Class[], Key[], Val[]);

/**
 * Use within plugin_precache().
 *
 * Removes a key value from map,
 * if exists. Map can be empty
 * (to apply on all maps).
 *
 * An example of removing
 * British (on all maps).
 *
 * ""
 *   "info_doddetect"
 *     "detect_allies_country"
 *       ""
 */
native bool: DoD_AddKeyValDel(Map[], Class[], Key[], Val[]);

/**
 * Returns true if the weapon is a primary weapon.
 * You can pass strings like "weapon_thompson" here,
 * but their shorter version, "thompson", work as well.
 * Primary weapons are all on slot #3.
 */
native bool: DoD_IsWeaponPrimary(const Weapon[]);

/**
 * Returns true if the weapon is a secondary weapon.
 * You can pass strings like "weapon_colt" here,
 * but their shorter version, "colt", work as well.
 * Secondary weapons are all on slot #2.
 */
native bool: DoD_IsWeaponSecondary(const Weapon[]);

/**
 * Returns true if the weapon is a grenade.
 * You can pass strings like "weapon_handgrenade" here,
 * but their shorter version, "handgrenade", work as well.
 * Grenades are all on slot #5.
 */
native bool: DoD_IsWeaponGrenade(const Weapon[]);

/**
 * Returns true if the weapon is a knife/ spade.
 * You can pass strings like "weapon_spade" here,
 * but their shorter version, "spade", work as well.
 * Knives/ spades are all on slot #1.
 */
native bool: DoD_IsWeaponKnife(const Weapon[]);

/**
 * Turns off the process of automatically
 * adding a scope to a freshly given
 * weapon_fg42 or weapon_enfield
 * during game's original DoD_OnPlayerSpawn()
 * depending on player's pev_playerclass.
 *
 * May be called once a map.
 * Returns true on success.
 */
native bool: DoD_DisableAutoScoping();

/**
 * May be called once a map.
 * Returns true on success.
 */
native bool: DoD_EnableAutoScoping();
