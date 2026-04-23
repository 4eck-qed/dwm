void
layoutmenu(const Arg *arg) {
	FILE *p;
	char c[3], *s;
	int i;

	char cmd[2048] = "echo '";

	unsigned int k;
	for (k = 0; k < LENGTH(layouts); k++) {
		char entry[256];
		const char* symbol = layouts[k].symbol;
		const char* desc = layouts[k].description;

		if (symbol == NULL)
			continue;

		if (desc == NULL)
			desc = "";

		snprintf(entry, sizeof(entry), "%s - %s\t%d\n", symbol, desc, k);
		strncat(cmd, entry, sizeof(cmd) - strlen(cmd) - 1);
	}
	
	strncat(cmd, "' | xmenu", sizeof(cmd) - strlen(cmd) - 1);

	if (!(p = popen(cmd, "r"))) {
		 return;
	}

	s = fgets(c, sizeof(c), p);
	pclose(p);

	if (!s || *s == '\0' || c[0] == '\0') {
		 return;
	}

	i = atoi(c);
	setlayout(&((Arg) { .v = &layouts[i] }));
}
