#include <string.h>

ZwmConfig config = 
{
	.num_views = 1,
	.border_width = 1,
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
		{"◦", client_iconify},
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
		{"Alt-Delete", close_window, NULL},
		{"Alt-F11", toggle_panel, NULL},
		{"Alt-g", run_once, "zweb;zweb"},
		{"Alt-h", flip, "simshell"},
		{"Alt-j", focus, "0"},
		{"Alt-k", focus, "1"},
		{"Alt-f", focus, "2"},
		{"Alt-l", banish, NULL},
		{"Alt-m", run_once, "Mutt;st -t Mail -c Mutt -e mutt"},
		{"Alt-n", run_once, "News;st -c News -t News -e newsbeuter"},
		{"Alt-p", run_once, "zmenu;dwmenu"},
		{"Alt-Return", client_zoom, NULL},
		{"Alt-r", run_once, "Rox;rox"},
		{"Alt-Shift-1", set_view, "0"},
		{"Alt-Shift-2", set_view, "1"},
		{"Alt-Shift-3", set_view, "2"},
		{"Alt-Shift-4", set_view, "3"},
		{"Alt-Shift-5", set_view, "4"},
		{"Alt-Shift-6", set_view, "5"},
		{"Alt-Shift-m", run_once, "Mixer;st -c Mixer -t Mixer -e alsamixer"},
		{"Alt-Shift-Return", run_once, "Screen;st -t Screen -c Screen -e screen -Rd"},
		{"Alt-Shift-space", toggle_floating, NULL},
		{"Alt-space", zwm_layout_set, NULL},
		{"Alt-Tab", zwm_layout_cycle, NULL},
		{"Ctrl-Alt-l", zwm_util_spawn, "standby"},
		{"Ctrl-Alt-q", zwm_wm_quit, NULL},
		{"Ctrl-Alt-Return", iconify, NULL},
		{"Ctrl-Alt-r", zwm_wm_restart, NULL},
		{"Ctrl-Shift-Return", zwm_util_spawn, "simshell"},
		{NULL, NULL, NULL},
	},
};


