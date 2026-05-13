
#include <amxmodx>
#include <dodx>

new g_Name[33][32];
new bool: g_Fake[33];
new bool: g_inServer[33];

public plugin_init()
    register_plugin("DoD Hacks: Killer", "1.0.0.4", "Hattrick HKS (claudiuhks)");

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
{
    g_inServer[Player] = true;
    g_Fake[Player] = bool: is_user_bot(Player);
    get_user_name(Player, g_Name[Player], charsmax(g_Name[]));
}

public DOD_ON_PLAYER_DISCONNECTED
{
    g_Fake[Player] = false;
    g_inServer[Player] = false;
}

public client_infochanged(Player)
    if (g_inServer[Player])
        get_user_info(Player, "name", g_Name[Player], charsmax(g_Name[]));

public client_death(Killer, Victim, Weapon, Place, isTeamKill)
{
    static weaponName[32], placeName[16];
    if (Victim == Killer || g_Fake[Victim])
        return PLUGIN_CONTINUE;
    xmod_get_wpnname(Weapon, weaponName, charsmax(weaponName));
    strtoupper(weaponName);
#if defined replace_stringex /** "MILLS BOMB EX" not implemented yet. It will be displayed as "MILLS BOMB". */
    replace_stringex(weaponName, charsmax(weaponName), "98", "AR", 2, 2, true); /// K98 => KAR
    replace_stringex(weaponName, charsmax(weaponName), "STG", "MP", 3, 2, true); /// STG44 => MP44
    replace_stringex(weaponName, charsmax(weaponName), "SPRINGFIELD", "SPRING", 11, 6, true);
    replace_stringex(weaponName, charsmax(weaponName), "PANZERSCHREK", "PSCHRECK", 12, 8, true);
    replace_stringex(weaponName, charsmax(weaponName), "KG", "K G", 2, 3, true); /// STICKGRENADE_EX => STICK GRENADE_EX
    replace_stringex(weaponName, charsmax(weaponName), "DG", "D G", 2, 3, true); /// HANDGRENADE_EX => HAND GRENADE_EX
    replace_stringex(weaponName, charsmax(weaponName), "_", " ", 1, 1, true); /// HAND GRENADE_EX => HAND GRENADE EX & STICK GRENADE_EX => STICK GRENADE EX
#else
    replace(weaponName, charsmax(weaponName), "98", "AR"); /// K98 => KAR
    replace(weaponName, charsmax(weaponName), "STG", "MP"); /// STG44 => MP44
    replace(weaponName, charsmax(weaponName), "SPRINGFIELD", "SPRING");
    replace(weaponName, charsmax(weaponName), "PANZERSCHREK", "PSCHRECK");
    replace(weaponName, charsmax(weaponName), "KG", "K G"); /// STICKGRENADE_EX => STICK GRENADE_EX
    replace(weaponName, charsmax(weaponName), "DG", "D G"); /// HANDGRENADE_EX => HAND GRENADE_EX
    replace(weaponName, charsmax(weaponName), "_", " "); /// HAND GRENADE_EX => HAND GRENADE EX & STICK GRENADE_EX => STICK GRENADE EX
#endif
    switch (g_inServer[Killer])
    {
        case true:
            switch (bool: is_user_alive(Killer))
            {
                case true:
                    if (readPlace(Place, placeName, charsmax(placeName)))
                        client_print(Victim, print_chat, "* Killed by %s (%d HP) with %s @ %s.",
                            g_Name[Killer], get_user_health(Killer), weaponName, placeName);
                    else
                        client_print(Victim, print_chat, "* Killed by %s (%d HP) with %s.",
                            g_Name[Killer], get_user_health(Killer), weaponName);
                default:
                    if (readPlace(Place, placeName, charsmax(placeName)))
                        client_print(Victim, print_chat, "* Killed by %s (DEAD) with %s @ %s.",
                            g_Name[Killer], weaponName, placeName);
                    else
                        client_print(Victim, print_chat, "* Killed by %s (DEAD) with %s.",
                            g_Name[Killer], weaponName);
            }
        default:
            if (readPlace(Place, placeName, charsmax(placeName)))
                client_print(Victim, print_chat, "* Killed by %s with %s @ %s.",
                    g_Name[Killer], weaponName, placeName);
            else
                client_print(Victim, print_chat, "* Killed by %s with %s.",
                    g_Name[Killer], weaponName);
    }
    return PLUGIN_CONTINUE;
}

bool: readPlace(Place, Buffer[], Size)
{
    switch (Place)
    {
        case HIT_HEAD:      copy(Buffer, Size, "HEAD");
        case HIT_CHEST:     copy(Buffer, Size, "CHEST");
        case HIT_STOMACH:   copy(Buffer, Size, "STOMACH");
        case HIT_LEFTARM:   copy(Buffer, Size, "LEFT ARM");
        case HIT_RIGHTARM:  copy(Buffer, Size, "RIGHT ARM");
        case HIT_LEFTLEG:   copy(Buffer, Size, "LEFT LEG");
        case HIT_RIGHTLEG:  copy(Buffer, Size, "RIGHT LEG");
    }
    return Place > HIT_GENERIC;
}
