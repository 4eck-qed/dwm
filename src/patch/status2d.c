int
drawstatusbar(Monitor *m, int bh, char* stext) {
	int ret, i, w, x, len;
	short isCode = 0;
	char *text;
	char *p;

	len = strlen(stext) + 1 ;
	if (!(text = (char*) malloc(sizeof(char)*len)))
		die("malloc");
	p = text;

#if P_STATUSCMD
	int j;

	i = -1, j = 0;
	while (stext[++i])
		if ((unsigned char)stext[i] >= ' ')
			text[j++] = stext[i];
	text[j] = '\0';
#else
	memcpy(text, stext, len);
#endif // P_STATUSCMD

	/* compute width of the status text */
	w = 0;
	i = -1;
	while (text[++i]) {
		if (text[i] == '^') {
			if (!isCode) {
				isCode = 1;
				text[i] = '\0';
				w += TEXTW(text) - lrpad;
				text[i] = '^';
				if (text[++i] == 'f')
					w += atoi(text + ++i);
			} else {
				isCode = 0;
				text = text + i + 1;
				i = -1;
			}
		}
	}
	if (!isCode)
		w += TEXTW(text) - lrpad;
	else
		isCode = 0;
	text = p;

#if P_BARPADDING || P_STATUSPADDING
	w += 0
	#if P_BARPADDING
		+ lrpad
	#endif
	#if P_STATUSPADDING
		+ horizpadbar
	#endif
	;
#else
	w += 2; /* 1px padding on both sides */
#endif

#if P_SYSTRAY
	ret = m->ww - w;
	x = m->ww - w - getsystraywidth();
#else
	ret = x = m->ww - w;
#endif

	drw_setscheme(drw, scheme[LENGTH(colors)]);
	drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
	drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
	drw_rect(
		drw,
		x
#if P_BARPADDING
		 - 2 * sp
#endif
		,
		0,
		w,
		bh,
		1,
		1
	);

#if P_BARPADDING || P_STATUSPADDING
	x += 0
	#if P_BARPADDING
		+ lrpad / 2
	#endif
	#if P_STATUSPADDING
		+ horizpadbar / 2
	#endif
	;
#else
	x++;
#endif

	/* process status text */
	i = -1;
	while (text[++i]) {
		if (text[i] == '^' && !isCode) {
			isCode = 1;

			text[i] = '\0';
			w = TEXTW(text) - lrpad;
			drw_text(
				drw,
				x
#if P_BARPADDING
				 - 2 * sp
#endif
				,
				0
#if P_BARPADDING
				 + vp / 2
#endif
				,
				w,
				bh
#if P_BARPADDING
				 - vp
#endif
				,
				0,
				text,
				0
			);

			x += w;

			/* process code */
			while (text[++i] != '^') {
				if (text[i] == 'c') {
					char buf[8];
					memcpy(buf, (char*)text+i+1, 7);
					buf[7] = '\0';

					drw_clr_create(
						drw,
						&drw->scheme[ColFg],
						buf
#if P_ALPHA
						, alphas[SchemeNorm][ColFg]
#endif
					);

					i += 7;
				} else if (text[i] == 'b') {
					char buf[8];
					memcpy(buf, (char*)text+i+1, 7);
					buf[7] = '\0';

					drw_clr_create(
						drw,
						&drw->scheme[ColBg],
						buf
#if P_ALPHA
						, alphas[SchemeNorm][ColBg]
#endif

					);

					i += 7;
				} else if (text[i] == 'd') {
					drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
					drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
				} else if (text[i] == 'r') {
					int rx = atoi(text + ++i);
					while (text[++i] != ',');
					int ry = atoi(text + ++i);
					while (text[++i] != ',');
					int rw = atoi(text + ++i);
					while (text[++i] != ',');
					int rh = atoi(text + ++i);

					drw_rect(
						drw,
						rx + x
#if P_BARPADDING
						 - 2 * sp
#endif
						,
						ry
#if P_BARPADDING
						 + vp / 2
#endif
						,
						rw,
#if P_BARPADDING
						MIN(rh, bh - vp),
#else
						rh,
#endif
						1,
						0
					);
				} else if (text[i] == 'f') {
					x += atoi(text + ++i);
				}
			}

			text = text + i + 1;
			i=-1;
			isCode = 0;
		}
	}

	if (!isCode) {
		w = TEXTW(text) - lrpad;
		drw_text(
			drw,
			x
#if P_BARPADDING
			 - 2 * sp
#endif
			,
			0,
			w,
			bh,
			0,
			text,
			0
		);
	}

	drw_setscheme(drw, scheme[SchemeNorm]);
	free(p);

	return ret;
}
