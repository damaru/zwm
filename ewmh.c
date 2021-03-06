#include "zwm.h"

static void active_window(Client *c);
static void client_list(void *p, void *p2);
static void client_state(Client *c , void *p);
static void process_state_atom(Client *c, Atom state, int set);
static void clientmessage(XEvent *e, void *p);

void zwm_ewmh_init(void) {
    unsigned long n = 1, c = 0;
    zwm_event_register(ZwmClientFocus, (ZwmEFunc)active_window, NULL);
    zwm_event_register(ZwmClientMap, (ZwmEFunc)client_list, NULL);
    zwm_event_register(ZwmClientUnmap, (ZwmEFunc)client_list, NULL);
    zwm_event_register(ZwmViewChange, (ZwmEFunc)client_list, NULL);
    zwm_event_register(ZwmClientFocus, (ZwmEFunc)client_list, NULL);
    zwm_event_register(ZwmClientView, (ZwmEFunc)client_list, NULL);
    zwm_event_register(ZwmClientState, (ZwmEFunc)client_state, NULL);
    zwm_event_register(ClientMessage, (ZwmEFunc)clientmessage, NULL);

    zwm_x11_atom_set(root, _NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, &n, 1);
    zwm_x11_atom_set(root, _NET_CURRENT_DESKTOP, XA_CARDINAL, &c, 1);

}

static void active_window(Client *c) {
    Window win = c->win;
    zwm_x11_atom_set(root, _NET_ACTIVE_WINDOW, XA_WINDOW, &win, 1);
}

static void client_list(void *p, void *p2) {
	Window wins[MAX_CLIENTS];
	Client *c;
	int n ;
	int view = zwm_current_view();
	for(n = 0, c = head; c; c = c->next)
	{
		if (zwm_view_mapped(c->view) && c->view == view)
			wins[n++] = c->win;
	}
	zwm_x11_atom_set(root, _NET_CLIENT_LIST, XA_WINDOW, wins, n);
	zwm_x11_atom_set(root, _NET_CLIENT_LIST_STACKING, XA_WINDOW, wins, n);

	XFlush(dpy);
}

static void client_state(Client *c , void *p) {
	Window win = c->win;
	long data[] = {c->state, None};
	zwm_x11_atom_set(win, WM_STATE, WM_STATE, (unsigned long *)data, 1);
	data[0] = c->state == IconicState? _NET_WM_STATE_SHADED :None;
	zwm_x11_atom_set(win, _NET_WM_STATE, _NET_WM_STATE, (unsigned long *)data, 1);
}

void zwm_ewmh_set_window_opacity(Window win, float opacity) {
	unsigned int o =(unsigned int)( opacity  * 0xFFFFFFFF);
    if (opacity == 1.0) {
        XDeleteProperty (dpy, win, _NET_WM_WINDOW_OPACITY);
    } else {
        zwm_x11_atom_set(win, _NET_WM_WINDOW_OPACITY, 
                XA_CARDINAL, (unsigned long *) &o, 1L);
    }
}

static void process_state_atom(Client *c, Atom state, int set) {
    if(state != _NET_WM_STATE_HIDDEN) {
	    zwm_client_raise(c, True);
    }
    if(state == _NET_WM_STATE_FULLSCREEN) {
        if(set) {
		zwm_client_fullscreen(c);
        } else  {
		zwm_client_unfullscreen(c);
        }
        zwm_layout_dirty();
        zwm_client_raise(c, False);
	client_list(NULL,NULL);
    }
}

static void clientmessage(XEvent *e, void *p) {
	XClientMessageEvent *ev = &e->xclient;
	Client *c;
	
	c = zwm_client_lookup(ev->window);
	if(!c){
		return;
	}

	if(ev->message_type == _NET_ACTIVE_WINDOW) {
		zwm_client_raise(c, False);
	} else if(ev->message_type == WM_CHANGE_STATE) {
		zwm_client_setstate(c, ev->data.l[0]);	
	} else if(ev->message_type == _NET_WM_DESKTOP) {
		zwm_client_set_view(c, ev->data.l[0]);
		client_list(NULL,NULL);
	} else if(ev->message_type == _NET_WM_STATE) {
		process_state_atom(c, (Atom) ev->data.l[1],
				ev->data.l[0]);
		if(ev->data.l[2]) {
			process_state_atom(c, (Atom) ev->data.l[2],
					ev->data.l[0]);
		}
	}
}

