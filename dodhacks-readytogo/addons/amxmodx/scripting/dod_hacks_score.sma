
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>

#define m_CBasePlayer_iObjScore 476
#define m_CBasePlayer_iDeaths   477

enum unsigned: DoD_Score
{
    m_Steam[64],
    m_Score,
    m_Deaths,
    Float: m_Frags,
};

new g_Entries;
new Array: g_Scores;
new g_Steam[33][64];
new bool: g_keepFakePlayers;
new bool: g_isPlayerFake[33];

public plugin_init()
{
    register_plugin("DoD Hacks: Score", "1.0.1.0", "Hattrick HKS (claudiuhks)");

    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_score.ini");
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
            parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;
        if (equali(Key, "@include_fake_players"))
        {
            g_keepFakePlayers = bool: str_to_num(Val);
            break;
        }
    }
    fclose(Config);

    g_Scores = ArrayCreate(DoD_Score);
    return PLUGIN_CONTINUE;
}

public client_connect(Player)
    g_Steam[Player][0] = EOS;

public client_putinserver(Player)
{
    static Iter, Score[DoD_Score], Name[32];
    if (get_user_authid(Player, g_Steam[Player], charsmax(g_Steam[])) > 0)
    { /// We need something unique!
        g_isPlayerFake[Player] = bool: is_user_bot(Player);
        if (g_isPlayerFake[Player])
        {
            if (!g_keepFakePlayers)
                return PLUGIN_CONTINUE; /// Skip fake players if needed.
            add(g_Steam[Player], charsmax(g_Steam[]), "@");
            get_user_name(Player, Name, charsmax(Name));
            add(g_Steam[Player], charsmax(g_Steam[]), Name); /// Fake players uniqueness.
        }
        for (Iter = 0; Iter < g_Entries; Iter++)
        { /// Search entries.
            ArrayGetArray(g_Scores, Iter, Score);
            if (equali(g_Steam[Player], Score[m_Steam]))
            { /// Found one.
                set_pdata_int(Player, m_CBasePlayer_iDeaths,   Score[m_Deaths]);
                set_pdata_int(Player, m_CBasePlayer_iObjScore, Score[m_Score]);
                set_pev      (Player, pev_frags,               Score[m_Frags]);
                ArrayDeleteItem(g_Scores, Iter);
                g_Entries--; /// Erase the entry once read & score restored.
                break;
            }
        }
    }
    return PLUGIN_CONTINUE;
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public DOD_ON_PLAYER_DISCONNECTED
{
    static Score[DoD_Score];
    if (g_Steam[Player][0] != EOS)
    { /// We need something unique!
        if (!g_keepFakePlayers && g_isPlayerFake[Player])
            return PLUGIN_CONTINUE; /// Skip fake players if needed.
        copy(Score[m_Steam], charsmax(Score[m_Steam]), g_Steam[Player]);
        Score[m_Deaths] = get_pdata_int(Player, m_CBasePlayer_iDeaths);
        Score[m_Score]  = get_pdata_int(Player, m_CBasePlayer_iObjScore);
        pev                            (Player, pev_frags, Score[m_Frags]);
        ArrayPushArray(g_Scores, Score);
        g_Steam[Player][0] = EOS;
        ++g_Entries; /// Entry stored.
    }
    return PLUGIN_CONTINUE;
}
