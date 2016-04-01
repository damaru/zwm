
static inline Client * client_next(Client *c, int dir, Bool wrap, Bool inc, Bool skipf){
#define NOT_FLOAT(C) (skipf?!(C)->isfloating:1) 
	int view = zwm_current_view();

	if (inc && zwm_client_visible(c, view) && NOT_FLOAT(c))
		return c;

	while(c) {
		Client *n = ((Client **)(&c->next))[dir];

		if (!n && wrap) {
			if (dir) {
				n = tail;
			} else {
				n = head;
			}
			wrap = 0;
		}

		if(zwm_client_visible(n, view) && NOT_FLOAT(n) ){
			return n;
		}
		c = n;
	}
	return NULL;
}

static int view_next(Bool occupied)
{
	int next = screen[zwm_current_screen()].prev;

	if (occupied) {
		if( !zwm_view_has_clients(next) ){
			next = (zwm_current_view() + 1) % (config.screen_count+1);
		} 
		return next;
	}

	if(next <0 || next == zwm_current_view() || next >= ZWM_ZEN_VIEW ){
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
	if(sel && sel->view < ZWM_ZEN_VIEW ){
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

void zwm_zen(const char *arg) {
	int s = zwm_current_screen();
	int v = ZWM_ZEN_VIEW + s;
	Client *c = sel;
	Client *t;

	if(!c){
		return;
	}

	if(c->view >= ZWM_ZEN_VIEW){
		c->view = screen[s].prev;
		zwm_view_set(c->view);
		zwm_client_raise(c, True);
		zwm_panel_show();
		return;
	}

	zwm_client_foreach(t) {
		if(t->view >= ZWM_ZEN_VIEW){
			t->view = t->fpos.view;
		}
	}

	c->fpos.view = c->view;
	views[v].current = c;
	zwm_client_set_view(c, v);
	zwm_view_set(v);
	zwm_layout_set("zen");
	zwm_panel_hide();
}

static void view_set_next(const char *arg) {
	int n = zwm_current_view() ;
	if(n+1 < config.num_views){
		zwm_view_set(n+1);
	} else
		zwm_view_set(0);
	
}

static void screen_next(const char *arg) {

	if(config.screen_count == 1) {
		view_set_next(arg);
	}

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


static void pull(const char *arg){
	char data[1024];
	char *cmd;
	char *cls;
	Client *c = NULL;
	strcpy(data,arg);
	zwm_client_foreach(c) {
		if(strcasecmp(c->cname, data) == 0) {
			zwm_client_zoom(c);
			return;
		}
	}
	zwm_client_foreach(c) {
		if(strcasestr(c->name, data)) {
			zwm_client_zoom(c);
			return;
		}
	}
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

static void toggle(const char *arg){
	char data[1024];
	char *cls;
	strcpy(data,arg);
	cls = strtok(data, ";");
	if(sel && (strcasecmp(sel->cname, cls) == 0 || strcasestr(sel->name, cls)) ){
		banish(arg);
	} else {
		run_once(arg);
	}
}

static void client_zoom(const char *arg) {
	Client *c = sel;
	if( c && c == head) {
		c = client_next(c, 0, True, False, True);
	}

	if(c) {
		zwm_client_zoom(c);
	}
}

static void move_next(const char *arg) {
	if(!sel){
		return;
	}
	Client *c = sel;
	Client *next = client_next(c, 0, False, False, True);
	if (next) {
		zwm_client_remove(c);
		zwm_client_push_next(c, next);
		zwm_layout_dirty();
		zwm_client_raise(c, True);
	} else {
		zwm_client_zoom(c);
	}
}

static void move_prev(const char *arg) {
	if(!sel){
		return;
	}
	Client *c = sel;
	Client *prev = client_next(c, 1, False, False, True);
	if (prev) {
		zwm_client_remove(c);
		if(prev->prev) {
			zwm_client_push_next(c, prev->prev);
			
		} else {
			zwm_client_push_head(c);
		}
	} else {
		zwm_client_remove(c);
		zwm_client_push_tail(c);
	}
	zwm_layout_dirty();
	zwm_client_raise(c, True);
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

static void focus(const char *arg) {
	int i = atoi(arg);
	if (!sel) {
		zwm_client_refocus();
		return;
	} else if (i < 2) {
		Client *c = client_next(sel, i, True, False, False);
		if(c) 
			do_focus(c);
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

void zwm_cycle(const char *arg) {
	Client *c, *next;

	if(!sel) {
		zwm_client_refocus();
		return;
	}

	c = client_next(head, 0, False, True, True);
	next = client_next(c, 0, False, False, True);
	if(next){
		zwm_event_emit(ZwmClientUnmap, c);
		zwm_client_remove(c);
		zwm_client_push_tail(c);
		zwm_event_emit(ZwmClientMap, next);
		zwm_layout_arrange();
	}
	focus("0");
}


void zwm_cycle2(const char *arg) {
	Client *c, *next;

	if(!sel) {
		zwm_client_refocus();
		return;
	}

	c = tail;
	next = head;
	if(c != next){
		zwm_event_emit(ZwmClientUnmap, c);
		zwm_client_remove(c);
		zwm_client_push_head(c);
		zwm_event_emit(ZwmClientMap, c);
		zwm_layout_arrange();
	}
}

static void fullscreen(const char *arg) {
	if(!sel){
		return;
	}

	if (sel->isfloating) {
		zwm_client_unfullscreen(sel);
	} else {
		zwm_client_fullscreen(sel);
	}

}

static void mwfact(const char *arg) {
	views[zwm_current_view()].mwfact += atof(arg);
	zwm_layout_dirty();
}

extern int super_on;

static void zwm_util_unfollow(const char *arg){
	super_on = 0;
	Client *c;
	for(c = head; c; c = c->next) {
		zwm_decor_update(c);
	}
}

static void zwm_util_follow(const char *arg){
	super_on = 1;
	KeySym keysym;
	Client *c;
	Client *n = NULL;
	int v = zwm_current_view();
	int count = 'a';

	if(!sel){
		return;
	}

	for(c = head; c; c = c->next) {
		c->oldpos.view = c->view;
		c->view = v;
		sprintf(c->key,"%c",count++);
	}

	zwm_layout_push("grid");
	zwm_layout_rearrange(1 );

	for(c = head; c; c = c->next) {
		zwm_decor_update(c);
	}

	keysym = zwm_getkey();
	count = XKeysymToString(keysym)[0];
	super_on = 0;

	for(c = head; c; c = c->next) {
		zwm_client_set_view(c, c->oldpos.view);
		if(c->key[0] == count){
			n = c;
		}
		zwm_decor_update(c);
	}

	zwm_layout_pop( );
	if(count >= '0' && count <= '9'){
		zwm_view_set(count-'0'-1);
	}
	if(n){
		zwm_view_set(n->oldpos.view);
	}
	if(n){
		zwm_client_zoom(n);
	}
}
