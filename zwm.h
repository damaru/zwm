#ifndef _ZWM_H
#define _ZWM_H

#define _GNU_SOURCE
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef unsigned long ulong;
typedef void (*ZwmLFunc)(int scrn, int view);
typedef int (*ZwmEFunc)(void*, void*);
typedef void (*KeyFunc)(const char*);

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

typedef struct ZwmGeom {
	int view;
	int screen;
	double x;
	double y;
	double w;
	double h;
} ZwmGeom;

typedef struct ZwmLayout {
	ZwmLFunc func;
	char* name;
	int skip;
	char* symbol;
} ZwmLayout;

typedef struct ZwmScreen {
	int prev;
	int view;
	int x;
	int y;
	int w;
	int h;
} ZwmScreen;

enum {
	ZwmNormalWindow,
	ZwmDockWindow,
	ZwmDialogWindow,
	ZwmSplashWindow,
	ZwmDesktopWindow,
	ZwmFullscreenWindow,
	ZwmFixedWindow
};

typedef struct
{
	const char* cname;
	int type;
	ZwmGeom pos;
} ZwmPolicy;

/* client */
typedef struct Client Client;
struct Client {
	union {
		struct {
			int view;
			int screen;
			double x;
			double y;
			double w;
			double h;
		};
		ZwmGeom geom;
	};

	double dx;
	double dy;
	double dw;
	double dh;

	int minh;
	int minw;

	int border;
	int focused;
	int hastitle;
	int ignore;
	int noanim;
	int state;
	int type;
	int dirty;
	int has_shape;
	int root_user;
	int tags;
	ulong pid;
	Bool isfloating;
	ZwmGeom oldpos;
	ZwmGeom fpos;
	ZwmGeom bpos;
	Window win;
	Window frame;
	Window lastfocused;
	Client* next;
	Client* prev;
	XftDraw* draw;
	char name[256];
	char cname[256];
	char cmd[256];
	char key[32];
} __attribute__((aligned));

typedef struct
{
	unsigned int border_width;
	unsigned int screen_count;
	unsigned int num_views;
	unsigned int num_clients;

	const char* normal_border_color;
	const char* focus_border_color;
	const char* normal_bg_color;
	const char* focus_bg_color;
	const char* normal_shadow_color;
	const char* focus_shadow_color;
	const char* normal_title_color;
	const char* focus_title_color;
	const char* float_bg_color;
	const char* root_bg_color;

	const char* font;
	const char* icons;
	const char* date_fmt;
	const char* menucmd;

	float opacity;
	float mwfact;
	float autofact;
	int anim_steps;
	int show_title;
	int title_height;
	int title_y;
	int icon_y;
	int icon_width;
	int icon_height;
	int button_width;
	int button_count;
	int systray_width;
	int minh;
	int minw;
	int zen_wallpaper;
	int attach_last;
	int focus_new;
	int sleep_time;

	unsigned int xcolor_nborder;
	unsigned int xcolor_fborder;
	unsigned int xcolor_fbg;
	unsigned int xcolor_nbg;
	unsigned int xcolor_fshadow;
	unsigned int xcolor_nshadow;
	unsigned int xcolor_ftitle;
	unsigned int xcolor_ntitle;
	unsigned int xcolor_flbg;
	unsigned int xcolor_root;

	char* viewnames[32];

	struct {
		char* c;
		void (*func)(Client*);
	} buttons[32];

	struct {
		const char* key;
		void* f;
		const char* arg;
	} keys[100];
	struct {
		const char* key;
		void* f;
		const char* arg;
	} relkeys[64];

	ZwmLayout layouts[16];

	ZwmPolicy policies[16];

    char _lastcmd[1024];
} ZwmConfig;

extern ZwmConfig config;

typedef struct {
	Window win;
	Client* icons;
} ZwmTray;

typedef struct ZwmView {
	ZwmLayout* layout;
	Client* current;
	double mwfact;
	int screen;
} ZwmView;

enum {
	ZwmEventPropagate,
	ZwmEventConsume
};

typedef enum {
	/* our event numbers start after X11 events */
	ZwmClientFocus = LASTEvent,
	ZwmClientMap,
	ZwmClientUnmap,
	ZwmClientView,
	ZwmClientState,
	ZwmClientConfigure,
	ZwmViewChange,
	ZwmScreenSize,
	ZwmMaxEvents
} ZwmEvent;

#define _X(name) extern Atom name
#include "atoms.h"
#undef _X
//#define DEBUG
#ifdef DEBUG
#define DBG_ENTER() printf("Enter %s\n", __FUNCTION__)
#define ZWM_DEBUG(fmt, args...) printf("%s:%d: " fmt, __FUNCTION__, __LINE__, ##args)
#else
#define DBG_ENTER()
#define ZWM_DEBUG(fmt, args...)
#endif

#define MAX_SCREENS 32
#define MAX_VIEWS 32
#define MAX_CLIENTS 100

extern ZwmScreen screen[MAX_SCREENS];
extern ZwmView views[MAX_SCREENS];
extern int scr;
extern Display* dpy;
extern Window root;
extern Cursor cursor_normal;
extern unsigned int numlockmask;
extern Client* sel;
extern Client* head;
extern Client* tail;
extern Colormap cmap;
extern int session_dirty;

Bool zwm_client_visible(Client* c, int view);
Client* zwm_client_lookup(Window w);
Client* zwm_client_manage(Window w, XWindowAttributes* wa);
int zwm_client_screen(Client*);
void zwm_client_configure_window(Client* c);
void zwm_client_float(Client*);
void zwm_client_focus(Client* c);
void zwm_client_fullscreen(Client* c);
void zwm_client_kill(Client* c);
void zwm_client_moveresize(Client* c, int, int, int, int);
void zwm_client_push_head(Client*);
void zwm_client_push_next(Client* c, Client* prev);
void zwm_client_push_tail(Client*);
void zwm_client_raise(Client* c, Bool warp);
void zwm_client_refocus(void);
void zwm_client_remove(Client* c);
void zwm_client_restore_geometry(Client* c, ZwmGeom* g);
void zwm_client_save_geometry(Client* c, ZwmGeom* g);
void zwm_client_scan(void);
void zwm_client_set_view(Client* c, int v);
void zwm_client_setstate(Client* c, int state);
void zwm_client_toggle_floating(Client* c);
void zwm_client_unfullscreen(Client* c);
void zwm_client_unmanage(Client*);
void zwm_client_update_hints(Client* c);
void zwm_client_update_name(Client*);
void zwm_client_zoom(Client*);

int zwm_current_screen();
int zwm_current_view();

void zwm_decor_dirty(Client* c);
void zwm_decor_init(void);
void zwm_decor_update(Client* c);

void zwm_event_emit(ZwmEvent e, void* p);
void zwm_event_init(void);
void zwm_event_loop(void);
void zwm_event_quit();
void zwm_event_register(ZwmEvent e, ZwmEFunc f, void* priv);

void zwm_ewmh_init(void);
void zwm_ewmh_set_window_opacity(Window win, float opacity);

void zwm_key_bind(const char* keyname, void* f, const char* arg, int);
void zwm_key_init(void);

void zwm_layout_arrange(void);
void zwm_layout_dirty(void);
void zwm_layout_moveresize(Client* c, int x, int y, int w, int h);
void zwm_layout_rearrange(Bool force);
void zwm_layout_register(ZwmLFunc f, char* name, int);
void zwm_layout_set(const char* name);
void zwm_layout_push(const char* name);
void zwm_layout_pop(void);
void zwm_layout_autofact(Client* c);

void zwm_mouse_init(Display* dpy);
void zwm_mouse_cleanup(Display* dpy);
void zwm_mouse_grab(Client* c, Bool focused);
void zwm_mouse_warp(Client* c);

void zwm_panel_hide();
void zwm_panel_init(void);
void zwm_panel_show();
void zwm_panel_toggle(void);
void zwm_panel_new(void);
void zwm_panel_update(void);

void zwm_systray_new(void);
void zwm_systray_update(void);
void zwm_systray_hide(void);
void zwm_systray_detach(void);

void zwm_screen_rescan(Bool);
void zwm_screen_set_view(int scr, int view);

void zwm_util_free(void*);
void* zwm_util_malloc(size_t size);
void zwm_util_perror(const char* str);
void zwm_util_spawn(const char* cmd);

Bool zwm_view_has_clients(int v);
Bool zwm_view_mapped(int v);
void zwm_view_rescan();
void zwm_view_set(int v);
void zwm_view_update();

void zwm_wm_quit(const char* arg);
void zwm_wm_restart(const char*);
void zwm_wm_fallback(const char*);

Bool zwm_x11_atom_check(Window win, Atom bigatom, Atom smallatom);
void zwm_x11_atom_init(void);
ulong zwm_x11_atom_list(Window w, Atom a, Atom type, ulong* ret, ulong nitems, ulong* left);
Bool zwm_x11_atom_set(Window w, Atom a, Atom type, ulong* val, ulong);
Bool zwm_x11_atom_text(Window w, Atom atom, char* text, unsigned int size);
void zwm_x11_flush_events(long mask);
Bool zwm_x11_atom_get(Window w, Atom a, Atom type, ulong* val);
Atom zwm_x11_atom_get_atom(Window w, Atom prop);

#define zwm_client_foreach(c, args...) for ((c) = head, ##args; (c); (c) = (c)->next)
#define ZWM_ZEN_VIEW 9

KeySym zwm_getkey(void);
void zwm_zen(const char*);
void zwm_cycle(const char*);
void zwm_cycle2(const char*);
void zwm_fullscreen(const char*);
void zwm_focus_next(const char*);
void zwm_focus_prev(const char*);

void zwm_session_save(void);
void zwm_session_restore(void);

int zwm_util_getenv(unsigned long pid, const char* key, char* value);

#endif
