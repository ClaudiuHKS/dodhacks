
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <dodconst>

new g_maxPlayers;
new g_entitiesNum;
new g_capAxis[32];
new g_capAlly[32];
new g_Entities[32];
new bool: g_inServer[33];
new bool: g_isFake[33];

public plugin_init()
{
    register_plugin("DoD Capture Manager", "1.0.0.1", "Vet(3TT3V)&claudiuhks");
    set_task(30.0, "Task_CheckPlayers", 535474909, "", 0, "b");
    g_maxPlayers = get_maxplayers();
}

public plugin_precache()
    register_forward(FM_KeyValue, "On_FM_KeyValue");

public On_FM_KeyValue(Entity, keyVal)
{
    static Key[32], Val[8], Num;
    if (!pev_valid(Entity))
        return FMRES_IGNORED;

    get_kvd(keyVal, KV_KeyName, Key, charsmax(Key));
    if (Key[0] != 'a' && Key[0] != 'A')
        return FMRES_IGNORED;

    if (equali(Key, "area_allies_numcap"))
    {
        get_kvd(keyVal, KV_Value, Val, charsmax(Val));
        Num = str_to_num(Val);
        if (Num > 1)
        {
            g_capAlly [g_entitiesNum  ] = Num;
            g_Entities[g_entitiesNum++] = Entity;
        }
    }
    else if (equali(Key, "area_axis_numcap"))
    {
        get_kvd(keyVal, KV_Value, Val, charsmax(Val));
        Num = str_to_num(Val);
        if (Num > 1)
        {
            g_capAxis [g_entitiesNum  ] = Num;
            g_Entities[g_entitiesNum++] = Entity;
        }
    }
    return FMRES_IGNORED;
}

public client_putinserver(Player)
{
    g_inServer[Player] = true;
    g_isFake[Player]   = bool: is_user_bot(Player);
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    g_inServer[Player] = false;
    g_isFake[Player]   = false;
}

public Task_CheckPlayers()
{
    static Axis, Ally, Player, Team;
    for (Player = 1, Axis = 0, Ally = 0; Player <= g_maxPlayers; Player++)
    {
        if (!g_inServer[Player] || g_isFake[Player])
            continue;
        Team = get_user_team(Player);
        switch (Team)
        {
            case ALLIES: Ally++;
            case AXIS:   Axis++;
        }
    }
    if (Axis < 3 || Ally < 3)
        singleCaps();
    else
        normalCaps();
}

singleCaps()
{
    static Iter;
    for (Iter = 0; Iter < g_entitiesNum; Iter++)
        addKeyVal(g_Entities[Iter], g_capAlly[Iter] ? "area_allies_numcap" : "area_axis_numcap",
            "1", "dod_capture_area");
}

normalCaps()
{
    static Buffer[8], Iter;
    for (Iter = 0; Iter < g_entitiesNum; Iter++)
        if (g_capAlly[Iter])
        {
            num_to_str(g_capAlly[Iter], Buffer, charsmax(Buffer));
            addKeyVal(g_Entities[Iter], "area_allies_numcap", Buffer, "dod_capture_area");
        }
        else
        {
            num_to_str(g_capAxis[Iter], Buffer, charsmax(Buffer));
            addKeyVal(g_Entities[Iter], "area_axis_numcap", Buffer, "dod_capture_area");
        }
}

addKeyVal(Entity, const Key[], const Val[], const Class[])
{
    set_kvd(0, KV_ClassName, Class);
    set_kvd(0, KV_KeyName,   Key);
    set_kvd(0, KV_Value,     Val);
    set_kvd(0, KV_fHandled,  0);

    return dllfunc(DLLFunc_KeyValue, Entity, 0);
}
