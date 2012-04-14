/*
 *    EWMH atom support. initial implementation borrowed from
 *    awesome wm, then partially reworked.
 *
 *    Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 *    Copyright © 2008 Alexander Polakov <polachok@gmail.com>
 *
 */
#include "zwm.h"

void
clientmessage(XEvent *e, void *p) ;

/* called on focus */
void
zwm_ewmh_set_active_window(Client *c) {

    Window win = c->win;

    zwm_x11_set_atoms(root, _NET_ACTIVE_WINDOW, XA_WINDOW, &win, 1);
}

/* called on manage */
void
zwm_ewmh_set_client_list(void *p, void *p2) {
	Window wins[zwm_client_count()];
	Client *c;
	int n ;
	zwm_x11_set_atoms(root, _NET_CLIENT_LIST, XA_WINDOW, wins, 0);
	for(n = 0, c = zwm_client_head();
			c;
			c = zwm_client_next(c))
	{
		wins[n++] = c->win;
	}
	zwm_x11_set_atoms(root, _NET_CLIENT_LIST, XA_WINDOW, wins, n);
	zwm_x11_set_atoms(root, _NET_CLIENT_LIST_STACKING, XA_WINDOW, wins, n);

	c = p;
	if(c && zwm_x11_check_atom(c->win, _NET_WM_STATE, _NET_WM_STATE_FULLSCREEN)){
		zwm_client_fullscreen(c);
	}
	XFlush(dpy);
}

void
zwm_ewmh_client_unmap(Client *cli, void *p2) {
	Window wins[zwm_client_count()-1];
	Client *c;
	int n;

	for(n = 0, c = zwm_client_head();
			c;
			c = zwm_client_next(c))
	{
		if(c != cli)
			wins[n++] = c->win;
	}
	zwm_x11_set_atoms(root, _NET_CLIENT_LIST, XA_WINDOW, wins, n);
	zwm_x11_set_atoms(root, _NET_CLIENT_LIST_STACKING, XA_WINDOW, wins, n);
	XFlush(dpy);
}

/* called on setting current_view */
void
zwm_ewmh_set_desktops(void *p1, void *p2) {
	char*  names[10];
	int i;
	zwm_x11_set_atoms(root, _NET_NUMBER_OF_DESKTOPS, XA_CARDINAL, &num_views, 1);
	for (i = 0; i < num_views; i++) {
		names[i] = malloc(10);
		sprintf(names[i], "%d",i);
	}
	XTextProperty text_prop;
	Xutf8TextListToTextProperty(dpy, names, num_views, XUTF8StringStyle, &text_prop);
	XSetTextProperty(dpy, root, &text_prop, _NET_DESKTOP_NAMES);
	for (i = 0; i < num_views; i++) {
		free(names[i]);
	}
	zwm_x11_set_atoms(root, _NET_CURRENT_DESKTOP, XA_CARDINAL,
			(unsigned long *) &current_view, 1);
}

/* called from set view */
void
zwm_ewmh_set_window_desktop(Client *c, void *p) {
	Window win = c->win;
	unsigned long view = c->view;
	zwm_x11_set_atoms(win, _NET_WM_DESKTOP, XA_CARDINAL, &view, 1);
}

/* no caller yet */
void
zwm_ewmh_client_state(Client *c , void *p) {
	Window win = c->win;
	long data[] = {c->state, None};
	//printf("set state %s %d\n",c->name, c->state);
	zwm_x11_set_atoms(win, WM_STATE, WM_STATE, (unsigned long *)data, 2);
}
#if 0
/* no caller yet */
void 
zwm_ewmh_set_window_opacity(Window win, float opacity) {
	unsigned int o =(unsigned int)( opacity  * 0xFFFFFFFF);
    if (opacity == 1)
        XDeleteProperty (dpy, win, _NET_WM_WINDOW_OPACITY);
    else
        zwm_x11_set_atoms(win, _NET_WM_WINDOW_OPACITY, 
                XA_CARDINAL, (unsigned long *) &o, 1L);
}
#endif

static void
ewmh_process_state_atom(Client *c, Atom state, int set) {
    if(state == _NET_WM_STATE_FULLSCREEN) {
        if(set) {
		zwm_client_fullscreen(c);
        } else  {
		c->isfloating = False;
		zwm_layout_moveresize(c, c->x, c->y, c->w, c->h);
        }
        zwm_layout_arrange();
        zwm_client_raise(c);
        zwm_client_focus(c);
        //zwm_layout_arrange();
    }
}

void
clientmessage(XEvent *e, void *p) {
	XClientMessageEvent *ev = &e->xclient;
	Client *c;

	if(e->type != ClientMessage)
		return;

	if(ev->message_type == _NET_ACTIVE_WINDOW) {
#define ZOOM_ON_ACTIVE
#ifdef ZOOM_ON_ACTIVE
		c = zwm_client_get(ev->window);;
		if(c) {
			zwm_client_remove(c);
			zwm_client_push_head(c);
			zwm_layout_arrange();
			zwm_client_focus(c);
			zwm_client_raise(c);
			zwm_client_warp(c);
			//zwm_layout_arrange();
		}

#else
		c = zwm_client_get(ev->window);
		if(c) {
			zwm_client_setstate(c, NormalState);	
			zwm_client_raise(c);
			zwm_client_focus(c);
			Client *master = zwm_client_next_visible(zwm_client_head());
			if(c != master){
				zwm_client_remove(c);
				zwm_client_insert_after(master,c);
			}
		}
#endif
	} else if(ev->message_type == WM_CHANGE_STATE) {

		c = zwm_client_get(ev->window);
		if(c) {
			zwm_client_setstate(c, ev->data.l[0]);	
		}
	} else if(ev->message_type == _NET_CURRENT_DESKTOP) {

		current_view = ev->data.l[0];
		zwm_layout_arrange();
		zwm_ewmh_set_desktops(NULL, NULL);
	} else if(ev->message_type == _NET_WM_DESKTOP) {

		c = zwm_client_get(ev->window);
		if(c) {
			zwm_client_set_view(c, ev->data.l[0]);
			zwm_ewmh_set_window_desktop(c, NULL);
		}
	} else if(ev->message_type == _NET_WM_STATE) {

		if((c = zwm_client_get(ev->window))){   
			ewmh_process_state_atom(c, (Atom) ev->data.l[1], ev->data.l[0]);
			if(ev->data.l[2])
				ewmh_process_state_atom(c, (Atom) ev->data.l[2], ev->data.l[0]);
		}
	}

}

void
zwm_ewmh_init(void) {
    zwm_event_register(ZenClientFocus, (ZenEFunc)zwm_ewmh_set_active_window, NULL);
    zwm_event_register(ZenClientMap, (ZenEFunc)zwm_ewmh_set_client_list, NULL);
    zwm_event_register(ZenClientUnmap, (ZenEFunc)zwm_ewmh_client_unmap, NULL);
    zwm_event_register(ZenView, (ZenEFunc)zwm_ewmh_set_desktops, NULL);
    zwm_event_register(ZenClientView, (ZenEFunc)zwm_ewmh_set_window_desktop, NULL);
    zwm_event_register(ZenClientView, (ZenEFunc)zwm_ewmh_set_desktops, NULL);
    //zwm_event_register(ZenClientState, (ZenEFunc)zwm_ewmh_client_state, NULL);
    zwm_event_register(ClientMessage, (ZenEFunc)clientmessage, NULL);
    zwm_ewmh_set_desktops(NULL, NULL);
}

