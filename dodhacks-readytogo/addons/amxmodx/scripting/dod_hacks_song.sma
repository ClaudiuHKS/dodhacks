
#include <amxmodx>

new bool: g_hasInTheArmy;

public plugin_init()
    register_plugin("DoD Hacks: Song", "1.0.1.1", "Hattrick HKS (claudiuhks)");

public plugin_precache()
    if (file_exists("sound/misc/in_the_army.mp3"))
    {
        precache_sound("misc/in_the_army.mp3");
        g_hasInTheArmy = true;
    }

public client_connect(Player)
    if (g_hasInTheArmy)
        switch (random_num(1, 10))
        {
            case  1: client_cmd(Player, "mp3 play media/Half-Life01.mp3");
            case  2: client_cmd(Player, "mp3 play media/Half-Life02.mp3");
            case  3: client_cmd(Player, "mp3 play media/Half-Life06.mp3");
            case  4: client_cmd(Player, "mp3 play media/Half-Life08.mp3");
            case  5: client_cmd(Player, "mp3 play media/Half-Life11.mp3");
            case  6: client_cmd(Player, "mp3 play media/Half-Life12.mp3");
            case  7: client_cmd(Player, "mp3 play media/Half-Life13.mp3");
            case  8: client_cmd(Player, "mp3 play media/Half-Life14.mp3");
            case  9: client_cmd(Player, "mp3 play media/Half-Life17.mp3");
            case 10: client_cmd(Player, "mp3 play sound/misc/in_the_army.mp3");
        }
    else
        switch (random_num(1, 9))
        {
            case 1: client_cmd(Player, "mp3 play media/Half-Life01.mp3");
            case 2: client_cmd(Player, "mp3 play media/Half-Life02.mp3");
            case 3: client_cmd(Player, "mp3 play media/Half-Life06.mp3");
            case 4: client_cmd(Player, "mp3 play media/Half-Life08.mp3");
            case 5: client_cmd(Player, "mp3 play media/Half-Life11.mp3");
            case 6: client_cmd(Player, "mp3 play media/Half-Life12.mp3");
            case 7: client_cmd(Player, "mp3 play media/Half-Life13.mp3");
            case 8: client_cmd(Player, "mp3 play media/Half-Life14.mp3");
            case 9: client_cmd(Player, "mp3 play media/Half-Life17.mp3");
        }

public client_putinserver(Player)
    client_cmd(Player, "mp3 stop");
