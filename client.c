#include "zwm.h"
#include <string.h>
#include <stdio.h>

DEFINE_GLOBAL_LIST_DEFINE(zwm_client,Client,node);
DEFINE_GLOBAL_LIST_HEAD(zwm_client,Client,node);
DEFINE_GLOBAL_LIST_TAIL(zwm_client,Client,node);
DEFINE_GLOBAL_LIST_NEXT(zwm_client,Client,node);
DEFINE_GLOBAL_LIST_PREV(zwm_client,Client,node);
DEFINE_GLOBAL_LIST_PUSH_HEAD(zwm_client,Client,node);
DEFINE_GLOBAL_LIST_PUSH_TAIL(zwm_client,Client,node);
DEFINE_GLOBAL_LIST_REMOVE(zwm_client,Client,node);

unsigned int num_floating = 0;
static int privcount = 0;
static void grabbuttons(Client *c, Bool focused);
Client *sel = NULL;

static int zwm_x11_window_type(Window w);
static void create_frame_window(Client *c);

void zwm_client_configure_window(Client *c) {
	int th = 0;

	if(c->hastitle){
		th = config.title_height;
	}

#if 0
	XWindowChanges wc;
	wc.x = c->x;
	wc.y = c->y+th;
	wc.width = c->w;
	wc.height = c->h-th;
	wc.border_width = c->border;

	if(config.reparent && c->hastitle){
		wc.x = 0;
		wc.y = th;
		wc.width = c->w - c->border;
		wc.height = c->h - th - c->border;
		wc.border_width = 0;
	}

	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight, &wc);
#endif
	XConfigureEvent ce;
	if(c->type != ZenNormalWindow)
		return;
	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->border;
	ce.above = None;
	ce.override_redirect = False;

	if(config.reparent && c->hastitle){
		ce.x = 0;
		ce.y = th;
		ce.width = c->w - c->border;
		ce.height = c->h - th - c->border;
		ce.border_width = 0;
	}

	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void zwm_client_iconify(Client *c) {
	if(c == NULL) {
		c = sel;
	}

	if(!c){
		return;
	}

	zwm_client_setstate(c, IconicState);
}

void zwm_client_setstate(Client *c, int state)
{
	c->state = state;
	if (state == NormalState){
		XRaiseWindow(dpy, c->frame);
		XRaiseWindow(dpy, c->win);
		zwm_client_focus(c);
	} else {
		XLowerWindow(dpy, c->win);
		XLowerWindow(dpy, c->frame);
		zwm_client_refocus();
	}
	zwm_event_emit(ZenClientState, c);
	zwm_layout_dirty();
}

void zwm_client_scan(void)
{
	Window *wins = NULL, d1, d2;
	XWindowAttributes wa;
	unsigned int num;
	int i;

	XQueryTree(dpy, root, &d1, &d2, &wins, &num);
	for (i = 0; i < num; i++) {
        	XGetWindowAttributes(dpy, wins[i], &wa);
		if (wa.map_state == IsViewable) {
			XUnmapWindow(dpy, wins[i]);
			Client *c = zwm_client_manage(wins[i], &wa);
			if(c)c->ignore=1;
		}
	}

	if(wins)
		XFree(wins);
}

Client* zwm_client_manage(Window w, XWindowAttributes *wa)
{
	int scr = zwm_current_screen();
	int vew = zwm_current_view();
	Client* old = zwm_client_get(w);
	
	if(old && old->win == w ){
		return NULL;
	}

	if(wa->override_redirect){
		return NULL;
	}

	Client *c =  zwm_util_malloc(sizeof(Client) + (sizeof(void*)*privcount));
	c->win = w;
	c->isfloating = False;
	c->x = wa->x;
	c->y = wa->y;
	c->w = wa->width;
	c->h = wa->height;
	c->state =  NormalState;
	c->view = vew;
	c->border = config.border_width;
	c->type = zwm_x11_window_type(w);
	zen_list_node_init(&c->node);
	zwm_client_update_name(c);

	switch (c->type) {
		case ZenDesktopWindow:
			c->hastitle = 0;
			c->isfloating = 0;
			XMapWindow(dpy, w);
			zwm_client_configure_window(c);
			break;
		case ZenDockWindow:
			c->border = 0;
			c->hastitle = 0;
			XSelectInput(dpy, w, PropertyChangeMask);
			XMapWindow(dpy, w);
			zwm_event_emit(ZenClientMap, c);
			zwm_client_configure_window(c);
			return c;
			break;
		case ZenDialogWindow:
			c->hastitle = 1;
			c->type = ZenNormalWindow;
		case ZenSplashWindow:
			c->isfloating = True;
			c->x = screen[scr].x + (wa->x?wa->x:((screen[scr].w - wa->width)/2));
			c->y = wa->y?wa->y:((screen[scr].h - wa->height)/2);
			num_floating++;
			break;
		case ZenFullscreenWindow:
			c->type = ZenNormalWindow;
			zwm_client_fullscreen(c);
			break;
		case ZenNormalWindow:
			c->hastitle = 1;
			break;
	}

	if(c->hastitle){
		c->h += config.title_height;
		create_frame_window(c);
	}

	XMapWindow(dpy, c->win);
	XRaiseWindow(dpy, c->win);

	if(c->isfloating)
		zwm_ewmh_set_window_opacity(c->frame, config.opacity);

	zwm_decor_update(c);
	zwm_client_save_geometry(c, &c->fpos);
	zwm_client_push_head(c);
	zwm_event_emit(ZenClientMap, c);
	zwm_layout_dirty();
	zwm_client_configure_window(c);
	zwm_client_warp(c);
	zwm_client_raise(c);
	return c;
}

void zwm_client_unmanage(Client *c) {
	if(c->ignore)
	{
		c->ignore--;
		return;
	}
	XGrabServer(dpy);

	if(config.reparent){
		XReparentWindow(dpy, c->win, root, c->x, c->y+config.title_height);
	}

	zwm_client_remove(c);
	if(c->isfloating){
		num_floating--;
	}	
	if(sel == c){
		Client *n = zwm_client_get(c->lastfocused);
		if (n && n->win == c->lastfocused) {
			zwm_client_raise(n);
			zwm_client_warp(n);
		} else {
			zwm_client_refocus();
		}
	}

	XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
	if(c->frame){
		XDestroyWindow(dpy, c->frame);
	}
	XUngrabServer(dpy);
	zwm_view_rescan();
	zwm_event_emit(ZenClientUnmap, c);
	free(c);
}

Client *zwm_client_get(Window w) {
	Client *c;

	for(c = zwm_client_head();
		c && (c->win != w && c->frame != w);
	       	c = zwm_client_next(c));
	return c;
}

int zwm_client_count(void)
{
	return zwm_client->count;
}

Bool zwm_client_visible(Client *c, int view) 
{
	return c->state == NormalState &&
		zwm_view_mapped(c->view) && c->view == view &&
		(c->type == ZenNormalWindow || c->type == ZenDialogWindow) ;
}

void zwm_client_warp(Client *c)
{
	if(c) {
		XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask );
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w / 2, c->h / 2);
		zwm_x11_flush_events(EnterWindowMask);
		XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);
	}
}

void zwm_client_refocus(void)
{
	Client *c = zwm_client_head();
	while(c && !zwm_client_visible(c, zwm_current_view()))
	{
		c = zwm_client_next(c);
	}
	if (c) {
		zwm_client_focus(c);
	} else {
		sel = NULL;
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	}
}

void zwm_client_focus(Client *c) 
{
	if(!zwm_client_visible(c, zwm_current_view())){
		zwm_client_refocus();
	}

	/* unfocus */
	if (sel && sel != c) {
		c->lastfocused = sel->win;
		sel->focused = False;
		grabbuttons(sel, False);
		zwm_decor_update(sel);
		zwm_event_emit(ZenClientUnFocus, sel);
	}

	/* focus */
	grabbuttons(c, True);
	sel = c;
	c->focused = True;
	XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
	zwm_decor_update(c);
	zwm_event_emit(ZenClientFocus, c);
}

void zwm_client_raise(Client *c)
{
	zwm_client_set_view(c, zwm_current_view());
	zwm_client_setstate(c, NormalState);
}

static void grab_one_button(Window win, unsigned int button)
{
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	int j;

	for(j = 0; j < sizeof(modifiers)/sizeof(unsigned int); j++) {
		XGrabButton(dpy, button, MODKEY | modifiers[j], win, False,
			       	BUTTONMASK, GrabModeAsync, GrabModeSync,
			       	None, None);
	}
}

static void grabbuttons(Client *c, Bool focused) {
	if(focused) {
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		grab_one_button(c->win, Button1);
		grab_one_button(c->win, Button2);
		grab_one_button(c->win, Button3);
	} else {
		XGrabButton(dpy, AnyButton, AnyModifier, c->win, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
	}
}

void zwm_client_moveresize(Client *c, int x, int y, int w, int h)
{

	c->x =  x;
	c->y =  y;
	c->w =  w;
	c->h =  h;

	if(c->hastitle && c->frame){
		if(config.reparent) {
			XMoveResizeWindow(dpy, c->frame, x, y, w, h);
			XMoveResizeWindow(dpy, c->win, c->border, config.title_height, 
					w-2*c->border, h-config.title_height-2*c->border);
		} else {
			XMoveResizeWindow(dpy, c->frame, x, y, w, config.title_height);
			XMoveResizeWindow(dpy, c->win, x, y+config.title_height, w, h-config.title_height);
		}
	} else {
		XMoveResizeWindow(dpy, c->win, x, y, w, h);
		if(c->frame)XMoveResizeWindow(dpy, c->frame, x, y, w, h);
	}
	XSync(dpy, False);
}

void zwm_client_fullscreen(Client *c)
{
	c->isfloating = True;
	c->hastitle = 0;
	num_floating++;
	c->border = 0;
	XSetWindowBorderWidth(dpy, c->win, c->border);
	zwm_client_moveresize(c, screen[0].x - c->border,
			   screen[0].y - c->border, 
			   screen[0].w , 
			   screen[0].h );
}

void zwm_client_unfullscreen(Client *c)
{
	c->isfloating = False;
	c->hastitle = 1;
	num_floating--;
	c->border = config.border_width;
	XSetWindowBorderWidth(dpy, c->win, c->border);
	zwm_decor_update(c);
	zwm_layout_arrange();
}

void zwm_client_float(Client *c)
{
	if(!c->isfloating){
		zwm_client_toggle_floating(c);
		zwm_layout_rearrange();
	}
}

void zwm_client_mousemove(Client *c) {
	int x1, y1, ocx, ocy, di, nx, ny;
	unsigned int dui;
	Window dummy;
	XEvent ev;
	DBG_ENTER();

	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, zwm_x11_cursor_get(CurMove), CurrentTime) != GrabSuccess)
		return;
	

	zwm_client_save_geometry(c, &c->fpos);
	zwm_client_float(c);

	XQueryPointer(dpy, root, &dummy, &dummy, &x1, &y1, &di, &di, &dui);

	do {
		XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			zwm_event_emit(ev.type, &ev);
			break;
		case MotionNotify:
			XSync(dpy, False);
			nx = ocx + (ev.xmotion.x - x1);
			ny = ocy + (ev.xmotion.y - y1);
			zwm_client_moveresize(c, nx, ny, c->w, c->h);
			break;
		}
	} while(ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
}

void zwm_client_mouseresize(Client *c) {
	int ocx, ocy;
	int nw, nh;
	XEvent ev;
	DBG_ENTER();

	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, zwm_x11_cursor_get(CurResize), CurrentTime) != GrabSuccess)
		return;

	zwm_client_save_geometry(c, &c->fpos);
	zwm_client_float(c);

	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->border - 1, c->h + c->border - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			zwm_event_emit(ev.type, &ev);
			break;
		case MotionNotify:
			XSync(dpy, False);
			if((nw = ev.xmotion.x - ocx - 2 * c->border + 1) <= 0)
				nw = 1;
			if((nh = ev.xmotion.y - ocy - 2 * c->border + 1) <= 0)
				nh = 1;
			zwm_client_moveresize(c, c->x, c->y, nw, nh);
			break;
		}
	} while(ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->border - 1,
		       	c->h + c->border - 1);
	XUngrabPointer(dpy, CurrentTime);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

static Bool
isprotodel(Client *c) {
	int i, n;
	Atom *protocols;
	Bool ret = False;

	if(XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		for(i = 0; !ret && i < n; i++)
			if(protocols[i] == WM_DELETE_WINDOW)
				ret = True;
		XFree(protocols);
	}
	return ret;
}

void zwm_client_kill(Client *c)
{
	XEvent ev;

	if(!c) {
		c = sel;

		if(!c) {
			return;
		}
	}

	if(isprotodel(c)) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = WM_PROTOCOLS;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = WM_DELETE_WINDOW;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	else 
	{
		XKillClient(dpy, c->win);
	}
}

void zwm_client_set_view(Client *c, int v)
{
	if (c->view != v) {
		c->view = v;
		if(v > config.num_views)
		{
			config.num_views = v+1;
		}
		zwm_view_rescan();
		zwm_event_emit(ZenClientView, c);
	}
}

void zwm_client_update_name(Client *c)
{
	if(!zwm_x11_atom_text(c->win, _NET_WM_NAME, c->name, sizeof c->name))
	zwm_x11_atom_text(c->win, WM_NAME, c->name, sizeof c->name);
	zwm_x11_atom_text(c->win, WM_CLASS, c->cname, sizeof c->cname);
	zwm_event_emit(ZenClientProperty, c);
	zwm_decor_update(c);

}

Client *zwm_client_next_visible(Client *c)
{
       for(; c; c = zwm_client_next(c)) {
               if(zwm_client_visible(c, zwm_current_view()) && !c->isfloating){
                       return c;
               }
       }
       return c;
}

void zwm_client_toggle_floating(Client *c) {
	if(!c){
		c = sel;
		if (!c) {
			return;
		}
	}
	c->isfloating = !c->isfloating;
	zwm_event_emit(ZenClientFloating, c);

	if(c->isfloating){
		zwm_ewmh_set_window_opacity(c->frame, config.opacity);
		num_floating++;
		zwm_client_restore_geometry(c, &c->fpos);
	} else {
		num_floating--;
		zwm_ewmh_set_window_opacity(c->frame, 1);
		zwm_client_save_geometry(c, &c->fpos);
	}
	zwm_layout_dirty();
}

void zwm_client_zoom(Client *c) {
	if(c) {
		zwm_client_remove(c);
		zwm_client_push_head(c);
		zwm_layout_arrange();
		zwm_client_raise(c);
		zwm_client_warp(c);
	}
}

void zwm_focus_prev(const char *arg) {
	Client *c;

	if(!sel) {
		zwm_client_refocus();
		return;
	}

	for(c = zwm_client_prev(sel);
		       	c && !zwm_client_visible(c, zwm_current_view());
		       	c = zwm_client_prev(c));
	if(!c) {
		for(c = zwm_client_tail();
		  c && !zwm_client_visible(c, zwm_current_view());
		  c = zwm_client_prev(c));
	}

	if(c) {
		zwm_client_raise(c);
		zwm_client_warp(c);
	}
}

void zwm_focus_next(const char *arg) {
	Client *c;

	if(!sel) {
		zwm_client_refocus();
		return;
	}

	for(c = zwm_client_next(sel);
		       	c && !zwm_client_visible(c, zwm_current_view());
		       	c = zwm_client_next(c));
	if(!c) {
		for(c = zwm_client_head();
		  c && !zwm_client_visible(c, zwm_current_view()); c = zwm_client_next(c));
	}

	if(c) {
		zwm_client_raise(c);
		zwm_client_warp(c);
	}
}

void zwm_client_save_geometry(Client *c, ZenGeom *g)
{
	/* we don't save view because we don't want the client to jump to
	 * unmapped view when floats are toggled */
	g->x = c->x;
	g->y = c->y;
	g->w = c->w;
	g->h = c->h;
}

void zwm_client_restore_geometry(Client *c, ZenGeom *g)
{
	zwm_layout_moveresize(c, g->x, g->y, g->w, g->h);
}

static int zwm_x11_window_type(Window w)
{
	if(zwm_x11_atom_check(w, _NET_WM_STATE, _NET_WM_STATE_FULLSCREEN)){
		return ZenFullscreenWindow;
	} else if(zwm_x11_atom_check(w, _NET_WM_WINDOW_TYPE, _NET_WM_STATE_MODAL)){
		return ZenDialogWindow;
	} else {
		Atom a[32];
		unsigned long left;
		unsigned long i;
		unsigned long n = zwm_x11_atom_list(w, _NET_WM_WINDOW_TYPE, XA_ATOM, 
				a, 32, &left);
		for(i = 0; i<n; i++) {
#define CHECK_RET(t, r) do{if(a[i] == t){return r;}}while(0)
			CHECK_RET(_NET_WM_WINDOW_TYPE_DOCK, ZenDockWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_NORMAL, ZenNormalWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DIALOG, ZenDialogWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_UTILITY, ZenDialogWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DESKTOP, ZenDesktopWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_SPLASH, ZenSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_TOOLBAR, ZenSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DND, ZenSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_MENU, ZenSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU, ZenSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_TOOLTIP, ZenSplashWindow);
		}
	}
	return ZenNormalWindow;
}

static void create_frame_window(Client *c)
{
	XClassHint hint = {"zwm", "ZWM"};
	int scr = DefaultScreen(dpy);
	XSetWindowAttributes pattr;
	pattr.override_redirect = True;
	pattr.event_mask = StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask |
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ExposureMask |
		EnterWindowMask;
	c->frame =  XCreateWindow(dpy, root, c->x, c->y, c->w, config.title_height, c->border,
			DefaultDepth(dpy, scr), CopyFromParent, DefaultVisual(dpy, scr),
			CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);
	if(config.show_title)XMapWindow(dpy,c->frame);
	XSetClassHint(dpy, c->frame, &hint);
	XRaiseWindow(dpy, c->frame);
	if (config.reparent) {
		XReparentWindow(dpy, c->win, c->frame, 0, config.title_height);
		XMoveResizeWindow(dpy, c->win, 0, config.title_height, c->w, c->h-config.title_height);
	}
}


