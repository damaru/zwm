#include "zwm.h"

static const char* attempting_to_grab = 0;
typedef struct HotKey {
	KeySym keysym;
	unsigned long modifiers;
	KeyFunc func;
	const char* args;
	struct HotKey *next;
	int rel;
} HotKey;

static HotKey *list = NULL;
static int key_error(Display* d, XErrorEvent* e);
static unsigned int key_parse(char* name, const char* full_spec);
static void key_press(XEvent *e, void *p);
static void key_release(XEvent *e, void *p);

void zwm_key_init(void)
{
	int i;

	zwm_event_register(KeyPress, (ZwmEFunc)key_press, NULL);
	zwm_event_register(KeyRelease, (ZwmEFunc)key_release, NULL);

	for(i = 0; config.keys[i].f; i++){
		zwm_key_bind(config.keys[i].key, config.keys[i].f, config.keys[i].arg, 0);
	}
	for(i = 0; config.relkeys[i].f; i++){
		zwm_key_bind(config.relkeys[i].key, config.relkeys[i].f, config.relkeys[i].arg, 1);
	}
}

#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))

void zwm_key_bind(const char* keyname, void *f, const char *arg, int rel)
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
		modifiers = key_parse(copy, keyname);
	}

	new_key = (HotKey*) zwm_util_malloc(sizeof(HotKey));
	new_key->keysym = XStringToKeysym(unmodified);
	new_key->modifiers = modifiers;
	new_key->func = f;
	new_key->args = arg;
	new_key->next = list;
	new_key->rel = rel;

	list = new_key;

	attempting_to_grab = keyname;
	p = XSetErrorHandler(key_error);

	KeyCode code = XKeysymToKeycode(dpy, new_key->keysym);
	for(i = 0; i < sizeof(keymods)/sizeof(unsigned int); i++) {
		XGrabKey(dpy, code, keymods[i] | modifiers, root, True,
				GrabModeAsync, GrabModeAsync);
	}
	free(copy);
	XSetErrorHandler(p);
}

static int key_error(Display* d, XErrorEvent* e)
{
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

static unsigned int key_parse(char* name, const char* full_spec)
{
	char* separator = strchr(name, '-');
	unsigned int modifiers = 0;
	if (separator != NULL) {
		*separator = 0;
		modifiers |= key_parse(separator+1, full_spec);
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

int super_on = 0;

static void key_release(XEvent *e, void *p)
{
	KeySym keysym;
	XKeyEvent *ev;
	HotKey* h;
	if(e->type != KeyRelease) {
		return;
	}

	ev = &e->xkey;
	
	keysym = XLookupKeysym(ev, 0);

	for (h = list; h != NULL; h = h->next) {
		if (h->keysym == keysym 
			&& h->rel == 1
			&& CLEANMASK(ev->state) == CLEANMASK(h->modifiers)) {
			h->func(h->args);
		}
	}
}

static void key_press(XEvent *e, void *p)
{
	KeySym keysym;
	XKeyEvent *ev;
	HotKey* h;
	if(e->type != KeyPress) {
		return;
	}

	ev = &e->xkey;
	
	keysym = XLookupKeysym(ev, 0);

	for (h = list; h != NULL; h = h->next) {
		if (h->keysym == keysym 
			&& h->rel == 0
			&& CLEANMASK(ev->state) == CLEANMASK(h->modifiers)) {
			h->func(h->args);
		}
	}
}


