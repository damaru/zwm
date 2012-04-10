#include "zwm.h"


#if 0
	static
Client *zwm_client_sel_next(void)
{
	Client *c;

	if(!sel) {
		return NULL;
	}

	for(c = zwm_client_next(sel);
		       	c ;
		       	c = zwm_client_next(c));
	if(!c) {
		for(c = zwm_client_head();
		  c ;
		  c = zwm_client_next(c));
	}
	return c;
}


static void
iconify(const char **arg, void *data) {
	if(sel)
	zwm_client_iconify(sel);
}

static void
setlayout(const char **arg, void *data) {
	zwm_layout_set(arg[0]);
}


static void
raise_next(const char **arg, void *data) {
	Client *c = zwm_client_sel_next();
	if(c) {
		zwm_client_raise(c);
		zwm_client_focus(c);
	}
	zwm_layout_arrange();
}

static void
raise_all(const char **arg, void *data) {
	Client *c;
	for(c = zwm_client_head();
		       	c ;
		       	c = zwm_client_next(c))
	{

		zwm_client_raise(c);
	}
	zwm_client_focus(NULL);
	zwm_layout_arrange();
}

static void
view_prev(const char **arg) {
	zwm_current_view( (current_view - 1) %  num_views );
}

static void
view_next(const char **arg) {
	zwm_current_view( (current_view + 1) %  num_views );
}

static void
view(const char **arg) {
	int v = atoi(arg[0]);
	zwm_current_view(v);
}

static void
set_view(const char **arg) {
	int v =  atoi(arg[0]);
	if(sel) {
		zwm_client_set_view(sel, v);
	}
}

void
zwm_quit(const char **arg) {
	exit(0);
}

static void
togglefloating(const char **arg) {
	if(!sel)
		return;

	zwm_client_toggle_floating(sel);
}

static void
moveresizeto(const char **argv, void *data) {
	int x,y,w,h;
	if(!sel)
		return;

	x = atoi(argv[0]);
	y = atoi(argv[1]);
	w = atoi(argv[2]);
	h = atoi(argv[3]);

	sel->isfloating = True;
	zwm_client_moveresize(sel, x, y, w, h);
	zwm_client_warp(sel);
}

static void
moveresize(const char **argv, void *data) {
	int x,y,w,h;
	if(!sel)
		return;

	x = atoi(argv[0]);
	y = atoi(argv[1]);
	w = atoi(argv[2]);
	h = atoi(argv[3]);

	sel->isfloating = True;
	zwm_client_moveresize(sel, sel->x+x, sel->y+y,
		       	sel->w+w, sel->h+h);
	zwm_client_warp(sel);
}

int showing_grid = 0;

void
keyrelease(XEvent *e)
{
	if(showing_grid){
		Client *c = sel;
		showing_grid = 0;
		if(c){
			zwm_client_remove(c);
			zwm_layout_set("max");
			zwm_client_push_head(c);
			zwm_layout_arrange();
		}
		zwm_layout_set("max");
		zwm_layout_arrange();
	}
}

static void
showgrid(const char **arg, void *data) {
	if(!showing_grid){
		zwm_layout_set("grid");
		showing_grid = 1;
	}

}

static void
focusmaster(const char **arg, void *data) {
	Client *c;

	if(!sel) {
		zwm_client_focus(NULL);
		return;
	}

	c = zwm_client_next_visible(zwm_client_head());
	if(c && c != sel) {
		zwm_client_remove(sel);
		zwm_client_insert_after(c,sel);
		zwm_client_raise(c);
		zwm_client_focus(c);
		zwm_client_warp(c);
	}
}
#endif


void
zwm_exec(const char *arg) {
	static char *shell = NULL;

	if(!shell && !(shell = getenv("SHELL")))
		shell = "/bin/sh";
	if(!arg)
		return;
	/* The double-fork construct avoids zombie processes and keeps the code
	 * clean from stupid signal handlers. */
	if(dpy)
		close(ConnectionNumber(dpy));
	setsid();
	execlp( shell, arg, (char *)NULL);
	fprintf(stderr, "zwm: execl '%s -c %s'", shell, arg);
	perror(" failed");
}


