
//
// DoD Hacks: Suicide - Shared Functions
//

#if defined _dod_hacks_suicide_included
    #endinput
#endif
#define _dod_hacks_suicide_included

/**
 * Returns true if players are
 * not allowed to commit suicide
 * right now.
 */
native bool: DoD_AreUsersBlockedFromSuicide();
