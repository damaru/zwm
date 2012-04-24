#include "zwm.h"

static void expose(XEvent *e) {
	Client *c;
	XExposeEvent *ev = &e->xexpose;
	 if((c = zwm_client_get(ev->window))) {
		 zwm_client_update_decoration(c);
	 }
}

typedef void (*ButtonFunc)(Client *);

static ButtonFunc bfuncs[] = {
	zwm_client_iconify,
	zwm_client_toggle_floating,
	zwm_client_kill,
};

static void
buttonpress(XEvent *e) {
	Client *c;
	XButtonPressedEvent *ev = &e->xbutton;
	DBG_ENTER();

	 if((c = zwm_client_get(ev->window))) {
		zwm_client_focus(c);

		if(ev->button == Button1) {
			int j = c->w - 3*config.title_height;
			if(ev->x > j )
			{
				int i = (ev->x - j)/config.title_height;
				if(i < 3) bfuncs[i](c);
				return;
			}
			zwm_client_raise(c);
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

static void
maprequest(XEvent *e) {
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
	DBG_ENTER();

	if(!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if(wa.override_redirect)
		return;
	if(!zwm_client_get(ev->window)) {
		zwm_client_manage(ev->window, &wa);
	} else {
		Client *c;
		c = zwm_client_get(ev->window);
		zwm_client_focus(c);
	}
}

static void
configurenotify(XEvent *e) {
	XConfigureEvent *ev = &e->xconfigure;

	if(ev->window == root &&
   	   (ev->width != screen[0].w || ev->height != screen[0].h)) {
		DBG_ENTER();
		screen[0].w = ev->width;
		screen[0].h = ev->height;
		zwm_update_screen_geometry(False);
	}
}

static void
configurerequest(XEvent *e) {
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;
	DBG_ENTER();

	c = zwm_client_get(ev->window);
	if(c && c->type == ZenNormalWindow) {
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
				zwm_x11_configure_window(c);

			if(zwm_client_visible(c, c->view))
			zwm_client_moveresize(c, c->x, c->y, c->w, c->h);

		} else {
			zwm_x11_configure_window(c);
			if(!zwm_client_visible(c, c->view))
				zwm_client_raise(c);
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
		zwm_event_emit(ZenClientConfigure,c);
	}
}

static void
destroynotify(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;
	DBG_ENTER();

	if((c = zwm_client_get(ev->window)))
		zwm_client_unmanage(c);
}

static void
enternotify(XEvent *e) {
	Client *c;
	XCrossingEvent *ev = &e->xcrossing;
	DBG_ENTER();

	if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
		return;

	if((c = zwm_client_get(ev->window)))
		zwm_client_focus(c);
}

static void
propertynotify(XEvent *e) {
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;
	DBG_ENTER();
	if(ev->state == PropertyDelete)
		return; /* ignore */
	if((c = zwm_client_get(ev->window))){ 
		switch (ev->atom) {
			default: break;
			case XA_WM_TRANSIENT_FOR:
				XGetTransientForHint(dpy, ev->window, &trans);
				if((zwm_client_get(trans) != NULL))
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


static void
unmapnotify(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;
	DBG_ENTER();

	if((c = zwm_client_get(ev->window)))
		zwm_client_unmanage(c);

}

void
zwm_event_flush_x11(long mask)
{
	XEvent ev;
	while(XCheckMaskEvent(dpy, mask, &ev));
}

void zwm_event(int fd, int mode, void *data)
{
	XEvent ev;
	while(XPending(dpy)) {
		XNextEvent(dpy, &ev);
		if(ev.type < LASTEvent) {
			zwm_event_emit(ev.type, &ev);
		} else {
			zwm_event_emit(ZenX11Event, &ev);
		}
		zwm_layout_rearrange();
	}
}

void zwm_event_quit(void)
{
	zen_events_quit();
}

void
zwm_event_loop(void) {
	XSync(dpy, False);
	zen_events_add(ConnectionNumber(dpy),ZEN_EVT_READ,zwm_event, NULL);
	zen_events_wait();
}

static ZenEventHandler *handlers[ZenMaxEvents];

void zwm_event_init()
{
	int i;
	for(i = 0; i < ZenMaxEvents; i++)
	{
		handlers[i] = NULL;
	}
	zen_events_init();
	zwm_event_register(ButtonPress, (ZenEFunc)buttonpress, NULL);
	zwm_event_register(Expose, (ZenEFunc)expose, NULL);
	zwm_event_register(EnterNotify, (ZenEFunc)enternotify, NULL);
	zwm_event_register(ConfigureRequest, (ZenEFunc)configurerequest, NULL);
	zwm_event_register(ConfigureNotify, (ZenEFunc)configurenotify, NULL);
	zwm_event_register(DestroyNotify, (ZenEFunc)destroynotify, NULL);
	zwm_event_register(MapRequest, (ZenEFunc)maprequest, NULL);
	zwm_event_register(UnmapNotify, (ZenEFunc)unmapnotify, NULL);
	zwm_event_register(PropertyNotify, (ZenEFunc)propertynotify, NULL);
}

void zwm_event_emit(ZenEvent e, void *p)
{
	ZenEventHandler *h = handlers[e];
	while(h){
		ZWM_DEBUG("ZenEvent %d handler %p\n",e, h->handler);
		h->handler(p, h->priv);
		h = h->next;
	}
}

void zwm_event_register(ZenEvent e, ZenEFunc f, void *priv)
{
	ZenEventHandler *h = zwm_malloc(sizeof (ZenEventHandler));
	h->handler =f ;
	h->priv = priv;
	h->next = handlers[e];
	handlers[e] = h;
}

