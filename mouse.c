
#include "zwm.h"
#include <X11/cursorfont.h>

#define MODKEY	   Mod1Mask
#define MOUSEMASK  (ButtonPressMask | ButtonReleaseMask | PointerMotionMask)
#define BUTTONMASK (ButtonPressMask | ButtonReleaseMask)

Cursor cursor_normal;
static Cursor cursor_resize;
static Cursor cursor_move;
static void ev_button_press(XEvent *e);
static void grab(Window win, unsigned int button);

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

void 
zwm_mouse_grab(Client *c, Bool focused)
{
	if(focused) {
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		grab(c->win, Button1);
		grab(c->win, Button2);
		grab(c->win, Button3);
	} else {
		XGrabButton(dpy, AnyButton, AnyModifier, c->win, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
	}
}

static void 
grab(Window win, unsigned int button)
{
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	int j;

	for(j = 0; j < sizeof(modifiers)/sizeof(unsigned int); j++) {
		XGrabButton(dpy, button, MODKEY | modifiers[j], win, False,
			       	BUTTONMASK, GrabModeAsync, GrabModeSync,
			       	None, None);
	}
}

enum {
	DoMove = 0,
	DoResize = 1
};

static void 
move_resize(Client *c, int resize, int x, int y)
{
	if (resize) {
		c->w = max(x - c->fpos.x - 2 * c->border + 1, c->minw);
		c->h = max(y - c->fpos.y - 2 * c->border + 1, c->minh);
	} else {
		int s = zwm_current_screen();
		int sx = screen[s].x;
		int sy = screen[s].y;
		c->x =  min(max(c->fpos.x + (x - c->bpos.x), sx), sx + screen[s].w - c->w);
		c->y =  min(max(c->fpos.y + (y - c->bpos.y), sy), sy + screen[s].h - c->h);
	}
}

static void 
mouse_move(Client *c, int resize)
{
	unsigned int dui;
	int di, mx, my;
	Window dummy;
	XEvent ev;
	Cursor cur = resize?cursor_resize:cursor_move;

	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync,
			GrabModeAsync, None, cur, CurrentTime) != GrabSuccess) 
				return;

	zwm_client_save_geometry(c, &c->fpos);
	zwm_client_float(c);
	XQueryPointer(dpy, root, &dummy, &dummy, &mx, &my, &di, &di, &dui);
	c->bpos.x = mx, c->bpos.y = my;
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

#if 0
			if(
					(c->bpos.x > c->x || c->bpos.y > c->y)
					&& c->x < screen[zwm_client_screen(c)].x + 10  
					&& c->y < screen[zwm_client_screen(c)].y +10
					&& !resize

				//	&& c->x+c->w == screen[zwm_client_screen(c)].w 
				//	&& c->y+c->h == screen[zwm_client_screen(c)].h
					){
				zwm_client_toggle_floating(c);
				zwm_client_remove(c);
				zwm_client_push_head(c);
				ev.type = ButtonRelease;
			}
#endif

			move_resize(c, resize, ev.xmotion.x, ev.xmotion.y);
			zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
			c->view = screen[zwm_client_screen(c)].view;

			break;
		}
		zwm_decor_update(c);
	} while(ev.type != ButtonRelease);

	XUngrabPointer(dpy, CurrentTime);
	zwm_client_save_geometry(c, &c->fpos);
	zwm_client_save_geometry(c, &c->bpos);
	zwm_client_save_geometry(c, &c->oldpos);
	zwm_x11_flush_events(EnterWindowMask);

	if(resize)return;

	if(c->x == screen[zwm_client_screen(c)].x && c->y == screen[zwm_client_screen(c)].y) {

				zwm_client_toggle_floating(c);
				zwm_client_remove(c);
				zwm_client_push_head(c);
	}
	if(c->x+c->w == screen[zwm_client_screen(c)].w && c->y+c->h == screen[zwm_client_screen(c)].h) {

				zwm_client_toggle_floating(c);
				zwm_client_remove(c);
				zwm_client_push_tail(c);
	}

}

static void
button_click(Client *c, int x) 
{
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

static void 
ev_button_press(XEvent *e) 
{
	XButtonPressedEvent *ev = &e->xbutton;
	Client *c;

	if(ev->window == root && config.menucmd){
		zwm_util_spawn(config.menucmd);
	}

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
			case Button4:
				zwm_cycle(NULL);
			break;
			case Button5:
				zwm_cycle2(NULL);
			break;
		}
	}
}

