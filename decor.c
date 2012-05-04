
#include "zwm.h"
#include <X11/Xft/Xft.h>
#include <time.h>

static XftFont *xfont = NULL;
static XftFont *ifont = NULL;
static char icons[64];
static XftColor xcolor;
Colormap cmap;
int date_width;
char title[1024];
char status[1024];

static void set_xcolor(unsigned long tcolor, unsigned short alpha);
static void get_status(char *s, int w)
{
	struct tm *tm;
	time_t t = 0;
	time(&t);
	tm = localtime(&t);
	strftime(s, w, "%r", tm);
}

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


	get_status(icons, 64);
	XftTextExtentsUtf8(dpy, xfont, (FcChar8*)icons, strlen(icons), &info);
	date_width = info.width;
	icons[0] = 0;

	for(i=0; i<32 && config.buttons[i].func; i++){
		strcat(icons, config.buttons[i].c);
		config.button_count = i+1;
	}

	
	if(config.button_count){
		XftTextExtentsUtf8(dpy, ifont, (FcChar8*)icons, strlen(icons), &info);
		config.button_width = info.width / config.button_count;
		config.icon_y = info.height + (config.title_height - info.height)/2 + 1;
	}
	config.title_y = config.title_height - xfont->descent - 2;
}

void zwm_decor_dirty(Client *c){
	c->dirty++;
}

void zwm_decor_update(Client *c)
{
	int bcolor = config.xcolor_fborder;
	int fill = config.xcolor_fbg;
	int shadow = config.xcolor_fshadow;
	int tcolor = config.xcolor_ftitle;

	if(!c->hastitle){
		return;
	}

	if(c->focused == False){
		bcolor = config.xcolor_nborder;
		fill = config.xcolor_nbg;
		shadow = config.xcolor_nshadow;
		tcolor = config.xcolor_ntitle;
	}

	if(c->frame) {

		XftDraw *draw = XftDrawCreate(dpy, c->frame, DefaultVisual(dpy, scr), cmap);
		int iw= c->w - (config.button_width*config.button_count) - 4*c->border;
		int y = config.title_y;
		sprintf(title, "%s %s", c->isfloating?"":config.viewnames[c->view], c->name);

		XSetWindowBackground(dpy, c->frame, fill);
		XSetWindowBorder(dpy, c->frame, bcolor);
		XSetForeground(dpy, gc, fill);
		XFillRectangle (dpy, c->frame, gc, 0, 0, c->w, c->h);

		set_xcolor(shadow, 0xFFFF);

		XftDrawStringUtf8(draw, &xcolor, xfont, 4*c->border, y, (XftChar8 *)title, strlen(title));
		XftDrawStringUtf8(draw, &xcolor, ifont, iw, config.icon_y, (FcChar8*)icons, strlen(icons));

		set_xcolor(tcolor, 0xFFFF);

		XftDrawStringUtf8(draw, &xcolor, xfont, 4*c->border-1, y-1, (XftChar8 *)title, strlen(title));
		XftDrawStringUtf8(draw, &xcolor, ifont, iw-1, config.icon_y-1, (FcChar8*)icons, strlen(icons));

		if(c->focused) {
			get_status(status, 1024);
			set_xcolor(shadow, 0xFFFF);
			XftDrawStringUtf8(draw, &xcolor, xfont, iw - date_width - 5, y, (XftChar8 *)status, strlen(status));
			set_xcolor(tcolor, 0xFFFF);
			XftDrawStringUtf8(draw, &xcolor, xfont, iw - date_width - 5 -1 , y-1, (XftChar8 *)status, strlen(status));
		}


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
