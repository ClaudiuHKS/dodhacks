
#pragma ctrlchar '\' /// Replace '^' with '\'.

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <hamsandwich>

#if !defined SOLID_NOT
#define      SOLID_NOT          0
#endif

#if !defined kRenderFxNone
#define      kRenderFxNone      0
#endif

#if !defined kRenderTransAlpha
#define      kRenderTransAlpha  4
#endif

#if !defined TE_BEAMPOINTS
#define      TE_BEAMPOINTS      0
#endif

#if !defined DEAD_DYING
#define      DEAD_DYING         1
#endif

#if !defined DMG_MORTAR
#define      DMG_MORTAR        (1 << 23)
#endif

#if !defined DMG_GRENADE
#define      DMG_GRENADE       (1 << 24)
#endif

#define      EXCLUDE_DMG_TYPES  DMG_CRUSH   | \
                                DMG_BLAST   | \
                                DMG_GRENADE | \
                                DMG_MORTAR /// You can add/ remove flags (hlsdk_const.inc file).

#define      EFFECTS_TO_REMOVE  EF_DIMLIGHT /** Mandatory flag: flashlight flag. */ | \
                                EF_LIGHT                                            | \
                                EF_MUZZLEFLASH                                      | \
                                EF_BRIGHTLIGHT /// You can add/ remove flags (hlsdk_const.inc file).

#define      DESIRED_CMD_ACCESS ADMIN_SLAY /// You can change this to ADMIN_KICK, ADMIN_BAN or ADMIN_RCON.

new bool: g_inUse                    =       false;
new g_Admin                          =           0;
new g_Victim                         =           0;
new g_Cheater                        =           0;
new g_msgFlashBat                    =          -1;
new g_msgFlashlight                  =          -1;
new g_Sprite                         =          -1;
new g_addToFullPack_Post             =          -1;
new g_cmdStart_Post                  =          -1;
new g_playbackEvent                  =          -1;
new g_emitSound                      =          -1;
new HamHook: g_playerKilled_Post     = HamHook: -1;
new HamHook: g_playerPostThink_Post  = HamHook: -1;
new HamHook: g_playerTakeDamage_Post = HamHook: -1;

new g_dmgType[33];
new g_batteryPerc[33];

public plugin_init()
{
    register_plugin(
        "Catch The Cheater",
        "1.0.0.3",
        "Hattrick HKS (claudiuhks)"
    );

    register_concmd(
        "amx_ctc_on",
        "On_ConsoleCommand_CTC_On",
        DESIRED_CMD_ACCESS,
        "<victim> <cheater> - turns victim transp. to cheater"
    );

    register_concmd(
        "amx_ctc_off",
        "On_ConsoleCommand_CTC_Off",
        DESIRED_CMD_ACCESS,
        "- all players now see everyone"
    );

#if defined amxclient_cmd && defined RegisterHamPlayer
    g_playerKilled_Post = RegisterHam(
        Ham_Killed,
        "player",
        "On_Ham_Player_Killed_Post",
        true,
        true /** "CZ Bots" support. */
    );

    g_playerPostThink_Post = RegisterHam(
        Ham_Player_PostThink,
        "player",
        "On_Ham_Player_PostThink_Post",
        true,
        true /** "CZ Bots" support. */
    );

    g_playerTakeDamage_Post = RegisterHam(
        Ham_TakeDamage,
        "player",
        "On_Ham_Player_TakeDamage_Post",
        true,
        true /** "CZ Bots" support. */
    );
#else
    g_playerKilled_Post = RegisterHam(
        Ham_Killed,
        "player",
        "On_Ham_Player_Killed_Post",
        true
    );

    g_playerPostThink_Post = RegisterHam(
        Ham_Player_PostThink,
        "player",
        "On_Ham_Player_PostThink_Post",
        true
    );

    g_playerTakeDamage_Post = RegisterHam(
        Ham_TakeDamage,
        "player",
        "On_Ham_Player_TakeDamage_Post",
        true
    );
#endif
    DisableHamForward(g_playerKilled_Post); /// Intentionally initially disabled.
    DisableHamForward(g_playerPostThink_Post); /// Intentionally initially disabled.
    DisableHamForward(g_playerTakeDamage_Post); /// Intentionally initially disabled.

    set_task(0.1, "On_Task_Admin_Beam", .flags = "b");

    g_msgFlashlight = get_user_msgid("Flashlight");

    if (g_msgFlashlight > 0)
    {
        register_message(g_msgFlashlight, "On_Message_Flashlight");
    }

    g_msgFlashBat = get_user_msgid("FlashBat");

    if (g_msgFlashBat > 0)
    {
        register_message(g_msgFlashBat, "On_Message_FlashBat");
    }
}

public plugin_precache()
{
    g_Sprite = precache_model("sprites/laserbeam.spr");
}

#if !defined client_disconnected
#define CTC_ON_PLAYER_DISCONNECTED client_disconnect(Player) /** Old AMX Mod X versions. */
#else
#define CTC_ON_PLAYER_DISCONNECTED client_disconnected(Player, bool: Drop, Msg[], Size) /** New AMX Mod X versions. */
#endif

public CTC_ON_PLAYER_DISCONNECTED
{
    if (g_Victim == Player ||
        Player == g_Cheater ||
        g_Admin == Player)
    {
        unregister_forward(
            FM_AddToFullPack,
            g_addToFullPack_Post,
            true
        );

        unregister_forward(
            FM_CmdStart,
            g_cmdStart_Post,
            true
        );

        unregister_forward(
            FM_EmitSound,
            g_emitSound,
            false
        );

        unregister_forward(
            FM_PlaybackEvent,
            g_playbackEvent,
            false
        );

        DisableHamForward(g_playerKilled_Post);
        DisableHamForward(g_playerPostThink_Post);
        DisableHamForward(g_playerTakeDamage_Post);

        g_inUse = false;

        g_Admin = 0;
        g_Victim = 0;
        g_Cheater = 0;

        g_addToFullPack_Post = -1;
        g_cmdStart_Post = -1;
        g_playbackEvent = -1;
        g_emitSound = -1;
    }

    return PLUGIN_CONTINUE;
}

public On_FM_AddToFullPack_Post(
    entityState,
    entityIndex,
    entityIndex_Copy,
    hostEntityIndex,
    hostFlags,
    isPlayer,
    Set
)
{
    if (isPlayer &&
        hostEntityIndex == g_Cheater &&
        entityIndex == g_Victim)
    {
        set_es(entityState, ES_RenderMode, kRenderTransAlpha);
        set_es(entityState, ES_RenderFx,   kRenderFxNone);
        set_es(entityState, ES_RenderAmt,  0);
        set_es(entityState, ES_Solid,      SOLID_NOT);
    } /// Targeted transparency (items and weapons are included).

    return FMRES_IGNORED;
}

public On_FM_EmitSound(
    Entity,
    Channel,
    const narrowSoundPath[],
    Float:
    Volume,
    Float: Attenuation,
    Flags,
    Pitch
)
{
    return Entity == g_Victim || pev(Entity, pev_owner) == g_Victim ?
        FMRES_SUPERCEDE : FMRES_IGNORED; /// Untargetted mute (items and weapons too).
}

public On_FM_PlaybackEvent(
    Flags,
    Invoker,
    evIdx,
    Float: Delay,
    Float: Pos[3],
    Float: Ang[3],
    Float: fParam1,
    Float: fparam2,
    nParam1,
    nParam2,
    bParam1,
    bParam2
)
{
    return Invoker == g_Victim || (pev_valid(Invoker) && pev(Invoker, pev_owner) == g_Victim) ?
        FMRES_SUPERCEDE : FMRES_IGNORED; /// Untargetted mute (items and weapons too).
}

public On_FM_CmdStart_Post(Player, userCommand)
{
    static Impulse;

    if (g_Victim == Player)
    {
        Impulse = get_uc(userCommand, UC_Impulse);

        if (Impulse == 201 || Impulse == 100)
        {
            set_uc(userCommand, UC_Impulse, 0);
        }

        Impulse = pev(Player, pev_impulse);

        if (Impulse == 201 || Impulse == 100)
        {
            set_pev(Player, pev_impulse, 0);
        }
    } /// Stop the sprays (201) and flashlight (100) [untargetted].

    return FMRES_IGNORED;
}

public On_Ham_Player_PostThink_Post(Player)
{
    if (Player == g_Victim)
    {
        set_pev(
            Player,
            pev_effects,
            pev(Player, pev_effects) & ~(EFFECTS_TO_REMOVE)
        ); /// Stop the flashlight effects (untargetted).
    }

    return HAM_IGNORED;
}

public On_Ham_Player_Killed_Post(Victim, Attacker, isSlaughter)
{
    static victimName[32], attackerName[32];

    if (g_Cheater == Attacker &&
        Victim == g_Victim &&
        !(g_dmgType[Victim] & (EXCLUDE_DMG_TYPES)))
    {
        get_user_name(Attacker, attackerName, charsmax(attackerName));
        get_user_name(Victim, victimName, charsmax(victimName));

        switch (g_Admin)
        { /// Notify the admin who issued the command.
            case 0:
            {
                server_print(
                    "* Victim \"%s\" got killed by \"%s\" while fully transp.",
                    victimName,
                    attackerName
                );
            }

            default:
            {
                client_print(
                    g_Admin,
                    print_chat,
                    "* \"%s\" got killed by \"%s\" while fully transp.",
                    victimName,
                    attackerName
                );

                client_print(
                    g_Admin,
                    print_center,
                    "* \"%s\" got killed by \"%s\" while fully transp.",
                    victimName,
                    attackerName
                );
            }
        }
    }

    return HAM_IGNORED;
}

public On_Ham_Player_TakeDamage_Post(Victim, Inflictor, Attacker, Float: Damage, dmgType)
{
    static victimName[32], attackerName[32];

    if (Damage > 0.0 &&
        Victim == g_Victim &&
        (Attacker == g_Cheater || Inflictor == g_Cheater))
    {
        g_dmgType[Victim] = dmgType;

        if (!(dmgType & (EXCLUDE_DMG_TYPES)))
        {
            get_user_name(g_Cheater, attackerName, charsmax(attackerName));
            get_user_name(Victim, victimName, charsmax(victimName));

            switch (g_Admin)
            { /// Notify the admin who issued the command.
                case 0:
                {
                    server_print(
                        "* \"%s\" got dmg. from \"%s\" while fully transp.",
                        victimName,
                        attackerName
                    );
                }

                default:
                {
                    client_print(
                        g_Admin,
                        print_center,
                        "* \"%s\" got dmg. from \"%s\" while fully transp.",
                        victimName,
                        attackerName
                    );
                }
            }
        }
    }

    return HAM_IGNORED;
}

public On_Task_Admin_Beam()
{
    static Float: From[3], Float: To[3];

    if (g_Admin && // Avoid sending beams if server executed the command.
        g_Victim != g_Admin &&
        (is_user_alive(g_Victim) || DEAD_DYING == pev(g_Victim, pev_deadflag)) &&
        pev(g_Admin, pev_origin, From) > 0 &&
        pev(g_Victim, pev_origin, To) > 0)
        sendBeam(g_Admin, From, To);

    return PLUGIN_CONTINUE;
}

public On_Message_FlashBat(Message, Destination, Entity)
{
    static argType;

    if (Entity > 0 &&
        get_msg_args() > 0)
    {
        argType = get_msg_argtype(1);

        if (ARG_CHAR  == argType ||
            ARG_LONG  == argType ||
            ARG_BYTE  == argType ||
            ARG_SHORT == argType)
        {
            g_batteryPerc[Entity] = get_msg_arg_int(1);
        }
    }

    return PLUGIN_CONTINUE;
}

public On_Message_Flashlight(Message, Destination, Entity)
{
    static argType;

    if (Entity > 0 &&
        get_msg_args() > 1)
    {
        argType = get_msg_argtype(2);

        if (ARG_CHAR  == argType ||
            ARG_LONG  == argType ||
            ARG_BYTE  == argType ||
            ARG_SHORT == argType)
        {
            g_batteryPerc[Entity] = get_msg_arg_int(2);
        }
    }

    return PLUGIN_CONTINUE;
}

public On_ConsoleCommand_CTC_On(Player, Level, Command)
{
    static Arg[32], adminName[32], adminSteam[32], victimName[32],
        victimSteam[32], cheaterName[32], cheaterSteam[32], Effects;

    if (!cmd_access(Player, Level, Command, 3))
    {
        return PLUGIN_HANDLED;
    }

    if (g_inUse)
    {
        console_print(Player, "* A player is already transp. to a cheater.");
        return PLUGIN_HANDLED;
    }

    read_argv(1, Arg, charsmax(Arg));
    g_Victim = cmd_target(Player, Arg, CMDTARGET_ALLOW_SELF);
    if (!g_Victim)
    {
        return PLUGIN_HANDLED;
    }

    read_argv(2, Arg, charsmax(Arg));
    g_Cheater = cmd_target(Player, Arg, CMDTARGET_ALLOW_SELF);
    if (!g_Cheater)
    {
        g_Victim = 0;

        return PLUGIN_HANDLED;
    }

    if (g_Cheater == g_Victim)
    {
        g_Victim = 0;
        g_Cheater = 0;

        console_print(Player, "* Victim and cheater have to be different players.");
        return PLUGIN_HANDLED;
    }

    if (g_msgFlashlight > 0 &&
        is_user_alive(g_Victim))
    {
        Effects = pev(g_Victim, pev_effects);

        if (Effects & (EF_DIMLIGHT) /** Flashlight flag. */)
        {
            set_pev(
                g_Victim,
                pev_effects,
                Effects & ~(EFFECTS_TO_REMOVE) /// This definition must include 'EF_DIMLIGHT' (mandatory).
            );

            message_begin(
                MSG_ONE,
                g_msgFlashlight,
                { 0, 0, 0 },
                g_Victim
            );

            write_byte(0);                       /// Flashlight off.
            write_byte(g_batteryPerc[g_Victim]); /// Battery perc.
            message_end();
        }
    }

    g_inUse = true;

    g_Admin = Player;

    g_addToFullPack_Post = register_forward(
        FM_AddToFullPack,
        "On_FM_AddToFullPack_Post",
        true
    );

    g_cmdStart_Post = register_forward(
        FM_CmdStart,
        "On_FM_CmdStart_Post",
        true
    );

    g_emitSound = register_forward(
        FM_EmitSound,
        "On_FM_EmitSound",
        false
    );

    g_playbackEvent = register_forward(
        FM_PlaybackEvent,
        "On_FM_PlaybackEvent",
        false
    );

    EnableHamForward(g_playerKilled_Post);
    EnableHamForward(g_playerPostThink_Post);
    EnableHamForward(g_playerTakeDamage_Post);

    get_user_name(Player, adminName, charsmax(adminName));
    get_user_authid(Player, adminSteam, charsmax(adminSteam));

    get_user_name(g_Victim, victimName, charsmax(victimName));
    get_user_authid(g_Victim, victimSteam, charsmax(victimSteam));

    get_user_name(g_Cheater, cheaterName, charsmax(cheaterName));
    get_user_authid(g_Cheater, cheaterSteam, charsmax(cheaterSteam));

    console_print(
        Player,
        "* Player '%s' is now fully transp. to '%s'.",
        victimName,
        cheaterName
    );

    log_amx(
        "Cmd: \"%s<%d><%s><>\" sets \"%s<%d><%s><>\" fully transp. to \"%s<%d><%s><>\"",
        adminName, get_user_userid(Player), adminSteam,
        victimName, get_user_userid(g_Victim), victimSteam,
        cheaterName, get_user_userid(g_Cheater), cheaterSteam
    );

    return PLUGIN_HANDLED;
}

public On_ConsoleCommand_CTC_Off(Player, Level, Command)
{
    static Name[32], Steam[32];

    if (!cmd_access(Player, Level, Command, 1))
    {
        return PLUGIN_HANDLED;
    }

    if (!g_inUse)
    {
        console_print(Player, "* All players already see everyone.");
        return PLUGIN_HANDLED;
    }

    unregister_forward(
        FM_AddToFullPack,
        g_addToFullPack_Post,
        true
    );

    unregister_forward(
        FM_CmdStart,
        g_cmdStart_Post,
        true
    );

    unregister_forward(
        FM_EmitSound,
        g_emitSound,
        false
    );

    unregister_forward(
        FM_PlaybackEvent,
        g_playbackEvent,
        false
    );

    DisableHamForward(g_playerKilled_Post);
    DisableHamForward(g_playerPostThink_Post);
    DisableHamForward(g_playerTakeDamage_Post);

    g_inUse = false;

    g_Admin = 0;
    g_Victim = 0;
    g_Cheater = 0;

    g_addToFullPack_Post = -1;
    g_cmdStart_Post = -1;
    g_playbackEvent = -1;
    g_emitSound = -1;

    console_print(Player, "* All players now see everyone.");

    get_user_name(Player, Name, charsmax(Name));
    get_user_authid(Player, Steam, charsmax(Steam));

    log_amx(
        "Cmd: \"%s<%d><%s><>\" sets all players visible to everyone",
        Name, get_user_userid(Player), Steam
    );

    return PLUGIN_HANDLED;
}

sendBeam(Player, const Float: From[3], const Float: To[3])
{
    engfunc(
        EngFunc_MessageBegin,
        MSG_ONE_UNRELIABLE,
        SVC_TEMPENTITY,
        From,
        Player
    );

    write_byte(TE_BEAMPOINTS /** 0 by default. */);
    engfunc(EngFunc_WriteCoord, To[0]);
    engfunc(EngFunc_WriteCoord, To[1]);
    engfunc(EngFunc_WriteCoord, To[2]);
    engfunc(EngFunc_WriteCoord, From[0]);
    engfunc(EngFunc_WriteCoord, From[1]);
    engfunc(EngFunc_WriteCoord, From[2]);
    write_short(g_Sprite);
    write_byte(0);   /// Starting frame.
    write_byte(1);   /// Frame rate.
    write_byte(1);   /// Life (10 means 1 second).
    write_byte(1);   /// Width.
    write_byte(0);   /// Noise amplitude.
    write_byte(255); /// Red.
    write_byte(255); /// Green.
    write_byte(255); /// Blue.
    write_byte(255); /// Brightness.
    write_byte(0);   /// Scroll speed.
    message_end();
}
