void set_view(const char *arg) {
	int v =  atoi(arg);
	if(sel) {
		zwm_client_set_view(sel, v);
	}
}

void goto_view(const char *arg) {
	int v = atoi(arg);
	zwm_view_set(v);
}


void banish(const char *arg) {
	if(sel){
		zwm_client_set_view(sel, ((zwm_current_view()+1)%2));
	}
}

void flip(const char *arg) {
	if(sel){
		int next = (zwm_current_view() + 1) % 2;
		zwm_view_set(next);
	}
}

void zwm_runonce(const char *arg){
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
			zwm_client_raise(c);
			return;
		}
	}
	for(c=zwm_client_head();
			c;
			c = zwm_client_next(c)) {
		if(strcasestr(c->name, class)) {
			ZWM_DEBUG("found NAME %s for %s\n",c->name, cmd);
			zwm_client_zoom(c);
			zwm_client_raise(c);
			return;
		}
	}
	zwm_util_spawn(cmd);
}

void client_zoom(const char *arg) {
	Client *c = sel;
	if( c && c == zwm_client_head()) {
		c = zwm_client_next_visible(zwm_client_next(c));
	}

	if(c) {
		zwm_client_zoom(c);
	}
}


