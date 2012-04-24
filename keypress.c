#define _GNU_SOURCE
#include "zwm.h"

static const char* attempting_to_grab = 0;
typedef struct HotKey HotKey;
struct HotKey {
	KeySym keysym;
	unsigned long modifiers;
	KeyFunc func;
	const char* args;
	HotKey *next;
};

static HotKey *list = NULL;

static int 
report_key_grab_error(Display* d, XErrorEvent* e) {
	const char* reason = "unknown reason";
	if (e->error_code == BadAccess) {
		reason = "the key/button combination is already in use by another client";
	} else if (e->error_code == BadValue) {
		reason = "the key code was out of range for XGrabKey";
	} else if (e->error_code == BadWindow) {
		reason = "the root window we passed to XGrabKey was incorrect";
	}
	(void) d;
	fprintf(stderr, " couldn't grab key \"%s\": %s (X error code %i)\n",  attempting_to_grab, reason, e->error_code);
	return 0;
}

static unsigned int
parse_modifiers(char* name, const char* full_spec)
{
	char* separator = strchr(name, '-');
	unsigned int modifiers = 0;
	if (separator != NULL) {
		*separator = 0;
		modifiers |= parse_modifiers(separator+1, full_spec);
	}
	if (!strcmp(name, "Shift")) {
		modifiers |= ShiftMask;
	} else if (!strcmp(name, "Control") || !strcmp(name, "Ctrl")) {
		modifiers |= ControlMask;
	} else if (!strcmp(name, "Alt") || !strcmp(name, "Mod1")) {
		modifiers |= Mod1Mask;
	} else if (!strcmp(name, "Mod2")) {
		modifiers |= Mod2Mask;
	} else if (!strcmp(name, "Mod3")) {
		modifiers |= Mod3Mask;
	} else if (!strcmp(name, "Super") || !strcmp(name, "Mod4")) {
		modifiers |= Mod4Mask;
	} else {
		fprintf(stderr, "ignoring unknown modifier \"%s\" in \"%s\"\n",  name, full_spec);
	}
	return modifiers;
}

static void
keypress(XEvent *e, void *p) {
	KeySym keysym;
	XKeyEvent *ev;
	HotKey* h;

	if(e->type != KeyPress) {
		return;
	}

	ev = &e->xkey;
	
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);

	for (h = list; h != NULL; h = h->next) {
		if (h->keysym == keysym 
			&& CLEANMASK(ev->state) == CLEANMASK(h->modifiers)) {
			h->func(h->args);
#ifdef DEBUG
//			printf("exec %s %s %s\n",h->funcname,h->argv[0], h->argv[1]);
#endif
		}
	}
}

void zwm_bindkey(const char* keyname, void *f, const char *arg)
{
	char* copy = strdup(keyname);
	HotKey* new_key;
	char* unmodified;
	void *p;
	unsigned int modifiers = 0;
	unsigned int keymods[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	int i;

	unmodified = strrchr(copy, '-');
	if (unmodified == NULL) {
		unmodified = copy;
	} else {
		*unmodified = 0;
		++ unmodified;
		modifiers = parse_modifiers(copy, keyname);
	}

	new_key = (HotKey*) zwm_malloc(sizeof(HotKey));
	new_key->keysym = XStringToKeysym(unmodified);
	new_key->modifiers = modifiers;
	new_key->func = f;
	new_key->args = arg;
	new_key->next = list;
	list = new_key;

	p = XSetErrorHandler(report_key_grab_error);

	KeyCode code = XKeysymToKeycode(dpy, new_key->keysym);
	for(i = 0; i < sizeof(keymods)/sizeof(unsigned int); i++) {
		XGrabKey(dpy, code, keymods[i] | modifiers, root, True,
				GrabModeAsync, GrabModeAsync);
	}
	free(copy);
	XSetErrorHandler(p);
}

static void
set_view(const char *arg) {
	int v =  atoi(arg);
	if(sel) {
		zwm_client_set_view(sel, v);
	}
}

static void
goto_view(const char *arg) {
	int v = atoi(arg);
	zwm_view_set(v);
}


static void banish(const char *arg)
{
	if(sel){
		zwm_client_set_view(sel, ((zwm_current_view()+1)%2));
	}
}

static void flip(const char *arg)
{
	if(sel){
		int next = (zwm_current_view() + 1) % 2;
		zwm_view_set(next);
	}
}

static void zwm_runonce(const char *arg){
	char data[1024];
	char *cmd;
	char *class;
	Client *c = NULL;
	strcpy(data,arg);
	class = strtok(data, ";");
	cmd = class + strlen(class) + 1;
	for(c=zwm_client_head();
			c;
			c = zwm_client_next(c)) {
		if(strcasecmp(class, c->cname) == 0) {
			printf("found CLASS %s, %s, for %s\n",c->cname, c->name, cmd);
			zwm_client_zoom(c);
			zwm_client_raise(c);
			return;
		}
	}
	for(c=zwm_client_head();
			c;
			c = zwm_client_next(c)) {
		if(strcasestr(c->name, class)) {
			printf("found NAME %s for %s\n",c->name, cmd);
			zwm_client_zoom(c);
			zwm_client_raise(c);
			return;
		}
	}
	zwm_spawn(cmd);
}

static void client_zoom(const char *arg) {
	Client *c = sel;
	if( c && c == zwm_client_head()) {
		c = zwm_client_next_visible(zwm_client_next(c));
	}

	if(c) {
		zwm_client_zoom(c);
	}
}

void zwm_quit(const char *arg) {
	Client *c = zwm_client_head();
	while(c){
		zwm_client_unmanage(c);
		c = zwm_client_head();
	}
	zwm_event_quit();
}

void zwm_keypress_init(void)
{
	zwm_event_register(KeyPress, (ZenEFunc)keypress, NULL);
	zwm_bindkey("Alt-Delete", zwm_client_kill, NULL);
	zwm_bindkey("Alt-j", zwm_focus_next, NULL);
	zwm_bindkey("Alt-k", zwm_focus_prev, NULL);
	zwm_bindkey("Alt-Return", client_zoom, NULL);
	zwm_bindkey("Alt-Shift-space", zwm_client_toggle_floating, NULL);
	zwm_bindkey("Alt-space", zwm_layout_set, NULL);
	zwm_bindkey("Alt-Tab", zwm_layout_cycle, NULL);
	zwm_bindkey("Ctrl-Alt-q", zwm_quit, NULL);
	zwm_bindkey("Ctrl-Alt-Return", zwm_client_iconify, NULL);
	zwm_bindkey("Ctrl-Alt-r", zwm_restart, NULL);
	zwm_bindkey("Ctrl-Shift-Return", zwm_spawn, "simshell");
	zwm_bindkey("Alt-F11", zwm_panel_toggle, NULL);

	zwm_bindkey("Alt-1", goto_view, "0");
	zwm_bindkey("Alt-Shift-1", set_view, "0");
	zwm_bindkey("Alt-2", goto_view, "1");
	zwm_bindkey("Alt-Shift-2", set_view, "1");
	zwm_bindkey("Alt-3", goto_view, "2");
	zwm_bindkey("Alt-Shift-3", set_view, "2");
	zwm_bindkey("Alt-4", goto_view, "3");
	zwm_bindkey("Alt-Shift-4", set_view, "3");
	zwm_bindkey("Alt-5", goto_view, "4");
	zwm_bindkey("Alt-Shift-5", set_view, "4");
	zwm_bindkey("Alt-6", goto_view, "5");
	zwm_bindkey("Alt-Shift-6", set_view, "5");

	zwm_bindkey("Alt-l", banish, NULL);
	zwm_bindkey("Alt-h", flip, "simshell");

	zwm_bindkey("Ctrl-Alt-l", zwm_spawn, "standby");
	zwm_bindkey("Alt-Shift-Return", zwm_runonce, "Screen;st -t Screen -c Screen -e screen -Rd");
	zwm_bindkey("Alt-Shift-m", zwm_runonce, "Mixer;st -c Mixer -t Mixer -e alsamixer");
	zwm_bindkey("Alt-g", zwm_runonce, "zweb;zweb");
	zwm_bindkey("Alt-m", zwm_runonce, "Mutt;st -t Mail -c Mutt -e mutt");
	zwm_bindkey("Alt-r", zwm_runonce, "Rox;rox");
	zwm_bindkey("Alt-n", zwm_runonce, "News;st -c News -t News -e newsbeuter");
	zwm_bindkey("Alt-p", zwm_runonce, "zmenu;dwmenu");
}
