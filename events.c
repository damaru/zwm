#include "zwm.h"

static void expose(XEvent *e);
static void buttonpress(XEvent *e);
static void maprequest(XEvent *e);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void enternotify(XEvent *e);
static void propertynotify(XEvent *e);
static void unmapnotify(XEvent *e);
static int quit = 0;

typedef struct Handler
{
	ZwmEFunc handler;
	void *priv;
	struct Handler *next;
}Handler;

static Handler *handlers[ZwmMaxEvents];

void zwm_x11_flush_events(long mask)
{
	XEvent ev;
	while(XCheckMaskEvent(dpy, mask, &ev));
}

void zwm_event_quit(void)
{
	quit = 1;
}

void zwm_event_loop(void) {
	XEvent ev;
	XSync(dpy, False);
	int fd = ConnectionNumber(dpy);
	fd_set fds;
	struct timeval t;
	while(!quit) {
		while(XPending(dpy) > 0) {
			XNextEvent(dpy, &ev);
			if(ev.type < LASTEvent) {
				zwm_event_emit(ev.type, &ev);
			} else {
				zwm_event_emit(ZwmX11Event, &ev);
			}

			zwm_layout_rearrange(False);
			Client *c;
			for(c = head; c; c = c->next) {
				if(c->dirty)zwm_decor_update(c);
				c->dirty = 0;
			}
		}
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		t.tv_sec = 1;
		t.tv_usec = 0;
		if(select(fd + 1, &fds, 0, &fds, &t) < 0) {
			break;
		}

		if(sel)zwm_decor_update(sel);

		if(quit){
			break;
		}
	}
}

void zwm_event_init()
{
	int i;
	for(i = 0; i < ZwmMaxEvents; i++)
	{
		handlers[i] = NULL;
	}
	zwm_event_register(ButtonPress, (ZwmEFunc)buttonpress, NULL);
	zwm_event_register(Expose, (ZwmEFunc)expose, NULL);
	zwm_event_register(EnterNotify, (ZwmEFunc)enternotify, NULL);
	zwm_event_register(ConfigureRequest, (ZwmEFunc)configurerequest, NULL);
	zwm_event_register(ConfigureNotify, (ZwmEFunc)configurenotify, NULL);
	zwm_event_register(DestroyNotify, (ZwmEFunc)destroynotify, NULL);
	zwm_event_register(MapRequest, (ZwmEFunc)maprequest, NULL);
	zwm_event_register(UnmapNotify, (ZwmEFunc)unmapnotify, NULL);
	zwm_event_register(PropertyNotify, (ZwmEFunc)propertynotify, NULL);
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

static void expose(XEvent *e) {
	Client *c;
	XExposeEvent *ev = &e->xexpose;
	 if((c = zwm_client_lookup(ev->window))){
		 zwm_client_configure_window(c);
		 zwm_decor_dirty(c);
	 }
}

static void buttonpress(XEvent *e) {
	Client *c;
	XButtonPressedEvent *ev = &e->xbutton;
	DBG_ENTER();

	 if((c = zwm_client_lookup(ev->window))) {
		zwm_client_focus(c);

		if(ev->button == Button1) {
			int j = c->w - config.button_count*config.button_width;
			if(ev->x > j )
			{
				int i = (ev->x - j)/config.button_width;
				if(i < config.button_count) config.buttons[i].func(c);
				return ;
			}
			zwm_client_raise(c, False);
			zwm_client_mousemove(c);
		}
		else if(ev->button == Button2) {
			zwm_client_toggle_floating(c);
		}
		else if(ev->button == Button3 ) {
			zwm_client_mouseresize(c);
		}
	}
}

static void maprequest(XEvent *e) {
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

static void configurenotify(XEvent *e) {
	XConfigureEvent *ev = &e->xconfigure;

	if(ev->window == root &&
   	   (ev->width != screen[0].w || ev->height != screen[0].h)) {
		DBG_ENTER();
		screen[0].w = ev->width;
		screen[0].h = ev->height;
		zwm_screen_rescan(False);
	}
}

static void configurerequest(XEvent *e) {
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;
	DBG_ENTER();

	c = zwm_client_lookup(ev->window);
	if(c && c->type == ZwmNormalWindow) {
		if( c->isfloating ) {
			if(ev->value_mask & CWX)
				c->x = ev->x;
			if(ev->value_mask & CWY)
				c->y = ev->y;
			if(ev->value_mask & CWWidth)
				c->w = ev->width+2*c->border;
			if(ev->value_mask & CWHeight)
				c->h = ev->height+config.title_height+c->border;

			if((ev->value_mask & (CWX | CWY))
			&& !(ev->value_mask & (CWWidth | CWHeight)))
				zwm_client_configure_window(c);

			if(zwm_client_visible(c, c->view))
			zwm_client_moveresize(c, c->x, c->y, c->w, c->h);

		} else {
			zwm_client_configure_window(c);
			if(!zwm_client_visible(c, c->view))
				zwm_client_raise(c, False);
		}
	} else {
		/* unmanaged window? pass on the event to the client */
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}

	if(c)
	{
		zwm_event_emit(ZwmClientConfigure,c);
	}
}

static void destroynotify(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;
	DBG_ENTER();

	if((c = zwm_client_lookup(ev->window)))
		zwm_client_unmanage(c);
}

static void enternotify(XEvent *e) {
	Client *c;
	XCrossingEvent *ev = &e->xcrossing;
	DBG_ENTER();

	if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
		return;

	if((c = zwm_client_lookup(ev->window)) && c->frame == ev->window)
		zwm_client_focus(c);
}

static void propertynotify(XEvent *e) {
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
				//updatesizehints(c);
				break;
		}
		if(ev->atom == _NET_WM_NAME ||
		   ev->atom == WM_NAME)
			zwm_client_update_name(c);
	}

}

static void unmapnotify(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;
	DBG_ENTER();

	if((c = zwm_client_lookup(ev->window)))
		zwm_client_unmanage(c);

}

