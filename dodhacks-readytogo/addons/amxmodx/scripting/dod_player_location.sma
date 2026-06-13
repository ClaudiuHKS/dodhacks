
#include <amxmodx>
#include <amxmisc>
#include <geoip>
#include <dodx> /// dod_get_map_info()

///
/// https://www.maxmind.com/en/geolite2/signup
///
/// sign up for free in order to download &
/// use MaxMind's free geo. databases
///
/// once downloaded, they'll be extracted
/// (unzipped) to ../amxmodx/data/
///
/// they should be MMDB files (do not download
/// the CSV files)
///

#define DPL_VERSION "1.0.0.2"

#if !defined HUD_PRINTTALK
#define      HUD_PRINTTALK 3
#endif

/**
 * Shared global vars. (both new and old AMXX versions use these).
 */
new g_historyRows;
new bool: g_alterBots;
new bool: g_blockDefJoinMsg;
new bool: g_areAlliesBritish;
new Array: g_History;

#if defined SYSTEM_IMPERIAL

/**
 * Beginning of dod_player_location plugin for new AMXX versions.
 */

new bool: g_isImperial;
new bool: g_incDistance;
new bool: g_isSvPrecise;
new Float: g_svLatitude;
new Float: g_svLongitude;
new g_distanceSym[4];

public plugin_init()
{
    register_plugin("DoD: Player Location", DPL_VERSION, "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_player_location.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[24], distanceSymStyle;
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key, "@server_ip_v4_address"))
        {
            g_svLatitude  = geoip_latitude (Val);
            g_svLongitude = geoip_longitude(Val);
            geoip_city(Val, Buffer, charsmax(Buffer), LANG_SERVER);
            if (Buffer[0])
                g_isSvPrecise = true;
            else
            {
                geoip_region_name(Val, Buffer, charsmax(Buffer), LANG_SERVER);
                if (Buffer[0])
                    g_isSvPrecise = true;
            }
        }
        else if (equali(Key,  "@include_player_distance"))
            g_incDistance     = bool: str_to_num(Val);
        else if (equali(Key,  "@is_distance_imperial"))
            g_isImperial      = bool: str_to_num(Val);
        else if (equali(Key,  "@distance_symbol_style"))
            distanceSymStyle  =       str_to_num(Val);
        else if (equali(Key,  "@alter_bots"))
            g_alterBots       = bool: str_to_num(Val);
        else if (equali(Key,  "@block_def_join_msg"))
            g_blockDefJoinMsg = bool: str_to_num(Val);
    }
    fclose(Config);

    switch (distanceSymStyle)
    {
        case 0:  copy(g_distanceSym, charsmax(g_distanceSym),
            g_isImperial ? " mi" : " km");
        case 1:  copy(g_distanceSym, charsmax(g_distanceSym),
            g_isImperial ? " Mi" : " Km");
        default: copy(g_distanceSym, charsmax(g_distanceSym),
            g_isImperial ? " MI" : " KM");
    }
    register_message(get_user_msgid("TextMsg"), "On_Message_TextMsg");
    g_areAlliesBritish = bool: dod_get_map_info(MI_ALLIES_TEAM);
    g_History = ArrayCreate(256);
    register_concmd("amx_locations", "On_ConsoleCmd_Locations",
        ADMIN_ADMIN, "- displays locations history");
    return PLUGIN_CONTINUE;
}

public On_Message_TextMsg(messageIndex, Destination)
{
    static Name[32], Phrase[24], Addr[24], Country[64], Region[64], City[64], Row[256],
        Player, Args, Float: Latitude, Float: Longitude, Distance[16], Team[16];
    if (Destination != MSG_BROADCAST && MSG_ALL != Destination)
        return PLUGIN_CONTINUE;
    Args = get_msg_args();
    if (Args < 3 || Args > 4 || ARG_BYTE != get_msg_argtype(1) ||
        HUD_PRINTTALK != get_msg_arg_int(1) || ARG_STRING != get_msg_argtype(2) ||
        ARG_STRING != get_msg_argtype(3) ||
        get_msg_arg_string(2, Phrase, charsmax(Phrase)) < 17)
        return PLUGIN_CONTINUE;
    if (equal(Phrase, "#game_joined_game"))
        return g_blockDefJoinMsg ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
    if (!equal(Phrase, "#game_joined_team") || get_msg_argtype(4) != ARG_STRING ||
        get_msg_arg_string(4, Team, charsmax(Team)) < 2)
        return PLUGIN_CONTINUE;
    get_msg_arg_string(3, Name, charsmax(Name));
    Player = get_user_index(Name);
    if (Player < 1)
        return PLUGIN_CONTINUE;
    if (g_areAlliesBritish && Team[1] == 'l' /** Allies. */)
        copy(Team, charsmax(Team), "British");
    if (g_alterBots && is_user_bot(Player))
        formatex(Addr, charsmax(Addr), "%d.%d.%d.%d",
            random_num(0, 255), random_num(0, 255), random_num(0, 255), random_num(0, 255));
    else
        get_user_ip(Player, Addr, charsmax(Addr), 1);
    geoip_country_ex (Addr, Country, charsmax(Country), LANG_SERVER);
    geoip_city       (Addr, City,    charsmax(City),    LANG_SERVER);
    geoip_region_name(Addr, Region,  charsmax(Region),  LANG_SERVER);
    Latitude  = geoip_latitude (Addr);
    Longitude = geoip_longitude(Addr);
    addCommasInt(
        floatround(
            geoip_distance(Latitude, Longitude, g_svLatitude, g_svLongitude,
                g_isImperial ? SYSTEM_IMPERIAL : SYSTEM_METRIC
            ), floatround_round
        ), Distance, charsmax(Distance)
    ); /// If neither server and player precise but same country, avoid displaying 0 Km.
    if (!g_isSvPrecise && !City[0] && !Region[0] && Distance[0] == '0')
        copy(Distance, charsmax(Distance), "server's"   );
    else
        add (Distance, charsmax(Distance), g_distanceSym);
    switch (g_incDistance)
    {
        case false:
            if (!Country[0] && !Region[0] && !City[0])
                client_print(0, print_chat, "* %s joins %s.",
                    Name, Team);
            else if (Country[0] && Region[0] && City[0])
                client_print(0, print_chat, "* %s joins %s from %s, %s, %s.",
                    Name, Team, Country, Region, City);
            else if (!Country[0] && Region[0] && City[0])
                client_print(0, print_chat, "* %s joins %s from %s, %s.",
                    Name, Team, Region, City);
            else if (Country[0] && !Region[0] && City[0])
                client_print(0, print_chat, "* %s joins %s from %s, %s.",
                    Name, Team, Country, City);
            else if (Country[0] && Region[0] && !City[0])
                client_print(0, print_chat, "* %s joins %s from %s, %s.",
                    Name, Team, Country, Region);
            else if (!Country[0] && !Region[0] && City[0])
                client_print(0, print_chat, "* %s joins %s from %s.",
                    Name, Team, City);
            else if (Country[0] && !Region[0] && !City[0])
                client_print(0, print_chat, "* %s joins %s from %s.",
                    Name, Team, Country);
            else
                client_print(0, print_chat, "* %s joins %s from %s.",
                    Name, Team, Region);
        default:
            if (!Country[0] && !Region[0] && !City[0])
                client_print(0, print_chat, "* %s joins %s.",
                    Name, Team);
            else if (Country[0] && Region[0] && City[0])
                client_print(0, print_chat, "* %s joins %s from %s, %s, %s (%s).",
                    Name, Team, Country, Region, City, Distance);
            else if (!Country[0] && Region[0] && City[0])
                client_print(0, print_chat, "* %s joins %s from %s, %s (%s).",
                    Name, Team, Region, City, Distance);
            else if (Country[0] && !Region[0] && City[0])
                client_print(0, print_chat, "* %s joins %s from %s, %s (%s).",
                    Name, Team, Country, City, Distance);
            else if (Country[0] && Region[0] && !City[0])
                client_print(0, print_chat, "* %s joins %s from %s, %s (%s).",
                    Name, Team, Country, Region, Distance);
            else if (!Country[0] && !Region[0] && City[0])
                client_print(0, print_chat, "* %s joins %s from %s (%s).",
                    Name, Team, City, Distance);
            else if (Country[0] && !Region[0] && !City[0])
                client_print(0, print_chat, "* %s joins %s from %s (%s).",
                    Name, Team, Country, Distance);
            else
                client_print(0, print_chat, "* %s joins %s from %s (%s).",
                    Name, Team, Region, Distance);
    }
    if ((Country[0] || Region[0] || City[0]) && ArrayFindString(g_History, Name) < 0)
    {
        if (Country[0] && Region[0] && City[0])
            formatex(Row, charsmax(Row), "%-31s %s, %s, %s (%s)",
                Name, Country, Region, City, Distance);
        else if (Country[0] && Region[0] && !City[0])
            formatex(Row, charsmax(Row), "%-31s %s, %s (%s)",
                Name, Country, Region, Distance);
        else if (Country[0] && !Region[0] && City[0])
            formatex(Row, charsmax(Row), "%-31s %s, %s (%s)",
                Name, Country, City, Distance);
        else if (!Country[0] && Region[0] && City[0])
            formatex(Row, charsmax(Row), "%-31s %s, %s (%s)",
                Name, Region, City, Distance);
        else if (Country[0] && !Region[0] && !City[0])
            formatex(Row, charsmax(Row), "%-31s %s (%s)",
                Name, Country, Distance);
        else if (!Country[0] && Region[0] && !City[0])
            formatex(Row, charsmax(Row), "%-31s %s (%s)",
                Name, Region, Distance);
        else if (!Country[0] && !Region[0] && City[0])
            formatex(Row, charsmax(Row), "%-31s %s (%s)",
                Name, City, Distance);
        ArrayPushString(g_History, Row);
        if (++g_historyRows > 32)
        {
            --g_historyRows;
            ArrayDeleteItem(g_History, 0);
        }
    }
    return PLUGIN_HANDLED;
}

addCommasInt(From, To[], Size)
{
    static absFrom;
    if (From >= 1000000000 || From <= -1000000000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d,%03d,%03d",
            From / 1000000000,
            (absFrom / 1000000) % 1000,
            (absFrom / 1000) % 1000,
            absFrom % 1000);
    }
    else if (From >= 1000000 || From <= -1000000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d,%03d",
            From / 1000000,
            (absFrom / 1000) % 1000,
            absFrom % 1000);
    }
    else if (From >= 1000 || From <= -1000)
    {
        absFrom = abs(From);
        formatex(To, Size, "%d,%03d",
            From / 1000,
            absFrom % 1000);
    }
    else
        num_to_str(From, To, Size);
}

#else

/**
 * Beginning of dod_player_location plugin for old AMXX versions.
 */

public plugin_init()
{
    register_plugin("DoD: Player Location", DPL_VERSION, "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_player_location.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        log_amx("Error opening '%s'!", Buffer);
        set_fail_state("Error opening plugin specific cfg. file!");
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[4];
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            2 != parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)))
            continue;
        if (equali(Key,       "@alter_bots"))
            g_alterBots       = bool: str_to_num(Val);
        else if (equali(Key,  "@block_def_join_msg"))
            g_blockDefJoinMsg = bool: str_to_num(Val);
    }
    fclose(Config);

    register_message(get_user_msgid("TextMsg"), "On_Message_TextMsg");
    g_areAlliesBritish = bool: dod_get_map_info(MI_ALLIES_TEAM);
    g_History = ArrayCreate(128);
    register_concmd("amx_locations", "On_ConsoleCmd_Locations",
        ADMIN_ADMIN, "- displays locations history");
    return PLUGIN_CONTINUE;
}

public On_Message_TextMsg(messageIndex, Destination)
{
    static Name[32], Phrase[24], Addr[24], Country[64], Player, Args, Team[16], Row[128];
    if (Destination != MSG_BROADCAST && MSG_ALL != Destination)
        return PLUGIN_CONTINUE;
    Args = get_msg_args();
    if (Args < 3 || Args > 4 || ARG_BYTE != get_msg_argtype(1) ||
        HUD_PRINTTALK != get_msg_arg_int(1) || ARG_STRING != get_msg_argtype(2) ||
        ARG_STRING != get_msg_argtype(3) ||
        get_msg_arg_string(2, Phrase, charsmax(Phrase)) < 17)
        return PLUGIN_CONTINUE;
    if (equal(Phrase, "#game_joined_game"))
        return g_blockDefJoinMsg ? PLUGIN_HANDLED : PLUGIN_CONTINUE;
    if (!equal(Phrase, "#game_joined_team") || get_msg_argtype(4) != ARG_STRING ||
        get_msg_arg_string(4, Team, charsmax(Team)) < 2)
        return PLUGIN_CONTINUE;
    get_msg_arg_string(3, Name, charsmax(Name));
    Player = get_user_index(Name);
    if (Player < 1)
        return PLUGIN_CONTINUE;
    if (g_areAlliesBritish && Team[1] == 'l' /** Allies. */)
        copy(Team, charsmax(Team), "British");
    if (g_alterBots && is_user_bot(Player))
        formatex(Addr, charsmax(Addr), "%d.%d.%d.%d",
            random_num(0, 255), random_num(0, 255), random_num(0, 255), random_num(0, 255));
    else
        get_user_ip(Player, Addr, charsmax(Addr), 1);
    geoip_country(Addr, Country, charsmax(Country));
    if (!Country[0])
        client_print(0, print_chat, "* %s joins %s.", Name, Team);
    else
        client_print(0, print_chat, "* %s joins %s from %s.", Name, Team, Country);
    if (Country[0] && ArrayFindString(g_History, Name) < 0)
    {
        formatex(Row, charsmax(Row), "%-31s %s", Name, Country);
        ArrayPushString(g_History, Row);
        if (++g_historyRows > 32)
        {
            --g_historyRows;
            ArrayDeleteItem(g_History, 0);
        }
    }
    return PLUGIN_HANDLED;
}

#endif

/**
 * Helper functions below are meant for both old and new AMXX versions.
 */

#if !defined ArrayResize /// If compiling with an older AMXX version.
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
#endif

public On_ConsoleCmd_Locations(Player, Level, Command)
{
    static Row;
    if (!cmd_access(Player, Level, Command, 1))
        return PLUGIN_HANDLED;
    if (0 == g_historyRows)
    {
        console_print(Player, "* No history at the moment.");
        return PLUGIN_HANDLED;
    }
    console_print(Player, "%3s %-31s %s^n--------------------------------------------",
        "#", "Name", "Location");
    for (Row = 0; Row < g_historyRows; Row++)
        console_print(Player, "%2d. %a", 1 + Row, ArrayGetStringHandle(g_History, Row));
    return PLUGIN_HANDLED;
}

/**
 * Changes
 * -------
 * v1.0.0.1 @ Jun 11 '26 => Replace `switch (Team[2])` with `switch (Team[1])` (bug fix).
 * v1.0.0.2 @ Jun 13 '26 => Optionally disable #game_join_game message and display geo.
 *                          location in #game_join_team message to reduce chat spam.
 */
