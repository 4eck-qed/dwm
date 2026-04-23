#define EXP_BAKKEBY_ALPHA_SYSTRAY_FIX_ADJ 1

unsigned int
getsystraywidth()
{
	unsigned int w = 0;
	Client *i;
	if(showsystray)
		for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;
	return w ? w + systrayspacing :
	#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		0
	#else
		1
	#endif
	;
}

void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!showsystray || !i)
		return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (ii)
		*ii = i->next;
	free(i);
}

void
resizebarwin(Monitor *m) {
	int x = m->wx, y = m->by;
	unsigned int w = m->ww;

	if (showsystray && m == systraytomon(m) && !systrayonleft)
		w -= getsystraywidth();

#if P_BARPADDING
	x += sp;
	y += vp;
	w -= 2 * sp;
#endif

	XMoveResizeWindow(dpy, m->barwin, x, y, w, bh);
}

void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		updatesystray(1);
#else
		resizebarwin(selmon);
		updatesystray();
#endif
	}
}

Monitor *
systraytomon(Monitor *m) {
	Monitor *t;
	int i, n;
	if(!systraypinning) {
		if(!m)
			return selmon;
		return m == selmon ? m : NULL;
	}
	for(n = 1, t = mons; t && t->next; n++, t = t->next) ;
	for(i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next) ;
	if(systraypinningfailfirst && n < systraypinning)
		return mons;
	return t;
}

void
updatesystrayicongeom(Client *i, int w, int h)
{
	if (i) {
		i->h = bh;
		if (w == h)
			i->w = bh;
		else if (h == bh)
			i->w = w;
		else
			i->w = (int) ((float)bh * ((float)w / (float)h));
		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
		/* force icons into the systray dimensions if they don't want to */
		if (i->h > bh) {
			if (i->w == i->h)
				i->w = bh;
			else
				i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
			i->h = bh;
		}

#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		if (i->w > 2*bh)
			i->w = bh;
#endif
	}
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;

	if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
			!(flags = getatomprop(i, xatom[XembedInfo])))
		return;

	if (flags & XEMBED_MAPPED && !i->tags) {
		i->tags = 1;
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	}
	else if (!(flags & XEMBED_MAPPED) && i->tags) {
		i->tags = 0;
		code = XEMBED_WINDOW_DEACTIVATE;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	}
	else
		return;
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}

void
updatesystray(
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	int updatebar
#else
	void
#endif
)
{
	XSetWindowAttributes wa;
	XWindowChanges wc;
	Client *i;
	Monitor *m = systraytomon(NULL);
	unsigned int x = m->mx + m->mw;

	unsigned int xpad = 0, ypad = 0;

#if P_BARPADDING
	xpad = sp;
	ypad = vp;
#endif

#if !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	unsigned int sw = TEXTW(stext) - lrpad + systrayspacing;
#endif
	unsigned int w = 1;


	if (!showsystray)
		return;
#if !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	if (systrayonleft)
		x -= sw + lrpad / 2;
#endif
	if (!systray) {
		/* init systray */
		if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
		
#if !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		systray->win = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, scheme[SchemeSel][ColBg].pixel);
#endif
		wa.event_mask        = ButtonPressMask | ExposureMask;
		wa.override_redirect = True;
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX_ADJ
		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
#else
		wa.background_pixel = 0;
#endif
		wa.border_pixel = 0;
		wa.colormap = cmap;

		systray->win = XCreateWindow(dpy, root, x - xpad, m->by + ypad, w, bh, 0, depth,
						InputOutput, visual,
						CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &wa);
#else
		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&systrayorientation, 1);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayVisual], XA_VISUALID, 32,
				PropModeReplace, (unsigned char *)&visual->visualid, 1);
		XChangeProperty(dpy, systray->win, netatom[NetWMWindowType], XA_ATOM, 32,
				PropModeReplace, (unsigned char *)&netatom[NetWMWindowTypeDock], 1);
#else
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
		XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel, &wa);
#endif /* EXP_BAKKEBY_ALPHA_SYSTRAY_FIX */
		XMapRaised(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
		}
		else {
			fprintf(stderr, "dwm: unable to obtain system tray.\n");
			free(systray);
			systray = NULL;
			return;
		}
	}

	for (w = 0, i = systray->icons; i; i = i->next) {
#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX && !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX_ADJ
		wa.background_pixel = 0;
#else
		/* make sure the background color stays the same */
		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
#endif
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		w += systrayspacing;
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		w += i->w;
		if (i->mon != m)
			i->mon = m;
	}
	w = w ? w + systrayspacing : 1;
	x -= w;
	XMoveResizeWindow(dpy, systray->win, x - xpad, m->by + ypad, w, bh);
	wc.x = x - xpad;
	wc.y = m->by + ypad;
	wc.width = w;
	wc.height = bh;
	wc.stack_mode = Above; wc.sibling = m->barwin;
	XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode, &wc);
	XMapWindow(dpy, systray->win);
	XMapSubwindows(dpy, systray->win);
#if !EXP_BAKKEBY_ALPHA_SYSTRAY_FIX || EXP_BAKKEBY_ALPHA_SYSTRAY_FIX_ADJ
	/* redraw background */
	XSetForeground(dpy, drw->gc, scheme[SchemeNorm][ColBg].pixel);
	XFillRectangle(dpy, systray->win, drw->gc, 0, 0, w, bh);
#endif
	XSync(dpy, False);

#if EXP_BAKKEBY_ALPHA_SYSTRAY_FIX
	if (updatebar)
		drawbar(m);
#endif
}

Client *
wintosystrayicon(Window w) {
	Client *i = NULL;

	if (!showsystray || !w)
		return i;
	for (i = systray->icons; i && i->win != w; i = i->next)
		;
	return i;
}

