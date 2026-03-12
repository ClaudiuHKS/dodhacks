
#pragma ctrlchar '\' /// Replace '^' with '\'.

#include <amxmodx>
#include <amxmisc>
#include <hamsandwich>
#include <dodhacks>

new g_maxPlayers; /// Maximum players the game server can handle.
new g_healthPerTask; /// Health to give during each task.
new g_soundFile[256]; /// Audio sound file, if any. << Populated only if 'g_Sound' is set to true >>.
new bool: g_isInServer[33]; /// Whether or not the player is in server.
new bool: g_isFakePlayer[33]; /// Whether or not the player is a BOT (a fake player).
new bool: g_canTrigger[33]; /// Whether or not the audio sound file may play for the player.
new bool: g_Sound; /// Whether or not to play an audio file during player's healing process.
new bool: g_speakToPlayer; /// If true, sound file will be played only to the player. Otherwise, players close to the healed will hear as well.
new Float: g_dmgTime[33]; /// Player's taken damage timestamp.
new Float: g_taskInterval; /// Interval in seconds between two tasks.
new Float: g_Delay; /// The delay in seconds between the moment of getting wounded and healing process.

public plugin_init()
{
    register_plugin("DoD Hacks: Auto Medic", "1.0.0.3", "Hattrick HKS (claudiuhks)");

    g_maxPlayers = get_maxplayers();
    set_task(g_taskInterval, "Task_HealPlayers", .flags = "b");

#if defined amxclient_cmd && defined RegisterHamPlayer
    RegisterHamPlayer(Ham_TakeDamage, "OnPlayerTakeDamage_Post", true);
#else
    RegisterHam(Ham_TakeDamage, "player", "OnPlayerTakeDamage_Post", true);
#endif
}

#if !defined client_disconnected
#define DOD_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** OLD AMX MOD X */
#else
#define DOD_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** NEW AMX MOD X */
#endif

public plugin_precache()
{
    new Buffer[256];
    get_configsdir(Buffer, charsmax(Buffer));
    add(Buffer, charsmax(Buffer), "/dod_hacks_automedic.ini");
    new Config = fopen(Buffer, "r");
    if (!Config)
    {
        set_fail_state("Error opening '%s'!", Buffer);
        return PLUGIN_HANDLED;
    }

    new Key[32], Val[256];
    while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
    {
        trim(Buffer);
        if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/' ||
            parse(Buffer, Key, charsmax(Key), Val, charsmax(Val)) < 2)
            continue;

        if (equali(Key, "@task_interval"))
            g_taskInterval = str_to_float(Val);
        else if (equali(Key, "@hp_amount_per_task"))
            g_healthPerTask = str_to_num(Val);
        else if (equali(Key, "@wound_delay"))
            g_Delay = str_to_float(Val);
        else if (equali(Key, "@play_sound"))
            g_Sound = bool: str_to_num(Val);
        else if (equali(Key, "@speak_player"))
            g_speakToPlayer = bool: str_to_num(Val);
        else if (g_Sound && equali(Key, "@sound_file"))
            copy(g_soundFile, charsmax(g_soundFile), Val);
    }
    fclose(Config);

    if (g_Sound)
    {
        if (!g_soundFile[0])
            g_Sound = false;
        else
        {
            while (contain(g_soundFile, "\\") > -1)
            { /// Convert all \ to / (which is cross-platform).
#if !defined replace_stringex
                replace(g_soundFile, charsmax(g_soundFile), "\\", "/");
#else
                replace_stringex(g_soundFile, charsmax(g_soundFile), "\\", "/", 1, 1, true);
#endif
            } /// Strip 'sound/' word if exists.
#if !defined replace_stringex
            strTransformEx(g_soundFile, "sound/", 0, 5, true); /// Transform the key to lower if insensitively contained.
            replace(g_soundFile, charsmax(g_soundFile), "sound/", "");
#else
            replace_stringex(g_soundFile, charsmax(g_soundFile), "sound/", "", 6, 0, false);
#endif
            precache_sound(g_soundFile); /// Make users download the file if needed.
            if (g_speakToPlayer)
            { /// Strip '.wav' word once the sound has been precached.
#if !defined replace_stringex
                strTransformEx(g_soundFile, ".wav", 1, 3, true); /// Transform the key to lower if insensitively contained.
                replace(g_soundFile, charsmax(g_soundFile), ".wav", "");
#else
                replace_stringex(g_soundFile, charsmax(g_soundFile), ".wav", "", 4, 0, false);
#endif
            }
        }
    }
    return PLUGIN_CONTINUE;
}

public OnPlayerTakeDamage_Post(Player)
    g_dmgTime[Player] = get_gametime();

public Task_HealPlayers()
{
    static Player, Added, Float: Time;
    for (Player = 1, Time = get_gametime(); Player <= g_maxPlayers; Player++)
    {
        if (g_isInServer[Player] && Time - g_dmgTime[Player] > g_Delay)
        { /// This call automatically excludes dead players.
            DoD_AddHealthIfWounded(Player, g_healthPerTask, Added);
            if (g_Sound)
            {
                if (DoD_IsPlayerFullHealth(Player))
                { /// pev_health == pev_maxhealth check.
                    g_canTrigger[Player] = true;
                }
                else if (Added && g_canTrigger[Player])
                {
                    if (g_speakToPlayer)
                    {
                        if (false == g_isFakePlayer[Player])
                            client_cmd(Player, "SPK \"%s\"", g_soundFile);
                    }
                    else
                        emit_sound(Player, CHAN_ITEM, g_soundFile, VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
                    g_canTrigger[Player] = false;
                }
            }
        }
    }
}

public client_putinserver(Player)
{
    g_isInServer[Player] = true;
    g_canTrigger[Player] = true;
    g_isFakePlayer[Player] = bool: is_user_bot(Player);
    g_dmgTime[Player] = 0.0;
}

public DOD_ON_PLAYER_DISCONNECTED
    g_isInServer[Player] = false;

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
