#include <string.h>

static void
set_view(const char *arg) {
	int v =  atoi(arg);
	if(sel) {
		zwm_client_set_view(sel, v);
	}
}

static void
goto_view(const char *arg) {
	int v = atoi(arg);
	zwm_view_set(v);
}


static void 
banish(const char *arg)
{
	if(sel){
		zwm_client_set_view(sel, ((zwm_current_view()+1)%2));
	}
}

static void 
flip(const char *arg)
{
	if(sel){
		int next = (zwm_current_view() + 1) % 2;
		zwm_view_set(next);
	}
}

static void 
zwm_runonce(const char *arg){
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

static void 
client_zoom(const char *arg) {
	Client *c = sel;
	if( c && c == zwm_client_head()) {
		c = zwm_client_next_visible(zwm_client_next(c));
	}

	if(c) {
		zwm_client_zoom(c);
	}
}


ZwmConfig config = 
{
	.num_views = 1,
	.border_width = 1,
	.screen_count = 1,
	.screen_x = 0,
	.screen_y = 0,
	.screen_w = 0,
	.screen_h = 0,
	.focus_border_color = "#AAA",
	.focus_bg_color = "#7B7B7B",
	.focus_shadow_color = "#BBB",
	.normal_border_color = "#AAA",
	.normal_bg_color = "#222222",
	.normal_shadow_color = "#000",
	.opacity = 0.95,
	.anim_steps = 50,
	.show_title = 1,
	.font = "-*-dejavu sans mono-bold-r-*-*-14-*-*-*-*-*-*-*",
 	.icons = "-*-webdings-bold-r-*-*-16-*-*-*-*-*-*-*",
	.title_height = 20,
	.reparent = 1,
	.buttons = {
		{"◦", zwm_client_iconify},
		{"✖", zwm_client_kill},
		{ NULL, NULL}
	},
	.viewnames = {
		[0] = "❶",
		[1] = "❷",
		[2] = "❸",
		[3] = "❹",
		[4] = "5",
		[5] = "6",
		[6] = "7",
		[7] = "8",
		[8] = "9",
		[9] = "10",
	},
	.keys = {
		{"Alt-1", goto_view, "0"},
		{"Alt-2", goto_view, "1"},
		{"Alt-3", goto_view, "2"},
		{"Alt-4", goto_view, "3"},
		{"Alt-5", goto_view, "4"},
		{"Alt-6", goto_view, "5"},
		{"Alt-Delete", zwm_client_kill, NULL},
		{"Alt-F11", zwm_panel_toggle, NULL},
		{"Alt-g", zwm_runonce, "zweb;zweb"},
		{"Alt-h", flip, "simshell"},
		{"Alt-j", zwm_focus_next, NULL},
		{"Alt-k", zwm_focus_prev, NULL},
		{"Alt-l", banish, NULL},
		{"Alt-m", zwm_runonce, "Mutt;st -t Mail -c Mutt -e mutt"},
		{"Alt-n", zwm_runonce, "News;st -c News -t News -e newsbeuter"},
		{"Alt-p", zwm_runonce, "zmenu;dwmenu"},
		{"Alt-Return", client_zoom, NULL},
		{"Alt-r", zwm_runonce, "Rox;rox"},
		{"Alt-Shift-1", set_view, "0"},
		{"Alt-Shift-2", set_view, "1"},
		{"Alt-Shift-3", set_view, "2"},
		{"Alt-Shift-4", set_view, "3"},
		{"Alt-Shift-5", set_view, "4"},
		{"Alt-Shift-6", set_view, "5"},
		{"Alt-Shift-m", zwm_runonce, "Mixer;st -c Mixer -t Mixer -e alsamixer"},
		{"Alt-Shift-Return", zwm_runonce, "Screen;st -t Screen -c Screen -e screen -Rd"},
		{"Alt-Shift-space", zwm_client_toggle_floating, NULL},
		{"Alt-space", zwm_layout_set, NULL},
		{"Alt-Tab", zwm_layout_cycle, NULL},
		{"Ctrl-Alt-l", zwm_util_spawn, "standby"},
		{"Ctrl-Alt-q", zwm_wm_quit, NULL},
		{"Ctrl-Alt-Return", zwm_client_iconify, NULL},
		{"Ctrl-Alt-r", zwm_wm_restart, NULL},
		{"Ctrl-Shift-Return", zwm_util_spawn, "simshell"},
		{NULL, NULL, NULL},
	},
};


