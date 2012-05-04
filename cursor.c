
#include "zwm.h"
#include <X11/cursorfont.h>

static Cursor cursor[CurLast];

void
zwm_x11_cursor_init(Display *dpy)
{
	cursor[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(dpy, XC_fleur);
}

void
zwm_x11_cursor_free(Display *dpy)
{
	XFreeCursor(dpy, cursor[CurNormal]);
	XFreeCursor(dpy, cursor[CurResize]);
	XFreeCursor(dpy, cursor[CurMove]);
}

Cursor
zwm_x11_cursor_get(ZwmCursor c)
{
	return cursor[c];
}


