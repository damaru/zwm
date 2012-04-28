#ifndef _ZWM_H
#define _ZWM_H

#define _GNU_SOURCE
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <libzen/events.h>
#include <libzen/list.h>

typedef enum {
       	CurNormal,
       	CurResize,
       	CurMove,
       	CurLast
} ZenCursor;

typedef void (*KeyFunc)(const char *);
typedef struct ZenGeom
{
	int view;
	int x;
	int y;
	int w;
	int h;
} ZenGeom;

enum
{
	ZenNormalWindow,
	ZenDockWindow,
	ZenDialogWindow,
	ZenSplashWindow,
	ZenDesktopWindow,
	ZenFullscreenWindow
};


/* client */
typedef struct Client Client;
struct Client
{
	ZenListNode node;
	double x;
	double y;
	double w;
	double h;
	double dx;
	double dy;
	double dw;
	double dh;
	double ox;
	double oy;
	double ow;
	double oh;
	int ignore;
	int border;
	int state;
	int focused;
	int type;
	int view;
	int screen;
	int noanim;
	int hastitle;
	int anim_steps;
	Bool isfloating;
	ZenGeom fpos;
	ZenGeom bpos;
	Window win;
	Window frame;
	Client *next;
	Client *prev;
	Window lastfocused;
	char name[256];
	char cname[256];
} __attribute__((aligned));

typedef struct
{
	unsigned int border_width;
	unsigned int fake_screens;
	unsigned int screen_x;
	unsigned int screen_y;
	unsigned int screen_w;
	unsigned int screen_h;
	unsigned int num_views;

	const char *normal_border_color;
	const char *focus_border_color;
	const char *normal_bg_color;
	const char *focus_bg_color;
	const char *normal_shadow_color;
	const char *focus_shadow_color;

	const char *font;
	const char *icons;

	float opacity;
	int anim_steps;
	int show_title;
	int title_height;
	int title_y;
	int button_width;
	int button_count;
	int reparent;

	unsigned int xcolor_nborder;
	unsigned int xcolor_fborder;
	unsigned int xcolor_fbg;
	unsigned int xcolor_nbg;
	unsigned int xcolor_fshadow;
	unsigned int xcolor_nshadow;
	
	char *viewnames[32];

	struct {
		char *c;
		void (*func)(Client *);
	} buttons[32];

	struct {
		const char *key;
		void *f;
		const char *arg;
	} keys[64];
} ZwmConfig;

extern ZwmConfig config;

enum
{
	ZenEventPropagate,
	ZenEventConsume
};


typedef enum
{
	/* our event numbers start after X11 events */
	ZenClientFocus = LASTEvent,
	ZenClientUnFocus,
	ZenClientProperty,
	ZenClientMap,
	ZenClientUnmap,
	ZenClientView,
	ZenClientState,
	ZenClientDamage,
	ZenClientResize,
	ZenClientFloating,
	ZenClientConfigure,
	ZenView,
	ZenLayoutChange,
	ZenX11Event,
	ZenScreenSize,
	ZenManageScreen,
	ZenAllEvents,
	ZenMaxEvents
}ZenEvent;

typedef int (*ZenLFunc)(int scrn, int view);
typedef int (*ZenEFunc)(void *, void *);


#define INIT_ATOM(name) extern Atom name
#include "atoms.h"
#undef INIT_ATOM

#define MAX_SCREENS 32
extern ZenGeom screen[MAX_SCREENS];
extern int screen_count;
extern int scr;
extern unsigned int border_w;
extern Bool running;
extern XFontSet fontset;
extern Display *dpy;
extern Window root;
extern unsigned int numlockmask;
extern unsigned int num_clients;
extern GC gc;
DECLARE_GLOBAL_LIST_FUNCTIONS(zwm_client,Client,node);
extern Client *sel;

void zwm_init_cursor(Display *dpy);
void zwm_free_cursors(Display *dpy);
Cursor zwm_get_cursor(ZenCursor c);
void zwm_perror(const char *str);
void *zwm_malloc(size_t size);
void zwm_free(void *);
void zwm_client_remove(Client *c);
void zwm_client_push_head(Client *);
void zwm_client_scan(void);
Client* zwm_client_manage(Window w, XWindowAttributes *wa);
void zwm_client_unmanage(Client *);
void zwm_client_mousemove(Client *c);
void zwm_client_mouseresize(Client *c);
void zwm_client_focus(Client *c);
void zwm_client_refocus(void);
void zwm_client_moveresize(Client *c, int,int,int,int);
void zwm_client_raise(Client *c);
void zwm_client_update_decoration(Client *c);
Bool zwm_client_visible(Client *c, int view);
int zwm_client_count(void);
Client *zwm_client_head(void);
Client *zwm_client_next_visible(Client *c);
Client *zwm_client_get(Window w);
void zwm_client_update_name(Client *);
void zwm_client_iconify(Client *c);
void zwm_client_warp(Client *c);
void zwm_client_set_view(Client *c, int v);
void zwm_client_setstate(Client *c, int state);
void zwm_view_set(int  v);
void zwm_client_kill(Client *c);
void zwm_client_toggle_floating(Client *c);
void zwm_client_fullscreen(Client *c);
void zwm_client_zoom(Client *);

void zwm_init_config(void);
void zwm_event_loop(void);
void zwm_update_screen_geometry(Bool);
void zwm_update_view();
void zwm_screen_set_view(int scr, int view);
void zwm_init_atoms(void);
void zwm_decor_init(void);

void zwm_x11_configure_window(Client *c);
Atom zwm_x11_get_window_type(Window win);
Atom zwm_x11_get_small_atom(Window win, Atom bigatom);
Bool zwm_x11_check_atom(Window win, Atom bigatom, Atom smallatom);
Bool zwm_x11_set_atoms(Window w, Atom a, Atom type, unsigned long *val, unsigned long);

#define BUTTONMASK		(ButtonPressMask | ButtonReleaseMask)
#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))
#define MOUSEMASK		(BUTTONMASK | PointerMotionMask)

#define MODKEY			Mod1Mask

#ifdef DEBUG
#define DBG_ENTER() printf("Enter %s\n",__FUNCTION__)
#define ZWM_DEBUG(fmt,args...) printf("%s:%d: "fmt,__FUNCTION__,__LINE__,##args)
#else
#define DBG_ENTER() 
#define ZWM_DEBUG(fmt,args...) 
#endif


void zwm_plugin_load(const char **args);
void zwm_event_init(void);
void zwm_event_flush_x11(long mask);
void zwm_event_emit(ZenEvent e, void *p);
void zwm_event_register(ZenEvent e, ZenEFunc f, void *priv);
ZenEvent zwm_event_byname(char *name);

/* layouts */
void zwm_layout_next(void);
void zwm_layout_init(void);
void zwm_layout_register(ZenLFunc f, char *name);
void zwm_layout_arrange(void);
void zwm_layout_set(const char *name);
const char *zwm_current_layout(void);

Bool zwm_x11_get_text_property(Window w, Atom atom, char *text, unsigned int size);

void zwm_layout_moveresize(Client *c, int x, int y, int w, int h);

void zwm_spawn(const char *cmd);
void zwm_bindkey(const char* keyname, void *f, const char *arg);
void zwm_restart(const char *);
void zwm_layout_cycle(const char *arg); 
void zwm_focus_prev(const char *arg) ;
void zwm_focus_next(const char *arg) ;
void zwm_panel_toggle(const char *args);

void zwm_ewmh_init(void);
void zwm_panel_init(void);
void zwm_keypress_init(void);
typedef struct ZenEventHandler ZenEventHandler;
struct ZenEventHandler
{
	ZenEFunc handler;
	void *priv;
	ZenEventHandler *next;
};

void zwm_auto_view();
void zwm_event_quit();
int zwm_current_view();
void zwm_layout_dirty(void);
void zwm_layout_rearrange(void);
void zwm_client_save_geometry(Client *c, ZenGeom *g);
void zwm_client_restore_geometry(Client *c, ZenGeom *g);
void zwm_client_unfullscreen(Client *c);
Bool zwm_view_mapped(int v);
int zwm_current_screen();
void zwm_quit(const char *arg);
void zwm_ewmh_set_window_opacity(Window win, float opacity);

unsigned long zwm_x11_get_atoms(Window w, Atom a, Atom type, unsigned long *ret, unsigned long nitems, unsigned long *left);
#endif
