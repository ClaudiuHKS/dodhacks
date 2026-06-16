
#pragma ctrlchar '\' /// Replace '^' with '\'.

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#tryinclude <dodhacks>          /** When players type "nextmap" into their console, show them the correct result. Works without this too. */
#tryinclude <dod_hacks_suicide> /// Works without this as well.

#if !defined MEXIT_FORCE || !defined SortADTArray || !defined MPROP_PAGE_CALLBACK
#error AMX Mod X version too old to handle dod_hacks_mapchooser plugin! Consider upgrading! ('SortADTArray()', 'MEXIT_FORCE' & 'MPROP_PAGE_CALLBACK' are needed ...)
#endif

#if defined _dodhacks_included /** When players type "nextmap" into their console, show them the correct result. */
    #define CDoDTeamPlay_sNextMap_32Cells   488 /** 'char CDoDTeamPlay::sNextMap[32]' variable. */
#endif

#if !defined _dod_hacks_suicide_included /// Works without this too.
native bool: DoD_AreUsersBlockedFromSuicide();
#endif

#define AUDIO_DURATION                      4.0 /** "Get ready to choose a map." audio phrase duration in sec. */
#define INTERMISSION_TRIGGER                0.1 /** Enough 'mp_timelimit' min. to trigger a map change when mp_timelimit is set to 0. */

#if !defined SVC_INTERMISSION
    #define  SVC_INTERMISSION                30 /** Scores table is being displayed to players now (before map change). */
#endif

enum unsigned: MapDecision_T {
    m_Votes,
    m_Name[64],
};

enum unsigned: Map_T {
    m_Nominations,
    m_Name[64],
    Array: m_Nominators,
};

new g_maxPlayers;
new g_loadedMaps = 0;
new g_knownDecisionVotes = 0;
new g_secBeforeVoteToAnnounce;
new g_playersDuringEndOfMapVote;
new g_mapsInMenuExcludingExtension;
new g_blockManualChangingAmxNextMap;
new g_flagsRockRequiredAccess;
new g_flagsNominationRequiredAccess;
new g_hudSyncHandle;
new g_hudDisplaySecondsRemaining;
new g_menuDecision;
new g_menuDecisionGrey;
new g_menuNominations;
new g_hookCmdStart;
new g_hookClientKill;
new g_hookClientCommand;
new g_hookPlayerPreThink;
new g_hookPlayerPostThink;
new g_hookUpdateClientData;
new g_hookCmdStart_Post;
new g_hookPlayerPreThink_Post;
new g_hookPlayerPostThink_Post;
new g_hookUpdateClientData_Post;
#if defined hook_cvar_change
new cvarhook: g_hookAmxNextMapConVarChange = cvarhook: -1;
new cvarhook: g_hookMpTimeLimitConVarChange = cvarhook: -1;
#endif
new g_cvarAmxNextMap = 0;
new g_cvarMultiPlayerTimeLimit = 0;
new g_wordToBeDecided[32];
new g_wordActualMapExtensionSuffix[32];
new g_nominationsMenuPlayerPage[33];
new g_playerSteam[33][32];
new g_actualMapName[64];
new bool: g_hudStyle;
new bool: g_hudMsgTimer;
new bool: g_hudMsgDecision;
new bool: g_hudMsgNomination;
new bool: g_featureNominations;
new bool: g_featureRockDecision;
new bool: g_exclVoteMapResource;
new bool: g_exclVoteMapSturmBot;
new bool: g_exclVoteMapShrikeBot;
new bool: g_exclVoteMapMarineBotWpt;
new bool: g_exclVoteMapMarineBotPth;
new bool: g_exclVoteMapOverviewBmp;
new bool: g_exclVoteMapOverviewTxt;
new bool: g_hideChatCommands;
new bool: g_greyDecisionMenu;
new bool: g_nominateByChatMapName;
new bool: g_skipActualMapExclExtension;
new bool: g_changeOnlyAfterActualRound;
new bool: g_blockManualChangingTimeLimit;
new bool: g_loadMpTimeLimitFromServerCfg;
new bool: g_badAreUsersBlockedFromSuicide;
new bool: g_isInServer[33];
new bool: g_isFakePlayer[33];
new bool: g_isAuthenticated[33];
new bool: g_shouldBeNoticed[33];
new bool: g_hasRockedTheDecision[33];
new Float: g_hudHorPos;
new Float: g_hudVerPos;
new Float: g_decisionDuration;
new Float: g_decisionStartTime = 0.0;
new Float: g_decisionGreyDuration;
new Float: g_mapMinimumTimeLimit;
new Float: g_mapMaximumTimeLimit;
new Float: g_mapNeededSecondsRemaining;
new Float: g_actualMapExtensionMinutes;
new Float: g_establishedDefaultMapMinutes;
new Float: g_rockTheDecisionMapStartDelay;
new Float: g_rockTheDecisionMinimumPercent;
new Array: g_arrayMaps;
new Array: g_arrayMapsDecision;

public plugin_init()
{
    register_plugin("DoD Hacks: Map Chooser", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    if (safeDisablePluginIfRunning("DoD.Hacks.MapChooser.LOG", "mapchooser.amxx") ||
        safeDisablePluginIfRunning("DoD.Hacks.MapChooser.LOG", "galileo.amxx"))
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
        return PLUGIN_HANDLED;
    }
    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_mapchooser.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[32], desiredMapsExcludingExtension, cycleType;
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key, "@mapchooser_mapcycle"))
            cycleType = clamp(str_to_num(Val), 0, 2);
        else if (equali(Key, "@mapchooser_extstep"))
            g_actualMapExtensionMinutes = floatmax(1.0, str_to_float(Val));
        else if (equali(Key, "@mapchooser_seconds"))
            g_mapNeededSecondsRemaining = floatmax(60.0, str_to_float(Val));
        else if (equali(Key, "@mapchooser_tbdname"))
            copy(g_wordToBeDecided, charsmax(g_wordToBeDecided), Val[0] ? Val : "[to be decided]");
        else if (equali(Key, "@mapchooser_duration"))
            g_decisionDuration = floatclamp(str_to_float(Val), 10.0, 35.0);
        else if (equali(Key, "@mapchooser_greymenuduration"))
            g_decisionGreyDuration = floatclamp(str_to_float(Val), 1.0, 5.0);
        else if (equali(Key, "@mapchooser_extsuffix"))
            copy(g_wordActualMapExtensionSuffix, charsmax(g_wordActualMapExtensionSuffix), Val);
        else if (equali(Key, "@mapchooser_nomflags"))
            g_flagsNominationRequiredAccess = read_flags(Val);
        else if (equali(Key, "@mapchooser_rockflags"))
            g_flagsRockRequiredAccess = read_flags(Val);
        else if (equali(Key, "@mapchooser_secannounce"))
            g_secBeforeVoteToAnnounce = max(str_to_num(Val), 5);
        else if (equali(Key, "@mapchooser_desiredmaps"))
            desiredMapsExcludingExtension = clamp(str_to_num(Val), 0, 8);
        else if (equali(Key, "@mapchooser_skipactual"))
            g_skipActualMapExclExtension = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_hudmsgtimer"))
            g_hudMsgTimer = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_hudmsgdecision"))
            g_hudMsgDecision = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_hudmsgnomination"))
            g_hudMsgNomination = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_hudstyle"))
            g_hudStyle = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_hidechatcmds"))
            g_hideChatCommands = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_rockthevote"))
            g_featureRockDecision = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_greymenu"))
            g_greyDecisionMenu = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_nominations"))
            g_featureNominations = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_mapbychat"))
            g_nominateByChatMapName = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_onlyafter"))
            g_changeOnlyAfterActualRound = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_exclres"))
            g_exclVoteMapResource = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_exclshrike"))
            g_exclVoteMapShrikeBot = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_exclmarinewpt"))
            g_exclVoteMapMarineBotWpt = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_exclmarinepth"))
            g_exclVoteMapMarineBotPth = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_exclsturm"))
            g_exclVoteMapSturmBot = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_exclovbmp"))
            g_exclVoteMapOverviewBmp = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_exclovtxt"))
            g_exclVoteMapOverviewTxt = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_hudverpos"))
            g_hudVerPos = str_to_float(Val);
        else if (equali(Key, "@mapchooser_hudhorpos"))
            g_hudHorPos = str_to_float(Val);
        else if (equali(Key, "@mapchooser_rockmapdelay"))
            g_rockTheDecisionMapStartDelay = floatmax(60.0, str_to_float(Val));
        else if (equali(Key, "@mapchooser_rockpercent"))
            g_rockTheDecisionMinimumPercent = str_to_float(Val);
        else if (equali(Key, "@mapchooser_blocknextmap"))
            g_blockManualChangingAmxNextMap = clamp(str_to_num(Val), 0, 2);
        else if (equali(Key, "@mapchooser_blocktimelimit"))
            g_blockManualChangingTimeLimit = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_loadsvcfg"))
            g_loadMpTimeLimitFromServerCfg = bool: str_to_num(Val);
        else if (equali(Key, "@mapchooser_mintimelimit"))
            g_mapMinimumTimeLimit = str_to_float(Val);
        else if (equali(Key, "@mapchooser_maxtimelimit"))
            g_mapMaximumTimeLimit = str_to_float(Val);
    } /// Cfg. ready.
    fclose(Config); /// Get stuff ready before maps are loaded.
    g_arrayMaps = ArrayCreate(Map_T);
    g_menuNominations = menu_create("Nominate A Map", "OnMenuItem");
    menu_setprop(g_menuNominations, MPROP_PAGE_CALLBACK, "OnMenuPage");
    get_mapname(g_actualMapName, charsmax(g_actualMapName));
    switch (cycleType)
    { /// Load the maps.
        case 0:
        { /// Load from "maps" folder.
            new Map[Map_T], Len, Dir = open_dir("maps", Map[m_Name], charsmax(Map[m_Name]));
            do { /// Receive all available maps.
                Len = strlen(Map[m_Name]);
                if (Len < 5 || !equali(Map[m_Name][Len - 4], ".bsp") ||
                    (g_skipActualMapExclExtension && equali(Map[m_Name], g_actualMapName, Len - 4)))
                    continue;
                Map[m_Name][Len - 4] = EOS;
                if (!mapPass(Map[m_Name]))
                    continue;
                Map[m_Nominations] = 0;
                Map[m_Nominators] = ArrayCreate(32);
                ArrayPushArray(g_arrayMaps, Map);
                menu_additem(g_menuNominations, Map[m_Name]);
            } while (next_file(Dir, Map[m_Name], charsmax(Map[m_Name])));
            close_dir(Dir);
        }
        case 1:
        { /// Load from mapcycle text file.
            get_cvar_string("mapcyclefile", Buffer, charsmax(Buffer));
            Config = fopen(Buffer, "r");
            if (!Config)
            {
                Config = fopen("mapcycle.txt", "r");
                if (!Config)
                    goto mapsLoaded;
            }
            new Map[Map_T];
            while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
            {
                trim(Buffer);
                if (!Buffer[0] || (g_skipActualMapExclExtension && equali(Buffer, g_actualMapName)) ||
                    !is_map_valid(Buffer) || !mapPass(Buffer))
                    continue;
                Map[m_Nominations] = 0;
                copy(Map[m_Name], charsmax(Map[m_Name]), Buffer);
                Map[m_Nominators] = ArrayCreate(32);
                ArrayPushArray(g_arrayMaps, Map);
                menu_additem(g_menuNominations, Map[m_Name]);
            }
            fclose(Config);
        }
        default:
        { /// Load from AMX Mod X maps text file.
            get_configsdir(Buffer, charsmax(Buffer));
            add(Buffer, charsmax(Buffer), "/maps.ini");
            Config = fopen(Buffer, "r");
            if (!Config)
                goto mapsLoaded;
            new Map[Map_T];
            while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
            {
                trim(Buffer);
                if (!Buffer[0] || Buffer[0] == ';' ||
                    (g_skipActualMapExclExtension && equali(Buffer, g_actualMapName)) ||
                    !is_map_valid(Buffer) || !mapPass(Buffer))
                    continue;
                Map[m_Nominations] = 0;
                copy(Map[m_Name], charsmax(Map[m_Name]), Buffer);
                Map[m_Nominators] = ArrayCreate(32);
                ArrayPushArray(g_arrayMaps, Map);
                menu_additem(g_menuNominations, Map[m_Name]);
            }
            fclose(Config);
        }
    }
mapsLoaded:
    g_maxPlayers = get_maxplayers();
    if (g_hudMsgTimer || g_hudMsgDecision || g_hudMsgNomination)
        g_hudSyncHandle = CreateHudSyncObj();
    g_arrayMapsDecision = ArrayCreate(MapDecision_T);
    g_loadedMaps = ArraySize(g_arrayMaps);
    g_mapsInMenuExcludingExtension = min(desiredMapsExcludingExtension, g_loadedMaps);
    g_cvarMultiPlayerTimeLimit = get_cvar_pointer("mp_timelimit");
    g_cvarAmxNextMap = get_cvar_pointer("amx_nextmap");
    if (g_changeOnlyAfterActualRound)
        register_event("HLTV", "OnNewRoundInitialization", "a", "1=0", "2=0");
    if (g_featureNominations) /** "maps" is a reserved word. It's already a registered console command. */
        register_clcmd("nominate", "OnPlayerClConCmd_Nominate", ADMIN_ALL, "- shows a list of maps to nominate from");
    if (g_featureRockDecision)
    {
        register_clcmd("rtv", "OnPlayerClConCmd_RockDecision", ADMIN_ALL, "- helps you start the end-of-map vote sooner");
        register_clcmd("rockthevote", "OnPlayerClConCmd_RockDecision", ADMIN_ALL, "- helps you start the end-of-map vote sooner");
    }
    set_task(1.0, "Task_BeginDecisionProcess", .flags = "b");
    set_task(0.000001, "Task_OnPluginCfg"); /// plugin_cfg() wouldn't work properly.
    register_message(SVC_INTERMISSION /** 30 */, "OnMsgIntermission");
    return PLUGIN_CONTINUE;
}

public plugin_natives()
{
    set_native_filter("nativesFilter");
    if (is_plugin_loaded("Nextmap Chooser"))
        pause("cd", "mapchooser.amxx");
    else if (is_plugin_loaded("Galileo"))
        pause("cd", "galileo.amxx");
}

public nativesFilter(const Native[], Index, bool: Found)
{
    if (('D' == Native[0] || Native[0] == 'd') && equali(Native, "DoD_AreUsersBlockedFromSuicide"))
        switch (Found)
        {
            case true:
                g_badAreUsersBlockedFromSuicide = false;
            default:
            {
                g_badAreUsersBlockedFromSuicide = true;
                return PLUGIN_HANDLED;
            }
        }
    return PLUGIN_CONTINUE;
}

public OnMsgIntermission()
    set_task(0.000001, "Task_AnnounceNewMap");

public Task_AnnounceNewMap()
{
    new Map[64];
    get_pcvar_string(g_cvarAmxNextMap, Map, charsmax(Map));
    client_print(0, print_chat, "* Changing %s with %s (%d VOTE%s or %0.0f%%).",
        g_actualMapName, Map, g_knownDecisionVotes, 1 == g_knownDecisionVotes ? "" : "S", votePercent(g_knownDecisionVotes));
}

public Task_OnPluginCfg()
{ /// Ensure cvars are properly managed.
    set_pcvar_string(g_cvarAmxNextMap, g_wordToBeDecided); /// Ensure there is no valid next map now.
#if defined _dodhacks_included /** When players type "nextmap" into their console, show them the correct result. */
    safeUpdateGameRulesNextMap_DoD(g_wordToBeDecided);
#endif
    g_establishedDefaultMapMinutes = get_pcvar_float(g_cvarMultiPlayerTimeLimit); /// Get default map time limit in minutes.
#if defined hook_cvar_change
    g_hookAmxNextMapConVarChange = hook_cvar_change(g_cvarAmxNextMap, "OnConVarChanged");
    g_hookMpTimeLimitConVarChange = hook_cvar_change(g_cvarMultiPlayerTimeLimit, "OnConVarChanged");
#endif
}

public plugin_end()
{
    new Map[Map_T];
    for (new Iter = g_loadedMaps - 1; Iter > -1; Iter--)
    { /// Ensure all allocated memory is properly freed.
        ArrayGetArray(g_arrayMaps, Iter, Map);
        ArrayDestroy(Map[m_Nominators]);
    } /// AMX Mod X will surely free the remaining stuff (mainly arrays and menus).
#if defined hook_cvar_change
    if (cvarhook: -1 != g_hookMpTimeLimitConVarChange)
        disable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
    if (g_loadMpTimeLimitFromServerCfg)
    {
        new Float: Mins;
        if (tryLoadMapMinutesFromServerCfg(Mins))
            g_establishedDefaultMapMinutes = Mins;
    }
    if (g_cvarMultiPlayerTimeLimit)
        set_pcvar_float(g_cvarMultiPlayerTimeLimit, g_establishedDefaultMapMinutes); /// Restore default map time limit in minutes.
#if defined hook_cvar_change
    if (cvarhook: -1 != g_hookMpTimeLimitConVarChange)
        enable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
    zeroPlayerVars(Player);

public client_connect(Player)
    zeroPlayerVars(Player);

public client_putinserver(Player)
{
    g_isInServer[Player] = true;
    g_isFakePlayer[Player] = bool: is_user_bot(Player);
    g_shouldBeNoticed[Player] = false == g_isFakePlayer[Player] && g_decisionStartTime &&
        get_gametime() - g_decisionStartTime < g_decisionDuration + AUDIO_DURATION;
    g_nominationsMenuPlayerPage[Player] = 0;
}

#if defined client_connectex
public client_authorized(Player, const Steam[])
#else
public client_authorized(Player)
#endif
{
#if !defined client_connectex
    static Steam[32];
    get_user_authid(Player, Steam, charsmax(Steam));
#endif
    g_isAuthenticated[Player] = true;
    copy(g_playerSteam[Player], charsmax(g_playerSteam[]), Steam);
}

public DoD_OnPlayerSpawn_Post(DoD_Address: CDoDTeamPlay, Player)
{
    if (g_shouldBeNoticed[Player])
    {
        if (get_gametime() - g_decisionStartTime < g_decisionDuration + AUDIO_DURATION)
            client_print(Player, print_chat, "* A map vote is currently ongoing.");
        g_shouldBeNoticed[Player] = false;
    }
}

public OnNewRoundInitialization()
{ /// Change the map right now if needed.
    if (hasDecided() && get_pcvar_float(g_cvarMultiPlayerTimeLimit) <= 0.0)
    {
#if defined hook_cvar_change
        disable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
        set_pcvar_float(g_cvarMultiPlayerTimeLimit, INTERMISSION_TRIGGER /** Enough to trigger the intermission. */);
#if defined hook_cvar_change
        enable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
    }
}

#if defined hook_cvar_change
public OnConVarChanged(conVarHandle, const oldValue[], const newValue[])
{
    if (!equal(oldValue, newValue))
        switch (conVarHandle == g_cvarAmxNextMap)
        {
            case true:
                if (!newValue[0] || 1 == g_blockManualChangingAmxNextMap ||
                    (2 == g_blockManualChangingAmxNextMap && false == hasDecided()) || !is_map_valid(newValue))
                {
                    disable_cvar_hook(g_hookAmxNextMapConVarChange);
                    set_pcvar_string(g_cvarAmxNextMap, oldValue);
                    enable_cvar_hook(g_hookAmxNextMapConVarChange);
                }
#if defined _dodhacks_included /** When players type "nextmap" into their console, show them the correct result. */
                else
                    safeUpdateGameRulesNextMap_DoD(newValue);
#endif
            default:
                if (g_blockManualChangingTimeLimit)
                {
                    disable_cvar_hook(g_hookMpTimeLimitConVarChange);
                    set_pcvar_string(g_cvarMultiPlayerTimeLimit, oldValue);
                    enable_cvar_hook(g_hookMpTimeLimitConVarChange);
                }
                else
                    g_establishedDefaultMapMinutes = floatclamp(str_to_float(newValue), g_mapMinimumTimeLimit, g_mapMaximumTimeLimit);
        }
}
#endif

public OnMenuPage(Player, Status)
{
    switch (Status)
    {
        case MENU_BACK:
            if (g_nominationsMenuPlayerPage[Player] > 0)
                --g_nominationsMenuPlayerPage[Player];
        case MENU_MORE:
            ++g_nominationsMenuPlayerPage[Player];
        case MENU_EXIT:
            g_nominationsMenuPlayerPage[Player] = 0;
    }
}

public OnPlayerClConCmd_Nominate(Player)
{
    static Map[64], Rocked, Float: Perc;
    if (g_isInServer[Player] && false == g_isFakePlayer[Player])
    {
        if (false == g_isAuthenticated[Player])
        { /// No Steam received yet.
            client_print(Player, print_chat, "* Awaiting Steam authentication to nominate or denominate maps.");
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (g_flagsNominationRequiredAccess != (get_user_flags(Player) & g_flagsNominationRequiredAccess))
        { /// No access for the command.
            client_print(Player, print_chat, "* No access to nominate or denominate maps.");
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (inDecision())
        { /// Already deciding.
            client_print(Player, print_chat, "* Server is already deciding, can't nominate or denominate now.");
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (hasDecided())
        { /// Already decided.
            get_pcvar_string(g_cvarAmxNextMap, Map, charsmax(Map));
            client_print(Player, print_chat, "* Server has already decided: %s (%d VOTE%s or %0.0f%%).",
                Map, g_knownDecisionVotes, g_knownDecisionVotes == 1 ? "" : "S", votePercent(g_knownDecisionVotes));
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (rockedTheDecision(Rocked, Perc))
        { /// Vote already rocked.
            client_print(Player, print_chat, "* Vote has already been rocked (%d PLAYER%s or %0.0f%%).",
                Rocked, 1 == Rocked ? "" : "S", Perc);
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (!isViewingAMenu(Player))
            showMenuNominations(Player, true);
        else
            client_print(Player, print_chat, "* You're already viewing a menu.");
    } /// Block showing "unknown command".
    return PLUGIN_HANDLED;
}

public OnPlayerClConCmd_RockDecision(Player)
{
    static Map[64], Rocked, Float: Time, Float: Perc;
    if (g_isInServer[Player] && false == g_isFakePlayer[Player])
    {
        Time = get_gametime();
        if (Time < g_rockTheDecisionMapStartDelay)
        {
            client_print(Player, print_chat, "* Rocking the vote is possible after %.1f sec.",
                g_rockTheDecisionMapStartDelay - Time);
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (g_flagsRockRequiredAccess != (get_user_flags(Player) & g_flagsRockRequiredAccess))
        { /// No access for the command.
            client_print(Player, print_chat, "* No access to rock the vote.");
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (inDecision())
        { /// Already deciding.
            client_print(Player, print_chat, "* Server is already deciding, can't rock the vote.");
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (hasDecided())
        { /// Already decided.
            get_pcvar_string(g_cvarAmxNextMap, Map, charsmax(Map));
            client_print(Player, print_chat, "* Server has already decided: %s (%d VOTE%s or %0.0f%%).",
                Map, g_knownDecisionVotes, g_knownDecisionVotes == 1 ? "" : "S", votePercent(g_knownDecisionVotes));
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        if (rockedTheDecision(Rocked, Perc))
        { /// Vote already rocked.
            client_print(Player, print_chat, "* Vote has already been rocked (%d PLAYER%s or %0.0f%%).",
                Rocked, 1 == Rocked ? "" : "S", Perc);
            return PLUGIN_HANDLED; /// Block showing "unknown command".
        }
        g_hasRockedTheDecision[Player] = !g_hasRockedTheDecision[Player];
        switch (g_hasRockedTheDecision[Player])
        {
            case true:
                client_print(Player, print_chat, "* You've rocked the vote.");
            default:
                client_print(Player, print_chat, "* You've taken back your vote rock.");
        }
    } /// Block showing "unknown command".
    return PLUGIN_HANDLED;
}

public Task_BeginDecisionProcess()
{
    static bool: Rocked, Float: Time, Players, Float: Perc;
    if (g_featureRockDecision)
    {
        if (inDecision() || hasDecided())
            return PLUGIN_CONTINUE;
        if (false == canDecide())
        {
            Rocked = rockedTheDecision(Players, Perc);
            if (false == Rocked)
                return PLUGIN_CONTINUE;
            Time = get_gametime();
#if defined hook_cvar_change
            disable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
            set_pcvar_float(g_cvarMultiPlayerTimeLimit, (Time + g_mapNeededSecondsRemaining) / 60.0);
#if defined hook_cvar_change
            enable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
            g_decisionStartTime = Time;
        }
        else
            g_decisionStartTime = get_gametime();
    }
    else
    {
        if (inDecision() || false == canDecide() || hasDecided())
            return PLUGIN_CONTINUE;
        Rocked = false;
        g_decisionStartTime = get_gametime();
    }
    g_playersDuringEndOfMapVote = 0;
    buildMenu();
    switch (Rocked)
    {
        case false: /// To ensure everyone sees "**** GET READY TO CHOOSE A MAP ****".
            client_print(0, print_chat, "\x1"); /// An empty chat line to ensure everyone sees the message below.
        default:
            client_print(0, print_chat, "* Vote has been rocked (%d PLAYER%s or %0.0f%%).",
                Players, 1 == Players ? "" : "S", Perc);
    }
    client_print(0, print_chat, "**** GET READY TO CHOOSE A MAP ****");
    if (g_hudMsgDecision)
    {
        set_hudmessage(20 /** Red. */, 200 /** Green. */, 40 /** Blue. */,
            g_hudHorPos /** Horizontal position. */, g_hudVerPos /** Vertical position. */,
            g_hudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
            AUDIO_DURATION /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
        ShowSyncHudMsg(0, g_hudSyncHandle, "GET READY TO VOTE!");
    }
    /**
     * -------------------------------------
     * Galileo 1.1.290 by Brad (2009-FEB-26)
     * -------------------------------------
     * https://forums.alliedmods.net/showthread.php?t=77391
     * https://forums.alliedmods.net/member.php?u=1106
     */
    client_cmd(0, "spk \"get red(e80) ninety(s45) to check(e20) use bay(s18) mass(e42) cap(s50)\"");
    g_hookCmdStart              = register_forward(FM_CmdStart,         "OnCmdStart_Pre");
    g_hookClientKill            = register_forward(FM_ClientKill,       "OnClientKill_Pre");
    g_hookClientCommand         = register_forward(FM_ClientCommand,    "OnClientCommand_Pre");
    g_hookUpdateClientData      = register_forward(FM_UpdateClientData, "OnUpdateClientData_Pre");
    g_hookPlayerPreThink        = register_forward(FM_PlayerPreThink,   "OnPlayerPreAndPostThink_Pre");
    g_hookPlayerPostThink       = register_forward(FM_PlayerPostThink,  "OnPlayerPreAndPostThink_Pre");
    g_hookCmdStart_Post         = register_forward(FM_CmdStart,         "OnCmdStart_Post",              true);
    g_hookPlayerPreThink_Post   = register_forward(FM_PlayerPreThink,   "OnPlayerPreAndPostThink_Post", true);
    g_hookPlayerPostThink_Post  = register_forward(FM_PlayerPostThink,  "OnPlayerPreAndPostThink_Post", true);
    g_hookUpdateClientData_Post = register_forward(FM_UpdateClientData, "OnUpdateClientData_Post",      true);
    updatePlayers(true); /// Stop alive players from moving.
    set_task(AUDIO_DURATION, g_greyDecisionMenu ? "Task_DisplayDecisionGreyMenu" : "Task_DisplayDecisionNormalMenu");
    return PLUGIN_CONTINUE;
}

public Task_ShowDecisionProgress()
{
    set_hudmessage(20 /** Red. */, 180 /** Green. */, 200 /** Blue. */,
        g_hudHorPos /** Horizontal position. */, g_hudVerPos /** Vertical position. */,
        g_hudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
        1.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
    ShowSyncHudMsg(0, g_hudSyncHandle, "%-2d SEC. LEFT!", g_hudDisplaySecondsRemaining--);
}

public Task_DisplayDecisionGreyMenu()
{
    static Player;
    set_task(g_decisionDuration, "Task_FinishDecision");
    set_task(g_decisionGreyDuration, "Task_DisplayDecisionMenu");
    if (g_hudMsgTimer)
    {
        g_hudDisplaySecondsRemaining = floatround(g_decisionDuration, floatround_tozero /** Truncate. */);
        set_hudmessage(20 /** Red. */, 180 /** Green. */, 200 /** Blue. */,
            g_hudHorPos /** Horizontal position. */, g_hudVerPos /** Vertical position. */,
            g_hudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
            1.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
        ShowSyncHudMsg(0, g_hudSyncHandle, "%-2d SEC. LEFT!", g_hudDisplaySecondsRemaining--);
        set_task(1.0, "Task_ShowDecisionProgress", 0, "", 0, "a", g_hudDisplaySecondsRemaining);
    }
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (g_isInServer[Player])
        {
            if (false == g_isFakePlayer[Player])
                menu_display(Player, g_menuDecisionGrey);
            updatePlayer(Player, true); /// Stop alive players from moving.
        }
}

public Task_DisplayDecisionNormalMenu()
{
    static Player;
    set_task(g_decisionDuration, "Task_FinishDecision");
    if (g_hudMsgTimer)
    {
        g_hudDisplaySecondsRemaining = floatround(g_decisionDuration, floatround_tozero /** Truncate. */);
        set_hudmessage(20 /** Red. */, 180 /** Green. */, 200 /** Blue. */,
            g_hudHorPos /** Horizontal position. */, g_hudVerPos /** Vertical position. */,
            g_hudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
            1.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
        ShowSyncHudMsg(0, g_hudSyncHandle, "%-2d SEC. LEFT!", g_hudDisplaySecondsRemaining--);
        set_task(1.0, "Task_ShowDecisionProgress", 0, "", 0, "a", g_hudDisplaySecondsRemaining);
    }
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (g_isInServer[Player])
        {
            if (false == g_isFakePlayer[Player] && ++g_playersDuringEndOfMapVote)
            {
                client_cmd(Player, "spk gman/gman_choose%d", random_num(1, 2));
                menu_display(Player, g_menuDecision);
            }
            updatePlayer(Player, true); /// Stop alive players from moving.
        }
}

public Task_DisplayDecisionMenu()
{
    static Player;
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (g_isInServer[Player])
        {
            if (false == g_isFakePlayer[Player] && ++g_playersDuringEndOfMapVote)
            {
                client_cmd(Player, "spk gman/gman_choose%d", random_num(1, 2));
                menu_display(Player, g_menuDecision);
            }
            updatePlayer(Player, true); /// Stop alive players from moving.
        }
}

public OnUpdateClientData_Pre(Player, sendWeapons, Pack) /// Only executed while voting. Executed for 'void UpdateClientData()'.
{ /// During the vote, alive players are not allowed to move, send commands or attack another players.
    if (g_isInServer[Player])
    {
        updatePlayer(Player, true);
        if (Pack && is_user_alive(Player))
            set_cd(Pack, CD_Velocity, Float: { 0.0, 0.0, 0.0 }); /// Ensure player not moving.
    }
}

public OnUpdateClientData_Post(Player, sendWeapons, Pack) /// Only executed while voting. Executed for 'void UpdateClientData()'.
{ /// During the vote, alive players are not allowed to move, send commands or attack another players.
    if (g_isInServer[Player])
    {
        updatePlayer(Player, true);
        if (Pack && is_user_alive(Player))
            set_cd(Pack, CD_Velocity, Float: { 0.0, 0.0, 0.0 }); /// Ensure player not moving.
    }
}

public OnClientCommand_Pre(Player) /// Only executed while voting. Executed for 'void ClientCommand()'.
{ /// During the vote, alive players can't send cmds. (including string cmds.) or kill themselves by using the 'kill' con. cmd.
    if (g_isInServer[Player])
    {
        updatePlayer(Player, true);
        if (is_user_alive(Player))
            return FMRES_SUPERCEDE; /// Block processing player commands.
    }
    return FMRES_IGNORED;
}

public OnClientKill_Pre(Player) /// Only executed while voting. Executed for 'void ClientKill()'.
{ /// During the vote, alive players can't send cmds. (including string cmds.) or kill themselves by using the 'kill' con. cmd.
    if (g_isInServer[Player])
    {
        updatePlayer(Player, true);
        if (is_user_alive(Player))
            return FMRES_SUPERCEDE; /// Block processing player commands.
    }
    return !g_badAreUsersBlockedFromSuicide && DoD_AreUsersBlockedFromSuicide() ? FMRES_SUPERCEDE : FMRES_IGNORED;
}

public OnCmdStart_Pre(Player, Pack) /// Only executed while voting. Executed for 'void CmdStart()'.
{ /// During the vote, alive players are not allowed to move, send commands or attack another players.
    if (g_isInServer[Player])
    {
        updatePlayer(Player, true);
        if (Pack && is_user_alive(Player))
        {
            set_uc(Pack, UC_Buttons, 0); /// Ensure player not pressing a key.
            set_uc(Pack, UC_Impulse, 0); /// Ensure player not using an impulse.
            set_uc(Pack, UC_ForwardMove, 0.0); /// Ensure player not moving.
            set_uc(Pack, UC_SideMove,    0.0);
            set_uc(Pack, UC_UpMove,      0.0);
        }
    }
}

public OnCmdStart_Post(Player, Pack) /// Only executed while voting. Executed for 'void CmdStart()'.
{ /// During the vote, alive players are not allowed to move, send commands or attack another players.
    if (g_isInServer[Player])
    {
        updatePlayer(Player, true);
        if (Pack && is_user_alive(Player))
        {
            set_uc(Pack, UC_Buttons, 0); /// Ensure player not pressing a key.
            set_uc(Pack, UC_Impulse, 0); /// Ensure player not using an impulse.
            set_uc(Pack, UC_ForwardMove, 0.0); /// Ensure player not moving.
            set_uc(Pack, UC_SideMove,    0.0);
            set_uc(Pack, UC_UpMove,      0.0);
        }
    }
}

public OnPlayerPreAndPostThink_Pre(Player) /// Only executed while voting. Executed for both 'void PlayerPreThink()' and 'void PlayerPostThink()'.
    if (g_isInServer[Player]) /// During the vote, alive players are not allowed to move or attack another players.
        updatePlayer(Player, true);

public OnPlayerPreAndPostThink_Post(Player) /// Only executed while voting. Executed for both 'void PlayerPreThink()' and 'void PlayerPostThink()'.
    if (g_isInServer[Player]) /// During the vote, alive players are not allowed to move or attack another players.
        updatePlayer(Player, true);

public client_command(Player)
{
    static Buffer[64], Map[Map_T], Len, Rocked, Iter, Float: Time, Float: Perc, bool: Continue;
    if (g_isFakePlayer[Player])
        return PLUGIN_CONTINUE; /// Skip, as a fake player is executing this command.
    Len = read_argv(0, Buffer, charsmax(Buffer));
    if (Len < 3) /// "say" cmd.
        return PLUGIN_CONTINUE; /// Skip.
    if (parseFilterConsolePlayerCommand(Player, Len, Buffer, Continue)) /// Filters amx_cvar and amx_rcon commands.
        return PLUGIN_HANDLED; /// Block, with details (details, only if put in server).
    if (!Continue || false == g_isInServer[Player])
        return PLUGIN_CONTINUE; /// Skip.
    if (equali(Buffer, "say") || equali(Buffer, "say_team"))
    {
        Len = read_argv(1, Buffer, charsmax(Buffer));
        if (g_featureRockDecision && Len > 2 /** "rtv" chat cmd. */ && (equali(Buffer, "rtv") || equali(Buffer, "rockthevote") ||
            ((Buffer[0] == ',' || Buffer[0] == '.' || Buffer[0] == '/' || Buffer[0] == '!') &&
            (equali(Buffer[1], "rtv") || equali(Buffer[1], "rockthevote")))))
        { /// Rock the vote feature.
            Time = get_gametime();
            if (Time < g_rockTheDecisionMapStartDelay)
            {
                client_print(Player, print_chat, "* Rocking the vote is possible after %.1f sec.",
                    g_rockTheDecisionMapStartDelay - Time);
                return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
            }
            if (g_flagsRockRequiredAccess != (get_user_flags(Player) & g_flagsRockRequiredAccess))
            { /// No access for the command.
                client_print(Player, print_chat, "* No access to rock the vote.");
                return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
            }
            if (inDecision())
            { /// Already deciding.
                client_print(Player, print_chat, "* Server is already deciding, can't rock the vote.");
                return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
            }
            if (hasDecided())
            { /// Already decided.
                get_pcvar_string(g_cvarAmxNextMap, Buffer, charsmax(Buffer));
                client_print(Player, print_chat, "* Server has already decided: %s (%d VOTE%s or %0.0f%%).",
                    Buffer, g_knownDecisionVotes, g_knownDecisionVotes == 1 ? "" : "S", votePercent(g_knownDecisionVotes));
                return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
            }
            if (rockedTheDecision(Rocked, Perc))
            { /// Vote already rocked.
                client_print(Player, print_chat, "* Vote has already been rocked (%d PLAYER%s or %0.0f%%).",
                    Rocked, 1 == Rocked ? "" : "S", Perc);
                return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
            }
            g_hasRockedTheDecision[Player] = !g_hasRockedTheDecision[Player];
            switch (g_hasRockedTheDecision[Player])
            {
                case true:
                    client_print(Player, print_chat, "* You've rocked the vote.");
                default:
                    client_print(Player, print_chat, "* You've taken back your vote rock.");
            }
            if (g_hideChatCommands)
                return PLUGIN_HANDLED;
        }
        else if (g_changeOnlyAfterActualRound && Len > 7 /** "timeleft" chat cmd. */ &&
            get_pcvar_float(g_cvarMultiPlayerTimeLimit) <= INTERMISSION_TRIGGER && equali(Buffer, "timeleft") && hasDecided())
        {
            client_print(Player, print_chat, "* Map will change when round ends.");
            return PLUGIN_HANDLED; /// Block engine's answer which would tell the seconds.
        }
        else if (g_featureNominations)
        { /// m_Nominations feature.
            if (Len > 3 /** "maps"/ "nominate" chat cmd. */ && (equali(Buffer, "nominate") || equali(Buffer, "maps") ||
                ((Buffer[0] == ',' || Buffer[0] == '.' || Buffer[0] == '/' || Buffer[0] == '!') &&
                (equali(Buffer[1], "nominate") || equali(Buffer[1], "maps")))))
            { /// "nominate"/ "maps" chat cmd.
                if (false == g_isAuthenticated[Player])
                { /// No Steam received yet.
                    client_print(Player, print_chat, "* Awaiting Steam authentication to nominate or denominate maps.");
                    return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                }
                if (g_flagsNominationRequiredAccess != (get_user_flags(Player) & g_flagsNominationRequiredAccess))
                { /// No access for the command.
                    client_print(Player, print_chat, "* No access to nominate or denominate maps.");
                    return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                }
                if (inDecision())
                { /// Already deciding.
                    client_print(Player, print_chat, "* Server is already deciding, can't nominate or denominate now.");
                    return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                }
                if (hasDecided())
                { /// Already decided.
                    get_pcvar_string(g_cvarAmxNextMap, Buffer, charsmax(Buffer));
                    client_print(Player, print_chat, "* Server has already decided: %s (%d VOTE%s or %0.0f%%).",
                        Buffer, g_knownDecisionVotes, g_knownDecisionVotes == 1 ? "" : "S", votePercent(g_knownDecisionVotes));
                    return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                }
                if (rockedTheDecision(Rocked, Perc))
                { /// Vote already rocked.
                    client_print(Player, print_chat, "* Vote has already been rocked (%d PLAYER%s or %0.0f%%).",
                        Rocked, 1 == Rocked ? "" : "S", Perc);
                    return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                }
                if (!isViewingAMenu(Player))
                    showMenuNominations(Player, true);
                else
                    client_print(Player, print_chat, "* You're already viewing a menu.");
                if (g_hideChatCommands)
                    return PLUGIN_HANDLED;
            }
            else if (g_nominateByChatMapName && Len > 0)
            { /// "dod_flash" (or other maps) chat cmd.
                if (g_skipActualMapExclExtension && equali(g_actualMapName, Buffer))
                {
                    client_print(Player, print_chat, "* Operator disabled nominating actual map.");
                    return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                }
                for (Iter = 0; Iter < g_loadedMaps; Iter++)
                {
                    ArrayGetArray(g_arrayMaps, Iter, Map);
                    if (!equali(Map[m_Name], Buffer))
                        continue;
                    if (false == g_isAuthenticated[Player])
                    { /// No Steam received yet.
                        client_print(Player, print_chat, "* Awaiting Steam authentication to nominate '%s'.",
                            Map[m_Name]);
                        return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                    }
                    if (g_flagsNominationRequiredAccess != (get_user_flags(Player) & g_flagsNominationRequiredAccess))
                    { /// No access for the command.
                        client_print(Player, print_chat, "* No access to nominate or denominate maps.");
                        return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                    }
                    if (inDecision())
                    { /// Already deciding.
                        client_print(Player, print_chat, "* Server is already deciding, can't nominate or denominate '%s'.",
                            Map[m_Name]);
                        return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                    }
                    if (hasDecided())
                    { /// Already decided.
                        get_pcvar_string(g_cvarAmxNextMap, Buffer, charsmax(Buffer));
                        client_print(Player, print_chat, "* Server decided: %s (%d VOTE%s or %0.0f%%), can't nominate or denominate '%s'.",
                            Buffer, g_knownDecisionVotes, g_knownDecisionVotes == 1 ? "" : "S", votePercent(g_knownDecisionVotes), Map[m_Name]);
                        return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                    }
                    if (rockedTheDecision(Rocked, Perc))
                    { /// Vote already rocked.
                        client_print(Player, print_chat, "* Vote has already been rocked (%d PLAYER%s or %0.0f%%), can't nominate or denominate '%s'.",
                            Rocked, 1 == Rocked ? "" : "S", Perc, Map[m_Name]);
                        return g_hideChatCommands ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
                    }
                    if (!isViewingAMenu(Player))
                    {
                        g_nominationsMenuPlayerPage[Player] = Iter / 7;
                        showMenuNominations(Player, false, g_nominationsMenuPlayerPage[Player]);
                    }
                    else
                        client_print(Player, print_chat, "* You're already viewing a menu, can't nominate or denominate '%s'.",
                            Map[m_Name]);
                    if (g_hideChatCommands)
                        return PLUGIN_HANDLED;
                }
            }
        }
    }
    else if (g_changeOnlyAfterActualRound && Len > 7 /** "timeleft" console cmd. */ &&
        get_pcvar_float(g_cvarMultiPlayerTimeLimit) <= INTERMISSION_TRIGGER && equali(Buffer, "timeleft") && hasDecided())
    {
        client_print(Player, print_chat, "* Map will change when round ends.");
        return PLUGIN_HANDLED; /// Block engine's answer which would tell the seconds.
    }
    return PLUGIN_CONTINUE;
}

public OnMenuDecisionItem(Player, Menu, Item)
{
    static Map[MapDecision_T];
    if (Item > -1)
    {
        ArrayGetArray(g_arrayMapsDecision, Item, Map);
        ++Map[m_Votes];
        switch (Item == g_mapsInMenuExcludingExtension)
        {
            case false: /// Voted a map.
                client_print(Player, print_chat, "* You voted %s.", Map[m_Name]);
            default: /// Voted to extend the actual map.
                client_print(Player, print_chat, "* You voted to extend this map.");
        }
        ArraySetArray(g_arrayMapsDecision, Item, Map);
    }
}

public OnMenuDecisionGreyItem(Player, Menu, Item)
{
    if (Item > -1)
        menu_display(Player, g_menuDecisionGrey);
    return Item > -1 ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
}

public OnMenuItem(Player, Menu, Item)
{
    static Map[Map_T], Next[64], Entry, Rocked, Float: Perc;
    if (Item > -1)
    {
        if (g_flagsNominationRequiredAccess != (get_user_flags(Player) & g_flagsNominationRequiredAccess))
        { /// No more access to nominate/ denominate a map.
            client_print(Player, print_chat, "* No more access to nominate or denominate maps.");
            return PLUGIN_CONTINUE;
        }
        if (inDecision())
        { /// Already deciding.
            client_print(Player, print_chat, "* Server is already deciding, can't nominate or denominate now.");
            return PLUGIN_CONTINUE;
        }
        if (hasDecided())
        { /// Already decided.
            get_pcvar_string(g_cvarAmxNextMap, Next, charsmax(Next));
            client_print(Player, print_chat, "* Server has already decided: %s (%d VOTE%s or %0.0f%%).",
                Next, g_knownDecisionVotes, g_knownDecisionVotes == 1 ? "" : "S", votePercent(g_knownDecisionVotes));
            return PLUGIN_CONTINUE;
        }
        if (rockedTheDecision(Rocked, Perc))
        { /// Vote already rocked.
            client_print(Player, print_chat, "* Vote has already been rocked (%d PLAYER%s or %0.0f%%).",
                Rocked, 1 == Rocked ? "" : "S", Perc);
            return PLUGIN_CONTINUE;
        }
        ArrayGetArray(g_arrayMaps, Item, Map);
        Entry = ArrayFindString(Map[m_Nominators], g_playerSteam[Player]);
        if (Entry > -1)
        { /// Denominate.
            --Map[m_Nominations];
            ArrayDeleteItem(Map[m_Nominators], Entry);
            client_print(Player, print_chat, "* You denominated %s.", Map[m_Name]);
            if (g_hudMsgNomination)
            {
                set_hudmessage(200 /** Red. */, 20 /** Green. */, 20 /** Blue. */,
                    g_hudHorPos /** Horizontal position. */, g_hudVerPos /** Vertical position. */,
                    g_hudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    2.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                ShowSyncHudMsg(Player, g_hudSyncHandle, "%s DENOMINATED!", Map[m_Name]);
            }
        }
        else
        { /// Nominate.
            ++Map[m_Nominations];
            ArrayPushString(Map[m_Nominators], g_playerSteam[Player]);
            client_print(Player, print_chat, "* You nominated %s.", Map[m_Name]);
            if (g_hudMsgNomination)
            {
                set_hudmessage(20 /** Red. */, 200 /** Green. */, 40 /** Blue. */,
                    g_hudHorPos /** Horizontal position. */, g_hudVerPos /** Vertical position. */,
                    g_hudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
                    2.0 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
                ShowSyncHudMsg(Player, g_hudSyncHandle, "%s NOMINATED!", Map[m_Name]);
            }
        }
        ArraySetArray(g_arrayMaps, Item, Map);
        showMenuNominations(Player, false, g_nominationsMenuPlayerPage[Player]);
        return PLUGIN_HANDLED;
    }
    return PLUGIN_CONTINUE;
}

public Task_FinishDecision()
{
    static Map[MapDecision_T], Seconds;
    unregister_forward(FM_CmdStart,         g_hookCmdStart);
    unregister_forward(FM_ClientKill,       g_hookClientKill);
    unregister_forward(FM_ClientCommand,    g_hookClientCommand);
    unregister_forward(FM_PlayerPreThink,   g_hookPlayerPreThink);
    unregister_forward(FM_PlayerPostThink,  g_hookPlayerPostThink);
    unregister_forward(FM_UpdateClientData, g_hookUpdateClientData);
    unregister_forward(FM_CmdStart,         g_hookCmdStart_Post, true);
    unregister_forward(FM_PlayerPreThink,   g_hookPlayerPreThink_Post, true);
    unregister_forward(FM_PlayerPostThink,  g_hookPlayerPostThink_Post, true);
    unregister_forward(FM_UpdateClientData, g_hookUpdateClientData_Post, true);
    SortADTArray(g_arrayMapsDecision, Sort_Descending, Sort_Integer); /// Sort maps desc. by votes.
    ArrayGetArray(g_arrayMapsDecision, random_num(0, sameEntries() - 1), Map); /// Get the most voted map or a random one if multiple maps were equally voted.
    if (equali(g_actualMapName, Map[m_Name]))
    { /// Extend the actual map.
#if defined hook_cvar_change
        disable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
        set_pcvar_float(g_cvarMultiPlayerTimeLimit, (get_gametime() + 60.0 * g_actualMapExtensionMinutes) / 60.0);
#if defined hook_cvar_change
        enable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
        client_print(0, print_chat, "* This map was extended %dm (%d VOTE%s or %0.0f%%), removed all vote rocks.",
            floatround(g_actualMapExtensionMinutes, floatround_tozero /** Truncate. */),
            Map[m_Votes], 1 == Map[m_Votes] ? "" : "S", votePercent(Map[m_Votes]));
    }
    else
    { /// Change the next map variable.
#if defined hook_cvar_change
        disable_cvar_hook(g_hookAmxNextMapConVarChange);
#endif
        set_pcvar_string(g_cvarAmxNextMap, Map[m_Name]);
#if defined hook_cvar_change
        enable_cvar_hook(g_hookAmxNextMapConVarChange);
#endif
#if defined _dodhacks_included /** When players type "nextmap" into their console, show them the correct result. */
        safeUpdateGameRulesNextMap_DoD(Map[m_Name]);
#endif
        if (g_changeOnlyAfterActualRound)
        {
#if defined hook_cvar_change
            disable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
            set_pcvar_float(g_cvarMultiPlayerTimeLimit, 0.0);
#if defined hook_cvar_change
            enable_cvar_hook(g_hookMpTimeLimitConVarChange);
#endif
            client_print(0, print_chat, "* The next map will be %s (%d VOTE%s or %0.0f%%) when round ends.",
                Map[m_Name], Map[m_Votes], 1 == Map[m_Votes] ? "" : "S", votePercent(Map[m_Votes]));
        }
        else
        {
            Seconds = floatround(60.0 * get_pcvar_float(g_cvarMultiPlayerTimeLimit) - get_gametime(), floatround_tozero /** Truncate. */);
            if (Seconds > 59)
                client_print(0, print_chat, "* The next map will be %s (%d VOTE%s or %0.0f%%) in %dm.",
                    Map[m_Name], Map[m_Votes], 1 == Map[m_Votes] ? "" : "S", votePercent(Map[m_Votes]), Seconds / 60);
            else
                client_print(0, print_chat, "* The next map will be %s (%d VOTE%s or %0.0f%%) in %ds.",
                    Map[m_Name], Map[m_Votes], 1 == Map[m_Votes] ? "" : "S", votePercent(Map[m_Votes]), Seconds);
        }
    } /// Destroy the menu.
    ArrayClear(g_arrayMapsDecision);
    menu_destroy(g_menuDecision);
    if (g_greyDecisionMenu)
        menu_destroy(g_menuDecisionGrey);
    updatePlayers(false); /// Let alive players move again.
    g_knownDecisionVotes = Map[m_Votes];
    arrayset(g_hasRockedTheDecision, false, sizeof g_hasRockedTheDecision);
}

buildMenu()
{
    static Iter, Map[Map_T], mapDecision[MapDecision_T], Added, Array: Rand, Buffer[128];
    g_menuDecision = menu_create("Choose A Map", "OnMenuDecisionItem");
    menu_setprop(g_menuDecision, MPROP_PERPAGE, false); /// Turn off Back/ Next/ Exit options.
    menu_setprop(g_menuDecision, MPROP_EXIT, MEXIT_FORCE); /// Force include Exit option.
    if (g_greyDecisionMenu)
    {
        g_menuDecisionGrey = menu_create("Choose A Map", "OnMenuDecisionGreyItem");
        menu_setprop(g_menuDecisionGrey, MPROP_PERPAGE, false); /// Turn off Back/ Next/ Exit options.
    }
    SortADTArray(g_arrayMaps, Sort_Descending, Sort_Integer); /// Sort nominated maps desc. by noms.
    Added = 0; /// The menu is empty.
    for (Iter = 0; Iter < g_mapsInMenuExcludingExtension; Iter++)
    { /// Add nominated maps, if any.
        ArrayGetArray(g_arrayMaps, Iter, Map);
        if (Map[m_Nominations] < 1)
            break; /// Never been nominated by a player.
        mapDecision[m_Votes] = 0;
        copy(mapDecision[m_Name], charsmax(mapDecision[m_Name]), Map[m_Name]);
        ArrayPushArray(g_arrayMapsDecision, mapDecision);
        menu_additem(g_menuDecision, mapDecision[m_Name]);
        if (g_greyDecisionMenu)
        {
            formatex(Buffer, charsmax(Buffer), "\\d%s", mapDecision[m_Name]);
            menu_additem(g_menuDecisionGrey, Buffer);
        }
        ++Added; /// Add nominated map to menu.
    }
    if (Added < g_mapsInMenuExcludingExtension)
    { /// If no nominated maps or not enough nominated maps were added, fill the menu up with random maps.
        Rand = ArrayCreate(); /// To not reuse old random numbers.
        do
        {
            Iter = random_num(Added, g_loadedMaps - 1);
            if (ArrayFindValue(Rand, Iter) > -1)
                continue; /// This map was already added into the menu, jump over it.
            ArrayPushCell(Rand, Iter);
            ArrayGetArray(g_arrayMaps, Iter, Map);
            mapDecision[m_Votes] = 0;
            copy(mapDecision[m_Name], charsmax(mapDecision[m_Name]), Map[m_Name]);
            ArrayPushArray(g_arrayMapsDecision, mapDecision);
            menu_additem(g_menuDecision, mapDecision[m_Name]);
            if (g_greyDecisionMenu)
            {
                formatex(Buffer, charsmax(Buffer), "\\d%s", mapDecision[m_Name]);
                menu_additem(g_menuDecisionGrey, Buffer);
            }
            ++Added; /// Add random map to menu.
        }
        while (Added < g_mapsInMenuExcludingExtension);
        ArrayDestroy(Rand); /// Clear old random numbers.
    } /// Now add an extension option.
    mapDecision[m_Votes] = 0;
    copy(mapDecision[m_Name], charsmax(mapDecision[m_Name]), g_actualMapName);
    ArrayPushArray(g_arrayMapsDecision, mapDecision);
    copy(Buffer, charsmax(Buffer), g_actualMapName);
    if (g_wordActualMapExtensionSuffix[0])
    {
        if (' ' != g_wordActualMapExtensionSuffix[0])
            add(Buffer, charsmax(Buffer), " "); /// Insert a space between the actual map name and the "extend" word if needed.
        add(Buffer, charsmax(Buffer), g_wordActualMapExtensionSuffix);
    }
    menu_additem(g_menuDecision, Buffer);
    if (g_greyDecisionMenu)
    {
        format(Buffer, charsmax(Buffer), "\\d%s", Buffer);
        menu_additem(g_menuDecisionGrey, Buffer);
        menu_addblank(g_menuDecisionGrey, 0);
    }
}

sameEntries()
{
    static Iter, Same, Votes;
    Same = 1;
    Votes = ArrayGetCell(g_arrayMapsDecision, 0);
    for (Iter = 1; Iter < g_mapsInMenuExcludingExtension; Iter++)
    {
        if (Votes != ArrayGetCell(g_arrayMapsDecision, Iter))
            break;
        ++Same;
    }
    return Same;
}

showMenuNominations(Player, bool: pageZero, Page = 0)
{
    if (pageZero)
        g_nominationsMenuPlayerPage[Player] = 0;
    menu_display(Player, g_menuNominations, Page);
}

bool: isViewingAMenu(Player)
{
    static Old, New;
    player_menu_info(Player, Old, New);
    return Old > 0 || New > -1;
}

updatePlayer(Player, bool: Freeze)
{
    switch (Freeze)
    {
        case true:
        {
            if (is_user_alive(Player))
            { /// During the vote, alive players are literally frozen. They are not allowed to move or attack another players.
                set_pev(Player, pev_flags, pev(Player, pev_flags) | FL_FROZEN);
                set_pev(Player, pev_button, 0);
                set_pev(Player, pev_oldbuttons, 0);
                set_pev(Player, pev_impulse, 0);
                set_pev(Player, pev_velocity, Float: { 0.0, 0.0, 0.0 });
            }
            else
                set_pev(Player, pev_flags, pev(Player, pev_flags) & ~FL_FROZEN);
        }
        default:
            set_pev(Player, pev_flags, pev(Player, pev_flags) & ~FL_FROZEN);
    }
}

updatePlayers(bool: Freeze)
{
    static Player;
    for (Player = 1; Player <= g_maxPlayers; Player++)
        if (g_isInServer[Player])
            updatePlayer(Player, Freeze);
}

bool: inDecision()
    return bool: ArraySize(g_arrayMapsDecision);

bool: hasDecided()
{
    static Map[32];
    get_pcvar_string(g_cvarAmxNextMap, Map, charsmax(Map));
    return !equali(Map, g_wordToBeDecided);
}

bool: canDecide()
{
    static Float: Time, Float: Difference;
    Time = get_pcvar_float(g_cvarMultiPlayerTimeLimit);
    if (Time <= 0.0)
        return false;
    Difference = 60.0 * Time - get_gametime();
    if (floatround(Difference, floatround_tozero /** Truncate. */) ==
        g_secBeforeVoteToAnnounce + floatround(g_mapNeededSecondsRemaining, floatround_tozero /** Truncate. */))
    {
        set_hudmessage(120 /** Red. */, 20 /** Green. */, 180 /** Blue. */,
            g_hudHorPos /** Horizontal position. */, g_hudVerPos /** Vertical position. */,
            g_hudStyle ? 1 : 0 /** Effect type. */, 0.5 /** Effect time. */,
            3.5 /** Duration. */, 0.1 /** Fade in time. */, 0.1 /** Fade out time. */);
        ShowSyncHudMsg(0, g_hudSyncHandle, "%d SEC. TILL VOTE!", g_secBeforeVoteToAnnounce);
    }
    return Difference <= g_mapNeededSecondsRemaining;
}

zeroPlayerVars(Player)
{
    g_isInServer[Player] = false;
    g_isFakePlayer[Player] = false;
    g_isAuthenticated[Player] = false;
    g_shouldBeNoticed[Player] = false;
    g_hasRockedTheDecision[Player] = false;
    g_nominationsMenuPlayerPage[Player] = 0;
}

bool: rockedTheDecision(&Players, &Float: Perc)
{
    Players = playersWhoRockedTheDecision();
    Perc = 100.0 * float(Players) / float(playingPlayers());
    return Perc >= g_rockTheDecisionMinimumPercent;
}

Float: votePercent(Votes)
    return 100.0 * float(Votes) / float(g_playersDuringEndOfMapVote);

playingPlayers()
{
    static Player, Playing;
    Playing = 0;
    for (Player = 1; Player <= g_maxPlayers; Player++)
    {
        if (false == g_isInServer[Player] || g_isFakePlayer[Player] || get_user_team(Player) < 1)
            continue;
        Playing++;
    }
    return Playing;
}

playersWhoRockedTheDecision()
{
    static Player, Rocked;
    Rocked = 0;
    for (Player = 1; Player <= g_maxPlayers; Player++)
    {
        if (false == g_hasRockedTheDecision[Player])
            continue;
        Rocked++;
    }
    return Rocked;
}

bool: mapPass(const Map[])
{
    static Path[256];
    if (g_exclVoteMapOverviewBmp)
    {
        formatex(Path, charsmax(Path), "overviews/%s.bmp", Map);
        if (!file_exists(Path))
            return false;
    }
    if (g_exclVoteMapOverviewTxt)
    {
        formatex(Path, charsmax(Path), "overviews/%s.txt", Map);
        if (!file_exists(Path))
            return false;
    }
    if (g_exclVoteMapSturmBot)
    {
        formatex(Path, charsmax(Path), "sturmbot/waypoints/%s.wpt", Map);
        if (!file_exists(Path) || isWPTFileMarineWayPoint(Path))
            return false;
    }
    if (g_exclVoteMapShrikeBot)
    {
        formatex(Path, charsmax(Path), "shrikebot/waypoints/%s.wps", Map);
        if (!file_exists(Path))
            return false;
    }
    if (g_exclVoteMapMarineBotWpt)
    {
        formatex(Path, charsmax(Path), "marinebot/defaultwpts/%s.wpt", Map);
        if (!file_exists(Path) || !isWPTFileMarineWayPoint(Path))
            return false;
    }
    if (g_exclVoteMapMarineBotPth)
    {
        formatex(Path, charsmax(Path), "marinebot/defaultwpts/%s.pth", Map);
        if (!file_exists(Path))
            return false;
    }
    if (g_exclVoteMapResource)
    {
        formatex(Path, charsmax(Path), "maps/%s.res", Map);
        if (!file_exists(Path))
            return false;
    }
    return true;
}

#if defined _dodhacks_included /** When players type "nextmap" into their console, show them the correct result. */
bool: safeUpdateGameRulesNextMap_DoD(const Map[])
{
    static safeMap[32];
    copy(safeMap, charsmax(safeMap), Map);
    return DoD_StoreGameRulesStr(CDoDTeamPlay_sNextMap_32Cells, safeMap);
}
#endif

bool: isWPTFileMarineWayPoint(const Path[])
{
    static File, Header;
    File = fopen(Path, "r");
    if (File)
    {
        fread(File, Header, BLOCK_CHAR);
        fclose(File);
        return 'F' == Header || Header == 'f';
    }
    return false;
}

bool: parseFilterConsolePlayerCommand(Player, Len, const Cmd[], &bool: Continue) /// Filters amx_cvar and amx_rcon console commands.
{ /// Before printing to player, ensure they are put in server.
    static argsInclCmd, Var[16], Val[64];
    Continue = true;
    if (Len < 8 /** amx_cvar and amx_rcon lengths. */ || Cmd[3] != '_')
        return false; /// Do not block.
    if (equali(Cmd, "amx_cvar"))
    { /// 11 characters: amx_nextmap and mp_timelimit.
        Continue = false; /// Player using another plugin.
        if (!mayPlayerAccessConsoleCommand(Player, "amx_cvar") || read_argv(1, Var, charsmax(Var)) < 11 || read_argc() /** argsInclCmd */ < 3)
            return false; /// i.e. "amx_cvar amx_nextmap dod_anzio".
        if (equali(Var, "amx_nextmap"))
        {
            if (read_argv(2, Val, charsmax(Val)) < 1) /// Map name.
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'amx_nextmap' can't be empty!");
                return true;
            }
            else if (!is_map_valid(Val))
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'amx_nextmap' can't be set to an invalid map (i.e. '%s')!", Val);
                return true;
            }
            else if (1 == g_blockManualChangingAmxNextMap)
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'amx_nextmap' can't be manually changed!");
                return true;
            }
            else if (2 == g_blockManualChangingAmxNextMap && false == hasDecided())
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'amx_nextmap' can't be changed now!");
                return true;
            }
        }
        else if (equali(Var, "mp_timelimit"))
        {
            if (g_blockManualChangingTimeLimit)
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'mp_timelimit' can't be changed now!");
                return true;
            }
            read_argv(2, Val, charsmax(Val)); /// Minutes.
            g_establishedDefaultMapMinutes = floatclamp(str_to_float(Val), g_mapMinimumTimeLimit, g_mapMaximumTimeLimit);
        }
    }
    else if (equali(Cmd, "amx_rcon"))
    { /// 8 characters: amx_cvar, amx_nextmap and mp_timelimit.
        Continue = false; /// Player using another plugin.
        if (!mayPlayerAccessConsoleCommand(Player, "amx_rcon") || read_argv(1, Var, charsmax(Var)) < 8)
            return false;
        argsInclCmd = read_argc();
        if (argsInclCmd < 3)
            return false; /// i.e. "amx_rcon amx_nextmap dod_anzio".
        if (equali(Var, "amx_nextmap"))
        {
            if (read_argv(2, Val, charsmax(Val)) < 1) /// Map name.
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'amx_nextmap' can't be empty!");
                return true;
            }
            else if (!is_map_valid(Val))
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'amx_nextmap' can't be set to an invalid map (i.e. '%s')!", Val);
                return true;
            }
            else if (1 == g_blockManualChangingAmxNextMap)
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'amx_nextmap' can't be manually changed!");
                return true;
            }
            else if (2 == g_blockManualChangingAmxNextMap && false == hasDecided())
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'amx_nextmap' can't be changed now!");
                return true;
            }
        }
        else if (equali(Var, "mp_timelimit"))
        {
            if (g_blockManualChangingTimeLimit)
            {
                if (g_isInServer[Player])
                    console_print(Player, "* 'mp_timelimit' can't be changed now!");
                return true;
            }
            read_argv(2, Val, charsmax(Val));
            g_establishedDefaultMapMinutes = floatclamp(str_to_float(Val), g_mapMinimumTimeLimit, g_mapMaximumTimeLimit);
        }
        else if (argsInclCmd > 3 && equali(Var, "amx_cvar"))
        { /// i.e. "amx_rcon amx_cvar amx_nextmap dod_anzio".
            if (read_argv(2, Var, charsmax(Var)) < 11)
                return false; /// 11 characters: amx_nextmap and mp_timelimit.
            if (equali(Var, "amx_nextmap"))
            {
                if (read_argv(3, Val, charsmax(Val)) < 1)
                {
                    if (g_isInServer[Player])
                        console_print(Player, "* 'amx_nextmap' can't be empty!");
                    return true;
                }
                else if (!is_map_valid(Val))
                {
                    if (g_isInServer[Player])
                        console_print(Player, "* 'amx_nextmap' can't be set to an invalid map (i.e. '%s')!", Val);
                    return true;
                }
                else if (1 == g_blockManualChangingAmxNextMap)
                {
                    if (g_isInServer[Player])
                        console_print(Player, "* 'amx_nextmap' can't be manually changed!");
                    return true;
                }
                else if (2 == g_blockManualChangingAmxNextMap && false == hasDecided())
                {
                    if (g_isInServer[Player])
                        console_print(Player, "* 'amx_nextmap' can't be changed now!");
                    return true;
                }
            }
            else if (equali(Var, "mp_timelimit"))
            {
                if (g_blockManualChangingTimeLimit)
                {
                    if (g_isInServer[Player])
                        console_print(Player, "* 'mp_timelimit' can't be changed now!");
                    return true;
                }
                read_argv(3, Val, charsmax(Val));
                g_establishedDefaultMapMinutes = floatclamp(str_to_float(Val), g_mapMinimumTimeLimit, g_mapMaximumTimeLimit);
            }
        }
    }
    return false;
}

bool: mayPlayerAccessConsoleCommand(Player, const Cmd[])
{
    static Access, Cmds, Iter, cmdName[32], Info[2], Flags;
    Access = get_user_flags(Player);
    Cmds = get_clcmdsnum(Access);
    for (Iter = 0; Iter < Cmds; Iter++)
    {
        get_clcmd(Iter, cmdName, charsmax(cmdName), Flags, Info, charsmax(Info), Access);
        if (Flags == (Access & Flags) && equali(cmdName, Cmd))
            return true;
    }
    return false;
}

bool: tryLoadMapMinutesFromServerCfg(&Float: Mins)
{
    Mins = 0.0;
    new Buffer[256];
    get_cvar_string("servercfgfile", Buffer, charsmax(Buffer));
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        Config = fopen("server.cfg", "r");
        if (!Config)
            return false;
    }
    new Var[16], Val[32];
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || ('m' != Buffer[0] && Buffer[0] != 'M' && Buffer[0] != '"') ||
            parse(Buffer, Var, charsmax(Var), Val, charsmax(Val)) < 2 || !equali("mp_timelimit", Var))
            continue;
        Mins = str_to_float(Val);
        fclose(Config);
        return true;
    }
    fclose(Config);
    return false;
}

bool: safeDisablePluginIfRunning(const logFile[], const pluginFileName[])
{
    new Buffer[256];
    get_localinfo("amxx_plugins", Buffer, charsmax(Buffer));
    new File = fopen(Buffer, "r");
    if (!File)
        return false;
    new newName[256];
    new nameLen = copy(newName, charsmax(newName), pluginFileName);
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
    log_to_file(logFile, "Disabling '%s' plugin in order to run this plugin properly!", pluginFileName);
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

#if !defined ArrayResize
ArrayFindString(Array: Which, const String[])
{
    static Iter, Size, Buffer[256];
    Size = ArraySize(Which);
    for (Iter = 0; Iter < Size; Iter++)
    {
        ArrayGetString(Which, Iter, Buffer, charsmax(Buffer));
        if (equal(Buffer, String))
            return Iter;
    }
    return -1;
}

ArrayFindValue(Array: Which, Value)
{
    static Iter, Size;
    Size = ArraySize(Which);
    for (Iter = 0; Iter < Size; Iter++)
        if (Value == ArrayGetCell(Which, Iter))
            return Iter;
    return -1;
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
