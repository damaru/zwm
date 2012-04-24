#if 0
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
#endif

