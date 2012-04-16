#include "zwm.h"

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
unsigned int xcolor_normal;
unsigned int xcolor_focus;
unsigned int xcolor_floating;
unsigned int xcolor_bg;
unsigned int xcolor_focus_bg;
unsigned int numlockmask = 0;
GC gc;
XFontStruct *font;

/* appearance */
ZenConfig config = 
{
	.auto_view = 1,
	.border_width = 1,
	.fake_screens = 1,
	.screen_x = 0,
	.screen_y = 0,
	.screen_w = 0,
	.screen_h = 0,
	.normal_border_color = "#9AA3BF",
	.focus_border_color = "#F49435",
	.focus_bg_color = "#BFB19A",
	.floating_border_color = "#0B2882",
	.bg_color = "#888",
	.opacity = 0.9,
	.anim_steps = 80,
	.show_title = 1,
};

static int
other_wm_handler(Display *dsply, XErrorEvent *ee) {
	fprintf(stderr,"zwm eror: another window manager is running\n");
	exit(-1);
	return -1;
}

void
load_font (XFontStruct **font_info)
{
  char *fontname = "-*-terminus-bold-r-*-*-18-*-*-*-*-*-*-*";
  /* Load font and get font information structure */
  if ((*font_info = XLoadQueryFont (dpy, fontname)) == NULL)
    {
      (void) fprintf (stderr, "zwm: Cannot open 9x15 font\n");
      exit (-1);
    }
}


static void
zwm_check(void) {
	XSetErrorHandler(other_wm_handler);

	/* this causes an an X error if some other window manager is running
	 * */
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
zwm_update_screen_geometry(void) {
	screen_count = 0;
	int i;
ZenGeom vscreen = {0, 0, 0, 0};

#ifdef CONFIG_XINERAMA
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
#endif
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
#if 0
	for(i = 0; i< screen_count; i++){
		if(config.screen_x)
			screen[i].x = config.screen_x;
		if(config.screen_y)
			screen[i].y = config.screen_y;
		if(config.screen_w)
			screen[i].w = config.screen_w;
		if(config.screen_h)
			screen[i].h = config.screen_h;
		ZWM_DEBUG( "Screen %d %d:  %d+%d+%dx%d\n", i,screen_count,
			       	 screen[i].x,  screen[i].y,
				 screen[i].w , screen[i].h);
	}
#endif
	for(i = 0; i< screen_count; i++){
		if(vscreen.x >= screen[i].x)
			vscreen.x = screen[i].x;
		if(vscreen.y >= screen[i].y)
			vscreen.y = screen[i].y;
		
		vscreen.w += screen[i].w ;

		if(vscreen.h < screen[i].h)
			vscreen.h = screen[i].h;

		ZWM_DEBUG( "Virtual Screen %d %d:  %d+%d+%dx%d\n", i,screen_count,
			       	 vscreen.x,  vscreen.y,
				vscreen.w , vscreen.h);
	}

	config.screen_x = vscreen.x;
	config.screen_y = vscreen.y;
	config.screen_w = vscreen.w;
	config.screen_h = vscreen.h;

	zwm_event_emit(ZenScreenSize, NULL);
	zwm_layout_arrange();
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
	zwm_update_screen_geometry();
	
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
	xcolor_normal = zwm_get_color(config.normal_border_color);
	xcolor_focus = zwm_get_color(config.focus_border_color);
	xcolor_floating = zwm_get_color(config.floating_border_color);
	xcolor_bg = zwm_get_color(config.bg_color);
	xcolor_focus_bg = zwm_get_color(config.focus_bg_color);
	load_font(&font);
	gc = XCreateGC(dpy, root, 0, NULL);
	XSetFont (dpy, gc, font->fid);

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

int
main(int argc, char *argv[]) {
	setlocale(LC_CTYPE, "");

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
	zwm_event_loop();
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

	/* The double-fork construct avoids zombie processes and keeps the code
	 * clean from stupid signal handlers. */
	if(dpy)
		close(ConnectionNumber(dpy));
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
	/* The double-fork construct avoids zombie processes and keeps the code
	 * clean from stupid signal handlers. */
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


