#include "zwm.h"
#include <stdio.h>
#include <locale.h>
#include <sys/wait.h>
#include <sys/stat.h>

int scr;
Display *dpy;
Window root;

unsigned int numlockmask = 0;

static int wm_error(Display *dsply, XErrorEvent *ee);
static int wm_error_dummy(Display *dpy, XErrorEvent *ee);
static void wm_check(void);
static void wm_numlock_init(void);
static void wm_init(void);
static void wm_cleanup(void);
static void wm_signal(int s);

#include "actions.h"
#include "layouts.h"
#include "config.h"

int main(int argc, char *argv[]) {
	struct stat stb;
	setlocale(LC_ALL, getenv("LANG"));

	if(!(dpy = XOpenDisplay(getenv("DISPLAY"))))
	{
		fprintf(stderr,"zwm: cannot open display\n");
		exit(-1);
	}

	ZWM_DEBUG("open display %s\n",getenv("DISPLAY"));
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	if(stat("/usr/lib/libxwmhacks.so", &stb) == 0){
		setenv("LD_PRELOAD", "/usr/lib/libxwmhacks.so", 1);
	}

	wm_check();

	wm_init();
	signal(SIGINT, wm_signal);
	signal(SIGTERM, wm_signal);
	signal(SIGHUP, wm_signal);
	zwm_event_loop();
	zwm_wm_quit(NULL);
	wm_cleanup();

	XCloseDisplay(dpy);
	return 0;
}

void zwm_util_perror(const char *str)
{
	fprintf(stderr, str);
	exit(-1);
}

void* zwm_util_malloc(size_t size)
{
	void *ptr = calloc(1, size);
	if(!ptr){
		zwm_util_perror( "memory allocation failure");
	}
	return ptr;
}

void zwm_util_free(void *p)
{
	free(p);
}

void zwm_wm_fallback(const char *p) {
	Client *c = head;
	while(c){
		zwm_client_unmanage(c);
		c = head;
	}
	XCloseDisplay(dpy);
	setsid();
	execlp( p, "", NULL);
	fprintf(stderr, "restart failed");
}

void zwm_wm_restart(const char *p) {
	Client *c = head;
	while(c){
		zwm_client_unmanage(c);
		c = head;
	}
	XCloseDisplay(dpy);
	setsid();
	execlp( "zwm", "", NULL);
	fprintf(stderr, "restart failed");
}

void zwm_util_spawn(const char *cmd)
{
	static const char *shell = NULL;
	if(!(shell = getenv("SHELL")))
		shell = "/bin/sh";
	if(!cmd)
		return;
	if(fork() == 0) {
		if(fork() == 0) {
			if(dpy)
				close(ConnectionNumber(dpy));
			setsid();
			setenv("ZWM_INSERT","START", 1);
			execl(shell, shell, "-c", cmd, (char *)NULL);
			fprintf(stderr, "zwm: execl '%s -c %s' failed (%s)", shell, cmd, sys_errlist[errno]);
		}
		exit(0);
	}
	wait(0);
}

void zwm_wm_quit(const char *arg) {
	zwm_session_save();
	Client *c = head;
	while(c){
		zwm_client_unmanage(c);
		c = head;
	}
	zwm_event_quit();
}

static int wm_error(Display *dsply, XErrorEvent *ee) {
	fprintf(stderr,"zwm eror: another window manager is running\n");
	exit(-1);
	return -1;
}

static void wm_check(void) {
	XSetErrorHandler(wm_error);
	XSelectInput(dpy, root, SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(NULL);
}

static void wm_numlock_init(void) {
	unsigned int i, j;
	XModifierKeymap *modmap;
	modmap = XGetModifierMapping(dpy);
	for(i = 0; i < 8; i++)
		for(j = 0; j < modmap->max_keypermod; j++) {
			if(modmap->modifiermap[i * modmap->max_keypermod + j]
			== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
		}
	XFreeModifiermap(modmap);
}

static int wm_error_dummy(Display *dpy, XErrorEvent *ee) {
	return 0;
}

static void wm_init(void) {
	XSetWindowAttributes swa;

	XSetErrorHandler(wm_error_dummy);

	zwm_x11_atom_init();
	zwm_event_init();
	zwm_mouse_init(dpy);

	wm_numlock_init();

	zwm_ewmh_init();
	zwm_panel_init();
	zwm_key_init();

	/* init geometry */
	zwm_screen_rescan(True);
	
	/* select for events */
	swa.event_mask = SubstructureRedirectMask |
		SubstructureNotifyMask |
		EnterWindowMask |
		LeaveWindowMask |
		ExposureMask|
		ButtonPressMask|
		PropertyChangeMask|
		StructureNotifyMask;

	swa.cursor = cursor_normal;
	
	XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &swa);
	XSelectInput(dpy, root, swa.event_mask);

	zwm_util_spawn("~/.zwm/init");
	zwm_decor_init();
	zwm_client_scan();
	if(config.num_clients == 0)
		zwm_session_restore();
}

static void wm_cleanup(void) {
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	zwm_mouse_cleanup(dpy);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XSync(dpy, False);
}

static void wm_signal(int s) {
	zwm_event_quit(NULL);
}

KeySym zwm_getkey(void) {
	XEvent ev;
	KeySym keysym;
	XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, 
			CurrentTime);
	do {
		XNextEvent(dpy, &ev);
		if(ev.type == KeyPress){
			keysym = XLookupKeysym(&ev.xkey, 0);
		}
	} while(ev.type != KeyPress);
	XUngrabKeyboard(dpy, CurrentTime);
	return keysym;
}
