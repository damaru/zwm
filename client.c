#include "zwm.h"

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

void zwm_auto_view(int v);

/*
 * send size information to the client
 */
void
zwm_client_send_configure(Client *c) {
	XConfigureEvent ce;
	if(c->type != ZenNormalWindow)
		return;
	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y+20;
	ce.width = c->w;
	ce.height = c->h-20;
	ce.border_width = c->border;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);

	ce.y = c->y;
	ce.height = 20;
	XSendEvent(dpy, c->frame, False, StructureNotifyMask, (XEvent *)&ce);
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.  */
int
xerror(Display *dpy, XErrorEvent *ee) {

#if 0
	if(ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
//	return xerrorxlib(dpy, ee); /* may call exit */
#endif
	return 0;
}


static int
get_type(Window w)
{
	Atom t =  zwm_x11_get_window_type(w);
#define CHECK_RET(a, b) do{if(t == a){return b;}}while(0)

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
	if(zwm_x11_check_atom(w, _NET_WM_WINDOW_TYPE, _NET_WM_STATE_MODAL)){
		return ZenDialogWindow;
	}
	return ZenNormalWindow;
}

void
zwm_client_iconify(Client *c) {
	if(c == NULL) {
		c = sel;
	}

	if(!c){
		return;
	}

	zwm_client_setstate(c, IconicState);
}

void
zwm_client_setstate(Client *c, int state)
{
	c->state = state;
	if (state == NormalState){
		XRaiseWindow(dpy, c->frame);
		XRaiseWindow(dpy, c->win);
	} else {
		XLowerWindow(dpy, c->win);
		XLowerWindow(dpy, c->frame);
	}
	zwm_event_emit(ZenClientState, c);
	zwm_layout_arrange();
	zwm_client_focus(NULL);
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
			zwm_client_manage(wins[i], &wa);
		}
	}
	if(wins)
		XFree(wins);
}

Client *zwm_alloc_client(Window w, XWindowAttributes *wa)
{
	Client *c =  zwm_malloc(sizeof(Client) + (sizeof(void*)*privcount));
	c->win = w;
	c->isfloating = False;
	c->x = wa->x;
	c->y = wa->y;
	c->w = wa->width;
	c->h = wa->height;
	c->state =  NormalState;
	c->view = current_view;
	c->border = config.border_width;
	c->color = xcolor_normal;
	c->type = ZenNormalWindow;
	zen_list_node_init(&c->node);
	zwm_client_update_name(c);
	return c;
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
	c->frame =  XCreateWindow(dpy, root, c->x, c->y, c->w, 20, 1,
			DefaultDepth(dpy, scr), CopyFromParent, DefaultVisual(dpy, scr),
			CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);
	if(config.show_title)XMapWindow(dpy,c->frame);
	XSetClassHint(dpy, c->frame, &hint);
	//XReparentWindow(dpy, c->win, c->frame, 0, 20);
}

void
zwm_client_update_decoration(Client *c)
{
	int color = xcolor_focus;
	int fill = xcolor_focus_bg;

	if(c->type != ZenNormalWindow && c->type != ZenDialogWindow){
		return;
	}

	if(c->focused == False){
		color = c->color;
		fill = xcolor_normal;
	}

	XSetWindowBorder(dpy, c->win, color);
	if(c->frame) {
		XSetWindowBackground(dpy, c->frame, fill);
		XSetWindowBorder(dpy, c->frame, color);
		XSetForeground(dpy, gc, fill);
		XFillRectangle (dpy, c->frame, gc, 0, 0, c->w, 20);
		XSetForeground(dpy, gc, xcolor_bg);//TODO make fontgc
		XDrawString(dpy, c->frame, gc, 5, font->ascent+font->descent-2, c->name, strlen(c->name));
		XSetForeground(dpy, gc, BlackPixel(dpy,0));//TODO make fontgc
		XDrawString(dpy, c->frame, gc, 4, font->ascent+font->descent-2-1, c->name, strlen(c->name));
	}
}



void zwm_client_manage(Window w, XWindowAttributes *wa)
{
	XWindowChanges wc;

	Client* old = zwm_client_get(w);
	if(old && old->win == w ){
		return;
	}

	if(wa->override_redirect){
		return ;
	}

	Client *c = zwm_alloc_client(w, wa);
	c->type = get_type(w);
	switch (c->type) {
		case ZenDesktopWindow:
			return;
			break;
		case ZenDockWindow:
			c->border = 0;
			XSelectInput(dpy, w, PropertyChangeMask);
			XConfigureWindow(dpy, w, CWBorderWidth, &wc);
			XMapWindow(dpy, w);
			break;
		case ZenDialogWindow:
			c->hastitle = 1;
			c->type = ZenNormalWindow;
		case ZenSplashWindow:
			c->isfloating = True;
			c->x = wa->x?wa->x:((screen[0].w - wa->width)/2);
			c->y = wa->y?wa->y:((screen[0].h - wa->height)/2);
			c->w = wa->width?wa->width:(screen[0].w/2);
			c->h = wa->height?wa->height:(screen[0].h/2);
			wc.border_width = 0;
			num_floating++;
			break;
		case ZenNormalWindow:
			c->hastitle = 1;
			break;
	}

	if(c->hastitle){
		c->h += 20;
		create_frame_window(c);
	}
	wc.x = c->x;
	wc.y = c->y;
	wc.width = c->w;
	wc.height = c->h;
	wc.border_width = c->border;
	XConfigureWindow(dpy, w, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	zwm_client_send_configure(c);
	XMapWindow(dpy, c->win);
	zwm_client_update_decoration(c);

	zwm_client_push_head(c);
	zwm_event_emit(ZenClientMap, c);
	zwm_layout_arrange();
	zwm_client_focus(c);
	zwm_client_warp(c);
	return;
}

int xerrordummy (Display *dpy, XErrorEvent *ee) 
{
	return 0;
}

void
zwm_client_unmanage(Client *c) {
	zwm_event_emit(ZenClientUnmap, c);
	XGrabServer(dpy);
	XSetErrorHandler(xerrordummy);
	zwm_client_remove(c);
	if(c->isfloating){
		num_floating--;
	}	
	if(sel == c) {
		zwm_client_focus(NULL);
	}

	XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
	if(c->frame){
		XUnmapWindow(dpy, c->frame);
		XDestroyWindow(dpy, c->frame);
	}
	free(c);

	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XUngrabServer(dpy);
	zwm_layout_arrange();
	zwm_auto_view(0);
}


Client *
zwm_client_get(Window w) {
	Client *c;

	for(c = zwm_client_head();
		c && (c->win != w && c->frame != w);
	       	c = zwm_client_next(c));
	return c;
}

int
zwm_client_count(void){
	return (zwm_client->count);
}

Bool
zwm_client_visible(Client *c) 
{
	return c->state == NormalState &&
		c->view == current_view && 
		(c->type == ZenNormalWindow || c->type == ZenDialogWindow) ;
}

void
zwm_client_warp(Client *c)
{
	if(c) {
		XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask );
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w / 2, c->h / 2);
		zwm_event_flush_x11(EnterWindowMask);
		XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);
	}
}

void
zwm_client_focus(Client *c) 
{
	if(!c || (c && !zwm_client_visible(c))){  
		/* find a client*/
		c = zwm_client_head();
		while(c && !zwm_client_visible(c))
		{
			c = zwm_client_next(c);
		}
	}

	/* unfocus */
	if (sel && sel != c) {
		sel->focused = False;
		grabbuttons(sel, False);
		zwm_client_update_decoration(sel);
		zwm_event_emit(ZenClientUnFocus, sel);
	}

	/* focus */
	if(c) {
		grabbuttons(c, True);
		sel = c;
		c->focused = True;
//		XSetWindowBorder(dpy, c->win, c->color);
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		zwm_client_update_decoration(c);
		zwm_event_emit(ZenClientFocus, c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	}
}

void
zwm_client_raise(Client *c)
{
	zwm_client_setstate(c, NormalState);
	zwm_client_set_view(c, current_view);
	zwm_client_update_decoration(c);
}

static void
grab_one_button(Window win, unsigned int button)
{
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	int j;

	for(j = 0; j < sizeof(modifiers)/sizeof(unsigned int); j++) {
		XGrabButton(dpy, button, MODKEY | modifiers[j], win, False,
			       	BUTTONMASK, GrabModeAsync, GrabModeSync,
			       	None, None);
	}
}

static void
grabbuttons(Client *c, Bool focused) {
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

void zwm_client_move(Client *c, int x, int y)
{
	c->x =  x;
	c->y =  y;
	if(c->frame){
		XMoveWindow(dpy, c->win, x, y + 20);
		XMoveWindow(dpy, c->frame, x, y);
	}else {
		XMoveWindow(dpy, c->win, x, y );
	}
}


void
zwm_client_moveresize(Client *c, int x, int y, int w, int h)
{

	c->x =  x;
	c->y =  y;
	c->w =  w;
	c->h =  h;
	if(c->frame){
		XMoveResizeWindow(dpy, c->win, x, y+20, w, h-20);
		XMoveResizeWindow(dpy, c->frame, x, y, w, 20);
	} else {
		XMoveResizeWindow(dpy, c->win, x, y, w, h);
	}
	XSync(dpy, False);
}

void
zwm_client_fullscreen(Client *c)
{
	c->isfloating = True;
	num_floating++;
	zwm_client_moveresize(c, screen[0].x - c->border,
			   screen[0].y - c->border, 
			   screen[0].w , 
			   screen[0].h );
}

void
zwm_client_float(Client *c)
{
	c->isfloating = True;
	num_floating++;
	c->color = xcolor_floating;
	zwm_layout_arrange();
}


void
zwm_client_mousemove(Client *c) {
	int x1, y1, ocx, ocy, di, nx, ny;
	unsigned int dui;
	Window dummy;
	XEvent ev;
	DBG_ENTER();

	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, zwm_get_cursor(CurMove), CurrentTime) != GrabSuccess)
		return;
	

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

void
zwm_client_mouseresize(Client *c) {
	int ocx, ocy;
	int nw, nh;
	XEvent ev;
	DBG_ENTER();

	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, zwm_get_cursor(CurResize), CurrentTime) != GrabSuccess)
		return;

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

void
zwm_client_kill(Client *c)
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

void
zwm_current_view(int  v)
{
	if (v < num_views && v != current_view) {
		current_view = v;
		zwm_layout_arrange();
		zwm_client_focus(NULL);
		zwm_event_emit(ZenView, (void*)current_view);
	}
}

void zwm_auto_view(int v)
{
	Client *c;
	int max_view = 0;
	int min_view = v;
	int current_count = 0;

	if (config.auto_view) {
		for(c = zwm_client_head();
		       	c ;
		       	c = zwm_client_next(c))
		{
			max_view = c->view > max_view ? c->view: max_view;
			min_view = c->view < min_view ? c->view: min_view;
			if(c->view == current_view)
			{
				current_count++;
			}
		}

		num_views = max_view + 1;
		if(!current_count)
			zwm_current_view(min_view);
	}
}

void
zwm_client_set_view(Client *c, int v)
{
	c->view = v;
	zwm_client_focus(NULL);
	zwm_auto_view(v);
	zwm_layout_arrange();
	zwm_event_emit(ZenClientView, c);
}

void
zwm_client_update_name(Client *c)
{
	if(!zwm_x11_get_text_property(c->win, _NET_WM_NAME, c->name, sizeof c->name))
	zwm_x11_get_text_property(c->win, WM_NAME, c->name, sizeof c->name);
	zwm_event_emit(ZenClientProperty, c);
	zwm_client_update_decoration(c);

}

Client *zwm_client_next_visible(Client *c)
{
	for(; c; c = zwm_client_next(c)) {
		if(zwm_client_visible(c) && !c->isfloating){
			return c;
		}
	}
	return c;
}

void
zwm_client_toggle_floating(Client *c) {
	if(!c){
		c = sel;
		if (!c) {
			return;
		}
	}
	c->isfloating = !c->isfloating;
	zwm_event_emit(ZenClientFloating, c);

	if(c->isfloating){
		num_floating++;
		c->color = xcolor_floating;
		zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
	} else {
		num_floating--;
		c->color = xcolor_normal;
	}
	zwm_layout_arrange();
}

int zwm_client_private_key(void)
{
	return privcount++;
}

void zwm_client_set_private(Client *c, int key, void *data)
{
	c->priv[key] = data;
}

void *zwm_client_get_private(Client *c, int key)
{
	return c->priv[key];
}

int zwm_client_num_floating(void)
{
	return num_floating;
}

void zwm_client_zoom(const char *arg) {
	Client *c = sel;
	if( c == zwm_client_head()) {
		c = zwm_client_next_visible(zwm_client_next(c));
	}

	if(c) {
		zwm_event_emit(ZenClientUnmap, c);
		zwm_client_remove(c);
		zwm_client_push_head(c);
		zwm_layout_arrange();
		zwm_client_focus(c);
		zwm_client_raise(c);
		zwm_client_warp(c);
		zwm_event_emit(ZenClientMap, c);
	}
}

void
zwm_focus_prev(const char *arg) {
	Client *c;

	if(!sel) {
		zwm_client_focus(NULL);
		return;
	}

	for(c = zwm_client_prev(sel);
		       	c && !zwm_client_visible(c);
		       	c = zwm_client_prev(c));
	if(!c) {
		for(c = zwm_client_tail();
		  c && !zwm_client_visible(c);
		  c = zwm_client_prev(c));
	}

	if(c) {
		zwm_client_raise(c);
		zwm_client_focus(c);
		zwm_client_warp(c);
	}
}

void
zwm_focus_next(const char *arg) {
	Client *c;

	if(!sel) {
		zwm_client_focus(NULL);
		return;
	}

	for(c = zwm_client_next(sel);
		       	c && !zwm_client_visible(c);
		       	c = zwm_client_next(c));
	if(!c) {
		for(c = zwm_client_head();
		  c && !zwm_client_visible(c); c = zwm_client_next(c));
	}

	if(c) {
		zwm_client_raise(c);
		zwm_client_focus(c);
		zwm_client_warp(c);
	}
}

