
#include "zwm.h"
#include <X11/Xft/Xft.h>

static XftFont *xfont = NULL;
static XftFont *ifont = NULL;
static char icons[64];
static XftColor xcolor;
Colormap cmap;
static void set_xcolor(unsigned long tcolor, unsigned short alpha);

void zwm_decor_init(void)
{
	int i;
	XGlyphInfo info;
	cmap = DefaultColormap(dpy, scr);

	memset(icons,0,64);

	xfont = XftFontOpenName (dpy, scr, "Sans-9:bold");
	if(!xfont)xfont = XftFontOpenXlfd(dpy, scr, config.font);
	ifont = XftFontOpenName (dpy, scr, "Sans-16:bold");
	if(!ifont)ifont = XftFontOpenXlfd(dpy, scr, config.icons);
	XftColorAllocValue(dpy, DefaultVisual(dpy, scr), cmap, &xcolor.color, &xcolor);

	for(i=0; i<32 && config.buttons[i].func; i++){
		strcat(icons, config.buttons[i].c);
		config.button_count = i+1;
	}

	if(config.button_count){
		XftTextExtentsUtf8(dpy, ifont, (FcChar8*)icons, strlen(icons), &info);
		config.button_width = info.width / config.button_count;
	}
	config.title_y = config.title_height - xfont->descent - 2;
}

void
zwm_client_update_decoration(Client *c)
{
	int bcolor = config.xcolor_fborder;
	int fill = config.xcolor_fbg;
	int shadow = config.xcolor_fshadow;
	int tcolor = BlackPixel(dpy, 0);
	char title[1024];

	if(!c->hastitle){
		return;
	}

	if(c->focused == False){
		bcolor = config.xcolor_nborder;
		fill = config.xcolor_nbg;
		shadow = config.xcolor_nshadow;
		tcolor = config.xcolor_fshadow;
	}

	if(c->frame) {

		XftDraw *draw = XftDrawCreate(dpy, c->frame, DefaultVisual(dpy, scr), cmap);
		int iw= c->w - (config.button_width*config.button_count) - 4*c->border;
		int y = config.title_y;

		sprintf(title, "%s %s", config.viewnames[c->view], c->name );

		XSetWindowBackground(dpy, c->frame, fill);
		XSetWindowBorder(dpy, c->frame, bcolor);
		XSetForeground(dpy, gc, fill);
		XFillRectangle (dpy, c->frame, gc, 0, 0, c->w, c->h);
		XSetForeground(dpy, gc, config.xcolor_nshadow);

		set_xcolor(shadow, 0xFFFF);

		XftDrawStringUtf8(draw, &xcolor, xfont, 4*c->border, y, (XftChar8 *)title, strlen(title));
		XftDrawStringUtf8(draw, &xcolor, ifont, iw, config.title_height, (FcChar8*)icons, strlen(icons));

		set_xcolor(tcolor, 0xFFFF);

		XftDrawStringUtf8(draw, &xcolor, xfont, 4*c->border-1, y-1, (XftChar8 *)title, strlen(title));
		XftDrawStringUtf8(draw, &xcolor, ifont, iw-1, config.title_height-1, (FcChar8*)icons, strlen(icons));

		free(draw);
	}
}

static void set_xcolor(unsigned long tcolor, unsigned short alpha)
{
	xcolor.color.red = ((tcolor  & 0xFF0000) >> 16 )* 0x101;
	xcolor.color.green = ((tcolor  & 0x00FF00) >> 8 )* 0x101;
	xcolor.color.blue = ((tcolor  & 0x0000FF) )* 0x101;
	xcolor.color.alpha = alpha;
	xcolor.pixel = 0xFFFFFF00;
}
