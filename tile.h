void tile(int scr, int v) {
	Client *c = zwm_client_head();
	unsigned int i, n;
	int B;
	if(c) {
       		B = 2*config.border_width;

	} else
	       	return;

	for(n = 0; c; c = zwm_client_next(c))
	{
		if(zwm_layout_visible(c, v) && !c->isfloating){
			c->noanim = 0;
			n++;
		}
	}

	for(i = 0, c = zwm_layout_head(v);
			c;
			c = zwm_layout_next_client(c, v)) {

		c->view = v;
		c->screen = scr;
		if(n == 1 ){
			zwm_layout_moveresize(c, 
					screen[scr].x, 
					screen[scr].y ,  
					screen[scr].w - B,
					screen[scr].h - B);
			return;
		} else {
			if( i == 0)
			{
				zwm_layout_moveresize(c,
						screen[scr].x,
						screen[scr].y,
						(screen[scr].w / 2) - B,
						screen[scr].h - B );
			} else {
				int h = (screen[scr].h/ (n-1));
				zwm_layout_moveresize(c, 
						screen[scr].x + (screen[scr].w/2),
						screen[scr].y+(i-1)*h ,
						(screen[scr].w/2)-B ,
						h );
			}
			i++;
		}
	}
}


