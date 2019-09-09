#
# Global Settings
#

INSTALL = install
DESTDIR ?= /
PREFIX  ?= $(DESTDIR)/usr

PATH_ZWM_GNOME = $(PREFIX)/bin/zwm-gnome
PATH_ZWM_GNOME_DESKTOP = $(PREFIX)/share/applications/zwm-gnome.desktop
PATH_ZWM_GNOME_SESSION = $(PREFIX)/share/gnome-session/sessions/zwm-gnome.session
PATH_ZWM_GNOME_XSESSION = $(PREFIX)/share/xsessions/zwm-gnome.desktop
PATH_GNOME_SESSION_ZWM = $(PREFIX)/bin/gnome-session-zwm

#
# Targets
#

all:
	@echo "Nothing to do"


install:
	$(INSTALL) -m0644 -D session/zwm-gnome-xsession.desktop $(PATH_ZWM_GNOME_XSESSION)
	$(INSTALL) -m0644 -D session/zwm-gnome.desktop $(PATH_ZWM_GNOME_DESKTOP)
	$(INSTALL) -m0644 -D session/zwm-gnome.session $(PATH_ZWM_GNOME_SESSION)
	$(INSTALL) -m0755 -D session/zwm-gnome $(PATH_ZWM_GNOME)
	$(INSTALL) -m0755 -D session/gnome-session-zwm $(PATH_GNOME_SESSION_ZWM)



uninstall:
	rm -f $(PATH_ZWM_GNOME)
	rm -f $(PATH_ZWM_GNOME_DESKTOP)
	rm -f $(PATH_ZWM_GNOME_SESSION)
	rm -f $(PATH_ZWM_GNOME_XSESSION)
	rm -f $(PATH_GNOME_SESSION_ZWM)



.PHONY: all install uninstall
