
#include <amxmodx>
#include <amxmisc>
#include <dodhacks>

public plugin_init()
{
	register_plugin("DoD Hacks: Key Values", "1.0", "Hattrick HKS (claudiuhks)");
}

public plugin_precache()
{
	new Buffer[256];
	get_configsdir(Buffer, charsmax(Buffer));
	add(Buffer, charsmax(Buffer), "/dod_hacks_keyvalues.ini");
	new Config = fopen(Buffer, "r");
	if (!Config)
	{
		set_fail_state("Error opening '%s'!", Buffer);
		return PLUGIN_HANDLED;
	}

	new Map[64], CfgMap[64], Class[64], Key[64], Val[64], bool: Add;
	get_mapname(Map, charsmax(Map));
	while (fgets(Config, Buffer, charsmax(Buffer)) > 0)
	{
		trim(Buffer);
		if (!Buffer[0] || Buffer[0] == ';' || Buffer[0] == '/')
		{
			continue;
		}
		if (parse(Buffer, CfgMap, charsmax(CfgMap), Class, charsmax(Class),
			Key, charsmax(Key), Val, charsmax(Val)) < 4)
		{
			Add = bool: equali(CfgMap, "@section_add");
			continue;
		}
		if (!CfgMap[0] || equali(CfgMap, Map))
		{
			if (false == Add)
			{
				DoD_AddKeyValDel(CfgMap, Class, Key, Val);
			}
			else
			{
				DoD_AddKeyValAdd(CfgMap, Class, Key, Val);
			}
		}
	}
	fclose(Config);
	return PLUGIN_CONTINUE;
}
