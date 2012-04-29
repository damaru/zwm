void tile(int scr, int v) {
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
	//int w = (screen[scr].w/(n>1?2:1)) - B;
	int w = (screen[scr].w / (1<<(n>1))) - B;
	if (n) {
		zwm_layout_moveresize(lh, screen[scr].x, 
				screen[scr].y, w, screen[scr].h - B );
		for(i=1, c = zwm_client_next(lh); c ; c = zwm_client_next(c)){
			int h = (screen[scr].h / (n-1));
			if(zwm_layout_visible(c, v) && !c->isfloating){
				zwm_layout_moveresize(c, 
						screen[scr].x + (screen[scr].w/2),
						screen[scr].y+(i-1)*h , 
						w , h );
				i++;
			}
		}
	}
}
