/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */

#include "patch.h"

#define EXP_BAKKEBY_ALPHA_SYSTRAY_FIX (1 && P_ALPHA) /* experimental, https://github.com/bakkeby/patches/blob/master/dwm/dwm-alpha-systray-6.3.diff */

#if P_TAGLABELS
#include <ctype.h> /* for making tab label lowercase, very tiny standard library */
#endif

#if P_WINICON
#include <limits.h>
#include <stdint.h>
#endif

#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"

/* conflicts */

#if P_RESIZEHERE && P_RESIZECORNERS
#error "Either disable P_RESIZEHERE or P_RESIZECORNERS"
#endif

#if P_FIBONACCI && P_VANITYGAPS
#error "Either disable P_FIBONACCI or P_VANITYGAPS"
#endif

#if P_STATUS2D && P_DWMBLOCKS
#error "Either disable P_STATUS2D or P_DWMBLOCKS"
#endif

/* dependencies */

#if P_TAGICON && !P_WINICON
#error "You need to enable P_WINICON too if you intend on using P_TAGICON"
#endif


#if P_SWALLOW
#include <X11/Xlib-xcb.h>
#include <xcb/res.h>
#ifdef __OpenBSD__
#include <sys/sysctl.h>
#include <kvm.h>
#endif /* __OpenBSD__ */
#endif /* P_SWALLOW */

/*
 * ##########################################################################
 * # 								MACROS									#
 * ##########################################################################
 */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))

#if P_PLACEMOUSE
#define INTERSECTC(x,y,w,h,z)   (MAX(0, MIN((x)+(w),(z)->x+(z)->w) - MAX((x),(z)->x)) \
                               * MAX(0, MIN((y)+(h),(z)->y+(z)->h) - MAX((y),(z)->y)))
#endif

#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

#if P_ALPHA
#define OPAQUE                  0xffU
#endif

#if P_FLOATBORDERCOLOR
#define ColFloat                3 /* index of float color */
#define COLOR_FIELDS 			4 /* amount of colors */
#else
#define COLOR_FIELDS 			3
#endif

#if P_SYSTRAY

#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */

#define SYSTEM_TRAY_REQUEST_DOCK    0
/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10
#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2
#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR
#endif /* P_SYSTRAY */

/*
 * ##########################################################################
 * # 								TYPES									#
 * ##########################################################################
 */

/* cursor */
enum {
	CurNormal,
	CurResize,
	CurMove,
	CurLast
};

/* color schemes */
enum {
	SchemeNorm,
	SchemeSel
};

/* EWMH atoms */
enum {
	NetSupported,
	NetWMName,

#if P_WINICON
	NetWMIcon,
#endif

	NetWMState,
	NetWMCheck,

#if P_SYSTRAY
	NetSystemTray,
	NetSystemTrayOP,
	NetSystemTrayOrientation,
	NetSystemTrayOrientationHorz,

#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	NetSystemTrayVisual,
	NetWMWindowTypeDock,
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */

#endif /* P_SYSTRAY */

    NetWMFullscreen,
	NetActiveWindow,
	NetWMWindowType,
    NetWMWindowTypeDialog,
	NetClientList,
	NetLast
};

#if P_SYSTRAY
/* Xembed atoms */
enum {
	Manager,
	Xembed,
	XembedInfo,
	XLast
};
#endif /* P_SYSTRAY */

/* default atoms */
enum {
	WMProtocols,
	WMDelete,
	WMState,
	WMTakeFocus,
	WMLast
};

/* clicks */
enum {
	ClkTagBar,
	ClkLtSymbol,
	ClkStatusText,
	ClkWinTitle,
    ClkClientWin,
	ClkRootWin,
	ClkLast
};

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;

#if P_VANITYGAPS
	float cfact;
#endif /* P_VANITYGAPS */

	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;

#if P_PLACEMOUSE
	int beingmoved;
#endif /* P_PLACEMOUSE */

#if P_SWALLOW
	int isterminal, noswallow;
	pid_t pid;
	Client *swallowing;
#endif /* P_SWALLOW */

#if P_WINICON
	unsigned int icw, ich;
	Picture icon;
#endif /* P_WINICON */

	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
	const char *description; /* custom addition */
} Layout;

#if P_PERTAG
typedef struct Pertag Pertag;
#endif

struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */

#if P_VANITYGAPS
	int gappih;           /* horizontal gap between windows */
	int gappiv;           /* vertical gap between windows */
	int gappoh;           /* horizontal outer gaps */
	int gappov;           /* vertical outer gaps */
#endif

	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	const Layout *lt[2];

#if P_ALTERNATIVETAGS
	unsigned int alttag;
#endif

#if P_PERTAG
	Pertag *pertag;
#endif
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	int isfloating;
	/* P_SWALLOW */
	int isterminal;
	int noswallow;
	/* --------- */
	int monitor;
} Rule;

#if P_SYSTRAY
typedef struct Systray   Systray;
struct Systray {
	Window win;
	Client *icons;
};
#endif

/*
 * ##########################################################################
 * # 							FN DECLARATIONS								#
 * ##########################################################################
 */

#if P_ALPHA
#include "patch/alpha.h"
#endif

#if P_ALTERNATIVETAGS
#include "patch/alternativetags.h"
#endif

#if P_AUTOSTART
#include "patch/coolautostart.h"
#endif

#if P_FULLSCREEN
#include "patch/fullscreen.h"
#endif

#if P_LAYOUTMENU
#include "patch/layoutmenu.h"
#endif

#if P_PLACEMOUSE
#include "patch/placemouse.h"
#endif

#if P_STATUS2D
#include "patch/status2d.h"
#endif

#if P_STATUSCMD
#include "patch/statuscmd.h"
#endif

#if P_SWALLOW
#include "patch/swallow.h"
#endif

#if P_SYSTRAY
#include "patch/systray.h"
#else
static int sendevent(Client *c, Atom proto);
#endif

#if P_VANITYGAPS
#include "patch/cfacts.h"
#include "patch/vanitygaps.h"
#else
static void tile(Monitor *m);
#endif

#if P_WINICON
#include "patch/winicon.h"
#endif

static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);


/*
 * ##########################################################################
 * # 								VARIABLES 								#
 * ##########################################################################
 */

#if P_ALPHA
static int useargb = 0;
static Visual *visual;
static int depth;
static Colormap cmap;
#endif

#if P_FULLSCREEN
Layout *last_layout;
#endif

#if P_STATUSCMD
static int statusw;
static int statussig;
static pid_t statuspid = -1;
#endif

#if P_SWALLOW
static xcb_connection_t *xcon;
#endif

static const char broken[] = "broken";
static char stext[
#if P_STATUS2D
	1024
#else
	256
#endif
];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar height */
static int lrpad;            /* sum of left and right padding for text */

#if P_BARPADDING
static int vp;               /* vertical padding for bar */
static int sp;               /* side padding for bar */
#endif

static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,

#if P_ALTERNATIVETAGS
	[KeyRelease] = keyrelease,
#endif

	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,

#if P_SYSTRAY
	[ResizeRequest] = resizerequest,
#endif

	[UnmapNotify] = unmapnotify,
};

#if P_SYSTRAY
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
#else
static Atom wmatom[WMLast], netatom[NetLast];
#endif

static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

#if P_SYSTRAY
static Systray *systray = NULL;

#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
static unsigned long systrayorientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
#endif
#endif

/* configuration, allows nested code to access above variables */
#include "config.h"

#if P_TAGLABELS || P_TAGICON
unsigned int tagw[LENGTH(tags)];
#endif

#if P_PERTAG
struct Pertag {
	unsigned int curtag, prevtag; 				/* current and previous tag */
	int nmasters[LENGTH(tags) + 1]; 			/* number of windows in master area */
	float mfacts[LENGTH(tags) + 1]; 			/* mfacts per tag */
	unsigned int sellts[LENGTH(tags) + 1]; 		/* selected layouts */
	const Layout *ltidxs[LENGTH(tags) + 1][2]; 	/* matrix of tags and layouts indexes  */
	int showbars[LENGTH(tags) + 1]; 			/* display bar for the current tag */
};
#endif


/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

#if P_AUTOSTART
/* dwm will keep pid's of processes from autostart array and kill them at quit */
static pid_t *autostart_pids;
static size_t autostart_len;
#endif

/* shared context that different parts of drawbar need */
struct DrawBarCtx {
	int x, w, tw, boxs, boxw;
	unsigned int occ, urg;
	Client *c;

#if P_FANCYBAR
	int mw, ew;
	unsigned int n;
#endif /* P_FANCYBAR */

#if P_SYSTRAY
	int stw;
#endif

#if P_TAGICON || P_WINICON
	unsigned int iconw;
#endif /* P_TAGICON || P_WINICON */

#if P_TAGICON
	const Client *tagclients[LENGTH(tags)];
	Bool taghasicon;
#endif /* P_TAGICON */

#if P_TAGLABELS
	char *masterclientontag[LENGTH(tags)];
#endif /* P_TAGLABELS */
};

/*
 * ##########################################################################
 * # 						FUNCTION IMPLEMENTATIONS 						#
 * ##########################################################################
 */

#if P_ALPHA
#include "patch/alpha.c"
#endif

#if P_ALTERNATIVETAGS
#include "patch/alternativetags.c"
#endif

#if P_AUTOSTART
#include "patch/coolautostart.c"
#endif

#if P_FULLSCREEN
#include "patch/fullscreen.c"
#endif

#if P_LAYOUTMENU
#include "patch/layoutmenu.c"
#endif

#if P_PLACEMOUSE
#include "patch/placemouse.c"
#endif

#if P_STATUS2D
#include "patch/status2d.c"
#endif

#if P_STATUSCMD
#include "patch/statuscmd.c"
#endif

#if P_SWALLOW
#include "patch/swallow.c"
#endif

#if P_SYSTRAY
#include "patch/systray.c"
#endif

#if P_VANITYGAPS
#include "patch/cfacts.c"
#include "patch/vanitygaps.c"
#else
void
tile(Monitor *m)
{
	unsigned int i, n, h, mw, my, ty;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	if (n > m->nmaster)
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		mw = m->ww;
	for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i);
			resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
			if (my + HEIGHT(c) < m->wh)
				my += HEIGHT(c);
		} else {
			h = (m->wh - ty) / (n - i);
			resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
			if (ty + HEIGHT(c) < m->wh)
				ty += HEIGHT(c);
		}
}
#endif /* P_VANITYGAPS */

#if P_WINICON
#include "patch/winicon.c"
#endif

/* Initializes the context and fulfills any preconditions required before drawing the tags. */
void
drawbar_init(Monitor *m, struct DrawBarCtx *ctx)
{
	ctx->tw = 0;
	ctx->boxs =  drw->fonts->h / 9;
	ctx->boxw = drw->fonts->h / 6 + 2;
	ctx->occ = 0;
	ctx->urg = 0;

	unsigned int i;
	int tw_base, tw_sub;

#if P_TAGICON || P_WINICON
	ctx->iconw = 0;

#endif /* P_TAGICON || P_WINICON */

#if P_TAGICON
	memset(ctx->tagclients, 0, sizeof(ctx->tagclients));
#endif

#if P_TAGLABELS
	memset(ctx.masterclientontag, 0, sizeof(ctx.masterclientontag));
#endif

#if P_FANCYBAR
	ctx.ew = 0;
	ctx.n = 0;
#endif /* P_FANCYBAR */

#if P_SYSTRAY
	ctx->stw = 0;

#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	if (showsystray && m == systraytomon(m)) {
		ctx->stw = getsystraywidth();
		drw_setscheme(drw, scheme[SchemeNorm]);
		drw_rect(drw, m->ww - ctx->stw, 0, ctx->stw, bh, 1, 1);
	}
#else
	if(showsystray && m == systraytomon(m) && !systrayonleft)
		stw = getsystraywidth();
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
#endif /* P_SYSTRAY */

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) { /* status is only drawn on selected monitor */
#if !P_STATUS2D
		drw_setscheme(drw, scheme[SchemeNorm]);
#endif /* !P_STATUS2D */

#if P_STATUSCMD && !P_STATUS2D
		x = 0;
		for (text = s = stext; *s; s++) {
			if ((unsigned char)(*s) < ' ') {
				ch = *s;
				*s = '\0';
				tw = TEXTW(text) - lrpad;
				drw_text(drw, m->ww - statusw + x, 0, tw, bh, 0, text, 0);
				x += tw;
				*s = ch;
				text = s + 1;
			}
		}
#endif /* P_STATUSCMD && !P_STATUS2D */

		tw_base = TEXTW(stext);
#if P_STATUS2D
		tw_base = m->ww;
#endif /* P_STATUS2D */

		tw_sub = lrpad + 2; /* 2px right padding */
#if P_STATUS2D
		tw_sub = drawstatusbar(m, bh, stext);
#elif P_SYSTRAY
#if P_STATUSPADDING
		tw_sub = -(lrpad / 2); /* subtraction of lrpad + 2 */
#else
		tw_sub = lrpad / 2 + 2; /* 2px extra right padding */
#endif /* P_STATUSPADDING */
#elif P_STATUSPADDING
		tw_sub = 0;
#endif /* P_STATUS2D */

		ctx->tw = tw_base - tw_sub;

#if !P_STATUS2D
		int x = m->ww - tw;
#if P_SYSTRAY
		x -= stw;
#endif /* P_SYSTRAY */
#if P_BARPADDING
		x -= 2 * sp;
#endif /* P_BARPADDING */

		lpad = 0;
#if P_SYSTRAY
		lpad = lrpad / 2 - 2;
#endif /* P_SYSTRAY */

		drw_text(drw, x, 0, tw, bh, lpad, stext, 0);
#endif /* !P_STATUS2D */

#if P_STATUSCMD
#if P_STATUS2D
		statusw = ctx->tw;
#else
		tw = statusw;
#endif /* P_STATUS2D */
#endif /* P_STATUSCMD */
	}

#if P_TAGLABELS
	for (i = 0; i < LENGTH(tags); i++)
		masterclientontag[i] = NULL;
#endif /* P_TAGLABELS */

#if P_SYSTRAY
	resizebarwin(m);
#endif /* P_SYSTRAY */

	for (ctx->c = m->clients; ctx->c; ctx->c = ctx->c->next) {
#if P_FANCYBAR
		if (ISVISIBLE(c))
			n++;
#endif /* P_FANCYBAR */

		ctx->occ |= ctx->c->tags
#if P_HIDEVACANTTAGS
 			   == TAGMASK ? 0 : c->tags
#endif /* P_HIDEVACANTTAGS */
		;

		if (ctx->c->isurgent)
			ctx->urg |= ctx->c->tags;

#if P_TAGLABELS || P_TAGICON
		for (i = 0; i < LENGTH(tags); i++) {

#if P_TAGLABELS
			if (!masterclientontag[i] && c->tags & (1<<i)) {
				XClassHint ch = { NULL, NULL };
				XGetClassHint(dpy, c->win, &ch);
				masterclientontag[i] = ch.res_class;
				if (lcaselbl)
					masterclientontag[i][0] = tolower(masterclientontag[i][0]);
			}
#endif /* P_TAGLABELS */

#if P_TAGICON
			if (!ctx->tagclients[i] && ctx->c->tags & (1<<i)) {
				ctx->tagclients[i] = ctx->c;
			}
#endif /* P_TAGICON */

		}
#endif /* P_TAGLABELS || P_TAGICON */
	}
	ctx->x = 0;
}

/* Draws the tags */
void
drawbar_tags(Monitor *m, struct DrawBarCtx *ctx)
{
	int w, lpad;
	unsigned int i;
	const char *tag_i;

#if P_ALTERNATIVETAGS
	int wdelta;
#endif

#if P_TAGICON || P_WINICON
	unsigned int iconw = 0;
#endif

#if P_TAGICON || P_TAGLABELS
	char tagbuf[64];
#endif

#if P_TAGICON
	Bool taghasicon;
#endif

	for (i = 0; i < LENGTH(tags); i++) {
		tag_i = tags[i];
#if P_ALTERNATIVETAGS
		tag_i = (selmon->alttag ? tagsalt[i] : tags[i]);
#endif /* P_ALTERNATIVETAGS */

#if P_TAGICON
		taghasicon = ctx->tagclients[i] && ctx->tagclients[i]->icon;
		iconw = 0;

		if (taghasicon) {
			if (overwritetag) {
				iconw = ctx->tagclients[i]->icw;
				tag_i = "";
			} else {
				iconw = ctx->tagclients[i]->icw + ICONSPACING;
				if (tagiconatstart) {
					snprintf(tagbuf, sizeof(tagbuf), "%s%s", tagiconsep, tag_i);
				} else {
					snprintf(tagbuf, sizeof(tagbuf), "%s%s", tag_i, tagiconsep);
				}
				tag_i = tagbuf;
			}
		}
#endif /* P_TAGICON */

#if P_HIDEVACANTTAGS
		/* Do not draw vacant tags */
		if(!(ctx->occ & 1 << i || m->tagset[m->seltags] & 1 << i))
			continue;
#endif /* P_HIDEVACANTTAGS */

		w = TEXTW(tag_i);

#if P_TAGLABELS
		if (ctx->masterclientontag[i])
			snprintf(tagbuf, sizeof(tagbuf), ptagf, tag_i, ctx->masterclientontag[i]);
		else
			snprintf(tagbuf, sizeof(tagbuf), etagf, tag_i);
		ctx->masterclientontag[i] = tagbuf;
		tagw[i] = w = TEXTW(ctx->masterclientontag[i]);

		tag_i = ctx->masterclientontag[i];
#endif /* P_TAGLABELS */

#if P_TAGICON /* ensure w isnt completely overriden after */
		tagw[i] = w += iconw;
#endif /* P_TAGICON */

#if P_ALTERNATIVETAGS /* ensures consistent lpad if tag has different width than alt tag */
		wdelta = selmon->alttag ? abs(TEXTW(tags[i]) - TEXTW(tagsalt[i])) / 2 : 0;
#endif /* P_ALTERNATIVETAGS */

		lpad = lrpad / 2;
#if P_ALTERNATIVETAGS
		lpad += wdelta;
#endif /* P_ALTERNATIVETAGS */
#if P_TAGICON
		if (overwritetag && taghasicon) {
			lpad = lrpad / 2; /* reset to base */
		} else if (tagiconatstart) {
			lpad += iconw;
		}
#endif /* P_TAGICON */
		
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, ctx->x, 0, w, bh, lpad, tag_i, ctx->urg & 1 << i);

#if P_TAGICON
		if (taghasicon) {
			drw_pic(
				drw,
				tagiconatstart ? ctx->x + lrpad / 2 : ctx->x + TEXTW(tag_i),
				(bh - ctx->tagclients[i]->ich) / 2,
				ctx->tagclients[i]->icw,
				ctx->tagclients[i]->ich,
				ctx->tagclients[i]->icon
			);
		}
#endif /* P_TAGICON */

#if !P_HIDEVACANTTAGS
		/* tag occupied indicator */
		if (
#if P_TAGICON
			!disabletagind &&
#endif
			(ctx->occ & 1 << i)
		) {
			drw_rect(drw, ctx->x + ctx->boxs, ctx->boxs, ctx->boxw, ctx->boxw, m == selmon && selmon->sel && selmon->sel->tags & 1 << i, ctx->urg & 1 << i);
		}
#endif /* !P_HIDEVACANTTAGS */

		ctx->x += w;
	}
}

/* Draws the layout symbol. */
void
drawbar_lsym(Monitor *m, struct DrawBarCtx *ctx)
{
	ctx->w = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeNorm]);
	ctx->x = drw_text(drw, ctx->x, 0, ctx->w, bh, lrpad / 2, m->ltsymbol, 0);
}

/* Draws the client/window title. */
void
drawbar_client(Monitor *m, struct DrawBarCtx *ctx)
{
	int lpad;
#if P_WINICON
	int iconw = 0;
#endif
#if P_FANCYBAR
	unsigned int i;
#endif

	if ((ctx->w = m->ww - ctx->tw - ctx->stw - ctx->x) > bh) {
#if P_FANCYBAR
		if (ctx->n > 0) {
			ctx->tw = TEXTW(m->sel->name) + lrpad;
			ctx->mw = (ctx->tw >= ctx->w || ctx->n == 1) ? 0 : (ctx->w - ctx->tw) / (ctx->n - 1);

			ctx->i = 0;
			for (ctx->c = m->clients; ctx->c; ctx->c = ctx->c->next) {
				if (!ISVISIBLE(ctx->c) || ctx->c == m->sel)
					continue;
				ctx->tw = TEXTW(ctx->c->name);
				if(ctx->tw < ctx->mw)
					ctx->ew += (ctx->mw - ctx->tw);
				else
					i++;
			}
			if (i > 0)
				ctx->mw += ctx->ew / i;

			for (ctx->c = m->clients; ctx->c; ctx->c = ctx->c->next) {
				if (!ISVISIBLE(c))
					continue;
				ctx->tw = MIN(m->sel == ctx->c ? ctx->w : ctx->mw, TEXTW(ctx->c->name));

				drw_setscheme(drw, scheme[m == selmon && m->sel == ctx->c ? SchemeSel : SchemeNorm]);
				if (ctx->tw > lrpad / 2) {
#if P_BARPADDING
					tw += -2 * sp;
#endif /* P_BARPADDING */
#if P_WINICON
					iconw = (ctx->c->icon ? ctx->c->icw + ICONSPACING : 0);
#if P_TAGICON
					if (disablewinicon)
						iconw = 0;
#endif /* P_TAGICON */
					/* tw += iconw; */ /* overlaps with status2d */
#endif /* P_WINICON */

					lpad = lrpad / 2;
#if P_WINICON
					lpad += iconw;
#endif /* P_WINICON */


					drw_text(drw, ctx->x, 0, ctx->tw, bh, lpad, ctx->c->name, 0);
				}

#if P_WINICON
				if (
#if P_TAGICON
					!disablewinicon &&
#endif /* P_TAGICON */
					ctx->c->icon
				) {
					drw_pic(drw, ctx->x + lrpad / 2, (bh - ctx->c->ich) / 2, c->icw, c->ich, c->icon);
				}
#endif /* P_WINICON */

				/* floating indicator */
				if (c->isfloating) {
					drw_rect(
						drw,
						x + boxs,
						boxs,
						boxw
/* #if P_BARPADDING */
/* 						 - 2 * sp */
/* #endif */
						,
						boxw,
						c->isfixed,
						0
					);
				}
				x += tw;
				w -= tw;
			}
		}
		drw_setscheme(drw, scheme[SchemeNorm]);
		drw_rect(drw, x, 0, w, bh, 1, 1);

#else /* !P_FANCYBAR */

		if (m->sel) {
			drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeNorm]);

#if P_WINICON
			iconw = (m->sel->icon ? m->sel->icw + ICONSPACING : 0);
#if P_TAGICON
			if (disablewinicon)
				iconw = 0;
#endif /* P_TAGICON */

			/* w += iconw; */ /* overlaps with status2d */
#endif /* P_WINICON */
#if P_BARPADDING
			ctx->w += -2 * sp;
#endif /* P_BARPADDING */

			lpad = lrpad / 2;
#if P_WINICON
			lpad += iconw;
#endif /* P_WINICON */


			/* application text */
			drw_text(drw, ctx->x, 0, ctx->w, bh, lpad, m->sel->name, 0);

#if P_WINICON
			if (
#if P_TAGICON
				!disablewinicon &&
#endif /* P_TAGICON */
				m->sel->icon
			) {
				drw_pic(drw, ctx->x + lrpad / 2, (bh - m->sel->ich) / 2, m->sel->icw, m->sel->ich, m->sel->icon);
			}
#endif /* P_WINICON */

			/* floating indicator */
			if (m->sel->isfloating)
				drw_rect(drw, ctx->x + ctx->boxs, ctx->boxs, ctx->boxw, ctx->boxw, m->sel->isfixed, 0);

		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_rect(drw, ctx->x, 0, ctx->w, bh, 1, 1);
		}
#endif /* P_FANCYBAR */
	}
}

/*
 * ##########################################################################
 * # 							DWM FUNCTIONS 								#
 * ##########################################################################
 */

void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = 0;
	c->tags = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
#if P_SWALLOW
			c->isterminal = r->isterminal;
			c->noswallow  = r->noswallow;
#endif
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		if (!c->hintsvalid)
			updatesizehints(c);
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof(m->ltsymbol));
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e)
{
	unsigned int i, x, click;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	click = ClkRootWin;
	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	if (ev->window == selmon->barwin) {
		i = x = 0;

#if P_HIDEVACANTTAGS
		unsigned int occ = 0;
		for(c = m->clients; c; c=c->next)
			occ |= c->tags == TAGMASK ? 0 : c->tags;
#endif

		do {
#if P_HIDEVACANTTAGS
			/* Do not reserve space for vacant tags */
			if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
				continue;
#endif

#if P_TAGLABELS || P_TAGICON
			x += tagw[i];
#else
			x += TEXTW(tags[i]);
#endif

		} while (ev->x >= x && ++i < LENGTH(tags));

		if (i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		} else if (ev->x < x + TEXTW(selmon->ltsymbol)) {
			click = ClkLtSymbol;
		}
		else if (ev->x > selmon->ww
#if P_STATUSCMD
			- statusw
#else
			- (int)TEXTW(stext)
#endif
#if P_SYSTRAY
			- getsystraywidth()
#endif
		) {
			click = ClkStatusText;

#if P_STATUSCMD
			char *text, *s, ch;

			statussig = 0;
			for (text = s = stext; *s && x <= ev->x; s++) {
				if ((unsigned char)(*s) < ' ') {
					ch = *s;
					*s = '\0';
					x += TEXTW(text) - lrpad;
					*s = ch;
					text = s + 1;
					if (x >= ev->x)
						break;

#if !P_STATUS2D
					/* reset on matching signal raw byte */
					if (ch == statussig)
						statussig = 0;
					else
						statussig = ch;
#else
					statussig = ch;
				} else if (*s == '^') {
					*s = '\0';
					x += TEXTW(text) - lrpad;
					*s = '^';
					if (*(++s) == 'f')
						x += atoi(++s);
					while (*(s++) != '^');
					text = s;
					s--;
				}
#endif /* !P_STATUS2D */
			}
#endif /* P_STATUSCMD */
		}
		else {
			click = ClkWinTitle;
		}
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;
	size_t i;

	view(&a);
	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	
#if P_SYSTRAY
	if (showsystray) {
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		while (systray->icons)
			removesystrayicon(systray->icons);
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */

		XUnmapWindow(dpy, systray->win);
		XDestroyWindow(dpy, systray->win);
		free(systray);
	}
#endif /* P_SYSTRAY */

	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);

#if P_STATUS2D
	for (i = 0; i < LENGTH(colors) + 1; i++)
#else
	for (i = 0; i < LENGTH(colors); i++)
#endif
		free(scheme[i]);

	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

#if P_SYSTRAY
	XWindowAttributes wa;
	XSetWindowAttributes swa;

	if (showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
		/* add systray icons */
		if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			if (!(c = (Client *)calloc(1, sizeof(Client))))
				die("fatal: could not malloc() %u bytes\n", sizeof(Client));
			if (!(c->win = cme->data.l[2])) {
				free(c);
				return;
			}
			c->mon = selmon;
			c->next = systray->icons;
			systray->icons = c;
			if (!XGetWindowAttributes(dpy, c->win, &wa)) {
#if !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
				/* use sane defaults */
				wa.width = bh;
				wa.height = bh;
				wa.border_width = 0;
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
			}
			c->x = c->oldx = c->y = c->oldy = 0;
			c->w = c->oldw = wa.width;
			c->h = c->oldh = wa.height;
			c->oldbw = wa.border_width;
			c->bw = 0;
			c->isfloating = True;
			/* reuse tags field as mapped status */
			c->tags = 1;
			updatesizehints(c);
			updatesystrayicongeom(c, wa.width, wa.height);
			XAddToSaveSet(dpy, c->win);
			XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
			XReparentWindow(dpy, c->win, systray->win, 0, 0);
			/* use parents background color */
			swa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
			XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
#if !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
			/* FIXME not sure if I have to send these events, too */
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
#endif /* !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
			XSync(dpy, False);
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
			setclientstate(c, NormalState);
			updatesystray(1);
#else
			resizebarwin(selmon);
			updatesystray();
			setclientstate(c, NormalState);
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
		}
		return;
	}
#endif /* P_SYSTRAY */

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */
#if !P_FAKEFULLSCREEN
				 && !c->isfullscreen
#endif /* !P_FAKEFULLSCREEN */
				 )));
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			updatebars();
			for (m = mons; m; m = m->next) {
#if !P_FAKEFULLSCREEN
				for (Client* c = m->clients; c; c = c->next) {
					if (c->isfullscreen) {
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
					}
				}
#endif /* !P_FAKEFULLSCREEN */

#if P_SYSTRAY
				resizebarwin(m);
#else
				XMoveResizeWindow(
					dpy,
					m->barwin,
					m->wx
#if P_BARPADDING
					+ sp
#endif /* P_BARPADDING */
					,
					m->by
#if P_BARPADDING
					+ vp
#endif /* P_BARPADDING */
					,
					m->ww
#if P_BARPADDING
					- 2 * sp
#endif /* P_BARPADDING */
					,
					bh
				);
#endif /* P_SYSTRAY */
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void)
{
	Monitor *m;

	m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;

#if P_VANITYGAPS
	m->gappih = gappih;
	m->gappiv = gappiv;
	m->gappoh = gappoh;
	m->gappov = gappov;
#endif

	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);

#if P_PERTAG
	m->pertag = ecalloc(1, sizeof(Pertag));
	m->pertag->curtag = m->pertag->prevtag = 1;

	unsigned int i;
	for (i = 0; i <= LENGTH(tags); i++) {
		m->pertag->nmasters[i] = m->nmaster;
		m->pertag->mfacts[i] = m->mfact;

		m->pertag->ltidxs[i][0] = m->lt[0];
		m->pertag->ltidxs[i][1] = m->lt[1];
		m->pertag->sellts[i] = m->sellt;

		m->pertag->showbars[i] = m->showbar;
	}
#endif

	return m;
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);

#if P_SWALLOW
	else if ((c = swallowingclient(ev->window)))
		unmanage(c->swallowing, 1);
#endif

#if P_SYSTRAY
	else if (
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		showsystray &&
	#endif
		(c = wintosystrayicon(ev->window))) {
		removesystrayicon(c);
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		updatesystray(1);
	#else
		resizebarwin(selmon);
		updatesystray();
	#endif
	}
#endif
}

void
detach(Client *c)
{
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

void
drawbar(Monitor *m)
{
	if (!m->showbar)
		return;

	struct DrawBarCtx ctx;
	drawbar_init(m, &ctx);
	drawbar_tags(m,	&ctx);
	drawbar_lsym(m, &ctx);
	drawbar_client(m, &ctx);

	/*
	 * ##########################################################################
	 * # 					MAP THE BAR ON THE SCREEN							#
	 * ##########################################################################
	 */
	ctx.w = m->ww;
#if P_SYSTRAY
	ctx.w -= ctx.stw;
#endif /* P_SYSTRAY */

	drw_map(drw, m->barwin, 0, 0, m->ww , bh);
}

void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);

#if P_SYSTRAY && EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	if (showsystray && !systraypinning)
		updatesystray(0);
#endif
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window))) {
		drawbar(m);
#if P_SYSTRAY
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		if (showsystray && m == systraytomon(m))
			updatesystray(0);
	#else
		if (m == selmon)
			updatesystray();
	#endif
#endif
	}
}

void
focus(Client *c)
{
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);

#if P_FLOATBORDERCOLOR	
		if(c->isfloating)
			XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColFloat].pixel);
		else
			XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
#else
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
#endif

		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);

#if P_CURSORWARP
	if (selmon->sel)
		XWarpPointer(dpy, None, selmon->sel->win, 0, 0, 0, 0, selmon->sel->w/2, selmon->sel->h/2);
#endif
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (
#if !P_FAKEFULLSCREEN /* unsure if thats the right move */
		selmon->sel->isfullscreen &&
#endif
		lockfullscreen /* < doesn't exist in patched version */
	))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
#if P_CURSORWARP
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);
#endif
	}
}

Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	Atom req = XA_ATOM;

#if P_SYSTRAY
	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	if (prop == xatom[XembedInfo])
		req = xatom[XembedInfo];
#endif

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;

#if P_SYSTRAY
		if (da == xatom[XembedInfo] && dl == 2)
			atom = ((Atom *)p)[1];
#endif

		XFree(p);
	}
	return atom;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void)
{
	updatenumlockmask();
	{
		unsigned int i, j, k;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		XDisplayKeycodes(dpy, &start, &end);
		syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
		if (!syms)
			return;
		for (k = start; k <= end; k++)
			for (i = 0; i < LENGTH(keys); i++)
				/* skip modifier codes, we do that ourselves */
				if (keys[i].keysym == syms[(k - start) * skip])
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(dpy, k,
							 keys[i].mod | modifiers[j],
							 root, True,
							 GrabModeAsync, GrabModeAsync);
		XFree(syms);
	}
}

void
incnmaster(const Arg *arg)
{
#if P_PERTAG
	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(selmon->nmaster + arg->i, 0);
#else
	selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
#endif

	arrange(selmon);
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
}

void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;

#if P_SYSTRAY
	if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
#else
	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
#endif
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;

#if P_SWALLOW
	Client *term = NULL;

	c->pid = winpid(w);
#endif

	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;

#if P_VANITYGAPS
	c->cfact = 1.0;
#endif

	
#if P_WINICON
	updateicon(c);
#endif

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	} else {
		c->mon = selmon;
		applyrules(c);

#if P_SWALLOW
		term = termforwin(c);
#endif
	}

	if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
		c->x = c->mon->wx + c->mon->ww - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
		c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);
	c->bw = borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);

#if P_FLOATBORDERCOLOR
	if(c->isfloating)
		XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColFloat].pixel);
	else
		XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
#else
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
#endif

	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating) {
		XRaiseWindow(dpy, c->win);

#if P_FLOATBORDERCOLOR
		XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColFloat].pixel);
#endif
	}
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);

#if P_CURSORWARP
	if (c && c->mon == selmon)
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);
#endif

#if P_SWALLOW		
	if (term)
		swallow(term, c);
#endif

	focus(NULL);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

#if P_SYSTRAY
	Client *i;
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	if (showsystray && (i = wintosystrayicon(ev->window))) {
		sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
		updatesystray(1);
	}
	#else
	if ((i = wintosystrayicon(ev->window))) {
		sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
		resizebarwin(selmon);
		updatesystray();
	}
	#endif
#endif

	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
monocle(Monitor *m)
{
	unsigned int n = 0;
	Client *c;

	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
			n++;
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;

#if !P_FAKEFULLSCREEN
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
#endif

	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

#if P_SYSTRAY
	if (
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		showsystray &&
	#endif
		(c = wintosystrayicon(ev->window))) {
		if (ev->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
			updatesystrayicongeom(c, c->w, c->h);
		}
		else
			updatesystrayiconstate(c, ev);
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		updatesystray(1);
	#else
		resizebarwin(selmon);
		updatesystray();
	#endif
	}
#endif

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
#if P_WINICON
		else if (ev->atom == netatom[NetWMIcon]) {
			updateicon(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
#endif

		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
quit(const Arg *arg)
{
#if P_AUTOSTART
	size_t i;

	/* kill child processes */
	for (i = 0; i < autostart_len; i++) {
		if (0 < autostart_pids[i]) {
			kill(autostart_pids[i], SIGTERM);
			waitpid(autostart_pids[i], NULL, 0);
		}
	}
#endif

	running = 0;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;

#if P_PLACEMOUSE
	if (c->beingmoved)
		return;
#endif

	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
	int nw, nh;

#if P_RESIZEHERE
	int x, y, ocw, och;
#else
	int ocx, ocy;
#endif

	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;


	if (!(c = selmon->sel))
		return;

#if !P_FAKEFULLSCREEN
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
#endif

	restack(selmon);

#if P_RESIZEHERE
	ocw = c->w;
	och = c->h;
#else
	ocx = c->x;
	ocy = c->y;
#endif

#if P_RESIZECORNERS
	int ocx2, ocy2, nx, ny;
	int horizcorner, vertcorner;
	int di;
	unsigned int dui;
	Window dummy;

	ocx2 = c->x + c->w;
	ocy2 = c->y + c->h;
#endif

	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;

#if P_RESIZECORNERS
	if (!XQueryPointer (dpy, c->win, &dummy, &dummy, &di, &di, &nx, &ny, &dui))
		return;
	horizcorner = nx < c->w / 2;
	vertcorner  = ny < c->h / 2;
	XWarpPointer (dpy, None, c->win, 0, 0, 0, 0,
			horizcorner ? (-c->bw) : (c->w + c->bw -1),
			vertcorner  ? (-c->bw) : (c->h + c->bw -1));
#elif P_RESIZEHERE
	if(!getrootptr(&x, &y))
		return;
#else
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
#endif

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

#if P_RESIZECORNERS
			nx = horizcorner ? ev.xmotion.x : c->x;
			ny = vertcorner ? ev.xmotion.y : c->y;
			nw = MAX(horizcorner ? (ocx2 - nx) : (ev.xmotion.x - ocx - 2 * c->bw + 1), 1);
			nh = MAX(vertcorner ? (ocy2 - ny) : (ev.xmotion.y - ocy - 2 * c->bw + 1), 1);
#elif P_RESIZEHERE
			nw = MAX(ocw + (ev.xmotion.x - x), 1);
			nh = MAX(och + (ev.xmotion.y - y), 1);
#else
			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
#endif

			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
#if P_RESIZECORNERS
				resize(c, nx, ny, nw, nh, 1);
#else
				resize(c, c->x, c->y, nw, nh, 1);
#endif

			break;
		}
	} while (ev.type != ButtonRelease);

#if P_RESIZECORNERS
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
		      horizcorner ? (-c->bw) : (c->w + c->bw - 1),
		      vertcorner ? (-c->bw) : (c->h + c->bw - 1));
#elif !P_RESIZEHERE
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
#endif

	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void)
{
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	while (running && !XNextEvent(dpy, &ev))
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

int
#if P_SYSTRAY
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
#else
sendevent(Client *c, Atom proto)
#endif
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;
	
	/* maybe patched */
	Bool mp_cond = True;
	int mp_mask = NoEventMask;

#if P_SYSTRAY
	Atom mt;
	Window mp_win = w;
	mp_mask = mask;

	if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
		mt = wmatom[WMProtocols];
		mp_cond = True;
	} else {
		mp_cond = False;
	}
#else
	Window mp_win = c->win;
#endif

	if (mp_cond && XGetWMProtocols(dpy, mp_win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}

#if P_SYSTRAY
	else {
		exists = True;
		mt = proto;
    }
#endif

	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = mp_win;
		ev.xclient.format = 32;

#if P_SYSTRAY
		ev.xclient.message_type = mt;
		ev.xclient.data.l[0] = d0;
		ev.xclient.data.l[1] = d1;
		ev.xclient.data.l[2] = d2;
		ev.xclient.data.l[3] = d3;
		ev.xclient.data.l[4] = d4;
#else
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
#endif
		
		XSendEvent(dpy, mp_win, False, mp_mask, &ev);
	}
	return exists;
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}

#if P_SYSTRAY
	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
#else
	sendevent(c, wmatom[WMTakeFocus]);
#endif
}

void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;

#if !P_FAKEFULLSCREEN
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
#endif
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;

#if !P_FAKEFULLSCREEN
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
#endif
	}
}

void
setlayout(const Arg *arg)
{
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt]) {
#if P_PERTAG
		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
#else
		selmon->sellt ^= 1;
#endif
	}

	if (arg && arg->v) {
#if P_PERTAG
		selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] = (Layout *)arg->v;
#else
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
#endif
	}

	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;

#if P_PERTAG
	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
#else
	selmon->mfact = f;
#endif /* P_PERTAG */

	arrange(selmon);
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;
	struct sigaction sa;

	/* do not transform children into zombies when they terminate */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);

#if P_AUTOSTART
	pid_t pid;

	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		pid_t *p, *lim;

		if (!(p = autostart_pids))
			continue;
		lim = &p[autostart_len];

		for (; p < lim; p++) {
			if (*p == pid) {
				*p = -1;
				break;
			}
		}

	}
#else
	/* clean up any zombies (inherited from .xinitrc etc) immediately */
	while (waitpid(-1, NULL, WNOHANG) > 0);
#endif /* P_AUTOSTART */

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);

#if P_ALPHA
	xinitvisual();
	drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
#else
	drw = drw_create(dpy, screen, root, sw, sh);
#endif /* P_ALPHA */

	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");

#if P_STATUSPADDING /*&& !P_STATUS2D*/
	lrpad = drw->fonts->h + horizpadbar;
#else
	lrpad = drw->fonts->h;
#endif /* P_STATUSPADDING */


#if P_STATUSPADDING	
	bh = drw->fonts->h + vertpadbar;
#else
	bh = drw->fonts->h + 2;
#endif /* P_STATUSPADDING */

#if P_BARPADDING
	sp = sidepad;
	vp = (topbar == 1) ? vertpad : - vertpad;
#endif /* P_BARPADDING */

	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
#if P_SYSTRAY
	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	netatom[NetSystemTrayVisual] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_VISUAL", False);
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
#endif /* P_SYSTRAY */
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
#if P_WINICON
	netatom[NetWMIcon] = XInternAtom(dpy, "_NET_WM_ICON", False);
#endif
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
#if P_SYSTRAY && EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	netatom[NetWMWindowTypeDock] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
#endif /* P_SYSTRAY && EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);

#if P_SYSTRAY
	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
#endif /* P_SYSTRAY */

	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);

	/* init appearance */
#if P_STATUS2D	
	scheme = ecalloc(LENGTH(colors) + 1, sizeof(Clr *));
	scheme[LENGTH(colors)] = drw_scm_create(
		drw,
		colors[0],
#if P_ALPHA
		alphas[0],
#endif /* P_ALPHA */
		COLOR_FIELDS
	);
#else
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
#endif /* P_STATUS2D */

	for (i = 0; i < LENGTH(colors); i++) {
		scheme[i] = drw_scm_create(
			drw,
			colors[i],
#if P_ALPHA
			alphas[i],
#endif
			COLOR_FIELDS
		);
	}

#if P_SYSTRAY
	/* init system tray */
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	if (showsystray)
		updatesystray(0);
#else
	updatesystray();
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
#endif /* P_SYSTRAY */

	/* init bars */
	updatebars();
	updatestatus();
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
	focus(NULL);
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating)
#if !P_FAKEFULLSCREEN
			&& !c->isfullscreen
#endif
		)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
spawn(const Arg *arg)
{
	struct sigaction sa;

	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();

		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

		execvp(((char **)arg->v)[0], (char **)arg->v);
		die("dwm: execvp '%s' failed:", ((char **)arg->v)[0]);
	}
}

void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		focus(NULL);
		arrange(selmon);

#if P_VIEWONTAG
		if(viewontag && ((arg->ui & TAGMASK) != TAGMASK))
			view(arg);
#endif
	}
}

void
tagmon(const Arg *arg)
{
	if (!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

void
togglebar(const Arg *arg)
{
#if P_PERTAG
	selmon->showbar = selmon->pertag->showbars[selmon->pertag->curtag] = !selmon->showbar;
#else
	selmon->showbar = !selmon->showbar;
#endif

	updatebarpos(selmon);

#if P_SYSTRAY && !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	resizebarwin(selmon);
	if (showsystray) {
		XWindowChanges wc;
		if (!selmon->showbar)
			wc.y = -bh;
		else if (selmon->showbar) {
			wc.y = 0;
			if (!selmon->topbar)
				wc.y = selmon->mh - bh;
		}
		XConfigureWindow(dpy, systray->win, CWY, &wc);
	}
#endif
#if !P_SYSTRAY || P_BARPADDING || EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	XMoveResizeWindow(
		dpy,
		selmon->barwin,
		selmon->wx
#if P_BARPADDING
		 + sp
#endif /* P_BARPADDING */
		,
		selmon->by
#if P_BARPADDING
		 + vp
#endif /* P_BARPADDING */
		,
		selmon->ww
#if P_BARPADDING
		 - 2 * sp
#endif /* P_BARPADDING */
		,
		bh
	);
#endif /* !P_SYSTRAY || P_BARPADDING || EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */

#if P_SYSTRAY && EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	if (showsystray) {
		XWindowChanges wc;
		if (!selmon->showbar)
			wc.y = -bh;
		else if (selmon->showbar) {
#if P_BARPADDING
			wc.y = vp;
			if (!selmon->topbar)
				wc.y = selmon->mh - bh + vp;
#else
			wc.y = 0;
			if (!selmon->topbar)
				wc.y = selmon->mh - bh;
#endif /* P_BARPADDING  */
		}
		XConfigureWindow(dpy, systray->win, CWY, &wc);
	}
#endif

	arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;

#if !P_FAKEFULLSCREEN
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
#endif

	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	
#if P_FLOATBORDERCOLOR
	if (selmon->sel->isfloating)
		XSetWindowBorder(dpy, selmon->sel->win, scheme[SchemeSel][ColFloat].pixel);
	else
		XSetWindowBorder(dpy, selmon->sel->win, scheme[SchemeSel][ColBorder].pixel);
#endif

	if(selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		selmon->sel->tags = newtags;
		focus(NULL);
		arrange(selmon);
	}
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

	if (newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;

#if P_PERTAG
		if (newtagset == ~0) {
			selmon->pertag->prevtag = selmon->pertag->curtag;
			selmon->pertag->curtag = 0;
		}

		/* test if the user did not select the same tag */
		if (!(newtagset & 1 << (selmon->pertag->curtag - 1))) {
			selmon->pertag->prevtag = selmon->pertag->curtag;

			int i;
			for (i = 0; !(newtagset & 1 << i); i++) ;
			selmon->pertag->curtag = i + 1;
		}

		/* apply settings for this view */
		selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
		selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
		selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
		selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];

		if (selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
			togglebar(NULL);
#endif

		focus(NULL);
		arrange(selmon);
	}
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);

#if P_FLOATBORDERCOLOR
    if (c->isfloating)
        XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColFloat].pixel);
    else
	    XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
#else
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
#endif

	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	XWindowChanges wc;

#if P_SWALLOW
	if (c->swallowing) {
		unswallow(c);
		return;
	}

	Client *s = swallowingclient(c->win);
	if (s) {
		free(s->swallowing);
		s->swallowing = NULL;
		arrange(m);
		focus(NULL);
		return;
	}
#endif

	detach(c);
	detachstack(c);

#if P_WINICON
	freeicon(c);
#endif

	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XSelectInput(dpy, c->win, NoEventMask);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);

#if P_SWALLOW
	if (!s) {
		arrange(m);
		focus(NULL);
		updateclientlist();
	}
#else
	focus(NULL);
	updateclientlist();
	arrange(m);
#endif

#if P_CURSORWARP
	if (m == selmon && m->sel)
		XWarpPointer(dpy, None, m->sel->win, 0, 0, 0, 0,
		             m->sel->w/2, m->sel->h/2);
#endif
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	}

#if P_SYSTRAY
	else if (
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
			 showsystray &&
	#endif
			 (c = wintosystrayicon(ev->window))) {
		/* KLUDGE! sometimes icons occasionally unmap their windows, but do
		 * _not_ destroy them. We map those windows back */
		XMapRaised(dpy, c->win);
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		updatesystray(1);
	#else
		updatesystray();
	#endif
	}
#endif
}

void
updatebars(void)
{
	unsigned int w;
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
#if P_ALPHA
		.background_pixel = 0,
		.border_pixel = 0,
		.colormap = cmap,
#else
		.background_pixmap = ParentRelative,
#endif
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"dwm", "dwm"};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		w = m->ww;

#if P_SYSTRAY
		if (showsystray && m == systraytomon(m))
			w -= getsystraywidth();
#endif /* P_SYSTRAY */

		m->barwin = XCreateWindow(
			dpy,
			root,
			m->wx
#if P_BARPADDING
			 + sp
#endif /* P_BARPADDING */
			,
			m->by
#if P_BARPADDING
			 + vp
#endif /* P_BARPADDING */
			,
			w
#if P_BARPADDING
			 - 2 * sp
#endif /* P_BARPADDING */
			,
			bh,
			0,
#if P_ALPHA
			depth,
			InputOutput,
			visual,
			CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask,
#else
			DefaultDepth(dpy, screen),
			CopyFromParent,
			DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask,
#endif /* P_ALPHA */
			&wa
		);

		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);

#if P_SYSTRAY
		if (showsystray && m == systraytomon(m))
			XMapRaised(dpy, systray->win);
#endif /* P_SYSTRAY */

		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh = m->wh
#if P_BARPADDING
				 - vertpad
#endif
				 - bh;

		m->by = m->topbar
			? m->wy
			: m->wy + m->wh
#if P_BARPADDING
			 + vertpad
#endif
		;

		m->wy = m->topbar 
			? m->wy + bh
#if P_BARPADDING
			 + vp
#endif
			: m->wy;
	} else {
		m->by = -bh
#if P_BARPADDING
			 - vp
#endif
		;
	}
}

void
updateclientlist(void)
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
}

int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;

		/* new monitors if nn > n */
		for (i = n; i < nn; i++) {
			for (m = mons; m && m->next; m = m->next);
			if (m)
				m->next = createmon();
			else
				mons = createmon();
		}
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				m->num = i;
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				updatebarpos(m);
			}
		/* removed monitors if n > nn */
		for (i = nn; i < n; i++) {
			for (m = mons; m && m->next; m = m->next);
			while ((c = m->clients)) {
				dirty = 1;
				m->clients = c->next;
				detachstack(c);
				c->mon = mons;
				attach(c);
				attachstack(c);
			}
			if (m == selmon)
				selmon = mons;
			cleanupmon(m);
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	c->hintsvalid = 1;
}

void
updatestatus(void)
{
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext))) {
		strcpy(stext, "dwm-"VERSION);
#if P_STATUSCMD && !P_STATUS2D
		statusw = TEXTW(stext) - lrpad + 2;
	} else {
		char *text, *s, ch;

		statusw  = 0;
		for (text = s = stext; *s; s++) {
			if ((unsigned char)(*s) < ' ') {
				ch = *s;
				*s = '\0';
				statusw += TEXTW(text) - lrpad;
				*s = ch;
				text = s + 1;
			}
		}
		statusw += TEXTW(text) - lrpad + 2;

	}
#else
	}
#endif

	drawbar(selmon);

#if P_SYSTRAY && !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	updatesystray();
#endif
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

void
view(const Arg *arg)
{
	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (arg->ui & TAGMASK) {
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;

#if P_PERTAG
		selmon->pertag->prevtag = selmon->pertag->curtag;

		if (arg->ui == ~0)
			selmon->pertag->curtag = 0;
		else {
			unsigned int i = 0;
			for (i = 0; !(arg->ui & 1 << i); i++) ;
			selmon->pertag->curtag = i + 1;
		}
	} else {
		unsigned int tmptag = selmon->pertag->prevtag;
		selmon->pertag->prevtag = selmon->pertag->curtag;
		selmon->pertag->curtag = tmptag;
	}

	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
	selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
	selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
	selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];

	if (selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
		togglebar(NULL);
#else
	}
#endif

	focus(NULL);
	arrange(selmon);
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("dwm: another window manager is already running");
	return -1;
}

void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating)
		return;
	if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
		return;
	pop(c);
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	else if (argc != 1)
		die("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");

#if P_SWALLOW
	if (!(xcon = XGetXCBConnection(dpy)))
		die("dwm: cannot get xcb connection\n");
#endif

	checkotherwm();

#if P_AUTOSTART
	autostart_exec();
#endif

	setup();

#ifdef __OpenBSD__
#if P_SWALLOW
	if (pledge("stdio rpath proc exec ps", NULL) == -1)
#else
	if (pledge("stdio rpath proc exec", NULL) == -1)
#endif
		die("pledge");
#endif /* __OpenBSD__ */

	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
