
#include "zwm.h"
#include <time.h>

static int dirty = 0;

void zwm_layout_set(const char *name)
{
	ZwmLayout *l = NULL, *t;
	for (t = config.layouts; name && t->func; t++) {
		if(strcmp(t->name, name) == 0){
			l = t;
			break;
		}
	}
	if (!l) {
		l = views[screen[zwm_current_screen()].view].layout + 1;
		while (l->func && l->skip) {
			l++;
		}
		if (!l->func) l = config.layouts;
	}
	views[screen[zwm_current_screen()].view].layout = l;
	zwm_layout_dirty();
}

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
	
	zwm_client_foreach(c) {
		if (c->noanim) {
			continue;
		}
		c->dx = (c->x - c->oldpos.x)/config.anim_steps;
		c->dy = (c->y - c->oldpos.y)/config.anim_steps;
		c->dw = (c->w - c->oldpos.w)/config.anim_steps;
		c->dh = (c->h - c->oldpos.h)/config.anim_steps;
		int w = c->w>c->oldpos.w?c->w:c->oldpos.w;
		int h = c->h>c->oldpos.h?c->h:c->oldpos.h;
		XMoveResizeWindow(dpy, c->win, c->border, config.title_height, 
				w-2*c->border, h-config.title_height-2*c->border);
		XSync(dpy, False);
	}

	//int d = 1000000;
	int d = 1;
	for(i = 0; i<config.anim_steps; i++){
		struct timespec req = {0, d/config.anim_steps };
		d *= 2;
		//d += 1000000;
		nanosleep(&req, NULL);
		zwm_client_foreach(c) {
			if(c->noanim){
				continue;
			}
			c->oldpos.x += c->dx;
			c->oldpos.y += c->dy;
			c->oldpos.w += c->dw;
			c->oldpos.h += c->dh;
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
		XSync(dpy, False);
	}
}

void zwm_layout_arrange(void)
{
	int i;
	Client *c;

	if (!head) 
		return;

	zwm_client_foreach(c) {
		c->anim_steps = config.anim_steps;
		c->noanim = 0;
		if( (c->type == ZwmNormalWindow || c->type == ZwmDialogWindow) && 
			(!zwm_client_visible(c, c->view) || c->isfloating) ) {
			if (c->bpos.w == 0) {
				zwm_client_save_geometry(c, &c->bpos);
			}
			zwm_layout_moveresize(c, c->x, c->y - 2*screen[0].h, c->w, c->h);
		}
		if (zwm_client_visible(c, c->view) && c->bpos.w) {
			zwm_client_restore_geometry(c, &c->bpos);
			c->bpos.w = 0;
		}
	}
	
	for(i = 0; i < config.screen_count; i++) {
		views[screen[i].view].layout->func(i, screen[i].view);
	}

	if (config.anim_steps) {
		zwm_layout_animate();
	}

	zwm_client_foreach (c) {
		zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
		c->noanim = 1;
	}
}

