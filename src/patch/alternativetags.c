void
keyrelease(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);

    for (i = 0; i < LENGTH(keys); i++) {
        if (momentaryalttags
        	&& keys[i].func && keys[i].func == togglealttag
        	&& selmon->alttag
        	&& (keysym == keys[i].keysym
        	|| CLEANMASK(keys[i].mod) == CLEANMASK(ev->state))) {
            keys[i].func(&(keys[i].arg));
		}
	}
}

void
togglealttag(const Arg *arg)
{
	selmon->alttag = !selmon->alttag;
	drawbar(selmon);
}
