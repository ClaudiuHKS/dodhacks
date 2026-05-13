
//
// DoD Hacks: Knives - Shared Functions
//

#if defined _dod_hacks_knives_included
    #endinput
#endif
#define _dod_hacks_knives_included

/**
 * Returns true if the user
 * has thrown a knife and
 * the thrown knife has not
 * touched anything yet.
 *
 * While this, do not equip
 * them any kind of weapon.
 *
 * While this, do not allow
 * them to pick up items.
 *
 * This is a workaround for
 * XStats module (DODX) to
 * register the kill as a
 * real knife kill.
 */
native bool: DoD_IsUserWaitingThrowingKnife(Player);
