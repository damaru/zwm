
static void zen_arrange(int scr, int v) {
	Client *c;
	zwm_client_foreach(c) {
		if (zwm_layout_visible(c,v) && !c->isfloating) {
			int x = screen[scr].x + config.border_width + 200;
			int y = screen[scr].y + 20;
			int w = screen[scr].w - 2*c->border - 400;
			int h = screen[scr].h - 40;
			c->noanim = 0;
			zwm_layout_moveresize(c, x, y , w, h);
		}
	}
}
