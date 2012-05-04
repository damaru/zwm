
#define ZVIEW 10 

static int view_next(Bool occupied)
{
	int next = screen[zwm_current_screen()].prev;

	if(occupied && zwm_view_has_clients(next) == False){
		next = 0;
		while(next < MAX_VIEWS && 
			(zwm_view_has_clients(next) == False || 
			 zwm_view_mapped(next))) {
			next++;
		}
	} else if(next == screen[zwm_current_screen()].view || next >= ZVIEW ){
		next = (zwm_current_view() + 1) % config.screen_count;
	}

	return next;
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
		zwm_event_emit(ZenClientUnmap, c);
		zwm_client_remove(c);
		zwm_client_push_tail(c);
		zwm_client_warp(next);
		zwm_event_emit(ZenClientMap, next);
		zwm_layout_dirty();
	}
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
	int next = view_next(True);
	zwm_view_set(screen[zwm_current_screen()].prev);
}

static void banish_non_class(const char *arg) {
	Client *c;
	int v = zwm_current_view();
	for(c = head; c; c = c->next) {
		if ( strcmp(c->cname, sel->cname) != 0 && zwm_client_visible(c, v)) {
			zwm_client_set_view(c, view_next(False));
		}
	}
	screen[zwm_current_screen()].prev = view_next(False);
}

static void banish_class(const char *arg) {
	Client *c;
	int v = zwm_current_view();
	for(c = head; c; c = c->next) {
		if ( strcmp(c->cname, sel->cname) == 0 && zwm_client_visible(c, v)) {
			zwm_client_set_view(c, view_next(False));
		}
	}
	screen[zwm_current_screen()].prev = view_next(False);
}

static void banish_all(const char *arg) {
	Client *c;
	int v = zwm_current_view();
	for(c = head; c; c = c->next) {
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
	for(c = head; c; c = c->next) {
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
		zwm_panel_show();
		zwm_view_set(c->view);
		zwm_client_raise(c, True);
		return;
	}

	c->fpos.view = c->view;
	zwm_client_set_view(c, v);
	zwm_panel_hide();
	zwm_view_set(v);
	zwm_layout_set("zen");
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
	char *class;
	Client *c = NULL;
	strcpy(data,arg);
	class = strtok(data, ";");
	cmd = class + strlen(class) + 1;
	for(c=head; c; c = c->next) {
		if(strcasecmp(class, c->cname) == 0) {
			ZWM_DEBUG("found CLASS %s, %s, for %s\n",c->cname, c->name, cmd);
			zwm_client_zoom(c);
			zwm_client_raise(c, True);
			return;
		}
	}
	for(c=head; c; c = c->next) {
		if(strcasestr(c->name, class)) {
			ZWM_DEBUG("found NAME %s for %s\n",c->name, cmd);
			zwm_client_zoom(c);
			zwm_client_raise(c, True);
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

