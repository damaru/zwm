
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
	int w = (screen[scr].w * (n>1? config.mwfact :1) ) - B;
	if (n) {
		zwm_layout_moveresize(lh, screen[scr].x, 
				screen[scr].y, w, screen[scr].h - B );
		for(i=1, c = lh->next; c ; c = c->next){
			if(zwm_layout_visible(c, v) && !c->isfloating){
				int h = (screen[scr].h / (n-1)) - config.border_width;
				zwm_layout_moveresize(c, 
						screen[scr].x + (screen[scr].w * config.mwfact),
						screen[scr].y + (i-1)*h , 
						screen[scr].w * (1-config.mwfact), h );
				i++;
			}
		}
	}
}
