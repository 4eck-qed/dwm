/* See LICENSE file for copyright and license details. */
#ifndef DWM_CONFIG_H
#define DWM_CONFIG_H

#include <X11/Xft/Xft.h>
#include <X11/XF86keysym.h>

typedef unsigned int uint;

#if __has_include("patch.def.h")
#include "patch.def.h"
#else
#include "patch.h"
#endif

/*
 * ##########################################################################################
 * #                                        APPEARANCE 										#
 * ########################################################################################## 
 */

#if P_BARPADDING
static const int vertpad                    = 5;    /* vertical padding of bar */
static const int sidepad                    = 10;   /* horizontal padding of bar */
#endif

#if P_STATUSPADDING
static const int horizpadbar                = 0;    /* horizontal padding for statusbar */
static const int vertpadbar                 = 10;   /* vertical padding for statusbar */
#endif

#if P_SWALLOW
static const Bool swallowfloating           = True; /* swallow floating windows by default */
#endif

#if P_SYSTRAY
static const uint systraypinning            = 0;    /* 0: sloppy systray follows sel mon    ; >0: pin systray to mon X */
static const uint systrayonleft             = 0;    /* 0: systray in the right corner       ; >0: systray on left of status text */
static const uint systrayspacing            = 2;    /* systray spacing */
static const Bool systraypinningfailfirst   = True; /* True: if pinning fails, display systray on the first mon ; False: display systray on the last monitor */
static const Bool showsystray               = True;
#endif

#if P_VANITYGAPS
/*
 * Note:
 * horizontal gap   = spacing in the vertical axis
 * vertical gap     = spacing in the horizontal axis
 * 
 * ╔═════════[screen edge]═══════╗
 * ║        ↕oh                  ║
 * ║    ┌────────┐    ┌────────┐ ║
 * ║←ov→│        │←iv→│        │ ║
 * ║    │        │    └────────┘ ║
 * ║    │        │        ↕ih    ║
 * ║    │        │    ┌────────┐ ║
 * ║    │        │    │        │ ║
 * ║    └────────┘    └────────┘ ║
 * ╚═════════════════════════════╝
 */
static const uint gappih                    = 0;    /* horiz inner gap between windows */
static const uint gappiv                    = 0;    /* vert inner gap between windows */
static const uint gappoh                    = 0;    /* horiz outer gap between windows and screen edge */
static const uint gappov                    = 0;    /* vert outer gap between windows and screen edge */
static int smartgaps                        = 0;    /* 1 means no outer gap when there is only one window */

#define FORCE_VSPLIT                        1       /* nrowgrid layout: force two clients to always split vertically */
#endif

#if P_VIEWONTAG
static const Bool viewontag                 = True; /* Switch view on tag switch */
#endif

#if P_WINICON
#define ICONSIZE                            30      /* icon size */
#define ICONSPACING                         5       /* space between icon and title */
#endif

#if P_TAGICON
static const Bool disablewinicon            = False;/* True: disable winicon on the client section */
static const Bool disabletagind             = True; /* True: disable that small rectangle that shows that a tag is occupied */
static const Bool overwritetag              = True; /* True: don't show tag (text) if icon is available */
static const Bool tagiconatstart            = True; /* True: place at tag start, False: place at tag end */
static const char tagiconsep[]              = ":";  /* Separator between tag and icon */
#endif

static const uint borderpx                  = 1;    /* border pixel of windows */
static const uint snap                      = 32;   /* snap pixel */
static const Bool showbar                   = True; /* False: no bar */
static const Bool topbar                    = True; /* False: bottom bar */
/* font size determines bar size */
static const char* fonts[] = {
    "Ubuntu:weight=regular:size=16:antialias=true:hinting=true",
    "Hack:size=16:antialias=true:autohint=true",
    "JoyPixels:size=16:antialias=true:autohint=true",
    "Hasklig:size=16:antialias=true:autohint=true"
};
static const char dmenufont[]   = "Hasklig:size=16";

#if P_ALPHA
static const uint baralpha      = 0xd0; /* lower value -> more transparent, 0xff = opaque (no transparency) */
static const uint borderalpha   = 0xd0;
static const uint alphas[][3]   = {
    /*               fg      bg        border     */
    [SchemeNorm] = { OPAQUE, baralpha, borderalpha },
	[SchemeSel]  = { OPAQUE, baralpha, borderalpha },
};
#endif

static const char col_gray0[]   = "#191D24";
static const char col_gray1[]   = "#2E3440";
static const char col_gray2[]   = "#3B4252";
static const char col_gray3[]   = "#434C5E";
static const char col_white0[]  = "#D8DEE9";
static const char col_white1[]  = "#ECEFF4";
static const char col_cyan[]    = "#005577";
static const char col_magenta[] = "#C800FF";
static const char* colors[][4]  = {
    /*                                                  |P_FLOATBORDERCOLOR|
     *               fg          bg         border      |float             | */
    [SchemeNorm] = { col_white0, col_gray1, col_gray2,   col_gray2 },
    [SchemeSel]  = { col_white1, col_gray3, col_magenta, col_cyan },
};

/*
 * ##########################################################################################
 * #                                        AUTOSTART 										#
 * ##########################################################################################
 */

#if P_AUTOSTART
static const char* const autostart[] = {
    "xsetroot", "-cursor_name", "left_ptr", NULL,
    /* "lxpolkit", NULL, */
    "dunst", NULL,
    "picom", NULL,
    "sh", "-c", "~/.xinitrc", "&", NULL,
    "sh" ,"-c", "~/.config/dwm/scripts/dbus", "&", NULL,
    "sh", "-c", "~/.config/dwm/scripts/autostart", "&", NULL,

#if P_STATUS2D
    "sh", "-c", "~/.config/dwm/scripts/status2d-bar", "&", NULL,
#endif

    /* terminate */
    NULL
};
#endif

/*
 * ##########################################################################################
 * #                                        WORKSPACES (TAGS) 								#
 * ##########################################################################################
 */

static const char* tags[]           = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };

#if P_ALTERNATIVETAGS
/* static const char* tagsalt[]        = { "𝟷", "𝟸", "𝟹", "𝟺", "𝟻", "𝟼", "𝟽", "𝟾", "𝟿", "𝟷𝟶", "𝟷𝟷", "𝟷𝟸" }; */
/* static const char* tagsalt[]        = { "Ⅰ", "Ⅱ", "Ⅲ", "Ⅳ", "Ⅴ", "Ⅵ", "Ⅶ", "Ⅷ", "Ⅸ", "Ⅹ", "Ⅺ", "Ⅻ" }; */
/* static const char* tagsalt[]        = { "➀", "➁", "➂", "➃", "➄", "➅", "➆", "➇", "➈", "➉", "⑪", "⑫" }; */
static const char* tagsalt[]        = { "１", "２", "３", "４", "５", "６", "７", "８", "９", "１０", "１１", "１２" };
static const int momentaryalttags   = 0; /* 1 means alttags will show only when key is held down*/
#endif

#if P_TAGLABELS
static const char ptagf[]           = "%s: %s";	/* format of a tag label */
static const char etagf[]           = "%s";	    /* format of an empty tag */
static const int lcaselbl           = 0;	    /* 1 means make tag label lowercase */	
#endif

/* 
 * ##########################################################################################
 * #                                        WINDOWRULES 									#
 * ##########################################################################################
 */
static const Rule rules[] = {
    /* xprop(1):
     *	WM_CLASS(STRING) = instance, class
     *	WM_NAME(STRING) = title
     */
    /* NOTE:
     *  due to count starting at 0 -> 0 = Workspace 1 => n = Workspace n+1
     *  to assign a window to a workspace set `tags mask` to: `1 << n`
     *  where 0 means unassigneda
     */

    /*                                                                              |       P_SWALLOW     |
     * class                    instance    title,          tags mask   isfloating  |isterminal  noswallow| monitor */
    { "St",                     NULL,       NULL,           0,          0,          1,          0,          -1 },
    { "kitty",                  NULL,       NULL,           0,          0,          1,          0,          -1 },
    { "alacritty",              NULL,       NULL,           0,          0,          1,          0,          -1 },
    { "Picture-in-Picture",     NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "Gimp",                   NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "Pinta",                  NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "Notes-up",               NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "vlc",                    NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "mpv",                    NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "Thunar",                 NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "thunar",                 NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "dolphin",                NULL,       NULL,           0,          1,          0,          0,          -1 },
    { "UnrealEditor",           NULL,       NULL,           0,          1,          0,          0,          -1 },
    // { "UnrealEditor",           NULL,       NULL,           0,          1,          0,          0,          0 },
    /* Right */
    /*  X */
    /*  1 */
    { "jetbrains-rider",        NULL,       NULL,           1 << 0,     0,          0,          0,          0 },
    /*  8 */
    { "lutris",                 NULL,       NULL,           1 << 7,     0,          0,          0,          0 },
    { "Lutris",                 NULL,       NULL,           1 << 7,     0,          0,          0,          0 },
    /*  9 */
    { "steam",                  NULL,       NULL,           1 << 8,     0,          0,          0,          0 },
    /*  10 */
    { "battle.net.exe",         NULL,       NULL,           1 << 9,     0,          0,          0,          0 },
    { "masterduel",             NULL,       NULL,           1 << 9,     1,          0,          0,          0 },
    /*  11 */
    { "gamescope",              NULL,       NULL,           1 << 10,    0,          0,          0,          0 },
    { "leagueclient.exe",       NULL,       NULL,           1 << 10,    1,          0,          0,          0 },
    { "leagueclientux.exe",     NULL,       NULL,           1 << 10,    1,          0,          0,          0 },
    { "league of legends.exe",  NULL,       NULL,           1 << 10,    0,          0,          0,          0 },
    { "xivlauncher",            NULL,       NULL,           1 << 10,    1,          0,          0,          0 },
    { "wowclassic.exe",         NULL,       NULL,           1 << 10,    1,          0,          0,          0 },
    { "wow.exe",                NULL,       NULL,           1 << 10,    1,          0,          0,          0 },
    /*  12 */
    { "steam_app_319510",       NULL,       NULL,           1 << 11,    1,          0,          1,          0 },
    { "PokerStars Lobby",       NULL,       NULL,           1 << 11,    1,          0,          1,          0 },
    { "cs2",                    NULL,       NULL,           1 << 11,    1,          0,          1,          0 },
    { "valheim.x86_64",         NULL,       NULL,           1 << 11,    0,          0,          1,          0 },
    /* Left */
    /*  X */
    { "exiled-exchange-2",      NULL,       NULL,           0,          0,          0,          0,          1 },
    /*  1 */
    { "brave-browser",          NULL,       NULL,           1 << 0,     0,          0,          0,          1 },
    { "Vivaldi-stable",         NULL,       NULL,           1 << 0,     0,          0,          0,          1 },
    /*  4 */
    { "discord",                NULL,       NULL,           1 << 3,     0,          0,          -1,         1 },
    { "WebCord",                NULL,       NULL,           1 << 3,     0,          0,          -1,         1 },

#if P_SWALLOW
    { NULL,                     NULL,       "Event Tester", 0,          0,          0,          1,          -1 }, /* xev */
#endif
};

/*
 * ##########################################################################################
 * #                                        LAYOUTS      									#
 * ##########################################################################################
 */

static const float mfact        = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster        = 1;    /* number of clients in master area */
static const int resizehints    = 1;    /* 1 means respect size hints in tiled resizals */

/*
 * True: will force focus on the fullscreen window
 * Maybe set this to False if you use fake fullscreen
 */
static const Bool lockfullscreen = True;

static const Layout layouts[] = {
    /* symbol   arrange fn              description */
    { "[T]",    tile,                   "Tiled" },      /* first entry is default */
    { "[F]",    NULL,                   "Floating" },   /* no layout function means floating behavior */
    { "[M]",    monocle,                "Monocle" },

#if P_FIBONACCI
#include "patch/fibonacci.c"
    { "[@]",    spiral,                 "Spiral" },
    { "[\\]",   dwindle,                "Dwindle" },
#elif P_VANITYGAPS
	{ "[@]",    spiral,                 "Spiral" },
	{ "[\\]",   dwindle,                "Dwindle" },
	{ "H[]",    deck,                   "Deck" },
	{ "TTT",    bstack,                 "B-Stack" },
	{ "===",    bstackhoriz,            "B-Stack Horizontal" },
	{ "HHH",    grid,                   "Grid" },
	{ "###",    nrowgrid,               "N-Row Grid" },
	{ "---",    horizgrid,              "Horizontal Grid" },
	{ ":::",    gaplessgrid,            "Gapless Grid" },
	{ "|M|",    centeredmaster,         "Centered Master" },
	{ ">M>",    centeredfloatingmaster, "Centered Floating Master" },
	{ NULL,     NULL,                   NULL },
#endif
};


/*
 * ##########################################################################################
 * #                                COMMANDS & DEFAULT PROGRAMS  							#
 * ##########################################################################################
 */
static const char term[]        = "alacritty";
static const char taskman[]     = "alacritty -e btop";
static const char browser[]     = "vivaldi";
static const char explorer[]    = "thunar";
static char dmenumon[2]         = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char* dmenucmd[] = {
    "dmenu_run",
    "-m",   dmenumon,   /* monitor */
    "-fn",  dmenufont,  /* font */
    "-nb",  col_gray0,  /* normal background */
    "-nf",  col_white0, /* normal foreground */
    "-sb",  col_gray3,  /* selected background */
    "-sf",  col_white1, /* selected foreground */
    NULL
};

static const char* termcmd[] = {term, NULL};

void tagandfocusmon(const Arg* arg) {
    tagmon(arg);
    focusmon(arg);
}

/*
 * ##########################################################################################
 * #                                        KEYBINDS 										#
 * ##########################################################################################
 */
#define MODKEY Mod4Mask
#define AltMask Mod1Mask

#define TAGKEYS(KEY, TAG) \
    { MODKEY,                       KEY, view,          {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY, toggleview,    {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY, tag,           {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY, toggletag,     {.ui = 1 << TAG} },

#define SHCMD(cmd) \
    { \
        .v = (const char*[]) { \
            "/bin/sh", "-c", cmd, NULL \
        } \
    }

#if P_DWMBLOCKS || P_STATUSCMD
#define STATUSBAR "dwmblocks"
#endif

static Key keys[] = {
    /* modifier                         key         function        argument */

    /* brightness and audio */
    { 0,                XF86XK_AudioLowerVolume,    spawn,          SHCMD("amixer sset Master 5%- unmute") },
	{ 0,                XF86XK_AudioMute,           spawn,          SHCMD("amixer sset Master $(amixer get Master | grep -q '\\[on\\]' && ""echo 'mute' || echo 'unmute')") },
	{ 0,                XF86XK_AudioRaiseVolume,    spawn,          SHCMD("amixer sset Master 5%+ unmute") },
	{ 0,				XF86XK_MonBrightnessUp,     spawn,          SHCMD("xbacklight -inc 10") },
	{ 0,				XF86XK_MonBrightnessDown,   spawn,          SHCMD("xbacklight -dec 10") },

#if P_ALTERNATIVETAGS
	{ MODKEY,                           XK_n,       togglealttag,   {0} },
#endif

#if P_FIBONACCI
	{ MODKEY,                           XK_r,       setlayout,      {.v = &layouts[3]} },
	{ MODKEY|ShiftMask,                 XK_r,       setlayout,      {.v = &layouts[4]} },
#elif P_VANITYGAPS
	{ MODKEY|ShiftMask,                 XK_h,       setcfact,       {.f = +0.25} },
	{ MODKEY|ShiftMask,                 XK_l,       setcfact,       {.f = -0.25} },
	{ MODKEY|ShiftMask,                 XK_o,       setcfact,       {.f =  0.00} },
	{ MODKEY|AltMask,                   XK_u,       incrgaps,       {.i = +1 } },
	{ MODKEY|AltMask|ShiftMask,         XK_u,       incrgaps,       {.i = -1 } },
	{ MODKEY|AltMask,                   XK_i,       incrigaps,      {.i = +1 } },
	{ MODKEY|AltMask|ShiftMask,         XK_i,       incrigaps,      {.i = -1 } },
	{ MODKEY|AltMask,                   XK_o,       incrogaps,      {.i = +1 } },
	{ MODKEY|AltMask|ShiftMask,         XK_o,       incrogaps,      {.i = -1 } },
	{ MODKEY|AltMask,                   XK_6,       incrihgaps,     {.i = +1 } },
	{ MODKEY|AltMask|ShiftMask,         XK_6,       incrihgaps,     {.i = -1 } },
	{ MODKEY|AltMask,                   XK_7,       incrivgaps,     {.i = +1 } },
	{ MODKEY|AltMask|ShiftMask,         XK_7,       incrivgaps,     {.i = -1 } },
	{ MODKEY|AltMask,                   XK_8,       incrohgaps,     {.i = +1 } },
	{ MODKEY|AltMask|ShiftMask,         XK_8,       incrohgaps,     {.i = -1 } },
	{ MODKEY|AltMask,                   XK_9,       incrovgaps,     {.i = +1 } },
	{ MODKEY|AltMask|ShiftMask,         XK_9,       incrovgaps,     {.i = -1 } },
	{ MODKEY|AltMask,                   XK_0,       togglegaps,     {0} },
	{ MODKEY|AltMask|ShiftMask,         XK_0,       defaultgaps,    {0} },
#endif

#if P_FULLSCREEN
    { MODKEY|ShiftMask,                 XK_m,       fullscreen,     {0} },
#endif

    { MODKEY,                           XK_Return,  spawn,          SHCMD(term) },
    { MODKEY,                           XK_t,       spawn,          SHCMD(taskman) },
    { MODKEY,                           XK_e,       spawn,          SHCMD(explorer) },
    { MODKEY,                           XK_b,       spawn,          SHCMD(browser) },
    { MODKEY,                           XK_p,       spawn,          SHCMD("flameshot full -p $HOME/Pictures/Screenshots/") },
    { MODKEY|ShiftMask,                 XK_p,       spawn,          SHCMD("flameshot gui -p $HOME/Pictures/Screenshots/") },
    { MODKEY|ShiftMask,                 XK_s,       spawn,          SHCMD("flameshot gui") },
    { MODKEY,                           XK_c,       spawn,          SHCMD("rofi -show drun") },
    
    { MODKEY,                           XK_r,       spawn,          {.v = dmenucmd} },
    { MODKEY,                           XK_x,       spawn,          {.v = termcmd} },
    { MODKEY|ShiftMask,                 XK_b,       togglebar,      {0} },
    { MODKEY|ShiftMask,                 XK_f,       togglefloating, {0} },
    { MODKEY,                           XK_Left,    focusmon,       {.i = -1} },
    { MODKEY,                           XK_Right,   focusmon,       {.i = +1} },
    { MODKEY|ShiftMask,                 XK_Left,    tagandfocusmon, {.i = -1} },
    { MODKEY|ShiftMask,                 XK_Right,   tagandfocusmon, {.i = +1} },
    { MODKEY,                           XK_j,       focusstack,     {.i = +1} },
    { MODKEY,                           XK_k,       focusstack,     {.i = -1} },
    { MODKEY,                           XK_i,       incnmaster,     {.i = +1} },
    { MODKEY,                           XK_d,       incnmaster,     {.i = -1} },
    { MODKEY,                           XK_h,       setmfact,       {.f = -0.05} },
    { MODKEY,                           XK_l,       setmfact,       {.f = +0.05} },
    { MODKEY,                           XK_z,       zoom,           {0} },
    { MODKEY,                           XK_Tab,     view,           {0} },
    { MODKEY,                           XK_f,       setlayout,      {.v = &layouts[1]} },
    { MODKEY,                           XK_space,   setlayout,      {0} },
    { MODKEY|ShiftMask,                 XK_q,       killclient,     {0} },
    { MODKEY|ControlMask|ShiftMask,     XK_q,       quit,           {0} },
    { MODKEY|ControlMask|ShiftMask,     XK_r,       spawn,          SHCMD("reboot") },
    { MODKEY|ControlMask|ShiftMask,     XK_p,       spawn,          SHCMD("shutdown now") },
    /* { MODKEY|ShiftMask,                 XK_period,  tagmon,         {.i = +1} }, */
    /* { MODKEY,                           XK_t,       setlayout,      {.v = &layouts[0]} }, */
    /* { MODKEY,                           XK_0,       view,           {.ui = ~0 } }, */
    /* { MODKEY|ShiftMask,                 XK_0,       tag,            {.ui = ~0 } }, */

    TAGKEYS(XK_1,       0)
    TAGKEYS(XK_2,       1)
    TAGKEYS(XK_3,       2)
    TAGKEYS(XK_4,       3)
    TAGKEYS(XK_5,       4)
    TAGKEYS(XK_6,       5)
    TAGKEYS(XK_7,       6)
    TAGKEYS(XK_8,       7)
    TAGKEYS(XK_9,       8)
    TAGKEYS(XK_0,       9)
    TAGKEYS(XK_minus,   10)
    TAGKEYS(XK_equal,   11)
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
 * ClkClientWin, or ClkRootWin */
/* placemouse options, choose which feels more natural:
 *    0 - tiled position is relative to mouse cursor
 *    1 - tiled postiion is relative to window center
 *    2 - mouse pointer warps to window center
 *
 * The moveorplace uses movemouse or placemouse depending on the floating
 * state of the selected client. Set up individual keybindings for the two
 * if you want to control these separately (i.e. to retain the feature to
 * move a tiled window into a floating position).
 */
static Button buttons[] = {
    /* click            event mask  button      function        argument */

#if !P_SYSTRAY
    { ClkLtSymbol,      0,          Button1,    setlayout,      {0} },
#endif

#if P_LAYOUTMENU
    { ClkLtSymbol,      0,          Button3,    layoutmenu,     {0} },
#elif !P_SYSTRAY
    { ClkLtSymbol,      0,          Button3,    setlayout,      {.v = &layouts[2]} },
#endif

    { ClkWinTitle,      0,          Button2,    zoom,           {0} },

#if P_STATUSCMD
	{ ClkStatusText,    0,          Button1,    sigstatusbar,   {.i = 1} },
	{ ClkStatusText,    0,          Button2,    sigstatusbar,   {.i = 2} },
	{ ClkStatusText,    0,          Button3,    sigstatusbar,   {.i = 3} },
#else
    { ClkStatusText,    0,          Button2,    spawn,          {.v = termcmd } },
#endif

#if P_PLACEMOUSE
	/* placemouse options, choose which feels more natural:
	 *    0 - tiled position is relative to mouse cursor
	 *    1 - tiled postiion is relative to window center
	 *    2 - mouse pointer warps to window center
	 *
	 * The moveorplace uses movemouse or placemouse depending on the floating state
	 * of the selected client. Set up individual keybindings for the two if you want
	 * to control these separately (i.e. to retain the feature to move a tiled window
	 * into a floating position).
	 */
	{ ClkClientWin,     MODKEY,     Button1,    moveorplace,    {.i = 0} },
#else
	{ ClkClientWin,     MODKEY,     Button1,    movemouse,      {0} },
#endif

    { ClkClientWin,     MODKEY,     Button2,    togglefloating, {0} },
    { ClkClientWin,     MODKEY,     Button3,    resizemouse, {0} },
    { ClkTagBar,        0,          Button1,    view, {0} },
    { ClkTagBar,        0,          Button3,    toggleview, {0} },
    { ClkTagBar,        MODKEY,     Button1,    tag, {0} },
    { ClkTagBar,        MODKEY,     Button3,    toggletag, {0} },
};

#endif /* DWM_CONFIG_H */
