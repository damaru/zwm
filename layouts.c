
#include "zwm.h"
#include <time.h>

typedef struct ZenLayout
{
	ZenLFunc handler;
	char name[64];
	struct ZenLayout *next;
}ZenLayout;

static ZenLayout *layouts = NULL;
static ZenLayout *sel_layout = NULL;
static int dirty = 0;

void zwm_layout_dirty(void)
{
	dirty = 1;
}

void zwm_layout_rearrange(Bool force)
{
	if(dirty || force){
		zwm_view_rescan();
		dirty = 0;
	}
}

Bool zwm_layout_visible(Client *c, int view) 
{
	return c->state == NormalState &&
		c->view == view && 
		(c->type == ZenNormalWindow || c->type == ZenDialogWindow) ;
}

void zwm_layout_moveresize(Client *c, int x, int y, int w, int h)
{
	if(c->noanim) {
		c->noanim = 0;
		c->ox = c->x;
		c->oy = c->y;
		c->ow = c->w;
		c->oh = c->h;
	}
	c->x  = x;
	c->y  = y;
	c->w  = w;
	c->h  = h;
}

void zwm_layout_animate(void)
{
	int i;
	Client *c;
	if(!config.anim_steps)
	{
		return;
	}
	for(c = head; c; c = c->next) {
		if(!c->noanim){
			c->dx = (c->x - c->ox)/config.anim_steps;
			c->dy = (c->y - c->oy)/config.anim_steps;
			//resize , but not move
			zwm_client_moveresize(c, c->ox, c->oy, c->w, c->h);
			XSync(dpy, False);
		}
	}
	for(i = 0; i<config.anim_steps; i++){
		struct timespec req = {0, 100000000/config.anim_steps };
		for(c = head; c; c = c->next) {
			if(!c->noanim){
				c->ox += c->dx;
				c->oy += c->dy;
				if((int)c->ox != (int)c->x || (int)c->oy != (int)c->y){
					zwm_client_moveresize(c, c->ox, c->oy, c->w, c->h);
				}
			}
		}
		nanosleep(&req, NULL);
		XSync(dpy, False);
	}
}

void zwm_layout_arrange(void)
{
	Client *c = head;
	int i;
	if (!c) {
		return;
	}
	for(; c; c = c->next) {
		if (zwm_client_visible(c, c->view) && c->bpos.w) {
			zwm_client_restore_geometry(c, &c->bpos);
			c->bpos.w = 0;
		}

		c->anim_steps = config.anim_steps;

		if( (c->type == ZenNormalWindow ||
			c->type == ZenDialogWindow) && 
			!zwm_client_visible(c, c->view)  ) {
			if (c->bpos.w == 0) {
				zwm_client_save_geometry(c, &c->bpos);
			}
			//TODO fix this
			if(c->state == IconicState) {
				zwm_layout_moveresize(c, c->x, c->y+screen[0].h, c->w, c->h);
			} else if(config.screen_count > 1){
				zwm_layout_moveresize(c, c->x, c->y-2*screen[0].h, c->w, c->h);
			} else {
				zwm_layout_moveresize(c, screen[config.screen_count-1].x + screen[config.screen_count-1].w + 2+c->x, c->y, c->w, c->h);
			}
		}
	}
	
	for(i = 0; i < config.screen_count; i++) {
		sel_layout->handler(i, screen[i].view);
	}

	zwm_layout_animate();

	for(c = head; c; c = c->next) {
		zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
		c->noanim = 1;
	}
}

void zwm_layout_register(ZenLFunc f, char *name)
{
	ZenLayout *l = zwm_util_malloc(sizeof (ZenLayout));
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
	zwm_layout_dirty();
}

static void layout_floating(int scr, int v) {
	Client *c;
	for(c = head; c; c = c->next) {
		if(zwm_layout_visible(c, v)) {
			zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
		}
	}
}

#include "max.h"
#include "grid.h"
#include "tile.h"

void zwm_layout_cycle(const char *arg) {
	Client *c, *next;

	if(!sel) {
		zwm_client_refocus();
		return;
	}

	c = zwm_client_next_visible(head);
	next = zwm_client_next_visible(c);
	if(next){
		zwm_event_emit(ZenClientUnmap, c);
		zwm_client_remove(c);
		zwm_client_push_tail(c);
		//zwm_client_raise(next);
		zwm_client_warp(next);
		zwm_event_emit(ZenClientMap, next);
		zwm_layout_dirty();
	}
}

void zwm_layout_init(void)
{
	zwm_layout_register((ZenLFunc)layout_floating, "floating");
//	zwm_layout_register((ZenLFunc)grid, "grid");
	zwm_layout_register((ZenLFunc)max_arrange, "max");
	zwm_layout_register((ZenLFunc)tile, "tile");
}
