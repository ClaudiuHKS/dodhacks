
#pragma semicolon 1
#pragma ctrlchar  '\'

#include <amxmodx>
#include <amxmisc>

#define PLUGIN  "Public Voteban"
#define VERSION "1.0.0.5"
#define AUTHOR  "Alka&claudiuhks"

enum cstrikeColor
{
    Grey = 33,
    Red  = 34,
    Blue = 35,
};

new bool: g_cstrikeRunning;
new bool: g_isPlayerInServer[33];
new bool: g_isPlayerFake[33];
new bool: g_isPlayerHLTV[33];
new g_playerMenuPage[33];
new g_playerVotedPlayers[33];
new g_playerBanVotes[33];
new g_playerPlayers[33][32];
new g_playersNum;
new g_cvarPerc;
new g_cvarMins;
new g_cvarType;
new g_cvarMinHumans;
new g_sayText;

public plugin_init()
{
    register_plugin(PLUGIN, VERSION, AUTHOR);

    register_menucmd(register_menuid("\\rVOTEBAN\\y Menu"),
        MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6,
        "Menu_VoteBan");

    /** To ensure such commands appear in amx_help. */
    register_clcmd("ban",     "OnPlayerClConsoleCmd_Voteban", ADMIN_ALL, "- vote a player off");
    register_clcmd("voteban", "OnPlayerClConsoleCmd_Voteban", ADMIN_ALL, "- vote a player off");

    g_cvarPerc       = register_cvar("vb_perc",       "50.001"); /// Required percent
    g_cvarMins       = register_cvar("vb_mins",       "1440");   /// Ban minutes
    g_cvarType       = register_cvar("vb_type",       "0");      /// 0 Steam & 1 IP address
    g_cvarMinHumans  = register_cvar("vb_min_humans", "4");      /// Minimum humans in server

    g_cstrikeRunning = bool: cstrike_running();

    if (g_cstrikeRunning)
        g_sayText    = get_user_msgid("SayText");
}

public OnPlayerClConsoleCmd_Voteban(Player)
{
    static Min, Players[32], Num;
    if (F_IsViewingAMenu(Player))
    {
        client_print(Player, print_console, "* You're already viewing a menu!");
        return PLUGIN_HANDLED;
    }
    get_players(Players, Num, "ch");

    Min = get_pcvar_num(g_cvarMinHumans);
    if (Num < Min)
    {
        client_print(Player, print_console, "* Minimum %d humans are required to vote!", Min);
        return PLUGIN_HANDLED;
    }

    client_print(Player, print_console,
        "* Menu displayed, min. needed ratio is %f%% and ban time is %d min(s).",
        get_pcvar_float(g_cvarPerc), get_pcvar_num(g_cvarMins));
    showMenu(Player, g_playerMenuPage[Player] = 0);
    return PLUGIN_HANDLED;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public client_putinserver(Player)
{
    g_isPlayerInServer[Player]   = true;

    g_playerBanVotes[Player]     = 0;
    g_playerVotedPlayers[Player] = 0;

    g_isPlayerFake[Player] = bool: is_user_bot(Player);
    g_isPlayerHLTV[Player] = bool: is_user_hltv(Player);
}

public DOD_ON_PLAYER_DISCONNECTED
{
    static Iter, Other;
    get_players(g_playerPlayers[Player], g_playersNum, "ch");

    if (g_playerVotedPlayers[Player])
    {
        for (Iter = 0; Iter < g_playersNum; Iter++)
        {
            Other = g_playerPlayers[Player][Iter];

            if (g_playerVotedPlayers[Player] & (1 << Other))
                g_playerBanVotes[Other]--;
        }
    }

    for (Iter = 0; Iter < g_playersNum; Iter++)
    {
        Other = g_playerPlayers[Player][Iter];

        if (g_playerVotedPlayers[Other] &   (1 << Player))
            g_playerVotedPlayers[Other] &= ~(1 << Player);
    }

    g_playerBanVotes[Player]     = 0;
    g_playerVotedPlayers[Player] = 0;

    g_isPlayerInServer[Player]   = false;
}

public client_command(Player)
{
    static Buffer[32], Len, Min, Players[32], Num;

    if (g_isPlayerInServer[Player] && !g_isPlayerFake[Player] && !g_isPlayerHLTV[Player])
    {
        Len = read_argv(0, Buffer, charsmax(Buffer));

        if (Len < 3)
            return PLUGIN_CONTINUE;

        if (equali(Buffer, "say") || equali(Buffer, "say_team"))
        {
            Len = read_argv(1, Buffer, charsmax(Buffer));

            if (Len < 3)
                return PLUGIN_CONTINUE;

            if (equali(Buffer, "ban") || equali(Buffer, "vote") || equali(Buffer, "voteban") ||
                ((Buffer[0] == ',' || Buffer[0] == '.' || Buffer[0] == '/' || Buffer[0] == '!') &&
                (equali(Buffer[1], "ban") || equali(Buffer[1], "vote") || equali(Buffer[1], "voteban"))))
            {
                if (F_IsViewingAMenu(Player))
                {
                    if (!g_cstrikeRunning)
                        client_print(Player, print_chat, "* You're already viewing a menu!");

                    else
                        colorMsg(Player, cstrikeColor: Red, "%c*%c You're already viewing a menu!", 1, 3);

                    return PLUGIN_CONTINUE;
                }
                get_players(Players, Num, "ch");

                Min = get_pcvar_num(g_cvarMinHumans);
                if (Num < Min)
                {
                    if (!g_cstrikeRunning)
                        client_print(Player, print_chat, "* Minimum %d humans are required to vote!", Min);

                    else
                        colorMsg(Player, cstrikeColor: Red, "%c* Minimum%c %d%c humans are required to vote!", 1, 3, Min, 1);

                    return PLUGIN_CONTINUE;
                }

                showMenu(Player, g_playerMenuPage[Player] = 0);
            }
        }
    }

    return PLUGIN_CONTINUE;
}

showMenu(Player, Page)
{
    static Iter, Ban, Name[32], Menu[512], Pos, Begin, End, Keys, Len;

    Pos = 0;
    Begin = Page * 4;
    get_players(g_playerPlayers[Player], g_playersNum, "ch");

    if (Begin >= g_playersNum)
        Begin = Page = g_playerMenuPage[Player] = 0;

    Len = formatex(Menu, charsmax(Menu), "\\rVOTEBAN\\y Menu\n");
    End = Begin + 4;
    Keys = MENU_KEY_6;

    if (End > g_playersNum)
        End = g_playersNum;

    for (Iter = Begin; Iter < End; Iter++)
    {
        Ban = g_playerPlayers[Player][Iter];
        get_user_name(Ban, Name, charsmax(Name));

        if (Player == Ban)
        {
            Pos++;
            Len += formatex(Menu[Len], charsmax(Menu) - Len, "\\r%d.\\d %s (\\r%.0f%%\\d)\n",
                Pos, Name, float(g_playerBanVotes[Ban]) / float(g_playersNum) * 100.0);
        }

        else if (!g_isPlayerInServer[Ban])
        {
            Pos++;
            Len += formatex(Menu[Len], charsmax(Menu) - Len, "\\r%d.\\d %s (\\r%.0f%%\\d)\\r *\n",
                Pos, Name, float(g_playerBanVotes[Ban]) / float(g_playersNum) * 100.0);
        }

        else if (g_playerVotedPlayers[Player] & (1 << Ban))
        {
            Pos++;
            Len += formatex(Menu[Len], charsmax(Menu) - Len, "\\r%d.\\d %s (\\r%.0f%%\\d)\\r *\n",
                Pos, Name, float(g_playerBanVotes[Ban]) / float(g_playersNum) * 100.0);
        }

        else
        {
            Keys |= (1 << Pos++);
            Len += formatex(Menu[Len], charsmax(Menu) - Len, "\\r%d.\\w %s\\d (\\r%.0f%%\\d)\n",
                Pos, Name, float(g_playerBanVotes[Ban]) / float(g_playersNum) * 100.0);
        }
    }

    if (End != g_playersNum)
    {
        formatex(Menu[Len], charsmax(Menu) - Len, "\\r5.\\y Next\n\\r6.\\y %s", Page ? "Back" : "Exit");
        Keys |= MENU_KEY_5;
    }

    else
        formatex(Menu[Len], charsmax(Menu) - Len, "\\r6.\\y %s", Page ? "Back" : "Exit");

    show_menu(Player, Keys, Menu, -1 /** No time limit. */);
    return PLUGIN_HANDLED;
}

public Menu_VoteBan(Player, Key)
{
    static Ban, banName[32], playerName[32];
    switch (Key)
    {
        case 4:
        {
            if (!g_isPlayerInServer[Player])
                return PLUGIN_HANDLED;

            showMenu(Player, ++g_playerMenuPage[Player]);
        }

        case 5:
        {
            if (!g_isPlayerInServer[Player] || !g_playerMenuPage[Player])
                return PLUGIN_HANDLED;

            showMenu(Player, --g_playerMenuPage[Player]);
        }

        default:
        {
            if (!g_isPlayerInServer[Player])
                return PLUGIN_HANDLED;

            Ban = g_playerPlayers[Player][g_playerMenuPage[Player] * 4 + Key];

            if (!g_isPlayerInServer[Ban])
            {
                client_print(Player, print_center, "Player no longer in server!");

                showMenu(Player, g_playerMenuPage[Player]);
                return PLUGIN_HANDLED;
            }

            if (Ban == Player)
            {
                client_print(Player, print_center, "Unable to vote yourself off!");

                showMenu(Player, g_playerMenuPage[Player]);
                return PLUGIN_HANDLED;
            }

            if (g_playerVotedPlayers[Player] & (1 << Ban))
            {
                client_print(Player, print_center, "Player already voted off!");

                showMenu(Player, g_playerMenuPage[Player]);
                return PLUGIN_HANDLED;
            }

            g_playerBanVotes[Ban]++;
            g_playerVotedPlayers[Player] |= (1 << Ban);

            get_user_name(Player, playerName, charsmax(playerName));
            get_user_name(Ban, banName, charsmax(banName));

            if (!g_cstrikeRunning)
                client_print(0, print_chat, "* %s voted %s off!", playerName, banName);

            else
                colorMsg(0, cstrikeColor: Red, "%c*%c %s%c voted%c %s%c off!", 1, 4, playerName, 1, 3, banName, 1);

            client_print(Player, print_center, "Voted %s off!", banName);

            computeBan(Ban);
            showMenu(Player, g_playerMenuPage[Player]);
        }
    }

    return PLUGIN_HANDLED;
}

computeBan(Player)
{
    static Auth[32], Name[32], Mins, Iter, Other;
    get_players(g_playerPlayers[Player], g_playersNum, "ch");

    if (float(g_playerBanVotes[Player]) / float(g_playersNum) * 100.0 >= get_pcvar_float(g_cvarPerc))
    {
        get_user_name(Player, Name, charsmax(Name));
        Mins = get_pcvar_num(g_cvarMins);

        switch (get_pcvar_num(g_cvarType))
        {
            case 0:
            {
                get_user_authid(Player, Auth, charsmax(Auth));
                server_cmd("kick #%d  You were voted off %d min(s)!;wait;wait;wait;banid %d \"%s\";wait;wait;wait;writeid",
                    get_user_userid(Player), Mins, Mins, Auth);

                if (!g_cstrikeRunning)
                    client_print(0, print_chat, "* %s (%s) was voted off %d min(s)!", Name, Auth, Mins);

                else
                    colorMsg(0, cstrikeColor: Red, "%c*%c %s (%s)%c was voted off%c %d%c mins!", 1, 3, Name, Auth, 1, 4, Mins, 1);
            }

            default:
            {
                get_user_ip(Player, Auth, charsmax(Auth), 1);
                server_cmd("kick #%d  You were voted off %d min(s)!;wait;wait;wait;addip %d \"%s\";wait;wait;wait;writeip",
                    get_user_userid(Player), Mins, Mins, Auth);

                if (!g_cstrikeRunning)
                    client_print(0, print_chat, "* %s was voted off %d min(s)!", Name, Mins);

                else
                    colorMsg(0, cstrikeColor: Red, "%c*%c %s%c was voted off%c %d%c mins!", 1, 3, Name, 1, 4, Mins, 1);
            }
        }

        g_playerBanVotes[Player] = 0;

        for (Iter = 0; Iter < g_playersNum; Iter++)
        {
            Other = g_playerPlayers[Player][Iter];

            if (g_playerVotedPlayers[Other] &   (1 << Player))
                g_playerVotedPlayers[Other] &= ~(1 << Player);
        }

        g_isPlayerInServer[Player] = false;
    }
}

colorMsg(Player, cstrikeColor: Color, const Raw[], any: ...)
{
    static Buffer[256], HLTV[256], Players[32], Num, Iter;

    if (vformat(Buffer, charsmax(Buffer), Raw, 4) < 1)
        return;

    if (Player > 0)
    {
        if (g_isPlayerFake[Player])
            return;

        Num        = 1;
        Players[0] = Player;
    }

    else
        get_players(Players, Num, "c");

    for (Iter = 0; Iter < Num; Iter++)
    {
        Player = Players[Iter];

        if (!g_isPlayerHLTV[Player])
        {
            message_begin(MSG_ONE_UNRELIABLE, g_sayText, { 0, 0, 0 }, Player);
            write_byte(noTag: Color);
            write_string(Buffer);
            message_end();
        }

        else
        {
            HLTV = Buffer;

            replace_all(HLTV, charsmax(HLTV), "\x01", "");
            replace_all(HLTV, charsmax(HLTV), "\x03", "");
            replace_all(HLTV, charsmax(HLTV), "\x04", "");

            client_print(Player, print_chat, HLTV);
        }
    }
}

bool: F_IsViewingAMenu(Player)
{
    static Old, New;
    player_menu_info(Player, Old, New);
    return Old > 0 || New > -1;
}
