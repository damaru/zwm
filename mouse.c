
#include "zwm.h"
#include <X11/cursorfont.h>

#define MOUSEMASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask)

Cursor cursor_normal;
static Cursor cursor_resize;
static Cursor cursor_move;
static void ev_button_press(XEvent *e);

void
zwm_mouse_init(Display *dpy)
{
	cursor_normal = XCreateFontCursor(dpy, XC_left_ptr);
	cursor_resize  = XCreateFontCursor(dpy, XC_lr_angle);
	cursor_move = XCreateFontCursor(dpy, XC_fleur);
	zwm_event_register(ButtonPress, (ZwmEFunc)ev_button_press, NULL);
}

void
zwm_mouse_cleanup(Display *dpy)
{
	XFreeCursor(dpy, cursor_normal);
	XFreeCursor(dpy, cursor_move);
	XFreeCursor(dpy, cursor_resize);
}

enum {
	DoMove = 0,
	DoResize = 1
};

void mouse_move(Client *c, int resize) {
	unsigned int dui;
	int di;
	Window dummy;
	XEvent ev;
	Cursor cur = resize?cursor_resize:cursor_move;

	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync,
			GrabModeAsync, None, cur, CurrentTime) != GrabSuccess) 
				return;

	zwm_client_save_geometry(c, &c->fpos);
	zwm_client_float(c);
	XQueryPointer(dpy, root, &dummy, &dummy, &c->bpos.x, &c->bpos.y, &di, &di, &dui);
	zwm_layout_rearrange(False);

	if (resize) {
		XWarpPointer(dpy, None, c->frame, 0, 0, 0, 0, c->w, c->h);
	}

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
			if (resize) {
				c->w = max(ev.xmotion.x - c->fpos.x - 2 * c->border + 1, c->minw);
				c->h = max(ev.xmotion.y - c->fpos.y - 2 * c->border + 1, c->minh);
			} else {
				int s = zwm_current_screen();
				int sx = screen[s].x;
				int sy = screen[s].y;
				c->x =  min(max(c->fpos.x + (ev.xmotion.x - c->bpos.x), sx), sx + screen[s].w - c->w);
				c->y =  min(max(c->fpos.y + (ev.xmotion.y - c->bpos.y), sy), sy + screen[s].h - c->h);
			}
			zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
			break;
		}
		zwm_decor_update(c);
	} while(ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	zwm_client_save_geometry(c, &c->fpos);
	zwm_client_save_geometry(c, &c->bpos);
	zwm_x11_flush_events(EnterWindowMask);
}

static void button_click(Client *c, int x) {
	int j = c->w - config.button_count*config.button_width - 4*c->border;
	if(x > j )
	{
		int i = (x - j)/config.button_width;
		if(i < config.button_count) config.buttons[i].func(c);
		return ;
	}
	zwm_client_raise(c, False);
	mouse_move(c, DoMove);
}

static void ev_button_press(XEvent *e) {
	XButtonPressedEvent *ev = &e->xbutton;
	Client *c;

	 if((c = zwm_client_lookup(ev->window))) {
		zwm_client_focus(c);
		switch(ev->button) {
			case Button1:
				button_click(c, ev->x);
			break;
			case Button2:
				zwm_client_toggle_floating(c);
			break;
			case Button3:
				mouse_move(c, DoResize);
			break;
		}
	}
}

