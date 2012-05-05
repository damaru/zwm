#ifndef _ZWM_H
#define _ZWM_H

#define _GNU_SOURCE
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>

typedef unsigned long ulong;
typedef int (*ZwmLFunc)(int scrn, int view);
typedef int (*ZwmEFunc)(void *, void *);
typedef void (*KeyFunc)(const char *);

typedef enum {
       	CurNormal,
       	CurResize,
       	CurMove,
       	CurLast
} ZwmCursor;

typedef struct ZwmGeom
{
	int view;
	int x;
	int y;
	int w;
	int h;
} ZwmGeom;

typedef struct ZwmLayout
{
	ZwmLFunc handler;
	int skip;
	char name[64];
	struct ZwmLayout *next;
}ZwmLayout;

typedef struct ZwmScreen
{
	int prev;
	int view;
	int x;
	int y;
	int w;
	int h;
} ZwmScreen;

typedef struct ZwmView
{
	ZwmLayout *layout;
	int screen;
} ZwmView;

enum
{
	ZwmNormalWindow,
	ZwmDockWindow,
	ZwmDialogWindow,
	ZwmSplashWindow,
	ZwmDesktopWindow,
	ZwmFullscreenWindow
};

/* client */
typedef struct Client Client;
struct Client
{
	int view;
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

	int anim_steps;
	int border;
	int focused;
	int hastitle;
	int ignore;
	int noanim;
	int state;
	int type;
	int dirty;
	Bool isfloating;
	ZwmGeom fpos;
	ZwmGeom bpos;
	Window win;
	Window frame;
	Window lastfocused;
	Client* next;
	Client* prev;
	char name[256];
	char cname[256];
} __attribute__((aligned));

typedef struct
{
	unsigned int border_width;
	unsigned int screen_count;
	unsigned int num_views;
	unsigned int num_clients;

	const char *normal_border_color;
	const char *focus_border_color;
	const char *normal_bg_color;
	const char *focus_bg_color;
	const char *normal_shadow_color;
	const char *focus_shadow_color;
	const char *normal_title_color;
	const char *focus_title_color;

	const char *font;
	const char *icons;

	float opacity;
	int anim_steps;
	int show_title;
	int title_height;
	int title_y;
	int icon_y;
	int button_width;
	int button_count;
	int reparent;

	unsigned int xcolor_nborder;
	unsigned int xcolor_fborder;
	unsigned int xcolor_fbg;
	unsigned int xcolor_nbg;
	unsigned int xcolor_fshadow;
	unsigned int xcolor_nshadow;
	unsigned int xcolor_ftitle;
	unsigned int xcolor_ntitle;
	
	char *viewnames[32];

	struct {
		char *c;
		void (*func)(Client* );
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
	ZwmEventPropagate,
	ZwmEventConsume
};

typedef enum
{
	/* our event numbers start after X11 events */
	ZwmClientFocus = LASTEvent,
	ZwmClientUnFocus,
	ZwmClientProperty,
	ZwmClientMap,
	ZwmClientUnmap,
	ZwmClientView,
	ZwmClientState,
	ZwmClientDamage,
	ZwmClientResize,
	ZwmClientFloating,
	ZwmClientConfigure,
	ZwmViewChange,
	ZwmLayoutChange,
	ZwmX11Event,
	ZwmScreenSize,
	ZwmManageScreen,
	ZwmAllEvents,
	ZwmMaxEvents
}ZwmEvent;

#define _X(name) extern Atom name
#include "atoms.h"
#undef _X

#ifdef DEBUG
#define DBG_ENTER() printf("Enter %s\n",__FUNCTION__)
#define ZWM_DEBUG(fmt,args...) printf("%s:%d: "fmt,__FUNCTION__,__LINE__,##args)
#else
#define DBG_ENTER() 
#define ZWM_DEBUG(fmt,args...) 
#endif

#define MAX_SCREENS 32
#define MAX_VIEWS 32
#define MAX_CLIENTS 100

extern ZwmScreen screen[MAX_SCREENS];
extern ZwmView views[MAX_SCREENS];
extern int scr;
extern Display *dpy;
extern Window root;
extern unsigned int numlockmask;
extern GC gc;
extern Client* sel;
extern Client* head;
extern Client* tail;

void zwm_client_configure_window(Client* c);
void zwm_client_focus(Client* c);
void zwm_client_fullscreen(Client* c);
Client* zwm_client_lookup(Window w);
void zwm_client_kill(Client* c);
Client* zwm_client_manage(Window w, XWindowAttributes *wa);
void zwm_client_mousemove(Client* c);
void zwm_client_mouseresize(Client* c);
void zwm_client_moveresize(Client* c, int,int,int,int);
Client* zwm_client_next_visible(Client* c);
void zwm_client_push_head(Client* );
void zwm_client_push_tail(Client* );
void zwm_client_raise(Client* c, Bool warp);
void zwm_client_refocus(void);
void zwm_client_remove(Client* c);
void zwm_client_restore_geometry(Client* c, ZwmGeom *g);
void zwm_client_save_geometry(Client* c, ZwmGeom *g);
void zwm_client_scan(void);
int zwm_client_screen(Client* );
void zwm_client_setstate(Client* c, int state);
void zwm_client_set_view(Client* c, int v);
void zwm_client_toggle_floating(Client* c);
void zwm_client_unfullscreen(Client* c);
void zwm_client_unmanage(Client* );
void zwm_client_update_name(Client* );
Bool zwm_client_visible(Client* c, int view);
void zwm_client_warp(Client* c);
void zwm_client_zoom(Client* );

int zwm_current_screen();
int zwm_current_view();

void zwm_decor_dirty(Client* c);
void zwm_decor_init(void);
void zwm_decor_update(Client* c);

void zwm_event_emit(ZwmEvent e, void *p);
void zwm_event_init(void);
void zwm_event_loop(void);
void zwm_event_quit();
void zwm_event_register(ZwmEvent e, ZwmEFunc f, void *priv);

void zwm_ewmh_init(void);
void zwm_ewmh_set_window_opacity(Window win, float opacity);

void zwm_key_bind(const char* keyname, void *f, const char *arg);
void zwm_key_init(void);

void zwm_layout_arrange(void);
void zwm_layout_dirty(void);
void zwm_layout_init(void);
void zwm_layout_moveresize(Client* c, int x, int y, int w, int h);
void zwm_layout_next(void);
void zwm_layout_rearrange(Bool force);
void zwm_layout_register(ZwmLFunc f, char *name, int);
void zwm_layout_set(const char *name);

void zwm_panel_hide();
void zwm_panel_init(void);
void zwm_panel_show();
void zwm_panel_toggle(void);

void zwm_screen_rescan(Bool);
void zwm_screen_set_view(int scr, int view);

void zwm_util_free(void *);
void *zwm_util_malloc(size_t size);
void zwm_util_perror(const char *str);
void zwm_util_spawn(const char *cmd);

Bool zwm_view_has_clients(int v);
Bool zwm_view_mapped(int v);
void zwm_view_rescan();
void zwm_view_set(int  v);
void zwm_view_update();

void zwm_wm_quit(const char *arg);
void zwm_wm_restart(const char *);

Bool zwm_x11_atom_check(Window win, Atom bigatom, Atom smallatom);
void zwm_x11_atom_init(void);
ulong zwm_x11_atom_list(Window w, Atom a, Atom type, ulong *ret, ulong nitems, ulong *left);
Bool zwm_x11_atom_set(Window w, Atom a, Atom type, ulong *val, ulong);
Bool zwm_x11_atom_text(Window w, Atom atom, char *text, unsigned int size);
void zwm_x11_cursor_free(Display *dpy);
Cursor zwm_x11_cursor_get(ZwmCursor c);
void zwm_x11_cursor_init(Display *dpy);
void zwm_x11_flush_events(long mask);

#define zwm_client_foreach(c) for((c)=head;(c);(c)=(c)->next)

#endif
