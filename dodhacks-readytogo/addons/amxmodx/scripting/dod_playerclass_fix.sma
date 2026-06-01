
#include <amxmodx>
#include <dodx>

new Trie: g_alliesClasses;
new Trie: g_britishClasses;
new Trie: g_axisParaClasses;
new Trie: g_playerClassInfo;

new bool: g_areAxisParas;
new bool: g_areAlliesBritish;
new bool: g_isPlayerInServer[33];

public plugin_init()
{
    register_plugin("DoD Player Class Fix", "1.0.0.3", "Hattrick HKS (claudiuhks)");
    /**
     * Thanks fysiks for recommending "Trie" calls.
     */
    g_playerClassInfo = TrieCreate();
    TrieSetCell(g_playerClassInfo, "cls_garand",   ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_carbine",  ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_tommy",    ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_grease",   ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_spring",   ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_bar",      ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_30cal",    ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_bazooka",  ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_enfield",  ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_sten",     ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_enfields", ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_bren",     ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_piat",     ALLIES);
    TrieSetCell(g_playerClassInfo, "cls_fg42",     AXIS);
    TrieSetCell(g_playerClassInfo, "cls_fg42s",    AXIS);
    TrieSetCell(g_playerClassInfo, "cls_k98",      AXIS);
    TrieSetCell(g_playerClassInfo, "cls_k43",      AXIS);
    TrieSetCell(g_playerClassInfo, "cls_mp40",     AXIS);
    TrieSetCell(g_playerClassInfo, "cls_mp44",     AXIS);
    TrieSetCell(g_playerClassInfo, "cls_k98s",     AXIS);
    TrieSetCell(g_playerClassInfo, "cls_mg34",     AXIS);
    TrieSetCell(g_playerClassInfo, "cls_mg42",     AXIS);
    TrieSetCell(g_playerClassInfo, "cls_pschreck", AXIS);

    g_alliesClasses = TrieCreate();
    TrieSetCell(g_alliesClasses, "cls_garand",  ALLIES);
    TrieSetCell(g_alliesClasses, "cls_carbine", ALLIES);
    TrieSetCell(g_alliesClasses, "cls_tommy",   ALLIES);
    TrieSetCell(g_alliesClasses, "cls_grease",  ALLIES);
    TrieSetCell(g_alliesClasses, "cls_spring",  ALLIES);
    TrieSetCell(g_alliesClasses, "cls_bar",     ALLIES);
    TrieSetCell(g_alliesClasses, "cls_30cal",   ALLIES);
    TrieSetCell(g_alliesClasses, "cls_bazooka", ALLIES);

    g_britishClasses = TrieCreate();
    TrieSetCell(g_britishClasses, "cls_enfield",  ALLIES);
    TrieSetCell(g_britishClasses, "cls_sten",     ALLIES);
    TrieSetCell(g_britishClasses, "cls_enfields", ALLIES);
    TrieSetCell(g_britishClasses, "cls_bren",     ALLIES);
    TrieSetCell(g_britishClasses, "cls_piat",     ALLIES);

    g_axisParaClasses = TrieCreate();
    TrieSetCell(g_axisParaClasses, "cls_fg42",  AXIS);
    TrieSetCell(g_axisParaClasses, "cls_fg42s", AXIS);

    g_areAxisParas     = bool: dod_get_map_info(MI_AXIS_PARAS);
    g_areAlliesBritish = bool: dod_get_map_info(MI_ALLIES_TEAM);
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
    g_isPlayerInServer[Player] = false;

public client_putinserver(Player)
    g_isPlayerInServer[Player] = true;

public client_command(Player)
{
    static Cmd[16], Team;
    if (!g_isPlayerInServer[Player] || read_argv(0, Cmd, charsmax(Cmd)) < 4 || ('c' != Cmd[0] && Cmd[0] != 'C'))
        return PLUGIN_CONTINUE;
    strtolower(Cmd);
    if (TrieGetCell(g_playerClassInfo, Cmd, Team))
    {
        if ((!g_areAxisParas     && TrieKeyExists(g_axisParaClasses, Cmd)) ||
            (g_areAlliesBritish  && TrieKeyExists(g_alliesClasses,   Cmd)) ||
            (!g_areAlliesBritish && TrieKeyExists(g_britishClasses,  Cmd)))
            return PLUGIN_HANDLED; /// Map-related filtering.
        return Team != get_user_team(Player) ? PLUGIN_HANDLED : PLUGIN_CONTINUE; /// Team-related filtering.
    }
    return equal(Cmd, "cls_", 4) && !equal(Cmd, "cls_random") ? PLUGIN_HANDLED : PLUGIN_CONTINUE; /// General filtering.
}

/**
 * v1.0.0.0 - Initial release.
 * v1.0.0.1 - Block "cls_*" console command(s) as well.
 * v1.0.0.2 - Allow "cls_random" command.
 * v1.0.0.3 - Disallow cmds. depending on actual map type. Use only "Trie" edition from now on.
 */
