
static void zen_arrange(int scr, int v) {
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
