
#include "zwm.h"
#include <stdint.h>
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
	zwm_client_foreach(c) {
		if(c->view == v) {
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

	for (i=0; i<MAX_VIEWS; i++) {
		views[i].screen = -1;
	}

	for (i=0; i<config.screen_count; i++) {
		views[screen[i].view].screen = i;
	}
}

static int
screen_coords (int x, int y) {
	int i = 0;
	for (i=0; i<config.screen_count; i++) {
		if (x >= screen[i].x && x <= screen[i].x+screen[i].w) {
			if (y >= screen[i].y && y <= screen[i].y+screen[i].h) {
				return i;
			}
		}
	}
	return i;
}

int zwm_client_screen(Client *c)
{
	return screen_coords(c->x, c->y);
}

int zwm_current_screen()
{
	Window rr,ch;
	int x, y;
	int wx, wy;
	unsigned int mask;
	Window root = DefaultRootWindow(dpy);

	Bool ret = XQueryPointer(dpy, root, &rr, &ch, &x, &y, &wx, &wy, &mask);
	if (ret) {
		return screen_coords(x, y);
	}

	if (sel) {
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

	zwm_client_foreach (c) 
	{
		max_view = c->view > max_view ? c->view: max_view;
		min_view = c->view < min_view ? c->view: min_view;
		viewc[c->view]++;
	}

	config.num_views = max_view + 1;

	for (i=0; i<config.screen_count; i++) {
		if (viewc[screen[i].view] <= 0) {
			int j;
			for (j=0; j<config.num_views; j++) {
				if (viewc[j]>0 && zwm_view_get_screen(j )== -1 ) {
					screen[i].view = j;
					viewc[j] = -1;
					break;
				}
			}
		}
	}

	for (i=0; i<MAX_VIEWS; i++) {
		views[i].screen = -1;
	}

	for (i=0; i<config.screen_count;i++) {
		views[screen[i].view].screen = i;
	}

	zwm_layout_arrange();
}

void zwm_view_set(int  v)
{
	uint64_t v64 = v;
	if (v < config.num_views && v != zwm_current_view()) {
		zwm_screen_set_view( zwm_current_screen(), v );
		zwm_layout_rearrange(True);
		zwm_client_refocus();
#if __x86_64__
		zwm_event_emit(ZwmViewChange, (void*)v64);
#else
		zwm_event_emit(ZwmViewChange, (void*)v);
#endif
	}
}

void zwm_screen_rescan(Bool init) {
	config.screen_count = 0;
	int i;

	if (XineramaIsActive(dpy)) {
		int nscreen;
		XineramaScreenInfo * info = XineramaQueryScreens( dpy, &nscreen );
		for (i = 0; i< nscreen; i++ ) {
			if( i == 0 || (info[i].x_org != info[i-1].x_org)) {
				screen[i].x = info[i].x_org;
				screen[i].y = info[i].y_org;
				screen[i].w = info[i].width;
				screen[i].h = info[i].height;
				fprintf(stderr,"%d %d %d\n",i, screen[i].w,screen[i].h);
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

		fprintf(stderr,"%d %d\n",screen[0].w,screen[0].h);
	} 

	if (init) {
		for(i=0; i<config.screen_count; i++) {
			zwm_screen_set_view(i, i);
		}

		for(i=0; i<MAX_VIEWS; i++){
			views[i].mwfact = config.mwfact;
			views[i].layout = config.layouts;
		}
	}

	zwm_event_emit(ZwmScreenSize, NULL);
	zwm_layout_dirty();
}


