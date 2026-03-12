
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
 * An address of
 * a function
 * or a pointer.
 */
enum DoD_Address
{
    DoD_Address_Null = 0,
};

/**
 * To be used
 * only within
 * DoD_GetFunctionAddress().
 */
enum DoD_FuncIndex
{
    DoD_FI_PlayerSpawn = 0,
    DoD_FI_GiveNamedItem,
    DoD_FI_DropPlayerItem,
    DoD_FI_GiveAmmo,
    DoD_FI_SetWaveTime,
    DoD_FI_GetWaveTime,
    DoD_FI_RemovePlayerItem,
    DoD_FI_AddPlayerItem,
    DoD_FI_RemoveAllItems,
    DoD_FI_SetBodygroup,
    DoD_FI_DestroyItem,
    DoD_FI_SubRemove,
    DoD_FI_PackWeapon,
    DoD_FI_InstallGameRules,
    DoD_FI_WpnBoxKill,
    DoD_FI_WpnBoxActivateThink,
    DoD_FI_Create,
    DoD_FI_UtilRemove,
    DoD_FI_CreateNamedEntity,
};

/**
 * To be used within
 * DoD_StoreToAddress()
 * and DoD_ReadFromAddress().
 */
enum DoD_MemSize
{
    DoD_Int8 = false,
    DoD_UInt8,
    DoD_Int16,
    DoD_UInt16,
    DoD_Int32,
};

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
 * Player is being spawned. Ensure the
 * player is in server during this call.
 * pev_playerclass checks, DoD_GiveNamedItem()
 * calls and weapon_enfield/ weapon_fg42 scope
 * installations are made in this game's
 * original function (if not disabled by
 * calling DoD_DisableAutoScoping() already).
 * Player may be altered. If the player is
 * in server, they should be alive now.
 *
 * CDoDTeamPlay (Game Rules) is an address
 * of a pointer.
 */
forward DoD_OnPlayerSpawn(DoD_Address: CDoDTeamPlay, &Player);

/**
 * Player got spawned. If they are in
 * server, they should be alive now.
 */
forward DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player);

/**
 * Player receives a named item.
 * Item's content may be altered during call.
 * In module's source code, Item's size is 64.
 * Strings like "weapon_scopedenfield" or
 * "weapon_scopedfg42" may be passed here.
 *
 * Note, the game never passes by itself
 * strings like "weapon_scopedenfield" or
 * "weapon_scopedfg42". Plugins can, if
 * they execute DoD_GiveNamedItem() with
 * ExecHooks parameter set to true.
 * "weapon_scopekar" may be passed by the
 * game. Just the two above are "unique".
 *
 * If returning PLUGIN_HANDLED, game will
 * then receive OverrideRes as the result
 * of the call. OverrideRes is initially
 * set to -1. -1 means a null entity. 0
 * would mean the worldspawn entity.
 */
forward DoD_OnGiveNamedItem(Player, Item[], ItemSize /** = 64 */, &OverrideRes);

/**
 * Player has received a named item.
 * ItemEntity is the resulted item
 * entity index, if any. -1 otherwise.
 */
forward DoD_OnGiveNamedItem_Post(Player, const Item[], ItemEntity);

/**
 * Player drops an item.
 * Item's content may be altered during call.
 * In module's source code, Item's size is 64.
 * If Force is set to true, the player will
 * be able to drop their knife or spade
 * (as an example). Item's content can be empty.
 * If empty string, current weapon will drop.
 */
forward DoD_OnDropPlayerItem(Player, Item[], ItemSize /** = 64 */, &bool: Force);

/**
 * Player dropped an item.
 */
forward DoD_OnDropPlayerItem_Post(Player, const Item[], Force);

/**
 * Item is removed from
 * player's inventory.
 * Item may be altered.
 *
 * If returning
 * PLUGIN_HANDLED, game
 * will then receive
 * OverrideRes as the
 * result of the call.
 * OverrideRes is
 * initially set to 0.
 * 0 means failure.
 * 1 success.
 */
forward DoD_OnRemovePlayerItem(Player, &Item, &OverrideRes);

/**
 * On success,
 * Res is set to 1.
 */
forward DoD_OnRemovePlayerItem_Post(Player, Item, Res);

/**
 * Item is added to
 * player's inventory.
 * Item may be altered.
 *
 * If returning
 * PLUGIN_HANDLED, game
 * will then receive
 * OverrideRes as the
 * result of the call.
 * OverrideRes is
 * initially set to 0.
 * 0 means failure.
 * 1 success.
 */
forward DoD_OnAddPlayerItem(Player, &Item, &OverrideRes);

/**
 * On success,
 * Res is set to 1.
 */
forward DoD_OnAddPlayerItem_Post(Player, Item, Res);

/**
 * Item is being
 * destroyed.
 */
forward DoD_OnDestroyItem(Item);

/**
 * Item has been
 * destroyed.
 */
forward DoD_OnDestroyItem_Post(Item);

/**
 * Entity is being removed
 * via SUB_Remove() game call.
 */
forward DoD_OnSubRemove(Entity);

/**
 * Entity got removed via
 * SUB_Remove() game call.
 */
forward DoD_OnSubRemove_Post(Entity);

/**
 * Entity is being removed
 * via UTIL_Remove() game call.
 * Entity may be altered.
 */
forward DoD_OnUtilRemove(&Entity);

/**
 * Entity got removed via
 * UTIL_Remove() game call.
 */
forward DoD_OnUtilRemove_Post(Entity);

/**
 * A weaponbox entity
 * is being killed.
 */
forward DoD_OnWpnBoxKill(Entity);

/**
 * A weaponbox entity
 * got killed.
 */
forward DoD_OnWpnBoxKill_Post(Entity);

/**
 * A weaponbox entity
 * gets ActivateThink()
 * function called.
 */
forward DoD_OnWpnBoxActivateThink(Entity);

/**
 * A weaponbox entity
 * got ActivateThink()
 * function called.
 */
forward DoD_OnWpnBoxActivateThink_Post(Entity);

/**
 * A named entity is
 * being created via
 * CBaseEntity::Create().
 * Name's content may be
 * altered during call.
 * In module's source code,
 * Name's size is 64.
 * Origin, Angles and
 * Owner may be altered.
 * Use -1 for null entity.
 *
 * This function also
 * executes
 * AddEntityHashValue().
 *
 * If returning
 * PLUGIN_HANDLED, game
 * will then receive
 * OverrideRes as the
 * result of the call.
 * OverrideRes is initially
 * set to -1. -1 means a
 * null entity. 0 would
 * mean the worldspawn
 * entity.
 *
 * When a weapon is dropped,
 * this function is called
 * (as an example).
 */
forward DoD_OnCreate(Name[], NameSize /** = 64 */, Float: Origin[3], Float: Angles[3], &Owner, &OverrideRes);

/**
 * A named entity got
 * created via
 * CBaseEntity::Create().
 * ItemRes is the created
 * entity index or -1.
 */
forward DoD_OnCreate_Post(const Name[], const Float: Origin[3], const Float: Angles[3], Owner, ItemRes);

/**
 * Creating a named entity.
 * Name's content may be
 * altered during call.
 * In module's source code,
 * Name's size is 64.
 *
 * This function also
 * executes
 * AddEntityHashValue().
 *
 * If returning
 * PLUGIN_HANDLED, game
 * will then receive
 * OverrideRes as the
 * result of the call.
 * OverrideRes is initially
 * set to -1. -1 means a
 * null entity. 0 would
 * mean the worldspawn
 * entity.
 */
forward DoD_OnCreateNamedEntity(Name[], NameSize /** = 64 */, &OverrideRes);

/**
 * A named entity got
 * created. Entity is the
 * created entity index
 * or -1.
 */
forward DoD_OnCreateNamedEntity_Post(const Name[], Entity);

/**
 * Game is asking for respawn wave time.
 * Team can be altered during call.
 * If PLUGIN_HANDLED is returned,
 * game's original function will not be called
 * anymore and it will just return what Time
 * variable says. Time variable is ignored if
 * returning PLUGIN_CONTINUE.
 * Time variable is initially zero (0.0).
 * CDoDTeamPlay (Game Rules) is
 * an address of a pointer.
 */
forward DoD_OnGetWaveTime(DoD_Address: CDoDTeamPlay, &Team, &Float: Time);

/**
 * Time variable is the result.
 * CDoDTeamPlay (Game Rules) is
 * an address of a pointer.
 */
forward DoD_OnGetWaveTime_Post(DoD_Address: CDoDTeamPlay, Team, Float: Time);

/**
 * Game is setting a respawn wave time.
 * Team can be changed during execution.
 * Time can be changed during execution.
 * CDoDTeamPlay (Game Rules) is
 * an address of a pointer.
 */
forward DoD_OnSetWaveTime(DoD_Address: CDoDTeamPlay, &Team, &Float: Time);

/**
 * Game set a respawn wave time.
 */
forward DoD_OnSetWaveTime_Post(DoD_Address: CDoDTeamPlay, Team, Float: Time);

/**
 * Player is having all items removed
 * from their inventory.
 * RemoveSuit may be changed during call.
 */
forward DoD_OnRemoveAllItems(Player, &RemoveSuit);

/**
 * Player got all their items removed.
 */
forward DoD_OnRemoveAllItems_Post(Player, RemoveSuit);

/**
 * Player receives ammo.
 * Ammo, Name and Max may
 * be altered during call.
 * For more details,
 * see DoD_GiveAmmo() native.
 *
 * If returning PLUGIN_HANDLED,
 * game will then receive
 * OverrideRes as the result
 * of the call. OverrideRes is
 * initially set to -1. -1
 * means failure.
 */
forward DoD_OnGiveAmmo(Player, &Ammo, Name[], NameSize /** = 64 */, &Max, &OverrideRes);

/**
 * Player has received ammo.
 * Res is the result returned
 * by the original game call.
 * -1 on failure.
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
 * Weapon is being packed
 * into a weaponbox entity.
 * Weapon may be altered.
 *
 * If returning
 * PLUGIN_HANDLED, game
 * will then receive
 * OverrideRes as the
 * result of the call.
 * OverrideRes is
 * initially set to 0.
 * 0 means failure.
 * 1 success.
 */
forward DoD_OnPackWeapon(Entity, &Weapon, &OverrideRes);

/**
 * Weapon was packed
 * into a weaponbox entity.
 * Res is the game call's result.
 * 1 on success and 0 on failure.
 */
forward DoD_OnPackWeapon_Post(Entity, Weapon, Res);

/**
 * CDoDTeamPlay (Game Rules)
 * is being installed.
 *
 * If returning
 * PLUGIN_HANDLED, game will
 * then receive OverrideRes
 * as the result of the call.
 * OverrideRes is initially
 * set to DoD_Address_Null.
 * DoD_Address_Null means a
 * null pointer.
 */
forward DoD_OnInstallGameRules(&DoD_Address: OverrideRes);

/**
 * CDoDTeamPlay (Game Rules)
 * installed. CDoDTeamPlay is
 * the resulted pointer address.
 */
forward DoD_OnInstallGameRules_Post(DoD_Address: CDoDTeamPlay);

/**
 * When ExecHooks parameter
 * is set to true for a native,
 * all related hooks, in all
 * plugins, will be executed.
 *
 * If a hook returns
 * PLUGIN_HANDLED, the native
 * can fail (of course, with
 * no error logs).
 */

/**
 * Returns true if the current
 * CDoDTeamPlay (Game Rules)
 * pointer address for this
 * map session is valid.
 */
native bool: DoD_AreGameRulesReady();

/**
 * Calls game's original
 * InstallGameRules()
 * function. Returns
 * CDoDTeamPlay (Game Rules)
 * pointer address.
 */
native DoD_InstallGameRules(bool: ExecHooks = false);

/**
 * Calls game's original
 * PlayerSpawn() function.
 * Returns true on success.
 */
native bool: DoD_PlayerSpawn(Player, bool: ExecHooks = false);

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
 * index or -1 on failure.
 * Strings like
 * "weapon_scopedenfield" or
 * "weapon_scopedfg42" may be
 * passed here successfully.
 *
 * Fails if signature for
 * DoD_GiveNamedItem is missing.
 */
native bool: DoD_GiveNamedItem(Player, const Item[], &ItemEntity, bool: ExecHooks = false);

/**
 * Sets entity's or
 * player's bodygroup.
 * Returns true on success.
 */
native bool: DoD_SetBodygroup(Entity, Group, Value, bool: ExecHooks = false);

/**
 * Destroys an item.
 */
native bool: DoD_DestroyItem(Item, bool: ExecHooks = false);

/**
 * Removes an entity by
 * calling game's SUB_Remove().
 */
native bool: DoD_SubRemove(Entity, bool: ExecHooks = false);

/**
 * Removes an entity by
 * calling game's UTIL_Remove().
 */
native bool: DoD_UtilRemove(Entity, bool: ExecHooks = false);

/**
 * Packs a weapon entity into
 * a weaponbox entity.
 * Returns true on success.
 * Res is the game call's result.
 * 0 failure and 1 success.
 */
native bool: DoD_PackWeapon(Entity, Weapon, &Res, bool: ExecHooks = false);

/**
 * Drops an item from player.
 * Returns true on success.
 * If empty string, current
 * item will be dropped. If
 * Force is true, spade/ knife
 * may be dropped as well.
 */
native bool: DoD_DropPlayerItem(Player, const Item[], Force, bool: ExecHooks = false);

/**
 * Gives ammo to a player.
 * Returns true on success.
 * Res is the result returned
 * by game call. On failure,
 * Res is -1.
 *
 * Ammo names are listed below (left side).
 *
 * ammo_agrens   @ HANDGRENADE     (Allies, British)
 * ammo_agrensex @ HANDGRENADE_EX  (Allies, British)
 * ammo_ggrens   @ STICKGRENADE    (Axis)
 * ammo_ggrensex @ STICKGRENADE_EX (Axis)
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
native bool: DoD_GiveAmmo(Player, Ammo, const Name[], Max, &Res, bool: ExecHooks = false);

/**
 * Removes all items from
 * player's inventory.
 * Returns true on success.
 */
native bool: DoD_RemoveAllItems(Player, RemoveSuit, bool: ExecHooks = false);

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
 * behavior. If Refresh is true, player's
 * screen will instantly show the scope
 * attached to the weapon, without player
 * having to switch between weapons
 * before, in order to see it there. If
 * Refresh is true, DoD_DeployItem() is
 * included in this call, once the scope
 * has been added.
 *
 * If the specified weapon already owns
 * a scope, DoD_DeployItem() will not
 * be executed within this call,
 * even if Refresh is set to true.
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
 * Returns true on success.
 * Res is set to what
 * game returns otherwise
 * (1 [on success] or 0).
 */
native bool: DoD_RemovePlayerItem(Player, Item, &Res, bool: ExecHooks = false);

/**
 * Adds an item to
 * player's inventory.
 * Returns true on success.
 * Res is set to what
 * game returns otherwise
 * (1 [on success] or 0).
 */
native bool: DoD_AddPlayerItem(Player, Item, &Res, bool: ExecHooks = false);

/**
 * Sets the respawn wave time
 * for a specific team. Team 1
 * for Allies and 2 for Axis.
 * Returns true on success.
 */
native bool: DoD_SetWaveTime(Team, Float: Time, bool: ExecHooks = false);

/**
 * Gets the respawn wave time
 * for a specific team. Team 1
 * for Allies and 2 for Axis.
 */
native Float: DoD_GetWaveTime(Team, bool: ExecHooks = false);

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
native bool: DoD_AddKeyValAdd(const Map[], const Class[], const Key[], const Val[]);

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
native bool: DoD_AddKeyValDel(const Map[], const Class[], const Key[], const Val[]);

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

/**
 * Returns true if auto.
 * scoping of weapon_fg42
 * and weapon_enfield
 * depending on
 * pev_playerclass
 * during PlayerSpawn()
 * is enabled.
 */
native bool: DoD_IsAutoScopingEnabled();

/**
 * Returns a function address.
 * If ExecHooks is true, hooks
 * related to Func will be
 * executed when Func gets
 * called.
 *
 * Returns DoD_Address_Null if
 * Func has not been found.
 */
native DoD_Address: DoD_GetFunctionAddress(DoD_FuncIndex: Func, bool: ExecHooks);

/**
 * Sets an entity think function,
 * think function delta and
 * its next think time.
 *
 * Time can't be smaller than 0.0.
 *
 * NextThinkTime should be a value
 * like 1.0, not get_gametime() +
 * 1.0 (if AddGameTime is true).
 */
native bool: DoD_SetEntityThinkFunc(Entity, DoD_Address: Func, bool: SetDelta, Delta, bool: SetNextThinkTime, bool: AddGameTime, Float: NextThinkTime);

/**
 * Gets the think function and
 * think function delta of a
 * given entity.
 */
native DoD_Address: DoD_GetEntityThinkFunc(Entity, &Delta);

/**
 * Kills a weaponbox entity.
 * Returns true on success.
 */
native bool: DoD_WpnBoxKill(Entity, bool: ExecHooks = false);

/**
 * Calls ActivateThink() on
 * a weaponbox entity.
 * Returns true on success.
 */
native bool: DoD_WpnBoxActivateThink(Entity, bool: ExecHooks = false);

/**
 * Creates a named entity
 * using CBaseEntity::Create().
 * Returns true on success.
 * ItemRes is the entity index
 * of the created entity, if any.
 * -1 otherwise. Returning true
 * does not guarantee ItemRes
 * is not set to -1.
 *
 * To set an entity as null, use
 * -1 value.
 *
 * This function also executes
 * AddEntityHashValue().
 */
native bool: DoD_Create(const Name[], const Float: Origin[3], const Float: Angles[3], Owner, &ItemRes, bool: ExecHooks = false);

/**
 * Creates a named entity.
 * Returns true on success.
 * Entity is the entity
 * index of the created
 * entity, if any. -1
 * otherwise.
 *
 * This function also
 * executes
 * AddEntityHashValue().
 */
native bool: DoD_CreateNamedEntity(const Name[], &Entity, bool: ExecHooks = false);

/**
 * Finds an address in
 * memory of a module
 * by using a signature.
 * IDA style. Returns
 * true on success.
 *
 * Module can be "dod",
 * "cs", "tfc", "hl",
 * "dmc", "ricochet"
 * or "mp".
 *
 * Module can be
 * "engine" on Linux
 * and "swds" on
 * Windows. You should
 * use
 * 'is_linux_server()'.
 *
 * Module can be
 * "file_i386",
 * "file_i486" and
 * "file_i686" on
 * Linux (as example).
 */
native bool: DoD_FindSignature(const Module[], const Signature[], bool: QuestionMarkAllowsZero, &DoD_Address: Address);

/**
 * Finds an address in
 * memory of a module
 * by using a symbol.
 * Returns true on
 * success.
 *
 * Module can be "dod",
 * "cs", "tfc", "hl",
 * "dmc", "ricochet"
 * or "mp".
 *
 * Module can be
 * "engine" on Linux
 * and "swds" on
 * Windows. You should
 * use
 * 'is_linux_server()'.
 *
 * Module can be
 * "file_i386",
 * "file_i486" and
 * "file_i686" on
 * Linux (as example).
 */
native bool: DoD_FindSymbol(const Module[], const Symbol[], &DoD_Address: Address);

/**
 * Reads a value from an
 * address. Returns true
 * on success.
 */
native bool: DoD_ReadFromAddress(DoD_Address: Address, DoD_MemSize: Size, &Value);

/**
 * Stores a value to an
 * address. Returns true
 * on success.
 */
native bool: DoD_StoreToAddress(DoD_Address: Address, Value, DoD_MemSize: Size);

/**
 * Allocates a string
 * via engine's
 * AllocString() if
 * not already
 * allocated by DoD
 * Hacks. Returns
 * true on success.
 * Allocated is the
 * allocated string.
 * On fail, Allocated
 * is set to -1.
 */
native bool: DoD_AllocString(const String[], &Allocated);

/**
 * Effects, installed on demand,
 * only if a plugin uses them.
 */

/**
 * Installs/ updates
 * pfnAddToFullPack_Post() for
 * this map and allows players
 * to see their own explo.
 * nade projectiles glowing.
 *
 * Solid is
 * 'short ::entity_state_s::solid'.
 * Can be SOLID_NOT.
 *
 * If alterSolidState is set to
 * false, Solid param. is ignored.
 *
 * Intended to be called in
 * plugin_init().
 *
 * Returns true.
 */
native bool: DoD_AddExploNadeProjGlow(Mode, Fx, Color[3], Amount, Solid, bool: alterSolidState);

/**
 * Disallows players see their
 * own explo. nade projectiles
 * glowing.
 *
 * Uninstalls
 * pfnAddToFullPack_Post().
 *
 * Returns true if it was
 * enabled. False otherwise.
 */
native bool: DoD_DelExploNadeProjGlow();

/**
 * Sets a player excluded
 * or unexcluded
 * from seeing explosive
 * grenade projectiles
 * glowing. Players are
 * initially unexcluded.
 */
native bool: DoD_SetExploNadeProjGlow(Player, bool: Excluded);

/**
 * Gets whether a player
 * is excluded from seeing
 * explosive grenade
 * projectiles glowing.
 */
native bool: DoD_GetExploNadeProjGlow(Player);

/**
 * Installs/ updates
 * pfnAddToFullPack_Post() for
 * this map and allows players
 * to see dropped explo. nades
 * glowing.
 *
 * Solid is
 * 'short ::entity_state_s::solid'.
 * Can be SOLID_NOT.
 *
 * If alterSolidState is set to
 * false, Solid param. is ignored.
 *
 * If includeArmedExploNades is
 * set to true, these dropped
 * explo. nades will glow as well.
 *
 * Intended to be called in
 * plugin_init().
 *
 * Returns true.
 */
native bool: DoD_AddDroppedExploNadeGlow(bool: includeArmedExploNades, Mode, Fx, Color[3], Amount, Solid, bool: alterSolidState);

/**
 * Disallows players see
 * dropped explo. nades
 * glowing.
 *
 * Uninstalls
 * pfnAddToFullPack_Post().
 *
 * Returns true if it was
 * enabled. False otherwise.
 */
native bool: DoD_DelDroppedExploNadeGlow();

/**
 * Sets a player excluded
 * or unexcluded
 * from seeing dropped
 * explosive grenades
 * glowing. Players are
 * initially unexcluded.
 */
native bool: DoD_SetDroppedExploNadeGlow(Player, bool: Excluded);

/**
 * Gets whether a player
 * is excluded from seeing
 * dropped explosive
 * grenades glowing.
 */
native bool: DoD_GetDroppedExploNadeGlow(Player);
