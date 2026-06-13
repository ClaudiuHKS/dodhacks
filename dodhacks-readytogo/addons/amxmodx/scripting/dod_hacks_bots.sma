
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <dodconst>
#include <dodhacks>

#define ofs_playerUsingRandomClass 1472

new bool: g_allowSnipers;
new bool: g_allowGunners;
new bool: g_allowRockets;
new bool: g_isPlayerFake[33];
new bool: g_isPlayerInGame[33];

public plugin_init()
{
    register_plugin("DoD Hacks: Bots", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_bots.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[32];
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key, "@allowSnipers"))
            g_allowSnipers = bool: str_to_num(Val);
        else if (equali(Key, "@allowGunners"))
            g_allowGunners = bool: str_to_num(Val);
        else if (equali(Key, "@allowRockets"))
            g_allowRockets = bool: str_to_num(Val);
    }
    fclose(Config);
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
{
    g_isPlayerInGame[Player] = true;
    g_isPlayerFake[Player] = bool: is_user_bot(Player);
    if (g_isPlayerFake[Player])
        set_task(0.000001, "Task_UpdateFakePlayer_Once", Player);
}

public Task_UpdateFakePlayer_Once(Player)
{
    static Res;
    if (g_isPlayerFake[Player] && g_isPlayerInGame[Player])
    {
        DoD_SetPvDataBool(Player, ofs_playerUsingRandomClass, true);
        DoD_ChooseRandomClass(Player, Res, true, false /** Don't update scores table with the class right now. */,
            false, false, DoD_RCA_Add, true);
    }
}

public DOD_ON_PLAYER_DISCONNECTED
{
    g_isPlayerFake[Player] = false;
    g_isPlayerInGame[Player] = false;
}

public DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player)
    if (g_isPlayerFake[Player] && g_isPlayerInGame[Player])
        DoD_SetPvDataBool(Player, ofs_playerUsingRandomClass, true);

public DoD_OnChooseRandomClass_Post
(DoD_Address: CDoDTeamPlay, Player, Res)
    if (g_isPlayerFake[Player])
        if (!g_allowGunners && !g_allowRockets && !g_allowSnipers)
            switch (Res)
            {
                case DODC_30CAL, DODC_MG34, DODC_SCHARFSCHUTZE,
                    DODC_BAZOOKA, DODC_MG42, DODC_PANZERJAGER,
                    DODC_MARKSMAN, DODC_PIAT, DODC_SNIPER:
                    DoD_ChooseRandomClass(Player, Res, true, false /** Don't update scores table with the class right now. */,
                        false, false, DoD_RCA_Add, true);
            }
        else if (!g_allowGunners && !g_allowRockets && g_allowSnipers)
            switch (Res)
            {
                case DODC_30CAL, DODC_BAZOOKA, DODC_PANZERJAGER,
                    DODC_MG34, DODC_MG42, DODC_PIAT:
                    DoD_ChooseRandomClass(Player, Res, true, false /** Don't update scores table with the class right now. */,
                        false, false, DoD_RCA_Add, true);
            }
        else if (!g_allowGunners && g_allowRockets && !g_allowSnipers)
            switch (Res)
            {
                case DODC_30CAL, DODC_SCHARFSCHUTZE, DODC_MG34,
                    DODC_MG42, DODC_MARKSMAN, DODC_SNIPER:
                    DoD_ChooseRandomClass(Player, Res, true, false /** Don't update scores table with the class right now. */,
                        false, false, DoD_RCA_Add, true);
            }
        else if (g_allowGunners && !g_allowRockets && !g_allowSnipers)
            switch (Res)
            {
                case DODC_BAZOOKA, DODC_SCHARFSCHUTZE, DODC_SNIPER,
                    DODC_PANZERJAGER, DODC_MARKSMAN, DODC_PIAT:
                    DoD_ChooseRandomClass(Player, Res, true, false /** Don't update scores table with the class right now. */,
                        false, false, DoD_RCA_Add, true);
            }
        else if (g_allowGunners && g_allowRockets && !g_allowSnipers)
            switch (Res)
            {
                case DODC_SCHARFSCHUTZE, DODC_MARKSMAN, DODC_SNIPER:
                    DoD_ChooseRandomClass(Player, Res, true, false /** Don't update scores table with the class right now. */,
                        false, false, DoD_RCA_Add, true);
            }
        else if (g_allowGunners && !g_allowRockets && g_allowSnipers)
            switch (Res)
            {
                case DODC_BAZOOKA, DODC_PANZERJAGER, DODC_PIAT:
                    DoD_ChooseRandomClass(Player, Res, true, false /** Don't update scores table with the class right now. */,
                        false, false, DoD_RCA_Add, true);
            }
        else if (!g_allowGunners && g_allowRockets && g_allowSnipers)
            switch (Res)
            {
                case DODC_30CAL, DODC_MG34, DODC_MG42:
                    DoD_ChooseRandomClass(Player, Res, true, false /** Don't update scores table with the class right now. */,
                        false, false, DoD_RCA_Add, true);
            }
