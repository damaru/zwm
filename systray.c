#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include "zwm.h"

/* macros */
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define die(fmt,args...) fprintf(stderr, fmt,##args)
#define SYSTEM_TRAY_REQUEST_DOCK    0

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10

#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2

#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

typedef struct Systray   Systray;
struct Systray {
	Window win;
	Client *icons;
};

static void clientmessage(XEvent *e);
static void destroynotify(XEvent *e);
static void maprequest(XEvent *e);
static void propertynotify(XEvent *e);
static void unmapnotify(XEvent *e);
static Atom getatomprop(Client *c, Atom prop);
static unsigned int getsystraywidth();
static void removesystrayicon(Client *i);
static void resizerequest(XEvent *e);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void setclientstate(Client *c, long state);
static void updatesizehints(Client *c);
static void zwm_systray_updateicongeom(Client *i, int w, int h);
static void zwm_systray_updateiconstate(Client *i, XPropertyEvent *ev);
static Client *wintosystrayicon(Window w);

/* variables */
static Systray *systray =  NULL;

GC gc;

static unsigned long alloc_color(const char *colstr) {
	Colormap cmap = DefaultColormap(dpy, scr);
	gc = XCreateGC(dpy, root, 0, NULL);
	XColor color;

	if(!XAllocNamedColor(dpy, cmap, colstr, &color, &color))
	{
		fprintf(stderr, "unable to allocate color\n");
	}
	return color.pixel;
}

static const unsigned int systrayspacing = 2;   /* systray spacing */


void
clientmessage(XEvent *e)
{
	XWindowAttributes wa;
	XSetWindowAttributes swa;
	XClientMessageEvent *cme = &e->xclient;
	if (cme->window == systray->win && cme->message_type == _NET_SYSTEM_TRAY_OPCODE) {
		/* add systray icons */
		if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
                Client *c;
			if (!(c = (Client *)calloc(1, sizeof(Client))))
				fprintf(stderr, "fatal: could not malloc() %lu bytes\n", sizeof(Client));
			if (!(c->win = cme->data.l[2])) {
				free(c);
				return;
			}
			c->next = systray->icons;
			systray->icons = c;
			XGetWindowAttributes(dpy, c->win, &wa);
			c->x = c->y = 0;
			c->w = wa.width;
			c->h = wa.height;
			c->isfloating = True;
			/* reuse tags field as mapped status */
			c->tags = 1;
			updatesizehints(c);
			zwm_systray_updateicongeom(c, wa.width, wa.height);
			XAddToSaveSet(dpy, c->win);
			XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
			XReparentWindow(dpy, c->win, systray->win, 0, 0);
			/* use parents background color */
			swa.background_pixel  = config.xcolor_fbg;
			XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
			sendevent(c->win, _XEMBED, StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			/* FIXME not sure if I have to send these events, too */
			sendevent(c->win, _XEMBED, StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, _XEMBED, StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			//sendevent(c->win, _XEMBED, StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			XSync(dpy, False);
			zwm_systray_update();
			setclientstate(c, NormalState);
		}
		return;
	}
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintosystrayicon(ev->window))) {
		removesystrayicon(c);
		zwm_systray_update();
	}
}


Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;
	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	Atom req = XA_ATOM;
	if (prop == _XEMBED_INFO)
		req = _XEMBED_INFO;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		if (da == _XEMBED_INFO && dl == 2)
			atom = ((Atom *)p)[1];
		XFree(p);
	}
	return atom;
}


unsigned int
getsystraywidth()
{
	unsigned int w = 0;
	Client *i;
		for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;
	return w ? w + systrayspacing : 1;
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
	Client *i;
	if ((i = wintosystrayicon(ev->window))) {
		sendevent(i->win, _XEMBED, StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
		zwm_systray_update();
	}

	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if (wa.override_redirect)
		return;
}

void
propertynotify(XEvent *e)
{
	Client *c;
	XPropertyEvent *ev = &e->xproperty;

	if ((c = wintosystrayicon(ev->window))) {
		if (ev->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
			zwm_systray_updateicongeom(c, c->w, c->h);
		}
		else
			zwm_systray_updateiconstate(c, ev);
		zwm_systray_update();
	}

}

void
removesystrayicon(Client *i)
{
	Client **ii;

	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (ii)
		*ii = i->next;
	free(i);
}



void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		zwm_systray_updateicongeom(i, ev->width, ev->height);
		zwm_systray_update();
	}
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, WM_STATE, WM_STATE, 32,
		PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
{
	int exists = 0;
	XEvent ev;
    ev.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = proto;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = d0;
    ev.xclient.data.l[1] = d1;
    ev.xclient.data.l[2] = d2;
    ev.xclient.data.l[3] = d3;
    ev.xclient.data.l[4] = d4;
    XSendEvent(dpy, w, False, mask, &ev);
	return exists;
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

    if ((c = wintosystrayicon(ev->window))) {
		/* KLUDGE! sometimes icons occasionally unmap their windows, but do
		 * _not_ destroy them. We map those windows back */
		XMapRaised(dpy, c->win);
		zwm_systray_update();
	}
}


void
updatesizehints(Client *c)
{
#if 0
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
#endif
}
void
zwm_systray_updateicongeom(Client *i, int w, int h)
{
    unsigned int bh = config.title_height;
	if (i) {
		i->h = bh;
		if (w == h)
			i->w = bh;
		else if (h == bh)
			i->w = w;
		else
			i->w = (int) ((float)bh * ((float)w / (float)h));
		/* force icons into the systray dimenons if they don't want to */
		if (i->h > bh) {
			if (i->w == i->h)
				i->w = bh;
			else
				i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
			i->h = bh;
		}
	}
}

void
zwm_systray_updateiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;

	if ( !i || ev->atom != _XEMBED_INFO ||
			!(flags = getatomprop(i, _XEMBED_INFO)))
		return;

	if (flags & XEMBED_MAPPED && !i->tags) {
		i->tags = 1;
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	}
	else if (!(flags & XEMBED_MAPPED) && i->tags) {
		i->tags = 0;
		code = XEMBED_WINDOW_DEACTIVATE;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	}
	else
		return;
	sendevent(i->win, _XEMBED, StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}

Client *
wintosystrayicon(Window w) {
	Client *i = NULL;

	if (!w)
		return i;
	for (i = systray->icons; i && i->win != w; i = i->next) ;
	return i;
}


void zwm_systray_new(void)
{ 
    XClassHint hint = { "zwmt", "ZWMT" };
   	XSetWindowAttributes wa;
	unsigned int x = 1000;
	unsigned int w = 1;

    systray = (Systray *)calloc(1, sizeof(Systray));
    systray->win = XCreateSimpleWindow(dpy, root, x, 0, w, config.title_height, config.border_width-1, 0, config.xcolor_fbg);
    wa.event_mask        = ButtonPressMask | ExposureMask;
    wa.override_redirect = True;
    wa.background_pixel  = config.xcolor_fbg;
    XSelectInput(dpy, systray->win, SubstructureNotifyMask);
    XChangeProperty(dpy, systray->win, _NET_SYSTEM_TRAY_ORIENTATION, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&_NET_SYSTEM_TRAY_ORIENTATION_HORZ, 1);
    XChangeProperty(dpy, systray->win, _NET_WM_WINDOW_TYPE, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&_NET_WM_WINDOW_TYPE_DOCK, 4);
    XSetClassHint(dpy, systray->win, &hint);
    XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel, &wa);
    XMapRaised(dpy, systray->win);
    XSetSelectionOwner(dpy, _NET_SYSTEM_TRAY_S0, systray->win, CurrentTime);
    if (XGetSelectionOwner(dpy, _NET_SYSTEM_TRAY_S0) == systray->win) {
            sendevent(root, MANAGER, StructureNotifyMask, CurrentTime, _NET_SYSTEM_TRAY_S0, systray->win, 0, 0);
            XSync(dpy, False);
    }
    else {
            fprintf(stderr, "dwm: unable to obtain system tray.\n");
            free(systray);
            systray = NULL;
            return;
    }

    XSetWindowBackground(dpy, systray->win, config.xcolor_fbg);
	XSetWindowBorder(dpy, systray->win, config.xcolor_fbg);
	zwm_systray_update();

	zwm_event_register(ClientMessage, (ZwmEFunc)clientmessage, NULL);
	zwm_event_register(DestroyNotify, (ZwmEFunc)destroynotify, NULL);
	zwm_event_register(MapRequest, (ZwmEFunc)maprequest, NULL);
	zwm_event_register(PropertyNotify, (ZwmEFunc)propertynotify, NULL);
	zwm_event_register(UnmapNotify, (ZwmEFunc)unmapnotify, NULL);
	zwm_event_register(ResizeRequest, (ZwmEFunc)resizerequest, NULL);
	zwm_event_register(ZwmClientFocus,  (ZwmEFunc)zwm_systray_update, NULL);
}

void zwm_systray_update(void){
	XWindowChanges wc;
	Client *i;
	unsigned int w = 1;
	unsigned int x = 1000;

	for (w = 0, i = systray->icons; i; i = i->next) {
	    XSetWindowAttributes wa;
		/* make sure the background color stays the same */
        wa.background_pixel  = config.xcolor_fbg;
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		w += systrayspacing;
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		w += i->w;
	}
	w = w ? w + systrayspacing : 1;
	x -= w;

    if(sel) {
    	XReparentWindow(dpy, systray->win, sel->frame, 0, 0);
	    XMoveResizeWindow(dpy, systray->win, sel->w - w - config.icon_width , 0, w, config.title_height);
    } else {
    	XReparentWindow(dpy, systray->win, root, x, 0);
	    XMoveResizeWindow(dpy, systray->win, x, 0, w, config.title_height);
    }

	wc.x = sel?(sel->x+sel->w-w-config.icon_width):screen[0].w-w-config.icon_width; 
	//wc.y = sel?sel->y:0; 
	wc.y = 0; 
    wc.width = w; 
    wc.height = config.title_height;
    config.systray_width = w;
	//XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight, &wc);
	XConfigureWindow(dpy, systray->win, CWWidth, &wc);
	XSetWindowBackground(dpy, systray->win, config.xcolor_fbg);
	XMapSubwindows(dpy, systray->win);
	XMapRaised(dpy, systray->win);
	XSync(dpy, False);
}

void zwm_systray_hide(){
    XUnmapWindow(dpy, systray->win);
    XSync(dpy, False);
}

void
zwm_systray_cleanup(void)
{
	XUnmapWindow(dpy, systray->win);
	XDestroyWindow(dpy, systray->win);
    free(systray);
	XSync(dpy, False);
}

void
zwm_systray_detach(void)
{
   	XReparentWindow(dpy, systray->win, root, screen[0].w-config.systray_width, 0);
}
