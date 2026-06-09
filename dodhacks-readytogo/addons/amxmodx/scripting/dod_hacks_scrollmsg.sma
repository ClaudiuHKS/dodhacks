// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

//
// Scrolling Message Plugin
//

#include <amxmodx>
#include <amxmisc>

/**
 * ---------
 * DoD Hacks
 * ---------
 */
#if !defined NULL_STRING && !defined engine_changelevel
#include <dodhacks> /// For 'DoD_ChangeMap(Map)' function. May be replaced with 'engine_changelevel(Map)' or 'engfunc(EngFunc_ChangeLevel, Map, NULL_STRING)' for removing DoD Hacks dependency (if your AMXX version has either of these 2).
#endif
/**
 * ---------
 */

#define SPEED 0.3
#define SCROLLMSG_SIZE	512

new g_startPos
new g_endPos
new g_scrollMsg[SCROLLMSG_SIZE]
new g_displayMsg[SCROLLMSG_SIZE]
new Float:g_xPos
new g_Length
new g_Frequency

public plugin_init()
{
	///register_plugin("Scrolling Message", AMXX_VERSION_STR, "AMXX Dev Team")
	/**
	 * ---------
	 * DoD Hacks
	 * ---------
	 */
	register_plugin("DoD Hacks: Scroll Msg", AMXX_VERSION_STR, "AMXX Dev Team");
	if (safeDisablePluginIfRunning("DoD.Hacks.ScrollMsg.LOG", "scrollmsg.amxx"))
	{
		new Map[64];
		get_mapname(Map, charsmax(Map));
#if defined engine_changelevel
		engine_changelevel(Map);
#else
#if defined NULL_STRING
		engfunc(EngFunc_ChangeLevel, Map, NULL_STRING);
#else
		DoD_ChangeMap(Map);
#endif
#endif
		return;
	}
	register_dictionary("dod_hacks_scrollmsg.txt");
	/**
	 * ---------
	 */
	///register_dictionary("scrollmsg.txt")
	register_dictionary("common.txt")
	register_srvcmd("amx_scrollmsg", "setMessage")
}

/**
 * ---------
 * DoD Hacks
 * ---------
 */
public plugin_natives()
    if (is_plugin_loaded("Scrolling Message"))
        pause("cd", "scrollmsg.amxx");
/**
 * ---------
 */

public showMsg()
{
	new a = g_startPos, i = 0

	while (a < g_endPos)
		g_displayMsg[i++] = g_scrollMsg[a++]

	g_displayMsg[i] = 0

	if (g_endPos < g_Length)
		g_endPos++

	if (g_xPos > 0.35)
		g_xPos -= 0.0063
	else
	{
		g_startPos++
		g_xPos = 0.35
	}

	set_hudmessage(200, 100, 0, g_xPos, 0.90, 0, SPEED, SPEED, 0.05, 0.05, 2)
	show_hudmessage(0, "%s", g_displayMsg)
}

public msgInit()
{
	g_endPos = 1
	g_startPos = 0
	g_xPos = 0.65
	
	new hostname[64]
	
	///get_cvar_string("hostname", hostname, charsmax(hostname))
	/**
	 * ---------
	 * DoD Hacks
	 * ---------
	 */
	static bool: bOnce, hostname_cvar, hostname_raw[64], hostname_len;
	if (!bOnce)
	{
		bOnce = true;
		hostname_cvar = get_cvar_pointer("hostname");
	}
	if (!hostname_cvar)
		hostname_len = copy(hostname_raw, charsmax(hostname_raw), "?");
	else
		hostname_len = get_pcvar_string(hostname_cvar, hostname_raw, charsmax(hostname_raw));
	stripServerName(hostname, charsmax(hostname), hostname_raw, hostname_len, true);
	/**
	 * ---------
	 */
	replace(g_scrollMsg, charsmax(g_scrollMsg), "%hostname%", hostname)
	
	g_Length = strlen(g_scrollMsg)
	
	set_task(SPEED, "showMsg", 123, "", 0, "a", g_Length + 48)
	client_print(0, print_console, "%s", g_scrollMsg)
}

public setMessage()
{
	remove_task(123)		/* remove current messaging */
	read_argv(1, g_scrollMsg, charsmax(g_scrollMsg))
	
	g_Length = strlen(g_scrollMsg)
	
	new mytime[32]
	
	read_argv(2, mytime, charsmax(mytime))
	
	g_Frequency = str_to_num(mytime)
	
	if (g_Frequency > 0)
	{
		new minimal = floatround((g_Length + 48) * (SPEED + 0.1))
		
		if (g_Frequency < minimal)
		{
			server_print("%L", LANG_SERVER, "MIN_FREQ", minimal)
			g_Frequency = minimal
		}

		server_print("%L", LANG_SERVER, "MSG_FREQ", g_Frequency / 60, g_Frequency % 60)
		set_task(float(g_Frequency), "msgInit", 123, "", 0, "b")
	}
	else
		server_print("%L", LANG_SERVER, "MSG_DISABLED")
	
	return PLUGIN_HANDLED
}

/**
 * ---------
 * DoD Hacks
 * ---------
 */
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

bool: safeDisablePluginIfRunning(const logFile[], const Name[])
{
    new Buffer[256];
    get_localinfo("amxx_plugins", Buffer, charsmax(Buffer));
    new File = fopen(Buffer, "r");
    if (!File)
        return false;
    new newName[256];
    new nameLen = copy(newName, charsmax(newName), Name);
#if defined mb_strtolower
    nameLen = mb_strtolower(newName, nameLen);
#else
    nameLen = strtolower(newName);
#endif
    new Array: Lines = ArrayCreate(256), Trimmed[256], bool: bExists, Size;
    while (fgets(File, Buffer, charsmax(Buffer)))
    {
        copy(Trimmed, charsmax(Trimmed), Buffer);
        trim(Trimmed);
        if (equali(Trimmed, newName, nameLen))
        {
            bExists = true;
#if defined replace_stringex
            new trimmedLen = formatex(Trimmed, charsmax(Trimmed), ";%s", newName);
            replace_stringex(Buffer, charsmax(Buffer), newName, Trimmed, nameLen, trimmedLen, false);
#else
            formatex(Trimmed, charsmax(Trimmed), ";%s", newName);
            strTransformEx(Buffer, newName, 0, nameLen, true); /// Transform the key to lower if insensitively contained.
            replace(Buffer, charsmax(Buffer), newName, Trimmed);
#endif
        }
        ArrayPushString(Lines, Buffer);
        ++Size;
    }
    fclose(File);
    if (!bExists || Size < 1)
    {
        ArrayDestroy(Lines);
        return false;
    }
    get_localinfo("amxx_plugins", Buffer, charsmax(Buffer));
    File = fopen(Buffer, "w");
    for (new Iter = 0; Iter < Size; Iter++)
        fprintf(File, "%a", ArrayGetStringHandle(Lines, Iter));
    fclose(File);
    ArrayDestroy(Lines);
    log_to_file(logFile, "Disabling '%s' plugin in order to run this plugin properly!", Name);
    return true;
}

#if !defined replace_stringex
strTransformEx(Buffer[], const Key[], Skip, Chars, bool: lowerCase)
{
    static Pos, Iter;
    Pos = containi(Buffer, Key);
    if (Pos > -1)
        for (Iter = Skip + Pos; Iter < Pos + Skip + Chars; Iter++)
            Buffer[Iter] = lowerCase ? char_to_lower(Buffer[Iter]) : char_to_upper(Buffer[Iter]);
}
#endif

#if !defined char_to_upper || !defined char_to_lower || !defined MAX_STRING_LENGTH
char_to_upper(c)
{
	if (c >= 'a' && c <= 'z')
		return (c & ~(1 << 5));
	return c;
}

char_to_lower(c)
{
	if (c >= 'A' && c <= 'Z')
		return (c | (1 << 5));
	return c;
}
#endif
/**
 * ---------
 */
