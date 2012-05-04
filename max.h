
static void max_arrange(int scr, int v) {
	Client *c;
	zwm_client_foreach(c) {
		if (zwm_layout_visible(c,v) && !c->isfloating) {
			int x = screen[scr].x + config.border_width;
			int y = screen[scr].y ;
			int w = screen[scr].w - 2*c->border;
			int h = screen[scr].h ;
			c->noanim = 0;
			zwm_layout_moveresize(c, x, y , w, h);
		}
	}
}
