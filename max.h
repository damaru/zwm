
static float mb = 0;
#define ZWMBORDER (mb * screen[0].h)

static void max_arrange(int scr, int v) {
	Client *c;
	zwm_client_foreach(c) {
		if (zwm_layout_visible(c,v) && !c->isfloating) {
			int w = screen[scr].w - (int)(ZWMBORDER*2) - 2*config.border_width ;
			int h = screen[scr].h - (int)(ZWMBORDER*2) - 2*config.border_width ;
			int x = screen[scr].x + ZWMBORDER + config.border_width;
			int y = screen[scr].y + ZWMBORDER ;
			c->noanim = 0;
			zwm_layout_moveresize(c, x, y , w, h);
		}
	}
}
