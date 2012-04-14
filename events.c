#include "zwm.h"

#define SNAP			1	/* snap pixel */

void
expose(XEvent *e) {
	Client *c;
	XButtonPressedEvent *ev = &e->xexpose;
	 if((c = zwm_client_get(ev->window))) {
		 zwm_client_update_decoration(c);
	 }
}

void
buttonpress(XEvent *e) {
	Client *c;
	XButtonPressedEvent *ev = &e->xbutton;
	DBG_ENTER();

	 if((c = zwm_client_get(ev->window))) {
		zwm_client_focus(c);

//		if(CLEANMASK(ev->state) != MODKEY) return;

		if(ev->button == Button1) {
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

void
mappingnotify(XEvent *e) {
	XMappingEvent *ev = &e->xmapping;
	DBG_ENTER();

	XRefreshKeyboardMapping(ev);
//	if(ev->request == MappingKeyboard)
//		keypress(NULL);
}

void
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

void
configurenotify(XEvent *e) {
	XConfigureEvent *ev = &e->xconfigure;

	if(ev->window == root &&
   	   (ev->width != screen[0].w || ev->height != screen[0].h)) {
		DBG_ENTER();
		screen[0].w = ev->width;
		screen[0].h = ev->height;
		zwm_update_screen_geometry();
	}
}

void
configurerequest(XEvent *e) {
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;
	DBG_ENTER();

	c = zwm_client_get(ev->window);
	if(c && c->type == ZenNormalWindow) {
//		c->ismax = False;
		if( c->isfloating ) {	

			if(ev->value_mask & CWX)
				c->x = ev->x;
			if(ev->value_mask & CWY)
				c->y = ev->y-20;
			if(ev->value_mask & CWWidth)
				c->w = ev->width;
			if(ev->value_mask & CWHeight)
				c->h = ev->height+20;
			if((c->x + c->w) > screen[0].w && c->isfloating)
				c->x = screen[0].w / 2 - c->w / 2; /* center in x direction */
			if((c->y + c->h) > screen[0].h && c->isfloating)
				c->y =  screen[0].h / 2 - c->h / 2; /* center in y direction */

			if((ev->value_mask & (CWX | CWY))
			&& !(ev->value_mask & (CWWidth | CWHeight)))
				zwm_client_send_configure(c);

			if(zwm_client_visible(c))
				zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
		} else {
			zwm_client_send_configure(c);
			if(!zwm_client_visible(c))
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

void
destroynotify(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;
	DBG_ENTER();

	if((c = zwm_client_get(ev->window)))
		zwm_client_unmanage(c);
}

void
leavenotify(XEvent *e) {
	XCrossingEvent *ev = &e->xcrossing;
	DBG_ENTER();

	if((ev->window == root) && !ev->same_screen) {
		//zwm_client_focus(NULL);
	}
}

void
enternotify(XEvent *e) {
	Client *c;
	XCrossingEvent *ev = &e->xcrossing;
	DBG_ENTER();

	if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
		return;

	if((c = zwm_client_get(ev->window)))
		zwm_client_focus(c);
}

void
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
			//	if(!c->isfloating && (c->isfloating = (getclient(trans) != NULL)))
				if((zwm_client_get(trans) != NULL))
					zwm_layout_arrange();
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


void
unmapnotify(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;
	DBG_ENTER();

	if((c = zwm_client_get(ev->window)))
		zwm_client_unmanage(c);

}

void keypress(XEvent *e);
Bool running = True;

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
	}
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
	zwm_event_register(LeaveNotify, (ZenEFunc)leavenotify, NULL);
	zwm_event_register(EnterNotify, (ZenEFunc)enternotify, NULL);
	zwm_event_register(ConfigureRequest, (ZenEFunc)configurerequest, NULL);
	zwm_event_register(ConfigureNotify, (ZenEFunc)configurenotify, NULL);
	zwm_event_register(DestroyNotify, (ZenEFunc)destroynotify, NULL);
//	zwm_event_register(MappingNotify, (ZenEFunc)mappingnotify, NULL);
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

