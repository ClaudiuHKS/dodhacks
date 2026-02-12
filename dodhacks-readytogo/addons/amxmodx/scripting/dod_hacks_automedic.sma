
#pragma ctrlchar '\' /// Original was '^'.

#include <amxmodx>
#include <amxmisc>
#include <dodhacks>
#include <hamsandwich>

new g_MaxPlayers;
new g_HealthPerTask;
new g_File[256];
new bool: g_InServer[33];
new bool: g_CanTrigger[33];
new bool: g_Sound;
new Float: g_DamageTime[33];
new Float: g_TaskInterval;
new Float: g_Delay;

public plugin_init()
{
	register_plugin("DoD Hacks: Auto Medic", "1.0", "Hattrick HKS (claudiuhks)");

	g_MaxPlayers = get_maxplayers();
	set_task(g_TaskInterval, "Task_HealPlayers", .flags = "b");

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
		{
			continue;
		}
		if (equali(Key, "@task_interval"))
		{
			g_TaskInterval = str_to_float(Val);
		}
		else if (equali(Key, "@hp_amount_per_task"))
		{
			g_HealthPerTask = str_to_num(Val);
		}
		else if (equali(Key, "@wound_delay"))
		{
			g_Delay = str_to_float(Val);
		}
		else if (equali(Key, "@play_sound"))
		{
			g_Sound = bool: str_to_num(Val);
		}
		else if (g_Sound && equali(Key, "@sound_file"))
		{
			copy(g_File, charsmax(g_File), Val);
		}
	}
	fclose(Config);

	if (g_Sound)
	{
		if (!g_File[0])
		{
			g_Sound = false;
		}
		else
		{
			while (contain(g_File, "\\") > -1)
			{
#if !defined replace_stringex
				replace(g_File, charsmax(g_File), "\\", "/");
#else
				replace_stringex(g_File, charsmax(g_File), "\\", "/", 1, 1, false);
#endif
			}
#if !defined replace_stringex
			replace(g_File, charsmax(g_File), "sound/", "");
#else
			replace_stringex(g_File, charsmax(g_File), "sound/", "", 6, 0, false);
#endif
			precache_sound(g_File);
#if !defined replace_stringex
			replace(g_File, charsmax(g_File), ".wav", "");
#else
			replace_stringex(g_File, charsmax(g_File), ".wav", "", 4, 0, false);
#endif
		}
	}
	return PLUGIN_CONTINUE;
}

public OnPlayerTakeDamage_Post(Player)
{
	g_DamageTime[Player] = get_gametime();
}

public Task_HealPlayers()
{
	static Player, Added, Float: Time;
	for (Player = 1, Time = get_gametime(); Player <= g_MaxPlayers; Player++)
	{
		if (g_InServer[Player] && Time - g_DamageTime[Player] > g_Delay)
		{ /// This call automatically excludes dead players.
			DoD_AddHealthIfWounded(Player, g_HealthPerTask, Added);
			if (g_Sound)
			{
				if (DoD_IsPlayerFullHealth(Player))
				{ /// pev_health == pev_maxhealth check.
					g_CanTrigger[Player] = true;
				}
				else if (Added && g_CanTrigger[Player])
				{
					client_cmd(Player, "SPK \"%s\"", g_File);
					g_CanTrigger[Player] = false;
				}
			}
		}
	}
}

public client_connect(Player)
{
	g_InServer[Player] = false;
	g_DamageTime[Player] = 0.0;
	g_CanTrigger[Player] = true;
}

public client_putinserver(Player)
{
	g_InServer[Player] = true;
}

public DOD_ON_PLAYER_DISCONNECTED
{
	g_InServer[Player] = false;
	g_DamageTime[Player] = 0.0;
	g_CanTrigger[Player] = true;
}
