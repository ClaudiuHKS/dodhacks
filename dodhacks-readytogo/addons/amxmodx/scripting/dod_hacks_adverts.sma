
#pragma ctrlchar '\' /// Replace '^' with '\'.

#include <amxmodx>
#include <amxmisc>

new g_hudSync;
new g_msgCount;
new g_maxPlayers;
new g_mapTime;
new g_hostName;
new g_friendlyFire;
new g_infMapTime[32];
new g_msgIdx[33];
new g_colorIdx[33];
new g_Buffer[512];
new bool: g_doConsole;
new bool: g_doRandom;
new bool: g_areThereMsgsForAll;
new bool: g_hudStyle;
new bool: g_doPause;
new bool: g_isFakePlayer[33];
new bool: g_isInServer[33];
new Float: g_hudVerPos;
new Float: g_hudHorPos;
new Float: g_hudHoldTime;
new Array: g_arrayMsgs;
new Array: g_arrayMsgFlags;

public plugin_init()
{
    register_plugin("DoD Hacks: Adverts", "1.0.0.4", "Hattrick HKS (claudiuhks)");

    get_configsdir(g_Buffer, charsmax(g_Buffer));
    add(g_Buffer, charsmax(g_Buffer), "/dod_hacks_adverts.ini");
    new Config = fopen(g_Buffer, "r");
    if (!Config)
    {
        set_fail_state("Error opening '%s'!", g_Buffer);
        return PLUGIN_HANDLED;
    }

    g_arrayMsgs = ArrayCreate(512);
    g_arrayMsgFlags = ArrayCreate();
#if defined replace_string
    new Flags, Map[64], Key[32], Val[512], Float: Interval;
#else
    new Pos, Flags, Map[64], Key[32], Val[512], Float: Interval;
#endif
    get_mapname(Map, charsmax(Map));
    while (fgets(Config, g_Buffer, charsmax(g_Buffer)) > 0)
    {
        trim(g_Buffer);
        if (!g_Buffer[0] || g_Buffer[0] == ';' || g_Buffer[0] == '/' ||
            2 != parse(g_Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        switch ('@' == Key[0])
        {
            case true:
            {
                if (equali(Key, "@hud_ver_pos"))
                    g_hudVerPos = str_to_float(Val);
                else if (equali(Key, "@hud_hor_pos"))
                    g_hudHorPos = str_to_float(Val);
                else if (equali(Key, "@hud_style"))
                    g_hudStyle = bool: str_to_num(Val);
                else if (equali(Key, "@hud_random"))
                    g_doRandom = bool: str_to_num(Val);
                else if (equali(Key, "@hud_hold"))
                    g_hudHoldTime = str_to_float(Val);
                else if (equali(Key, "@task_interval"))
                    Interval = str_to_float(Val);
                else if (equali(Key, "@hud_console"))
                    g_doConsole = bool: str_to_num(Val);
                else if (equali(Key, "@hud_pause"))
                    g_doPause = bool: str_to_num(Val);
                else if (equali(Key, "@msg_infMapTimeinite"))
                    copy(g_infMapTime, charsmax(g_infMapTime), Val);
            }
            default:
            {
#if defined replace_string
                replace_string(Val, charsmax(Val), "\\n", "\n", false);
                replace_string(Val, charsmax(Val), "%q%", "\"", false);
                replace_string(Val, charsmax(Val), "%m%", Map, false);
#else
                while ((Pos = containi(Val, "\\n")) > -1)
                {
                    strTransform(Val, 1, 1, true, Pos); /// Transform the key to lower if insensitively contained.
                    replace(Val, charsmax(Val), "\\n", "\n");
                }
                while ((Pos = containi(Val, "%q%")) > -1)
                {
                    strTransform(Val, 1, 1, true, Pos); /// Transform the key to lower if insensitively contained.
                    replace(Val, charsmax(Val), "%q%", "\"");
                }
                while ((Pos = containi(Val, "%m%")) > -1)
                {
                    strTransform(Val, 1, 1, true, Pos); /// Transform the key to lower if insensitively contained.
                    replace(Val, charsmax(Val), "%m%", Map);
                }
#endif
                Flags = read_flags(Key);
                ArrayPushString(g_arrayMsgs, Val);
                ArrayPushCell(g_arrayMsgFlags, Flags);
                if (!Flags)
                    g_areThereMsgsForAll = true;
            }
        }
    }
    fclose(Config);
    g_msgCount = ArraySize(g_arrayMsgs);
    if (g_msgCount)
    {
        set_task(Interval, "Task_ShowAds", .flags = "b");
        g_hudSync = CreateHudSyncObj();
        g_maxPlayers = get_maxplayers();
        g_hostName = cvar_exists("hostname") ?
            get_cvar_pointer("hostname") : 0;
        g_mapTime = cvar_exists("mp_timelimit") ?
            get_cvar_pointer("mp_timelimit") : 0;
        g_friendlyFire = cvar_exists("mp_friendlyfire") ?
            get_cvar_pointer("mp_friendlyfire") : 0;
    }
    return PLUGIN_CONTINUE;
}

public plugin_natives()
    if (is_plugin_loaded("Info. Messages"))
        pause("cd", "imessage.amxx");

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
    if (g_msgCount)
    {
        g_msgIdx[Player] = g_doRandom ?
            random_num(0, g_msgCount - 1) : 0;
        g_colorIdx[Player] = random_num(0, 9);
        g_isInServer[Player] = true;
        g_isFakePlayer[Player] = bool: is_user_bot(Player);
    }

public DOD_ON_PLAYER_DISCONNECTED
    if (g_msgCount)
        g_isInServer[Player] = false;

public Task_ShowAds()
{
#if defined replace_string
    static Player, R, G, B, Flags, Sec, Time[32], Float: Val,
        bool: friendlyFire, Name[128], cleanName[128], Len;
#else
    static Pos, Player, R, G, B, Flags, Sec, Time[32], Float: Val,
        bool: friendlyFire, Name[128], cleanName[128], Len;
#endif
    Val = g_mapTime ? get_pcvar_float(g_mapTime) : 0.0;
    if (Val > 0.0)
    {
        Sec = max(0, floatround(60.0 * Val - get_gametime(),
            floatround_tozero /** 2.7 => 2 & -2.7 => -2 */));
        if (Sec < 60)
            formatex(Time, charsmax(Time), "%ds", Sec);
        else
            formatex(Time, charsmax(Time), "%dm", Sec / 60);
    }
    else
        copy(Time, charsmax(Time), g_infMapTime);
    friendlyFire = g_friendlyFire && get_pcvar_num(g_friendlyFire);
    if (g_hostName)
    {
        Len = get_pcvar_string(g_hostName, Name, charsmax(Name));
        stripServerName(cleanName, charsmax(cleanName), Name, Len, true);
    }
    else
        copy(cleanName, charsmax(cleanName), "Server");
    for (Player = 1; Player <= g_maxPlayers; Player++)
    {
        if (false == g_isInServer[Player] || g_isFakePlayer[Player])
            continue;
Resume:
        Flags = ArrayGetCell(g_arrayMsgFlags, g_msgIdx[Player]);
        if (Flags != (get_user_flags(Player) & Flags))
        {
            if (g_msgCount == ++g_msgIdx[Player])
                g_msgIdx[Player] = 0;
            if (false == g_doPause && g_areThereMsgsForAll)
                goto Resume;
            continue;
        }
        switch (g_colorIdx[Player]++)
        {
            case  0: R =  20, G =  60, B = 180; /// D. blue.
            case  1: R = 200, G =  20, B =  20; /// Red.
            case  2: R = 200, G = 100, B =  20; /// Orange.
            case  3: R =  20, G = 180, B = 200; /// L. blue.
            case  4: R = 200, G = 160, B =  20; /// Yellow.
            case  5: R = 120, G =  20, B = 180; /// Purple.
            case  6: R = 180, G = 180, B = 180; /// White.
            case  7: R = 180, G =  20, B = 100; /// Magenta.
            case  8: R =  20, G = 200, B =  40; /// Green.
            default: R =  20, G = 180, B = 120; /// Teal.
        }
        set_hudmessage(R /** red */, G /** green */, B /** blue */,
            g_hudHorPos /** horizontal pos */, g_hudVerPos /** vertical pos */,
            g_hudStyle ? 1 : 0 /** effect type */, 0.5 /** effect time */,
            g_hudHoldTime /** duration */, 0.1 /** fade in time */, 0.1 /** fade out time */);
        ArrayGetString(g_arrayMsgs, g_msgIdx[Player]++,
            g_Buffer, charsmax(g_Buffer));
#if defined replace_string
        replace_string(g_Buffer, charsmax(g_Buffer), "%t%",
            Time, false);
        replace_string(g_Buffer, charsmax(g_Buffer), "%h%",
            cleanName, false);
        replace_string(g_Buffer, charsmax(g_Buffer), "%f%",
            friendlyFire ? "on" : "off", false);
#else
        while ((Pos = containi(g_Buffer, "%t%")) > -1)
        {
            strTransform(g_Buffer, 1, 1, true, Pos); /// Transform the key to lower if insensitively contained.
            replace(g_Buffer, charsmax(g_Buffer), "%t%",
                Time);
        }
        while ((Pos = containi(g_Buffer, "%h%")) > -1)
        {
            strTransform(g_Buffer, 1, 1, true, Pos); /// Transform the key to lower if insensitively contained.
            replace(g_Buffer, charsmax(g_Buffer), "%h%",
                cleanName);
        }
        while ((Pos = containi(g_Buffer, "%f%")) > -1)
        {
            strTransform(g_Buffer, 1, 1, true, Pos); /// Transform the key to lower if insensitively contained.
            replace(g_Buffer, charsmax(g_Buffer), "%f%",
                friendlyFire ? "on" : "off");
        }
#endif
        ShowSyncHudMsg(Player, g_hudSync, g_Buffer);
        if (g_doConsole)
            console_print(Player, g_Buffer);
        if (g_msgCount == g_msgIdx[Player])
            g_msgIdx[Player] = 0;
        if (g_colorIdx[Player] == 10)
            g_colorIdx[Player] = 0;
    }
}

stripServerName(cleanName[], Size, const Name[], Len, bool: Trim)
{
    static Iter, Chars;
    for (Chars = 0, Iter = 0; Iter < Len && Chars < Size; Iter++)
        if (Name[Iter] >= ' ')
            cleanName[Chars++] = Name[Iter];
    cleanName[Chars] = EOS;
    if (Trim)
        return trim(cleanName);
    return Chars;
}

#if !defined replace_string
strTransform(Buffer[], Skip, Chars, bool: lowerCase, Pos)
{
    static Iter;
    for (Iter = Skip + Pos; Iter < Pos + Skip + Chars; Iter++)
        Buffer[Iter] = lowerCase ? char_to_lower(Buffer[Iter]) :
            char_to_upper(Buffer[Iter]);
}
#endif
