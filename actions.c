static void set_view(const char *arg) {
	int v =  atoi(arg);
	if(sel) {
		zwm_client_set_view(sel, v);
	}
}

static void goto_view(const char *arg) {
	int v = atoi(arg);
	zwm_view_set(v);
}


static void banish(const char *arg) {
	if(sel){
		zwm_client_set_view(sel, ((zwm_current_view()+1)%2));
	}
}

static void flip(const char *arg) {
	if(sel){
		int next = (zwm_current_view() + 1) % 2;
		zwm_view_set(next);
	}
}

static void run_once(const char *arg){
	char data[1024];
	char *cmd;
	char *class;
	Client *c = NULL;
	strcpy(data,arg);
	class = strtok(data, ";");
	cmd = class + strlen(class) + 1;
	for(c=zwm_client_head();
			c;
			c = zwm_client_next(c)) {
		if(strcasecmp(class, c->cname) == 0) {
			ZWM_DEBUG("found CLASS %s, %s, for %s\n",c->cname, c->name, cmd);
			zwm_client_zoom(c);
			zwm_client_raise(c, True);
			return;
		}
	}
	for(c=zwm_client_head();
			c;
			c = zwm_client_next(c)) {
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
	if( c && c == zwm_client_head()) {
		c = zwm_client_next_visible(zwm_client_next(c));
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
		zwm_client_raise(c, True);
	}
}

static inline Client * client_next(Client *c, int dir){
	return ((Client **)(&c->node))[dir];
}

static void focus(const char *arg) {
	int i = atoi(arg);
	if (!sel) {
		zwm_client_refocus();
		return;
	} else if (i < 2) {
		int view = zwm_current_view();
		Client *c = client_next(sel, i);
		if (!c) {
			zwm_client_refocus();
			return;
		}
		for(; c; c = client_next(c, i)){
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

