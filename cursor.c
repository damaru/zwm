
#include "zwm.h"
#include <X11/cursorfont.h>

static Cursor cursor[CurLast];

void
zwm_init_cursor(Display *dpy)
{
	cursor[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(dpy, XC_fleur);
}

void
zwm_free_cursors(Display *dpy)
{
	XFreeCursor(dpy, cursor[CurNormal]);
	XFreeCursor(dpy, cursor[CurResize]);
	XFreeCursor(dpy, cursor[CurMove]);
}

Cursor
zwm_get_cursor(ZenCursor c)
{
	return cursor[c];
}


