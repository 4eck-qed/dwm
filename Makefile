
# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

# in
SRCDIR = src
SRC = $(SRCDIR)/drw.c $(SRCDIR)/dwm.c $(SRCDIR)/util.c

# out
OBJDIR = build
OBJ = $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJ_FILES = $(OBJ)

# resources
RESDIR = resources

# binaries
BINDIR = .

# Default target: dwm
all: configs dwm

# Rule to compile .c files to .o files in OBJDIR
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

# Ensure configs are up to date
configs:
	cp config.def.h $(SRCDIR)/config.h
	cp patch.def.h $(SRCDIR)/patch.h

# Target to build the main dwm binary
dwm: $(OBJ_FILES)
	$(CC) -o $@ $(OBJ_FILES) $(LDFLAGS)

# Clean all build artifacts
clean:
	rm -rf $(BINDIR)/dwm $(OBJ_FILES) $(SRCDIR)/*.orig $(SRCDIR)/*.rej

# Install dwm binary, man pages, and other files
install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -Dm755 $(BINDIR)/dwm $(DESTDIR)$(PREFIX)/bin/dwm
	install -Dm755 $(BINDIR)/dwm_d $(DESTDIR)$(PREFIX)/bin/dwm_d
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < $(RESDIR)/dwm.1 > $(DESTDIR)$(MANPREFIX)/man1/dwm.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/dwm.1
	mkdir -p /usr/share/xsessions/
	test -f /usr/share/xsessions/dwm.desktop || install -Dm644 dwm.desktop /usr/share/xsessions/
	test -f /usr/share/xsessions/dwm-debug.desktop || install -Dm644 dwm-debug.desktop /usr/share/xsessions/
	mkdir -p release
	cp -f $(BINDIR)/dwm release/
	tar -czf release/dwm-$(VERSION).tar.gz -C release $(BINDIR)/dwm

# Uninstall the dwm binary and related files
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dwm\
		$(DESTDIR)$(PREFIX)/bin/dwm_d\
		$(DESTDIR)$(MANPREFIX)/man1/dwm.1\
		$(DESTDIR)$(PREFIX)/share/xsession/dwm.desktop\
		$(DESTDIR)$(PREFIX)/share/xsession/dwm-debug.desktop

# Release target
release: dwm
	mkdir -p release
	cp -f dwm release/
	tar -czf release/dwm-$(VERSION).tar.gz -C release $(BINDIR)/dwm

.PHONY: all clean install uninstall release

