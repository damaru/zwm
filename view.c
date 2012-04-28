
#include "zwm.h"

static unsigned long views = 0;

int zwm_view_get_screen(int view)
{
	int i;
	for(i = 0; i<screen_count; i++){
		if(screen[i].view == view){
			return i;
		}
	}
	return -1;
}

void zwm_screen_set_view(int scr, int view)
{
	int i = zwm_view_get_screen(view);
	if (i == scr) {
		return;
	}

	if (i >= 0) {
		screen[i].view = screen[scr].view;
	}
	screen[scr].view = view;
	views = 0;
	for(i=0;i<screen_count;i++)
		views |= 1<<(screen[i].view);
}

Bool zwm_view_mapped(int v)
{
	return views&(1<<v)?True:False;
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
		for(i=0;i<screen_count;i++){
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

void zwm_auto_view(void)
{
	Client *c;
	int i = 0;
	int max_view = 0;
	int min_view = 32;
	int viewc[32] = { [0 ... 31] = 0,  };
	for(c = zwm_client_head();
			c ;
			c = zwm_client_next(c))
	{
		max_view = c->view > max_view ? c->view: max_view;
		min_view = c->view < min_view ? c->view: min_view;
		viewc[c->view]++;
	}
	config.num_views = max_view + 1;


	for(i=0; i<screen_count; i++){
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
	views = 0;
	for(i=0;i<screen_count;i++)
		views |= 1<<(screen[i].view);
	zwm_layout_arrange();
}

void zwm_view_set(int  v)
{
	if (v < config.num_views && v != zwm_current_view()) {
		zwm_screen_set_view( zwm_current_screen(), v );
		zwm_layout_arrange();
		zwm_client_refocus();
		zwm_event_emit(ZenView, (void*)v);
	}
}

