#include "zwm.h"
#include <stdio.h>
#include <X11/extensions/Xinerama.h>
#include <locale.h>
#include <sys/wait.h>

int scr;
Display *dpy;
Window root;

int screen_count = 1;
unsigned long num_views = 1;
unsigned long current_view = 0;

unsigned int num_floating;
unsigned int border_w;

ZenGeom screen[MAX_SCREENS];

/* X color definitions */
unsigned int numlockmask = 0;
GC gc;
XftFont *xfont;
XftFont *ifont;

#include "config.h"

static int
other_wm_handler(Display *dsply, XErrorEvent *ee) {
	fprintf(stderr,"zwm eror: another window manager is running\n");
	exit(-1);
	return -1;
}

static void load_font ()
{
	xfont = XftFontOpenXlfd(dpy, scr, config.font);
	ifont = XftFontOpenXlfd(dpy, scr, config.icons);
}

static void
zwm_check(void) {
	XSetErrorHandler(other_wm_handler);
	XSelectInput(dpy, root, SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(NULL);
}

unsigned long
zwm_get_color(const char *colstr)
{
	Colormap cmap = DefaultColormap(dpy, scr);
	XColor color;

	if(!XAllocNamedColor(dpy, cmap, colstr, &color, &color))
	{
		zwm_perror("unable to allocate color\n");
	}
	return color.pixel;
}

void
zwm_update_screen_geometry(Bool init) {
	screen_count = 0;
	int i;

	if ( XineramaIsActive(dpy) ) 
	{
		int nscreen;
		XineramaScreenInfo * info = XineramaQueryScreens( dpy, &nscreen );

		ZWM_DEBUG("Xinerama is active\n");

		for( i = 0; i< nscreen; i++ ) {
			if( i == 0 || (info[i].x_org != info[i-1].x_org)) {
				ZWM_DEBUG( "Screen %d %d: (%d) %d+%d+%dx%d\n", i,screen_count,
				       	info[i].screen_number,
					info[i].x_org, info[i].y_org,
					info[i].width, info[i].height );

				screen[i].x = info[i].x_org;
				screen[i].y = info[i].y_org;
				screen[i].w = info[i].width;
				screen[i].h = info[i].height;
				screen_count++;
			}
		}
		XFree(info);
	} else 
	{
		ZWM_DEBUG("Xinerama is not active\n");
		screen_count = 1;
		screen[0].x = 0;
		screen[0].y = 0;
		screen[0].w = DisplayWidth(dpy, scr);
		screen[0].h = DisplayHeight(dpy, scr);

		ZWM_DEBUG( "Screen %d %d: (%d) %d+%d+%dx%d\n", 0,1,
			       	0, screen[0].x,  screen[0].y,
				 screen[0].w , screen[0].h);
	} 

	if(screen_count == 1 && config.fake_screens == 2){
		screen[0].w = DisplayWidth(dpy, scr)/2;

		screen[1].x = DisplayWidth(dpy, scr)/2;
		screen[1].y = 0;
		screen[1].w = DisplayWidth(dpy, scr)/2;
		screen[1].h = DisplayHeight(dpy, scr);
		screen_count = 2;
	}

	if(init){
		for(i=0;i<screen_count;i++)
			zwm_screen_set_view(i, i);
	}
	zwm_event_emit(ZenScreenSize, NULL);
	zwm_layout_dirty();
}

void 
init_numlock_mask(void)
{
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

static int
xerror(Display *dpy, XErrorEvent *ee) {

	return 0;
}

void
zwm_init(void) {
	XSetWindowAttributes swa;

	XSetErrorHandler(xerror);
	/* init atoms */
	zwm_init_atoms();
	zwm_event_init();
	zwm_layout_init();
	/* init cursors */
	zwm_init_cursor(dpy);

	init_numlock_mask();

	zwm_ewmh_init();
	zwm_panel_init();
	zwm_keypress_init();

	/* init geometry */
	zwm_update_screen_geometry(True);
	
	/* select for events */
	swa.event_mask = SubstructureRedirectMask |
		SubstructureNotifyMask |
		EnterWindowMask |
		LeaveWindowMask |
		ExposureMask|
		PropertyChangeMask|
		StructureNotifyMask;

	swa.cursor = zwm_get_cursor(CurNormal);
	
	XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &swa);
	XSelectInput(dpy, root, swa.event_mask);

	/* init appearance */
	config.xcolor_nborder = zwm_get_color(config.normal_border_color);
	config.xcolor_fborder = zwm_get_color(config.focus_border_color);
	config.xcolor_nbg = zwm_get_color(config.normal_bg_color);
	config.xcolor_fbg = zwm_get_color(config.focus_bg_color);
	config.xcolor_nshadow = zwm_get_color(config.normal_shadow_color);
	config.xcolor_fshadow = zwm_get_color(config.focus_shadow_color);
	gc = XCreateGC(dpy, root, 0, NULL);
	load_font();
	zwm_spawn("~/.zwm/init");
	zwm_client_scan();
}

void
zwm_cleanup(void) {
	//TODO: should cleanup clients
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	zwm_free_cursors(dpy);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XSync(dpy, False);
}

void zwm_quit(const char *arg);
void
sig(int s)
{
	zwm_event_quit(NULL);
}

int
main(int argc, char *argv[]) {
	setlocale(LC_ALL, getenv("LANG"));

	if(!(dpy = XOpenDisplay(getenv("DISPLAY"))))
	{
		fprintf(stderr,"zwm: cannot open display\n");
		exit(-1);
	}

	ZWM_DEBUG("open display %s\n",getenv("DISPLAY"));
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	zwm_check();

	zwm_init();
	signal(SIGINT, sig);
	signal(SIGTERM, sig);
	signal(SIGHUP, sig);
	zwm_event_loop();
	zwm_quit(NULL);
	zwm_cleanup();

	XCloseDisplay(dpy);
	return 0;
}

void
zwm_perror(const char *str)
{
	fprintf(stderr, str);
	exit(-1);
}


void*
zwm_malloc(size_t size)
{
	void *ptr = calloc(1, size);
	if(!ptr){
		zwm_perror( "memory allocation failure");
	}
	return ptr;
}

void
zwm_free(void *p)
{
	free(p);
}

void
zwm_restart(const char *p) {
	Client *c = zwm_client_head();
	while(c){
		zwm_client_unmanage(c);
		c = zwm_client_head();
	}
	XCloseDisplay(dpy);
	setsid();
	execlp( "zwm", "", NULL);
	fprintf(stderr, "restart failed");
}

void zwm_spawn(const char *cmd)
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
			execl(shell, shell, "-c", cmd, (char *)NULL);
			fprintf(stderr, "zwm: execl '%s -c %s' failed (%s)", shell, cmd, sys_errlist[errno]);
		}
		exit(0);
	}
	wait(0);
}


