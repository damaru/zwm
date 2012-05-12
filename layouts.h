

static Bool zwm_layout_visible(Client *c, int view) 
{
	return c->state == NormalState &&
		c->view == view && 
		(c->type == ZwmNormalWindow || c->type == ZwmDialogWindow) ;
}


static void zen(int scr, int v) {
	Client *c;
	int x,y,w,h;
	if(v<ZWM_ZEN_VIEW)
		return;
	zwm_client_foreach(c) {
		if (zwm_layout_visible(c,v) && !c->isfloating) {
			if (config.zen_wallpaper) {
				x = screen[scr].x + config.border_width + 200;
				y = screen[scr].y + 20;
				w = screen[scr].w - 2*c->border - 400;
				h = screen[scr].h - 40;
			} else {
				x = screen[scr].x - 2*c->border;
				y = screen[scr].y - c->border - config.title_height;;
				w = screen[scr].w + 2*c->border;
				h = screen[scr].h + 2*c->border+ config.title_height;
			}
			c->noanim = 0;
			if(views[v].current != c) {
				zwm_client_set_view(c, screen[scr].prev);
			} else {
				zwm_layout_moveresize(c, x, y, w, h);
			}
		}
	}
}

static void floating(int scr, int v) {
	Client *c;
	for(c = head; c; c = c->next) {
		c->noanim = 1;
		if(zwm_layout_visible(c, v)) {
			zwm_client_moveresize(c, c->x, c->y, c->w, c->h);
		}
	}
}

static void tile(int scr, int v) {
	Client *c, *lh = NULL;
	unsigned int i, n = 0;
	int B = 2*config.border_width;
	zwm_client_foreach (c) {
		if(zwm_layout_visible(c, v) && !c->isfloating){
			c->noanim = 0;
			if(!lh) lh = c;
			n++;
		}
	}
	int w = (screen[scr].w * (n>1? views[v].mwfact :1) ) - B;
	if (n) {
		zwm_layout_moveresize(lh, screen[scr].x, 
				screen[scr].y, w, screen[scr].h - B );
		for(i=1, c = lh->next; c ; c = c->next){
			if(zwm_layout_visible(c, v) && !c->isfloating){
				int h = (screen[scr].h / (n-1)) - config.border_width;
				zwm_layout_moveresize(c, 
						screen[scr].x + (screen[scr].w * views[v].mwfact),
						screen[scr].y + (i-1)*h , 
						screen[scr].w * (1-views[v].mwfact) - B, h );
				i++;
			}
		}
	}
}

static void monocle(int scr, int v) {
	Client *c;
	zwm_client_foreach(c) {
		if (zwm_layout_visible(c,v) && !c->isfloating) {
			int x = screen[scr].x;
			int y = screen[scr].y;
			int w = screen[scr].w - 2 * config.border_width;
			int h = screen[scr].h;
			c->noanim = 0;
			zwm_layout_moveresize(c, x, y , w, h);
		}
	}
}


