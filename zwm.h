#ifndef _ZWM_H
#define _ZWM_H

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xinerama.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <libzen/events.h>
#include <libzen/list.h>

typedef enum {
       	CurNormal,
       	CurResize,
       	CurMove,
       	CurLast
} ZenCursor;

typedef void (*KeyFunc)(const char *);
void zwm_init_cursor(Display *dpy);
void zwm_free_cursors(Display *dpy);
Cursor zwm_get_cursor(ZenCursor c);
void zwm_client_zoom(const char *arg);

#define INIT_ATOM(name) extern Atom name
#include "atoms.h"
#undef INIT_ATOM

struct ZenGeom
{
	int x;
	int y;
	int w;
	int h;
};

typedef struct ZenGeom ZenGeom;
#define MAX_SCREENS 32
extern ZenGeom screen[MAX_SCREENS];
extern int screen_count;
extern int scr;
extern unsigned int border_w;
extern Bool running;

void zwm_perror(const char *str);
void *zwm_malloc(size_t size);
void zwm_free(void *);

extern Display *dpy;
extern Window root;
extern unsigned int xcolor_normal;
extern unsigned int xcolor_focus;
extern unsigned int xcolor_focus_bg;
extern unsigned int xcolor_floating;
extern unsigned int xcolor_bg;
extern unsigned int numlockmask;
extern unsigned int num_clients;

/* client */

enum
{
	ZenNormalWindow,
	ZenDockWindow,
	ZenDialogWindow,
	ZenSplashWindow,
	ZenDesktopWindow
};

typedef struct Client Client;
struct Client
{
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
	ZenGeom fpos;
	ZenGeom bpos;
	int border;
	int color;
	int oborder;
	Window win;
	Window frame;
	int state;
	int focused;
	int type;
	int view;
	int noanim;
	int hastitle;
	int anim_steps;
	Bool isfloating;
	char name[256];
	ZenListNode node;
	Client *next;
	Client *prev;
	void *priv[0];
};
extern GC gc;
extern XFontStruct *font;
DECLARE_GLOBAL_LIST_FUNCTIONS(zwm_client,Client,node);
extern Client *sel;


Client *zwm_client_head(void);
void zwm_client_remove(Client *c);
void zwm_client_push_head(Client *);
void zwm_client_scan(void);
void zwm_client_manage(Window w, XWindowAttributes *wa);
void zwm_client_unmanage(Client *);
void zwm_client_mousemove(Client *c);
void zwm_client_mouseresize(Client *c);
void zwm_client_focus(Client *c);
void zwm_client_moveresize(Client *c, int,int,int,int);
void zwm_client_raise(Client *c);
void zwm_client_send_configure(Client *c);
void zwm_client_update_decoration(Client *c);
Bool zwm_client_visible(Client *c);
Client *zwm_client_next_visible(Client *c);
Client * zwm_client_get(Window w);
void zwm_client_update_name(Client *);
void zwm_client_iconify(Client *c);
void zwm_client_warp(Client *c);
void zwm_client_set_view(Client *c, int v);
void zwm_client_setstate(Client *c, int state);
int zwm_client_count(void);
void zwm_current_view(int  v);
void zwm_client_kill(Client *c);
void zwm_client_toggle_floating(Client *c);
void zwm_client_fullscreen(Client *c);
int zwm_client_private_key(void);
void zwm_client_set_private(Client *c, int key, void *data);
void *zwm_client_get_private(Client *c, int key);

void zwm_init_config(void);
void zwm_event_loop(void);
void zwm_update_screen_geometry();

int xerror(Display *dpy, XErrorEvent *ee);

void zwm_init_atoms(void);

//#define CONFIG_XINERAMA 1

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

typedef struct
{
	unsigned int auto_view;
	unsigned int border_width;
	unsigned int fake_screens;
	unsigned int screen_x;
	unsigned int screen_y;
	unsigned int screen_w;
	unsigned int screen_h;

	const char *normal_border_color;
	const char *focus_border_color;
	const char *focus_bg_color;
	const char *floating_border_color;
	const char *bg_color;

	float		opacity;
	int anim_steps;
	int show_title;
} ZenConfig;

extern ZenConfig config;

extern unsigned long num_views;
extern unsigned long current_view;


void zwm_plugin_load(const char **args);

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

typedef int (*ZenLFunc)(void );
typedef int (*ZenEFunc)(void *, void *);
typedef int (*ZenEFunc2)(ZenEvent, void *, void *);

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

typedef void (*ZenFunc)(const char **arg, void *);
ZenFunc zwm_user_func_get(const char* name);
void zwm_user_func_add(const char* name, ZenFunc func);
void zwm_user_init(void);
#define  USE_ZENCONFIG
#ifdef USE_ZENCONFIG
#include <libzen/config.h>
#else
typedef enum {
       	ZenTypeInt, ZenTypeString, ZenTypeFloat, ZenTypeUser,
	ZenTypeNONE 
} ZenConfigType ;
#endif
void zwm_config_add(const char* name, ZenConfigType type, void *addr);
int zwm_client_count(void);
int zwm_client_num_floating(void);
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
void zwm_client_move(Client *c, int x, int y);
struct ZenEventHandler
{
	ZenEFunc handler;
	ZenEFunc2 handler2;
	void *priv;
	ZenEventHandler *next;
};

void zwm_client_save_geometry(Client *c, ZenGeom *g);
void zwm_client_restore_geometry(Client *c, ZenGeom *g);
#endif
