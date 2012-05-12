
#include "zwm.h"
#include <X11/extensions/Xinerama.h>

ZwmScreen screen[MAX_SCREENS];
ZwmView views[MAX_VIEWS];

int zwm_view_get_screen(int view)
{
	return views[view].screen;
}

Bool zwm_view_mapped(int v)
{
	return (views[v].screen >= 0);
}

Bool zwm_view_has_clients(int v)
{
	Client *c;
	zwm_client_foreach(c){
		if(c->view == v){
			return True;
		}
	}
	return False;
}

void zwm_screen_set_view(int scr, int view)
{
	int i = zwm_view_get_screen(view);
	if (i == scr) {
		return;
	}

	if (i >= 0) {
		screen[i].prev = screen[i].view;
		screen[i].view = screen[scr].view;
	}
	screen[scr].prev = screen[scr].view;
	screen[scr].view = view;
	for(i=0;i<MAX_VIEWS;i++){
		views[i].screen = -1;
	}
	for(i=0;i<config.screen_count;i++){
		views[screen[i].view].screen = i;
	}
}

int zwm_client_screen(Client *c)
{
	int i;
	for(i=0;i<config.screen_count;i++){
		if(c->x >= screen[i].x && c->x <=screen[i].x+screen[i].w){
			if(c->y >= screen[i].y && c->y <=screen[i].y+screen[i].h){
				return i;
			}
		}
	}
	return i;
}

int zwm_current_screen()
{
	Window rr,ch;
	int x, y;
	int wx, wy;
	unsigned int mask;
	Window root = DefaultRootWindow(dpy);
	int i;

	Bool ret = XQueryPointer(dpy, root, &rr, &ch, &x, &y, &wx, &wy, &mask);
	if(ret){
		for(i=0;i<config.screen_count;i++){
			if(x > screen[i].x && x <screen[i].x+screen[i].w){
				if(y>=screen[i].y && y <= screen[i].y+screen[i].h){
					return i;
				}
			}
		}
	}

	if(sel){
		return zwm_view_get_screen(sel->view);
	}

	return 0;
}

int zwm_current_view()
{
	return screen[zwm_current_screen()].view;
}

void zwm_view_rescan(void)
{
	Client *c;
	int i = 0;
	int max_view = 0;
	int min_view = 32;
	int viewc[32] = { [0 ... 31] = 0,  };
	for(c = head; c ; c = c->next)
	{
		max_view = c->view > max_view ? c->view: max_view;
		min_view = c->view < min_view ? c->view: min_view;
		viewc[c->view]++;
	}
	config.num_views = max_view + 1;

	for(i=0; i<config.screen_count; i++){
		if(viewc[screen[i].view] <= 0){
			int j;
			for(j=0; j<config.num_views; j++){
				if(viewc[j]>0 && zwm_view_get_screen(j )== -1 ){
					screen[i].view = j;
					viewc[j] = -1;
					break;
				}
			}
		}
	}
	for(i=0;i<MAX_VIEWS;i++){
		views[i].screen = -1;
	}
	for(i=0;i<config.screen_count;i++){
		views[screen[i].view].screen = i;
	}

	zwm_layout_arrange();
}

void zwm_view_set(int  v)
{
	if (v < config.num_views && v != zwm_current_view()) {
		zwm_screen_set_view( zwm_current_screen(), v );
		zwm_layout_rearrange(True);
		zwm_client_refocus();
		zwm_event_emit(ZwmViewChange, (void*)v);
	}
}

void zwm_screen_rescan(Bool init) {
	config.screen_count = 0;
	int i;

	if (XineramaIsActive(dpy)) {
		int nscreen;
		XineramaScreenInfo * info = XineramaQueryScreens( dpy, &nscreen );
		ZWM_DEBUG("Xinerama is active\n");
		for( i = 0; i< nscreen; i++ ) {
			if( i == 0 || (info[i].x_org != info[i-1].x_org)) {
				ZWM_DEBUG( "Screen %d %d: (%d) %d+%d+%dx%d\n", i,config.screen_count,
				       	info[i].screen_number,
					info[i].x_org, info[i].y_org,
					info[i].width, info[i].height );

				screen[i].x = info[i].x_org;
				screen[i].y = info[i].y_org;
				screen[i].w = info[i].width;
				screen[i].h = info[i].height;
				config.screen_count++;
			}
		}
		XFree(info);
	} else {
		config.screen_count = 1;
		screen[0].x = 0;
		screen[0].y = 0;
		screen[0].w = DisplayWidth(dpy, scr);
		screen[0].h = DisplayHeight(dpy, scr);
		ZWM_DEBUG( "Screen %d %d: (%d) %d+%d+%dx%d\n", 0,1,
			       	0, screen[0].x,  screen[0].y,
				 screen[0].w , screen[0].h);
	} 

	if(init){
		for(i=0;i<config.screen_count;i++) {
			views[i].mwfact = config.mwfact;
			zwm_screen_set_view(i, i);
		}
	}
	zwm_event_emit(ZwmScreenSize, NULL);
	zwm_layout_dirty();
}


