
#include "zwm.h"

struct ZenLayout;
typedef struct ZenLayout ZenLayout;

struct ZenLayout
{
	ZenLFunc handler;
	char name[64];
	ZenLayout *next;
};

static ZenLayout *layouts = NULL;
static ZenLayout *sel_layout = NULL;

const char *zwm_current_layout(void)
{
	return sel_layout->name;
}

void zwm_layout_moveresize(Client *c, int x, int y, int w, int h)
{
	c->x  = x;
	c->y  = y;
	c->w  = w;
	c->h  = h;
}

void zwm_layout_animate(void)
{
	int i;
	Client *c;
	for(c = zwm_client_head();
			c;
			c = zwm_client_next(c)) {
		if(config.anim_steps && !c->noanim && zwm_client_visible(c) /*&& c->x >= 0 && c->x <= screen[0].w*/ ){
			c->dx = (c->x - c->ox)/config.anim_steps;
			c->dy = (c->y - c->oy)/config.anim_steps;
			c->dw = (c->w - c->ow)/config.anim_steps;
			c->dh = (c->h - c->oh)/config.anim_steps;
			//resize , but not move
			zwm_client_moveresize(c, c->ox, c->oy, c->w, c->h);
			XSync(dpy, False);
		} 
	}
	for(i = 0; i<config.anim_steps; i++){
		for(c = zwm_client_head();
				c;
				c = zwm_client_next(c)) {
			if(!c->noanim){
				c->ox += c->dx;
				c->oy += c->dy;
				c->ow += c->dw;
				c->oh += c->dh;
				zwm_client_moveresize(c, c->ox, c->oy, c->w, c->h);
				XSync(dpy, False);
			}
		}
	}
}


void zwm_layout_arrange(void)
{
	Client *c;
	for(c = zwm_client_head();
	       	c;
	       	c = zwm_client_next(c)) {
		c->noanim = 1;
		c->anim_steps = config.anim_steps;
		c->ox = c->x;
		c->oy = c->y;
		c->ow = c->w;
		c->oh = c->h;

		if( (c->type == ZenNormalWindow ||
			c->type == ZenDialogWindow) && 
			!zwm_client_visible(c)  ) {
			zwm_layout_moveresize(c, screen[0].w+2, c->y,c->w, c->h);
		}
	}
	
	sel_layout->handler();
#define DOANIM
#ifdef DOANIM
	zwm_layout_animate();
#endif
	for(c = zwm_client_head();
		    c;
		    c = zwm_client_next(c)) {
		zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
		zwm_client_update_decoration(c);
	}
}

void zwm_layout_register(ZenLFunc f, char *name)
{
	ZenLayout *l = zwm_malloc(sizeof (ZenLayout));
	l->handler = f ;
	strcpy(l->name , name);
	l->next = layouts;
	layouts = l;
	sel_layout = l;
}

void zwm_layout_next(void)
{
	if(sel_layout)
	sel_layout = sel_layout->next;

	if(!sel_layout || !sel_layout->next){	
		sel_layout = layouts;
	}
}

void zwm_layout_set(const char *name)
{
	ZenLayout *l = NULL;
	if (name) {
		l = layouts;
		while(l)
		{
			if(strcmp(l->name, name) == 0)
				break;
			l = l->next;
		}
	}

	if(l)
	{
		sel_layout = l;
	} else {
		zwm_layout_next();
	}
	zwm_event_emit(ZenLayoutChange, sel_layout->name);	
	zwm_layout_arrange();
}

static void
layout_floating(void) {
	Client *c;
	for(c = zwm_client_head();
	       	c;
	       	c = zwm_client_next(c)) {
		if(zwm_client_visible(c)) {
			zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
		}
	}
}

static float mb = 0.1;

#define ZWMBORDER (mb * screen[0].h)

static void
max_arrange(void) {
	Client *c;
	Client *second = NULL;
	int i = 0, j = 0;

	for( c = zwm_client_head(); c && j == 0; c = zwm_client_next(c)) {
		if(zwm_client_visible(c) && !c->isfloating && c->state != IconicState){
			int w = screen[i].w - (int)(ZWMBORDER*2) - 2*c->border ;
			int h = screen[i].h - (int)(ZWMBORDER*2) - 2*c->border ;
			int x = screen[i].x + ZWMBORDER;
			int y = screen[i].y + ZWMBORDER ;
			c->noanim = 0;
			zwm_layout_moveresize(c, x, y , w, h);
			j++;
			if(i < screen_count-1)i++;
		}
	}


	for(i = 0; c; c = zwm_client_next(c)) {
		if(zwm_client_visible(c) && !c->isfloating && c->state != IconicState){
			if(!second) {
				second = c;
			}

			c->noanim = 0;
			int w = screen[i].w - (int)(ZWMBORDER*2) - 2*c->border ;
			int h = screen[i].h - (int)(ZWMBORDER*2) - 2*c->border ;
			int x = screen[i].x + ZWMBORDER;
			int y = screen[i].y + ZWMBORDER;
			zwm_layout_moveresize(c, x + screen[i].w, y, w, h );
			//zwm_layout_moveresize(c, x, y, w, h );
			if(i < screen_count-1)i++;
		}
	}
	if(second){
		second->noanim = 1;
		int w = screen[i].w - (int)(ZWMBORDER*2) - 2*second->border ;
		int h = screen[i].h - (int)(ZWMBORDER*2) - 2*second->border ;
		int x = screen[i].x + ZWMBORDER;
		int y = screen[i].y + ZWMBORDER ;

//		zwm_layout_moveresize(second, x, y, w, h );
		zwm_layout_moveresize(second, x - screen[i].w, y, w, h );
	}
}

static void
grid() {
	unsigned int i, cx, cy, cw, ch, aw, ah, ax=0, cols, rows;
	Client *c;
	unsigned int n = 0;

	for(c = zwm_client_next_visible(zwm_client_head());
			c;
			c = zwm_client_next(c)) {

			if(zwm_client_visible(c) && !c->isfloating) {
				c->noanim = 0;
				n++;
			}
	}

	c = zwm_client_next_visible(zwm_client_head());

	if(screen_count > 1 && c)
	{
		zwm_layout_moveresize(c, screen[0].x, screen[0].y,
					screen[0].w - 2*config.border_width,
				       	screen[0].h - 2*config.border_width);
		c = zwm_client_next_visible(zwm_client_next(c));
		ax = screen[1].x;
		n--;
	}

	/* grid dimensions */
	for(rows = 0; rows <= n/2; rows++)
		if(rows*rows >= n)
			break;
	cols = (rows && (rows - 1) * rows >= n) ? rows - 1 : rows;

	/* window geoms (cell height/width) */
	ch = screen[0].h / (rows ? rows : 1);
	cw = screen[0].w / (cols ? cols : 1);

	for(i = 0; c; c = zwm_client_next(c)) {
		if(zwm_client_visible(c) && !c->isfloating){
			cx = ax + (i / rows) * cw;
			cy = (i % rows) * ch + screen[0].y;
			/* adjust height/width of last row/column's windows */
			ah = ((i + 1) % rows == 0) ? screen[0].h - ch * rows : 0;
			aw = (i >= rows * (cols - 1)) ? screen[0].w - cw * cols : 0;
			zwm_layout_moveresize(c, cx, cy, cw - 2 + aw - config.border_width,
				       	ch - 2  + ah -  config.border_width);
			i++;
		}
	}
}

void zwm_layout_cycle(const char *arg) {
	Client *c, *next;

	if(!sel) {
		zwm_client_focus(NULL);
		return;
	}

	c = zwm_client_next_visible(zwm_client_head());
	next = zwm_client_next_visible(c);
	zwm_event_emit(ZenClientUnmap, c);
	zwm_client_remove(c);
	zwm_client_push_tail(c);
	zwm_layout_arrange();
	zwm_client_raise(next);
	zwm_client_focus(next);
	zwm_event_emit(ZenClientMap, c);
}

void
tile() {
	Client *c = zwm_client_head();
	unsigned int i, n;
	int b;
	int B;
	if(c) {
	       	b = c->border;
       		B = 2*c->border;

	} else
	       	return;

	for(n = 0, c = zwm_client_next_visible(c); c;
		       	c = zwm_client_next_visible(zwm_client_next(c)))
	{
			if(zwm_client_visible(c) && !c->isfloating){
				c->noanim = 0;
				n++;
			}

	}

	for(i = 0, c = zwm_client_head();
			c;
			c = zwm_client_next_visible(zwm_client_next(c))) {

		if(zwm_client_visible(c) && !c->isfloating){

				c->noanim = 0;
			if(n == 1 ){
				zwm_layout_moveresize(c, b + screen[0].x, b +
						screen[0].y ,  
						screen[0].w - B-b,
						screen[0].h - B);
			} else {
				if( i == 0)
				{
					zwm_layout_moveresize(c,
							b+screen[0].x,
							b+screen[0].y,
							(screen[0].w / 2) - B,
							screen[0].h - B );
				} else {
					int h = (screen[0].h/ (n-1));
					zwm_layout_moveresize(c, screen[0].x + (screen[0].w/2)+B ,
							screen[0].y+(i-1)*h ,
							(screen[0].w/2)-2*B ,
							h );
				}
				i++;
			}
		}
	}
}
/* initialize default layout */
void
zwm_layout_init(void)
{
	zwm_layout_register((ZenLFunc)layout_floating, "floating");
//	zwm_layout_register((ZenLFunc)grid, "grid");
	zwm_layout_register((ZenLFunc)max_arrange, "max");
	zwm_layout_register((ZenLFunc)tile, "tile");
}
