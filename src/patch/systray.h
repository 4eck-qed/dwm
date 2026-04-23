#ifndef DWM_PATCH_SYSTRAY_H
#define DWM_PATCH_SYSTRAY_H

static unsigned int getsystraywidth();
static void removesystrayicon(Client *i);
static void resizebarwin(Monitor *m);
static void resizerequest(XEvent *e);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static Monitor *systraytomon(Monitor *m);
static void updatesystray(
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	int updatebar
	#else
	void
	#endif
);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static Client *wintosystrayicon(Window w);

#endif // DWM_PATCH_SYSTRAY_H
