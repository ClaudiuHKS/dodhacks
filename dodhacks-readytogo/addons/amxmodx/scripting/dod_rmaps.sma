
#pragma semicolon 1   /// Do not touch.
#pragma ctrlchar  '\' /// Do not touch.

#include <amxmodx>
#include <amxmisc>

new bool: g_Valid = false;

new g_Map[64];
new g_Phrase[256];

new bool: g_inServer[33];

public plugin_init()
{
    register_plugin("DoD: Recent Maps", "1.0.0.1", "Hattrick HKS (claudiuhks)");

    register_clcmd("recentmaps",           "Show_Recent_Maps", ADMIN_ALL, "- shows recent maps");

    register_clcmd("say recentmaps",       "Show_Recent_Maps", ADMIN_ALL, "- shows recent maps");
    register_clcmd("say_team recentmaps",  "Show_Recent_Maps", ADMIN_ALL, "- shows recent maps");

    register_clcmd("say /recentmaps",      "Show_Recent_Maps", ADMIN_ALL, "- shows recent maps");
    register_clcmd("say_team /recentmaps", "Show_Recent_Maps", ADMIN_ALL, "- shows recent maps");

    new Buffer[256];
    get_datadir(Buffer, charsmax(Buffer));

    add(Buffer, charsmax(Buffer), "/dod_rmaps.ini");

    new File = fopen(Buffer, "r");

    if (!File)
    {
        File = fopen(Buffer, "w");

        if (File)
            fclose(File);
    }

    else
        fclose(File);
}

public plugin_cfg()
{
    get_mapname(g_Map, charsmax(g_Map));

    new Buffer[256];
    get_datadir(Buffer, charsmax(Buffer));

    add(Buffer, charsmax(Buffer), "/dod_rmaps.ini");

    new File = fopen(Buffer, "r");
    if (!File)
        return PLUGIN_CONTINUE;

    g_Valid = true;

    new Array: Maps = ArrayCreate(64);

    copy(g_Phrase, charsmax(g_Phrase), "* ");

    new Map[128];

    while (fgets(File, Map, charsmax(Map)) > 0)
    {
        trim(Map);

        if (!Map[0] || Map[0] == ';' || Map[0] == '/')
            continue;

        ArrayPushString(Maps, Map);

        add(g_Phrase, charsmax(g_Phrase), Map);
        add(g_Phrase, charsmax(g_Phrase), ", ");
    }

    fclose(File);

    new Size = strlen(g_Phrase);

    if (g_Phrase[Size - 2] == '*')
        formatex(g_Phrase, charsmax(g_Phrase), "* Nothing to show. Actual map is '%s'.", g_Map);

    else if (',' == g_Phrase[Size - 2])
    {
        g_Phrase[Size - 2] = '.';
        g_Phrase[Size - 1] =   0;
    }

    ArrayPushString(Maps, g_Map);

    Size = ArraySize(Maps);
    while (Size > 16)
    {
        ArrayDeleteItem(Maps, 0);

        --Size;
    }

    File = fopen(Buffer, "w");
    if (!File)
        goto cleanUp;

    for (new Item = 0; Item < Size; Item++)
        fprintf(File, "%a\n", ArrayGetStringHandle(Maps, Item));

    fclose(File);

cleanUp:

    ArrayDestroy(Maps);

    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    g_inServer[Player] = false;
}

public client_putinserver(Player)
{
    g_inServer[Player] = true;
}

public Show_Recent_Maps(Player)
{
    if (!g_inServer[Player])
        return PLUGIN_HANDLED;

    if (!g_Valid)
    {
        client_print(Player, print_chat,    "* Nothing to show. Actual map is '%s'.", g_Map);
        client_print(Player, print_console, "* Nothing to show. Actual map is '%s'.", g_Map);

        return PLUGIN_HANDLED;
    }

    client_print(Player, print_chat,    g_Phrase);
    client_print(Player, print_console, g_Phrase);

    return PLUGIN_HANDLED;
}
