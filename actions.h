
#define ZVIEW 9

static int view_next(Bool occupied)
{
	int next = screen[zwm_current_screen()].prev;

	if (occupied) {
		if( !zwm_view_has_clients(next) ){
			next = (zwm_current_view() + 1) % (config.screen_count+1);
		} 
		return next;
	}

	if(next <0 || next == zwm_current_view() || next >= ZVIEW ){
		next = (zwm_current_view() + 1) % (config.screen_count+1);
	}

	return next;
}

static void set_view(const char *arg) {
	int v =  atoi(arg);
	if(sel) {
		zwm_client_set_view(sel, v);
		screen[zwm_current_screen()].prev = v;
	}
}

static void goto_view(const char *arg) {
	int v = atoi(arg);
	zwm_view_set(v);
}

static void banish(const char *arg) {

	int next = view_next(False);
	if(sel && sel->view < ZVIEW ){
		zwm_client_set_view(sel, next);
		screen[zwm_current_screen()].prev = next;
	}
}

static void flip(const char *arg) {
	zwm_view_set(view_next(True));
}

static void banish_non_class(const char *arg) {
	Client *c;
	int v = zwm_current_view();
	zwm_client_foreach(c) {
		if ( strcmp(c->cname, sel->cname) != 0 && zwm_client_visible(c, v)) {
			zwm_client_set_view(c, view_next(False));
		}
	}
	screen[zwm_current_screen()].prev = view_next(False);
}

static void banish_class(const char *arg) {
	Client *c;
	int v = zwm_current_view();
	zwm_client_foreach(c) {
		if ( strcmp(c->cname, sel->cname) == 0 && zwm_client_visible(c, v)) {
			zwm_client_set_view(c, view_next(False));
		}
	}
	screen[zwm_current_screen()].prev = view_next(False);
}

static void banish_all(const char *arg) {
	Client *c;
	int v = zwm_current_view();
	zwm_client_foreach(c) {
		if ( c != sel && zwm_client_visible(c, v)) {
			zwm_client_set_view(c, view_next(False));
		}
	}
	screen[zwm_current_screen()].prev = ((zwm_current_view()+1)%2);
}

static void show_all(const char *arg) {
	Client *c;
	Client *s = sel;
	int v = zwm_current_view();
	zwm_client_foreach(c) {
		zwm_client_set_view(c, v);
		zwm_client_setstate(c, NormalState);
	}
	zwm_client_raise(s, True);
}

static void zen(const char *arg) {
	int s = zwm_current_screen();
	int v = ZVIEW + s;
	Client *c = sel;

	if(!c){
		return;
	}

	if(c->view >= ZVIEW){
		c->view = c->fpos.view;
		zwm_view_set(c->view);
		zwm_client_raise(c, True);
		zwm_panel_show();
		return;
	}

	c->fpos.view = c->view;
	zwm_client_set_view(c, v);
	zwm_view_set(v);
	zwm_layout_set("zen");
	zwm_panel_hide();
}

static void screen_next(const char *arg) {
	int s = (zwm_current_screen()+1) % config.screen_count;
	if(s < config.screen_count) {
		XWarpPointer(dpy, None, root, 0,0,0,0, screen[s].x+100, screen[s].y+100);
	}
	zwm_client_refocus();
	zwm_layout_dirty();
}

static void warp_to_screen(const char *arg) {
	int s = atoi(arg);
	if(s < config.screen_count) {
		XWarpPointer(dpy, None, root, 0,0,0,0, screen[s].x+100, screen[s].y+100);
	}
		zwm_client_refocus();
	zwm_layout_dirty();
}


static void run_once(const char *arg){
	char data[1024];
	char *cmd;
	char *cls;
	Client *c = NULL;
	strcpy(data,arg);
	cls = strtok(data, ";");
	cmd = cls + strlen(cls) + 1;
	zwm_client_foreach(c) {
		if(strcasecmp(c->cname, cls) == 0) {
			zwm_client_zoom(c);
			return;
		}
	}
	zwm_client_foreach(c) {
		if(strcasestr(c->name, cls)) {
			zwm_client_zoom(c);
			return;
		}
	}
	zwm_util_spawn(cmd);
}

static void client_zoom(const char *arg) {
	Client *c = sel;
	if( c && c == head) {
		c = zwm_client_next_visible(c->next);
	}

	if(c) {
		zwm_client_zoom(c);
	}
}

static void close_window(const char *arg) {
	if (sel) {
		zwm_client_kill(sel);
	}
}

static void  toggle_floating(const char *arg) {
	if (sel) {
		zwm_client_toggle_floating(sel);
	}
}

static void toggle_panel(const char *arg) {
	zwm_panel_toggle();
}

static void do_focus(Client *c)
{
	if (c) {
		zwm_client_raise(c, False);
		zwm_client_warp(c);
	}
}

static inline Client * client_next(Client *c, int dir){
	Client *n = ((Client **)(&c->next))[dir];
	if (!n) {
		if (dir) {
			return tail;
		} else {
			return head;
		}
	} else {
		return n;
	}
}

static void focus(const char *arg) {
	int i = atoi(arg);
	if (!sel) {
		zwm_client_refocus();
		return;
	} else if (i < 2) {
		int view = zwm_current_view();
		Client *c = client_next(sel, i);
		for(; c && c != sel; c = client_next(c, i)){
			if(zwm_client_visible(c, view)){
				do_focus(c);
				return;
			}
		}
	} else {
		zwm_client_refocus();
		if(sel){
			do_focus(sel);
		}
	}
}

static void iconify(const char *args) {
	if (sel) {
		zwm_client_setstate(sel, IconicState);
	}
}

static void client_iconify(Client *c) {
	zwm_client_setstate(c, IconicState);
}

static void cycle(const char *arg) {
	Client *c, *next;

	if(!sel) {
		zwm_client_refocus();
		return;
	}

	c = zwm_client_next_visible(head);
	next = zwm_client_next_visible(c);
	if(next){
		zwm_event_emit(ZwmClientUnmap, c);
		zwm_client_remove(c);
		zwm_client_push_tail(c);
		zwm_client_warp(next);
		zwm_event_emit(ZwmClientMap, next);
		zwm_layout_dirty();
	}
}

static void fullscreen(const char *arg) {
	if(!sel){
		return;
	}

	if(sel->isfloating) {
		zwm_client_unfullscreen(sel);
	} else {
		zwm_client_fullscreen(sel);
	}

}
