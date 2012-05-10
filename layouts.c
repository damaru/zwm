
#include "zwm.h"
#include <time.h>

static ZwmLayout *layouts = NULL;
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
		(c->type == ZwmNormalWindow || c->type == ZwmDialogWindow) ;
}

void zwm_layout_moveresize(Client *c, int x, int y, int w, int h)
{
	if(c->noanim) {
		c->noanim = 0;
		zwm_client_save_geometry(c, &c->oldpos);
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
			c->dx = (c->x - c->oldpos.x)/config.anim_steps;
			c->dy = (c->y - c->oldpos.y)/config.anim_steps;
			c->dh = (c->h - c->oldpos.h)/config.anim_steps;
			c->dw = (c->w - c->oldpos.w)/config.anim_steps;
			int h = c->h>c->oldpos.h?c->h:c->oldpos.h;
			int w = c->w>c->oldpos.w?c->w:c->oldpos.w;
			XMoveResizeWindow(dpy, c->win, c->border, config.title_height, 
					w-2*c->border, h-config.title_height-2*c->border);
			XSync(dpy, False);
		}
	}

	for(i = 0; i<config.anim_steps; i++){
		struct timespec req = {0, 100000000/config.anim_steps };
		for(c = head; c; c = c->next) {
			if(!c->noanim){
				c->oldpos.x += c->dx;
				c->oldpos.y += c->dy;
				c->oldpos.h += c->dh;
				c->oldpos.w += c->dw;
				if( abs(c->y - c->oldpos.y) > 0 || 
				    abs(c->x - c->oldpos.x) > 0 ||
				    abs(c->w - c->oldpos.w) > 0 ||
				    abs(c->h - c->oldpos.h) > 0 ) {
					XMoveResizeWindow(dpy, c->frame, 
							c->oldpos.x,
							c->oldpos.y,
							c->oldpos.w,
							c->oldpos.h);
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
		if (zwm_client_visible(c, screen[zwm_client_screen(c)].view) && c->bpos.w) {
			zwm_client_restore_geometry(c, &c->bpos);
			c->bpos.w = 0;
		}

		c->anim_steps = config.anim_steps;

		if( !c->isfloating && (c->type == ZwmNormalWindow ||
			c->type == ZwmDialogWindow) && 
			!zwm_client_visible(c, screen[zwm_client_screen(c)].view)  ) {
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
		views[screen[i].view].layout->handler(i, screen[i].view);
	}

	zwm_layout_animate();

	for(c = head; c; c = c->next) {
		zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
		c->noanim = 1;
	}
}

void zwm_layout_register(ZwmLFunc f, char *name, int skip)
{
	ZwmLayout *l = zwm_util_malloc(sizeof (ZwmLayout));
	l->handler = f ;
	l->skip = skip;
	strcpy(l->name , name);
	l->next = layouts;
	layouts = l;
	views[screen[zwm_current_screen()].view].layout = l;
}

void zwm_layout_next(void)
{
	ZwmLayout  *sel_layout = views[screen[zwm_current_screen()].view].layout;
	while(sel_layout) {
		sel_layout = sel_layout->next;
		if(sel_layout && !sel_layout->skip){
			break;
		}
	}

	if(!sel_layout){	
		sel_layout = layouts;
	}
	views[screen[zwm_current_screen()].view].layout = sel_layout;
}

void zwm_layout_set(const char *name)
{
	ZwmLayout *l = NULL;
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
		views[screen[zwm_current_screen()].view].layout = l;
	} else {
		zwm_layout_next();
	}
	zwm_event_emit(ZwmLayoutChange, views[screen[zwm_current_screen()].view].layout->name);	
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
#include "zen.h"

void zwm_layout_init(void)
{
	zwm_layout_register((ZwmLFunc)layout_floating, "floating", 1);
//	zwm_layout_register((ZwmLFunc)grid, "grid", 0);
	zwm_layout_register((ZwmLFunc)max_arrange, "max", 0);
	zwm_layout_register((ZwmLFunc)zen_arrange, "zen", 1);
	zwm_layout_register((ZwmLFunc)tile, "tile", 0);
	int i;
	for(i=0; i<MAX_VIEWS; i++){
		views[i].layout = layouts;
	}
}
