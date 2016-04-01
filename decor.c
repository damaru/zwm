
#include "zwm.h"
#include <time.h>

static XftFont *xfont = NULL;
static XftFont *ifont = NULL;
static char icons[64];
static XftColor xcolor;
Colormap cmap;
static int date_width;
static int char_width;
static char title[1024];
static GC gc;

static ulong alloc_color(const char *colstr);
static void set_xcolor(unsigned long tcolor, unsigned short alpha);
static void draw_text(Client *c, XftFont *fnt, int fill, int tcol, int scol,
		int x, int y, const char *str);
static void get_status(char *s, int w);

void zwm_decor_init(void)
{
	int i;
	XGlyphInfo info;

	cmap = DefaultColormap(dpy, scr);
	gc = XCreateGC(dpy, root, 0, NULL);

	memset(icons,0,64);

	xfont = XftFontOpenName (dpy, scr, "DejaVu Sans-9:bold");
	if(!xfont)xfont = XftFontOpenXlfd(dpy, scr, config.font);

	ifont = XftFontOpenName (dpy, scr, "DejaVu Sans-16:bold");
	if(!ifont)ifont = XftFontOpenXlfd(dpy, scr, config.icons);

	XftColorAllocValue(dpy, DefaultVisual(dpy, scr), cmap, &xcolor.color, &xcolor);

	config.xcolor_nborder = alloc_color(config.normal_border_color);
	config.xcolor_fborder = alloc_color(config.focus_border_color);
	config.xcolor_nbg = alloc_color(config.normal_bg_color);
	config.xcolor_fbg = alloc_color(config.focus_bg_color);
	config.xcolor_flbg = alloc_color(config.float_bg_color);
	config.xcolor_nshadow = alloc_color(config.normal_shadow_color);
	config.xcolor_fshadow = alloc_color(config.focus_shadow_color);
	config.xcolor_ntitle = alloc_color(config.normal_title_color);
	config.xcolor_ftitle = alloc_color(config.focus_title_color);
	config.xcolor_root = alloc_color(config.root_bg_color);

	get_status(icons, 64);
	XftTextExtentsUtf8(dpy, xfont, (FcChar8*)" [000] ", 7, &info);
	char_width = info.width;
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

void decor_win_follow(Client *c)
{
	int bcolor = config.xcolor_fborder;
	int fill = config.xcolor_fbg;
	int shadow = config.xcolor_fshadow;
	int tcolor = config.xcolor_ftitle;

	if(!c->hastitle){
		return;
	}

	if (!c->frame) {
		return;
	}


	int ty = config.title_y;
	char n[256];

	XSetWindowBackground(dpy, c->frame, fill);
	XSetWindowBorder(dpy, c->frame, bcolor);

	XSetForeground(dpy, gc, fill);
	XFillRectangle (dpy, c->frame, gc, 0, 0, c->w, c->h);
	sprintf(n, "%s %s : %s",config.viewnames[c->oldpos.view], c->key, c->name);
	draw_text(c, xfont, fill, tcolor, shadow, 3, ty, n); 
}


extern int super_on;


void zwm_decor_update(Client *c)
{

	if(super_on){
		decor_win_follow(c);
		return;
	}

	int bcolor = c->root_user?config.xcolor_root:config.xcolor_fborder;
	int fill = c->root_user?config.xcolor_root:config.xcolor_fbg;
	int shadow = config.xcolor_fshadow;
	int tcolor = config.xcolor_ftitle;

	if(!c->hastitle){
		return;
	}

	if(c->focused == False && !super_on){
		bcolor = config.xcolor_nborder;
		fill = config.xcolor_nbg;
		shadow = config.xcolor_nshadow;
		tcolor = config.xcolor_ntitle;
	}

	if(c->isfloating){
		fill = config.xcolor_flbg;
		bcolor = config.xcolor_flbg;
	}

	if (c->frame) {
		int iw= c->w - (config.button_width*config.button_count) - 4*c->border;
		int vx = 4*c->border;
		const char* vtxt = config.viewnames[c->view];
		int tx = vx + config.button_width;
		int ty = config.title_y;
		char n[256];

		XSetWindowBackground(dpy, c->frame, fill);
		XSetWindowBorder(dpy, c->frame, bcolor);

		XSetForeground(dpy, gc, fill);
		XFillRectangle (dpy, c->frame, gc, 0, 0, c->w, c->h);

		draw_text(c, xfont, fill, tcolor, shadow, vx, ty, vtxt); 
		sprintf(n, "%s %s", c->isfloating?"▬":views[c->view].layout->symbol, c->name );
		draw_text(c, xfont, fill, tcolor, shadow, tx, ty, n); 
//		draw_text(c, xfont, fill, tcolor, shadow, tx + config.button_width, ty, c->name); 
		if(c->focused){
			get_status(title, 1024);
			char tmp[256];
			char cap[32];

			sprintf(tmp, "%s[%lu]",title, c->ucount/60);
			strcpy(title,tmp);
			draw_text(c, xfont, fill, tcolor, shadow, iw - date_width - 20 - char_width, ty, title); 
			draw_text(c, ifont, fill, tcolor, shadow, iw, config.icon_y, icons); 

			FILE *b = fopen("/sys/class/power_supply/BAT0/capacity", "r");
			if(b) {
				fgets(cap, 32, b);
				cap[3] = 0;
				int p = atoi(cap) ;
				if(p != 100) {
					//int x = iw - date_width - 20 - char_width - 15;
					int x = iw - config.button_width;
					XSetForeground(dpy, gc, config.xcolor_ftitle);
					XFillRectangle (dpy, c->frame, gc, x, 2, 8, ty);
					XSetForeground(dpy, gc, config.xcolor_nborder);
					XFillRectangle (dpy, c->frame, gc, x, 2, 8, (ty) * (100-p) / 100 );
				}
				fclose(b);
			}
		}
	}
}

static void draw_text(Client *c, XftFont *fnt, int fill, int tcol, int scol,
		int x, int y, const char *str)
{
	XSetForeground(dpy, gc, fill);
	XFillRectangle (dpy, c->frame, gc, x-5, 0, c->w, config.title_height);
	set_xcolor(scol, 0xFFFF);
	XftDrawStringUtf8(c->draw, &xcolor, fnt, x, y, (XftChar8 *)str, strlen(str));
	set_xcolor(tcol, 0xFFFF);
	XftDrawStringUtf8(c->draw, &xcolor, fnt, x-1, y-1, (XftChar8 *)str, strlen(str));
}

static void set_xcolor(unsigned long tcolor, unsigned short alpha)
{
	xcolor.color.red = ((tcolor  & 0xFF0000) >> 16 )* 0x101;
	xcolor.color.green = ((tcolor  & 0x00FF00) >> 8 )* 0x101;
	xcolor.color.blue = ((tcolor  & 0x0000FF) )* 0x101;
	xcolor.color.alpha = alpha;
	xcolor.pixel = 0xFFFFFF00;
}

static void get_status(char *s, int w)
{
	struct tm *tm;
	time_t t = 0;
	time(&t);
	tm = localtime(&t);
	strftime(s, w, config.date_fmt, tm);
}

static ulong alloc_color(const char *colstr) {
	Colormap cmap = DefaultColormap(dpy, scr);
	XColor color;

	if(!XAllocNamedColor(dpy, cmap, colstr, &color, &color))
	{
		zwm_util_perror("unable to allocate color\n");
	}
	return color.pixel;
}
