#include "zwm.h"
#include <X11/extensions/shape.h>
#include <stdio.h>
#include <string.h>

unsigned int num_floating = 0;
static int privcount = 0;
Client* sel = NULL;

int session_dirty = 0;
static int jcount = 0;
static int window_type(Window w);
static void create_frame_window(Client* c);

static inline void view_set(Client* c, int view)
{
	c->view = view;
	zwm_x11_atom_set(c->win, _ZWM_VIEW, XA_INTEGER, (ulong*)&c->view, 1);
}

void zwm_client_configure_window(Client* c)
{
	int th = 0;

	if (c->hastitle) {
		th = config.title_height;
	}

	XConfigureEvent ce;
	if (c->type != ZwmNormalWindow)
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

	if (c->hastitle) {
		ce.x = 0;
		ce.y = th;
		ce.width = c->w - c->border;
		ce.height = c->h - th - c->border;
		ce.border_width = 0;
	}

	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent*)&ce);
}

void zwm_client_setstate(Client* c, int state)
{
	if (c->state != state) {
		c->state = state;
		zwm_layout_dirty();
	}
	if (state == NormalState) {
		XRaiseWindow(dpy, c->frame);
		XRaiseWindow(dpy, c->win);
		zwm_client_focus(c);
	} else {
		XLowerWindow(dpy, c->win);
		XLowerWindow(dpy, c->frame);
		zwm_client_refocus();
	}
	zwm_event_emit(ZwmClientState, c);
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
			Client* c = zwm_client_manage(wins[i], &wa);
			if (c)
				c->ignore = 1;
		}
	}

	if (wins)
		XFree(wins);
}

void set_shape(Client* c)
{
	int n, order;
	XRectangle* dummy;
	dummy = XShapeGetRectangles(dpy, c->win, ShapeBounding, &n, &order);
	if (n > 1) {
		c->hastitle = 0;
		c->has_shape = 1;
	}
	XFree(dummy);
}

Client* zwm_client_manage(Window w, XWindowAttributes* wa)
{
	int scr = zwm_current_screen();
	int vew = zwm_current_view();
	int i;

	if (vew >= ZWM_ZEN_VIEW) {
		vew = screen[scr].prev;
	}

	Client* old = zwm_client_lookup(w);

	if (old && old->win == w) {
		return NULL;
	}

	if (wa->override_redirect) {
		return NULL;
	}

	Client* c = zwm_util_malloc(sizeof(Client) + (sizeof(void*) * privcount));
	c->win = w;
	c->isfloating = False;
	c->state = NormalState;
	c->border = config.border_width;
	c->type = window_type(w);
	zwm_client_update_name(c);

	for (i = 0; i < 16 && config.policies[i].cname; i++) {
		if (strcmp(c->cname, config.policies[i].cname) == 0) {
			c->type = config.policies[i].type;
			break;
		}
	}

	ulong cv = 0;
	if (zwm_x11_atom_get(c->win, _ZWM_VIEW, XA_INTEGER, &cv)) {
		c->view = -1;
		zwm_client_set_view(c, cv);
		zwm_view_set(cv);
	} else {
		view_set(c, vew);
	}

	c->w = wa->width;
	c->h = wa->height;
	c->x = (screen[scr].w - c->w) / 2;
	c->y = (screen[scr].h - c->h) / 2;
	zwm_client_save_geometry(c, &c->fpos);
	c->fpos.view = c->view;
	c->fpos.screen = scr;
	zwm_client_update_hints(c);

	zwm_client_update_name(c);

	for (i = 0; i < 16 && config.policies[i].cname; i++) {
		if (strcmp(c->cname, config.policies[i].cname) == 0) {
			c->type = config.policies[i].type;
			c->x = config.policies[i].pos.x;
			if (c->x < 0) {
				c->x = screen[0].w + config.policies[i].pos.x;
			}
			c->y = config.policies[i].pos.y;
			c->w = config.policies[i].pos.w;
			c->h = config.policies[i].pos.h;
			break;
		}
	}

	switch (c->type) {
		case ZwmDesktopWindow:
			c->hastitle = 0;
			c->isfloating = 0;
			XMapWindow(dpy, w);
			zwm_client_configure_window(c);
			break;
		case ZwmDockWindow:
			c->border = 0;
			c->hastitle = 0;
			XSelectInput(dpy, w, PropertyChangeMask);
			XMapWindow(dpy, w);
			zwm_event_emit(ZwmClientMap, c);
			zwm_client_configure_window(c);
			return c;
			break;
		case ZwmDialogWindow:
			c->hastitle = 1;
			c->type = ZwmNormalWindow;
		case ZwmSplashWindow:
			c->isfloating = True;
			c->x = screen[scr].x + (wa->x ? wa->x : ((screen[scr].w - wa->width) / 2));
			c->y = wa->y ? wa->y : ((screen[scr].h - wa->height) / 2);
			num_floating++;
			break;
		case ZwmFullscreenWindow:
			c->type = ZwmNormalWindow;
			zwm_client_fullscreen(c);
			break;
		case ZwmFixedWindow:
			c->hastitle = 0;
			XMapWindow(dpy, w);
			break;
		case ZwmNormalWindow:
			c->hastitle = 1;
			break;
	}
	set_shape(c);

	if (c->hastitle) {
		c->h += config.title_height;
		create_frame_window(c);
	}

	XMapWindow(dpy, c->win);
	XRaiseWindow(dpy, c->win);

	if (c->has_shape) {
		zwm_client_float(c);
	}

	if (c->isfloating)
		zwm_ewmh_set_window_opacity(c->frame, config.opacity);

	if (strstr(c->cname, "sun-awt")) {
		config.anim_steps = 0;
		jcount++;
		zwm_client_configure_window(c);
	}

	zwm_decor_update(c);
    if (config.attach_last) {
            zwm_client_push_tail(c);
    } else {
            char zwm_insert[1024];
            zwm_util_getenv(c->pid, "ZWM_INSERT", zwm_insert, 1020);
            if(strcmp(zwm_insert, "START") == 0){
                    zwm_client_push_head(c);
            } else {
                    zwm_client_push_tail(c);
            }
    }
	zwm_view_rescan();
	zwm_event_emit(ZwmClientMap, c);
	zwm_layout_dirty();
	zwm_client_configure_window(c);
	if (config.focus_new && c->view == vew) {
		zwm_client_raise(c, True);
	}
	zwm_layout_rearrange(True);
	config.num_clients++;
	session_dirty++;
	return c;
}

void zwm_client_unmanage(Client* c)
{
	if (c->ignore > 0) {
		c->ignore--;
		return;
	}

	XGrabServer(dpy);
	XReparentWindow(dpy, c->win, root, c->x, c->y);

	if (c->frame) {
		free(c->draw);
		zwm_systray_detach();
		XDestroyWindow(dpy, c->frame);
	}

	zwm_client_remove(c);
	if (c->isfloating) {
		num_floating--;
	}
	if (sel == c) {
		Client* n = zwm_client_lookup(c->lastfocused);
		if (n && n->win == c->lastfocused && zwm_client_visible(n, zwm_current_view())) {
			zwm_client_raise(n, True);
		} else {
			zwm_client_refocus();
		}
	}

	if (strstr(c->cname, "sun-awt")) {
		jcount--;
		if (jcount == 0) {
			config.anim_steps = 20;
		}
	}

	XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
	XUngrabServer(dpy);
	zwm_view_rescan();
	zwm_event_emit(ZwmClientUnmap, c);
	free(c);
	config.num_clients--;
	session_dirty++;
	zwm_session_save();
}

Client* zwm_client_lookup(Window w)
{
	Client* c;

	for (c = head; c && (c->win != w && c->frame != w);
	     c = c->next)
		;
	return c;
}

Bool zwm_client_visible(Client* c, int view)
{
	return c && c->state == NormalState && zwm_view_mapped(c->view) && c->view == view && (c->type == ZwmNormalWindow || c->type == ZwmDialogWindow);
}

void zwm_client_refocus(void)
{
	Client* c = head;
	while (c && !zwm_client_visible(c, zwm_current_view())) {
		c = c->next;
	}
	if (c) {
		zwm_client_raise(c, True);
	} else {
		sel = NULL;
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
	}
}

void zwm_client_focus(Client* c)
{
	int v = zwm_current_view();

	/* unfocus */
	if (sel && sel != c) {
		c->lastfocused = sel->win;
		sel->focused = False;
		zwm_mouse_grab(sel, False);
		zwm_decor_dirty(sel);
	}

	/* focus */
	zwm_mouse_grab(c, True);
	if (!c->isfloating) {
		views[v].current = c;
	}
	sel = c;
	c->focused = True;
	XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
	zwm_decor_dirty(c);
	zwm_event_emit(ZwmClientFocus, c);
}

void zwm_client_raise(Client* c, Bool warp)
{
	zwm_client_set_view(c, zwm_current_view());
	zwm_client_setstate(c, NormalState);
	if (warp) {
		zwm_layout_rearrange(True);
		zwm_mouse_warp(c);
	}
	zwm_client_configure_window(c);
}

void zwm_client_moveresize(Client* c, int x, int y, int w, int h)
{
	c->x = x;
	c->y = y;
	c->w = w;
	c->h = h;

	if (c->hastitle && c->frame) {
		XMoveResizeWindow(dpy, c->frame, x, y, w, h);
		XMoveResizeWindow(dpy, c->win, c->border, config.title_height,
		    w - 2 * c->border, h - config.title_height - 2 * c->border);
	} else {
		XMoveResizeWindow(dpy, c->win, x, y, w, h);
		if (c->frame)
			XMoveResizeWindow(dpy, c->frame, x, y, w, h);
	}
	XSync(dpy, False);

	if (c->isfloating) {
		c->screen = zwm_client_screen(c);
		if (c->screen < config.screen_count) {
			view_set(c, screen[c->screen].view);
		} else if (zwm_client_visible(c, c->view)) {
			c->screen = 0;
			view_set(c, screen[0].view);
			zwm_client_toggle_floating(c);
		}
	}
}

void zwm_client_fullscreen(Client* c)
{
	int s = zwm_client_screen(c);
	int v = ZWM_ZEN_VIEW + s;
	Client* t;

	if (c->view >= ZWM_ZEN_VIEW) {
		view_set(c, screen[s].prev);
		zwm_view_set(c->view);
		zwm_client_raise(c, True);
		zwm_panel_show();
		return;
	}

	zwm_client_foreach(t)
	{
		if (t->view >= ZWM_ZEN_VIEW) {
			t->view = t->fpos.view;
		}
	}

	c->fpos.view = c->view;
	views[v].current = c;
	zwm_client_set_view(c, v);
	zwm_screen_set_view(zwm_current_screen(), v);
	zwm_layout_set("fullscreen");
}

void zwm_client_unfullscreen(Client* c)
{
	int s = zwm_client_screen(c);
	if (c->view >= ZWM_ZEN_VIEW) {
		view_set(c, screen[s].prev);
		zwm_view_set(c->view);
		zwm_client_raise(c, True);
		zwm_panel_show();
		return;
	}

	c->isfloating = False;
	c->hastitle = 1;
	//	num_floating--;
	c->border = config.border_width;
	XSetWindowBorderWidth(dpy, c->win, c->border);
	zwm_decor_dirty(c);
	zwm_layout_rearrange(True);
}

void zwm_client_float(Client* c)
{
	if (!c->isfloating) {
		zwm_client_toggle_floating(c);
	}
}

static Bool
isprotodel(Client* c)
{
	int i, n;
	Atom* protocols;
	Bool ret = False;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		for (i = 0; !ret && i < n; i++)
			if (protocols[i] == WM_DELETE_WINDOW)
				ret = True;
		XFree(protocols);
	}
	return ret;
}

void zwm_client_kill(Client* c)
{
	if (isprotodel(c)) {
		XEvent ev;
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = WM_PROTOCOLS;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = WM_DELETE_WINDOW;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	} else {
		zwm_client_moveresize(c, c->x, -c->y - c->h, c->w, c->h);
		XKillClient(dpy, c->win);
	}
}

void zwm_client_set_view(Client* c, int v)
{
	if (c->view != v) {
		view_set(c, v);
		if (v >= config.num_views) {
			config.num_views = v + 1;
		}
		zwm_layout_dirty();
		zwm_event_emit(ZwmClientView, c);
	}
}

void zwm_client_update_name(Client* c)
{
	int i;
	zwm_x11_atom_list(c->win, _NET_WM_PID, AnyPropertyType, &c->pid, 1, NULL);
	if (!zwm_x11_atom_text(c->win, _NET_WM_NAME, c->name, sizeof c->name))
		zwm_x11_atom_text(c->win, WM_NAME, c->name, sizeof c->name);
	zwm_x11_atom_text(c->win, WM_CLASS, c->cname, sizeof c->cname);
	char path[256];
	sprintf(path, "/proc/%d/cmdline", (int)c->pid);
	FILE* fp = fopen(path, "r");
	if (fp) {
		struct stat st;
		stat(path, &st);

		if (st.st_uid == 0) {
			c->root_user = 1;
		} else {
			c->root_user = 0;
		}

		int count = fread(c->cmd, 1, 256, fp);
		fclose(fp);
		for (i = 0; i < count; i++) {
			if (c->cmd[i] == 0) {
				c->cmd[i] = ' ';
			}
		}
		c->cmd[i-1] = 0;
		c->cmd[count] = 0;
	}
	ZWM_DEBUG("%X: name:%s, clas:%s, cmd:%s\n",
	    c->win, c->name, c->cname, c->cmd);
	zwm_client_configure_window(c);
	zwm_decor_dirty(c);
}

void zwm_client_toggle_floating(Client* c)
{
	c->isfloating = !c->isfloating;
	if (c->isfloating) {
		zwm_client_raise(c, True);
		zwm_ewmh_set_window_opacity(c->frame, config.opacity);
		num_floating++;
		zwm_client_restore_geometry(c, &c->fpos);
		zwm_decor_update(c);
	} else {
		num_floating--;
		zwm_ewmh_set_window_opacity(c->frame, 1);
		zwm_client_save_geometry(c, &c->fpos);
	}
	zwm_layout_dirty();
}

void zwm_client_zoom(Client* c)
{
	if (c) {
		zwm_client_remove(c);
		zwm_client_push_head(c);
		zwm_layout_rearrange(False);
		zwm_client_raise(c, True);
	}
}

void zwm_client_save_geometry(Client* c, ZwmGeom* g)
{
	*g = c->geom;
	g->screen = zwm_client_screen(c);
}

void zwm_client_restore_geometry(Client* c, ZwmGeom* g)
{
	int delta = screen[views[c->view].screen].x - screen[g->screen].x;
	zwm_layout_moveresize(c, g->x + delta, g->y, g->w, g->h);
}

static int window_type(Window w)
{
	if (zwm_x11_atom_check(w, _OL_DECOR_DEL, _OL_DECOR_HEADER)) {
		return ZwmDialogWindow;
	}

	if (zwm_x11_atom_check(w, _NET_WM_STATE, _NET_WM_STATE_FULLSCREEN)) {
		return ZwmFullscreenWindow;
	} else if (zwm_x11_atom_check(w, _NET_WM_WINDOW_TYPE, _NET_WM_STATE_MODAL)) {
		return ZwmDialogWindow;
	} else {
		Atom a[32];
		unsigned long left;
		unsigned long i;
		unsigned long n = zwm_x11_atom_list(w, _NET_WM_WINDOW_TYPE, XA_ATOM,
		    a, 32, &left);
		for (i = 0; i < n; i++) {
#define CHECK_RET(t, r)                                   \
	do {                                              \
		if (a[i] == t) {                          \
			printf("%d: %s\n", __LINE__, #r); \
			return r;                         \
		}                                         \
	} while (0)
			CHECK_RET(_NET_WM_WINDOW_TYPE_SPLASHSCREEN, ZwmSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_SPLASH, ZwmSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DOCK, ZwmDockWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_NORMAL, ZwmNormalWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DIALOG, ZwmDialogWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_UTILITY, ZwmDialogWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DESKTOP, ZwmDesktopWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_TOOLBAR, ZwmSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DND, ZwmSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_COMBO, ZwmSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_MENU, ZwmSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU, ZwmSplashWindow);
			CHECK_RET(_NET_WM_WINDOW_TYPE_TOOLTIP, ZwmSplashWindow);
		}
	}
	return ZwmNormalWindow;
}

static void create_frame_window(Client* c)
{
	XClassHint hint = { "zwm", "ZWM" };
	int scr = DefaultScreen(dpy);
	XSetWindowAttributes pattr;
	pattr.override_redirect = True;
	pattr.event_mask = StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ExposureMask | EnterWindowMask;
	c->frame = XCreateWindow(dpy, root, c->x, c->y, c->w, config.title_height, c->border,
	    DefaultDepth(dpy, scr), CopyFromParent, DefaultVisual(dpy, scr),
	    CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask, &pattr);
	if (config.show_title)
		XMapWindow(dpy, c->frame);
	XSetClassHint(dpy, c->frame, &hint);
	XRaiseWindow(dpy, c->frame);
	XReparentWindow(dpy, c->win, c->frame, 0, config.title_height);
	XMoveResizeWindow(dpy, c->win, 0, config.title_height, c->w, c->h - config.title_height);
	c->draw = XftDrawCreate(dpy, c->frame, DefaultVisual(dpy, scr), cmap);
}

void zwm_client_update_hints(Client* c)
{
	XSizeHints size;
	long sz;

	XGetWMNormalHints(dpy, c->win, &size, &sz);
	if (size.flags & PMinSize) {
		c->minw = size.min_width + 2 * c->border;
		c->minh = size.min_height + config.title_height + 2 * c->border;
	} else {
		c->minw = config.minw;
		c->minh = config.minh;
	}
}
