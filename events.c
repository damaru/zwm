#include "zwm.h"
#include <time.h>

static void ev_expose(XEvent *e);
static void ev_map(XEvent *e);
static void ev_configure_notify(XEvent *e);
static void ev_configure_request(XEvent *e);
static void ev_destroy(XEvent *e);
static void ev_enter(XEvent *e);
static void ev_property(XEvent *e);
static void ev_unmap(XEvent *e);
static int ev_wait(void);

typedef struct Handler
{
	ZwmEFunc handler;
	void *priv;
	struct Handler *next;
}Handler;

static Handler *handlers[ZwmMaxEvents];

void zwm_event_init()
{
	int i;
	for(i = 0; i < ZwmMaxEvents; i++)
	{
		handlers[i] = NULL;
	}
	zwm_event_register(Expose, (ZwmEFunc)ev_expose, NULL);
	zwm_event_register(EnterNotify, (ZwmEFunc)ev_enter, NULL);
	zwm_event_register(ConfigureRequest, (ZwmEFunc)ev_configure_request, NULL);
	zwm_event_register(ConfigureNotify, (ZwmEFunc)ev_configure_notify, NULL);
	zwm_event_register(DestroyNotify, (ZwmEFunc)ev_destroy, NULL);
	zwm_event_register(MapRequest, (ZwmEFunc)ev_map, NULL);
	zwm_event_register(UnmapNotify, (ZwmEFunc)ev_unmap, NULL);
	zwm_event_register(PropertyNotify, (ZwmEFunc)ev_property, NULL);
}

void zwm_x11_flush_events(long mask)
{
	XEvent ev;
	while(XCheckMaskEvent(dpy, mask, &ev));
}

static int quit = 0;

void zwm_event_quit(void)
{
	quit = 1;
}

void zwm_event_process(void) {
	XEvent ev;
	while(XPending(dpy) > 0) {
		XNextEvent(dpy, &ev);
		if(ev.type < LASTEvent) {
			zwm_event_emit(ev.type, &ev);
		}
		zwm_layout_rearrange(False);
		Client *c;
		for(c = head; c; c = c->next) {
			if(c->dirty)zwm_decor_update(c);
			c->dirty = 0;
		}
	}
}

void zwm_event_loop(void) {
	XEvent ev;
	XSync(dpy, False);
	while(!quit) {
		while(XPending(dpy) > 0) {
			XNextEvent(dpy, &ev);
			if(ev.type < LASTEvent) {
				zwm_event_emit(ev.type, &ev);
			}
			zwm_layout_rearrange(False);
			Client *c;
			for(c = head; c; c = c->next) {
				if(c->dirty)zwm_decor_update(c);
				c->dirty = 0;
			}
		}
	//	if(session_dirty)
			zwm_session_save();

		if(!quit && ev_wait() ==  0 && sel) {
			zwm_decor_update(sel);
		}
	}
}

void zwm_event_emit(ZwmEvent e, void *p)
{
	Handler *h = handlers[e];
	while(h){
		ZWM_DEBUG("ZwmEvent %d handler %p\n",e, h->handler);
		h->handler(p, h->priv);
		h = h->next;
	}
}

void zwm_event_register(ZwmEvent e, ZwmEFunc f, void *priv)
{
	Handler *h = zwm_util_malloc(sizeof (Handler));
	h->handler =f ;
	h->priv = priv;
	h->next = handlers[e];
	handlers[e] = h;
}

static int ev_wait(void) {
	struct timeval t;
	int fd = ConnectionNumber(dpy);
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	t.tv_sec = config.sleep_time;
	t.tv_usec = 0;
	int tcount = time(NULL);
	int ret  = select(fd + 1, &fds, 0, &fds, &t);
	tcount = time(NULL) - tcount;
	if(sel){
		sel->ucount += tcount;
		zwm_x11_atom_set(sel->win, _ZWM_COUNT, XA_INTEGER, &sel->ucount, 1);
	}
	return ret;
}

static void ev_expose(XEvent *e) {
	Client *c;
	XExposeEvent *ev = &e->xexpose;
	 if((c = zwm_client_lookup(ev->window))){
	//	 zwm_client_configure_window(c);
		 zwm_decor_dirty(c);
	 }
}

static void ev_map(XEvent *e) {
	XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
	DBG_ENTER();

	if(!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if(wa.override_redirect)
		return;
	if(!zwm_client_lookup(ev->window)) {
		zwm_client_manage(ev->window, &wa);
	} else {
		Client *c;
		c = zwm_client_lookup(ev->window);
		zwm_client_focus(c);
	}
}

static void ev_configure_notify(XEvent *e) {
	XConfigureEvent *ev = &e->xconfigure;

	if(ev->window == root &&
   	   (ev->width != screen[0].w || ev->height != screen[0].h)) {
		DBG_ENTER();
		screen[0].w = ev->width;
		screen[0].h = ev->height;
		zwm_screen_rescan(False);
	}
}

static void ev_configure_request(XEvent *e) {
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;
	int x, y, w, h;
	DBG_ENTER();

	c = zwm_client_lookup(ev->window);
	if(!c){
		/* unmanaged window? pass on the event to the client */
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
		XSync(dpy, False);
		return;
	}

	if (ev->value_mask & CWX)
		x = ev->x;
	if (ev->value_mask & CWY)
		y = ev->y;
	if (ev->value_mask & CWWidth)
		w = ev->width+2*c->border;
	if (ev->value_mask & CWHeight)
		h = ev->height+config.title_height+2*c->border;


	if(c->type == ZwmNormalWindow && c->isfloating) {
		if(!(ev->value_mask & (CWX | CWY))
			&& (ev->value_mask & (CWWidth | CWHeight)))
			zwm_client_moveresize(c, c->x, c->y, w, h);

		if((ev->value_mask & (CWX | CWY))
				&& !(ev->value_mask & (CWWidth | CWHeight)))
			zwm_client_moveresize(c, x, y, c->w, c->h);

		if((ev->value_mask & (CWX | CWY))
				&& (ev->value_mask & (CWWidth | CWHeight)))
			zwm_client_moveresize(c, x, y, w, h);

		if(ev->value_mask & CWStackMode)
			zwm_client_configure_window(c);

		if(!zwm_client_visible(c, c->view))
			zwm_client_raise(c, False);

		zwm_event_emit(ZwmClientConfigure,c);
	}
}

static void ev_destroy(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;
	DBG_ENTER();

	if((c = zwm_client_lookup(ev->window)))
		zwm_client_unmanage(c);
}

static void ev_enter(XEvent *e) {
	Client *c;
	XCrossingEvent *ev = &e->xcrossing;
	DBG_ENTER();

	if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
		return;

	if((c = zwm_client_lookup(ev->window)) && c->frame == ev->window)
		zwm_client_focus(c);
}

static void ev_property(XEvent *e) {
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;
	DBG_ENTER();
	if(ev->state == PropertyDelete)
		return; /* ignore */
	if((c = zwm_client_lookup(ev->window))){ 
		switch (ev->atom) {
			default: break;
			case XA_WM_TRANSIENT_FOR:
				XGetTransientForHint(dpy, ev->window, &trans);
				if((zwm_client_lookup(trans) != NULL))
					zwm_layout_dirty();
				break;
			case XA_WM_NORMAL_HINTS:
				zwm_client_update_hints(c);
				break;
			case XA_WM_HINTS:        //todo
				break;
		}
		if(ev->atom == XA_WM_NAME || ev->atom == _NET_WM_NAME)
			zwm_client_update_name(c);
	}
}

static void ev_unmap(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;
	DBG_ENTER();

	if((c = zwm_client_lookup(ev->window)))
		zwm_client_unmanage(c);

}
