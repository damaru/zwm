#include <string.h>

#define FOCUS_FG "#F0F0F0"
#define FOCUS_BG "#5F87D7"
#define SHADOW "#444"
#define FG  "#C9C9C9"
#define BG  "#5C5C5C"
#define FLOAT_BG "#5e74a1"
#define ROOT_BG "#FF0000"

#if 0
static inline void zwm_mail(Client *c) { run_once("zmail;zmail"); }
static inline void zwm_menu(Client *c) { run_once("zmenu;dwmenu"); }
static inline void zwm_browser(Client *c) { run_once("zweb;zweb"); }
#endif

ZwmConfig config = 
{
	.num_views = 1,
	.border_width = 1,
	.attach_last = 1,
	.focus_border_color = FOCUS_BG,
	.focus_bg_color = FOCUS_BG,
	.focus_shadow_color = SHADOW,
	.focus_title_color = FOCUS_FG,
	.normal_border_color = BG,
	.normal_bg_color = BG,
	.normal_shadow_color = SHADOW,
	.normal_title_color = FG,
	.float_bg_color = FLOAT_BG,
	.root_bg_color = ROOT_BG,
	.date_fmt = "%a %d %b, %l:%M %p ",
	.menucmd = "dwmenu",
	.opacity = 0.85,
	.anim_steps = 30,
	.show_title = 1,
	.font = "-*-dejavu sans mono-bold-r-*-*-14-*-*-*-*-*-*-*",
 	.icons = "-*-webdings-bold-r-*-*-16-*-*-*-*-*-*-*",
	.title_height = 20,
	.minh = 200,
	.minw = 400,
	.mwfact = 0.5,
	.zen_wallpaper = 1,
	.sleep_time = 10,
	.buttons = {
#if 0
		{"✇",  zwm_browser},
		{"✉",  zwm_mail},
		{"❤",  zwm_menu},
		{"▭", client_iconify},
		{"⇔", flip},
		{"↪",  banish},
		{"□",  banish_all},
#endif
		{"✖", zwm_client_kill},
		{ NULL, NULL}
	},
	.viewnames = {
		[0] = "➊",
		[1] = "➋",
		[2] = "➌",
		[3] = "❹",
		[4] = "➎",
		[5] = "➏",
		[6] = "➐",
		[7] = "➑",
		[8] = "➒",
		[9] = "●",
		[10] = "➀",
		[11] = "➁",
		[12] = "➂",
		[13] = "➃",
	},
	.keys = {
		{"Super-Tab", screen_next, ""},
		{"Super-j", screen_next, ""},
		{"Ctrl-Alt-j", screen_next, ""},

		{"Super-1", warp_to_screen, "0"},
		{"Super-2", warp_to_screen, "1"},
		{"Super-3", warp_to_screen, "2"},

		{"Alt-1", goto_view, "0"},
		{"Alt-2", goto_view, "1"},
		{"Alt-3", goto_view, "2"},
		{"Alt-4", goto_view, "3"},
		{"Alt-5", goto_view, "4"},
		{"Alt-6", goto_view, "5"},
		{"Alt-7", goto_view, "6"},
		{"Alt-8", goto_view, "7"},
		{"Alt-9", goto_view, "8"},
		{"Alt-0", goto_view, "9"},
		{"Alt-h", flip, NULL},

		{"Alt-l", banish, NULL},
		{"Alt-b", banish_all, NULL},
		{"Alt-Shift-b", banish_non_class, NULL},
		{"Alt-c", banish_class, NULL},
		{"Alt-Shift-1", set_view, "0"},
		{"Alt-Shift-2", set_view, "1"},
		{"Alt-Shift-3", set_view, "2"},
		{"Alt-Shift-4", set_view, "3"},
		{"Alt-Shift-5", set_view, "4"},
		{"Alt-Shift-6", set_view, "5"},
		{"Alt-Shift-7", set_view, "6"},
		{"Alt-Shift-8", set_view, "7"},
		{"Alt-Shift-9", set_view, "8"},
		{"Alt-Shift-0", set_view, "9"},

		{"Alt-Delete", close_window, NULL},

		{"Alt-j", focus, "0"},
		{"Alt-k", focus, "1"},
		{"Alt-F11", fullscreen, NULL},
		{"Alt-F10", toggle_panel, NULL},

		{"Alt-Shift-Up", mwfact, "0.05"},
		{"Alt-Shift-Down", mwfact, "-0.05"},

		{"Alt-Return", zwm_zen, NULL},
		{"Alt-Shift-r", show_all, NULL},

		{"Alt-Shift-space", toggle_floating, NULL},
		{"Alt-space", zwm_layout_set, NULL},
		{"Alt-Shift-h", client_zoom, NULL},
		{"Alt-Shift-j", move_next, NULL},
		{"Alt-Shift-k", move_prev, NULL},
		{"Alt-Tab", zwm_cycle, NULL},
		{"Alt-Shift-Tab", zwm_cycle2, NULL},
		{"Ctrl-Alt-l", zwm_util_spawn, "standby"},
		{"Ctrl-Alt-q", zwm_wm_quit, NULL},
		{"Ctrl-Alt-Return", iconify, NULL},
		{"Ctrl-Alt-r", zwm_wm_restart, NULL},
		{"Ctrl-Shift-Alt-r", zwm_wm_fallback, "rewm"},
		{"Ctrl-Shift-Return", zwm_util_spawn, "simshell"},

		{"Alt-g", run_once, "zweb;zweb"},
		{"Alt-n", run_once, "news;xterm -class news -e newsbeuter"},
		{"Alt-p", run_once, "zmenu;dwmenu"},
		{"Alt-r", run_once, "Ranger;xterm -T Ranger -class Ranger -tn xterm-256color -e ranger"},
		{"Alt-m", run_once, "zmail;zmail"},
		{"Alt-Shift-v", run_once, "Mixer;xterm -T Mixer -class Mixer -tn xterm-256color -e alsamixer"},
		{"Alt-Shift-m", run_once, "Mutt;xterm -T Mail -class Mutt -tn xterm-256color -e mutt"},
		{"Alt-Shift-Return", run_once, "Screen;xterm -title Screen -class Screen -e screen -Rd"},
		{"Alt-x", zwm_util_spawn, "xterm"},
		{"Alt-f", zwm_util_follow, "xterm"},
		{"Super_L", zwm_util_follow, "xterm"},

		{NULL, NULL, NULL},
	},

	.relkeys = {
		{"Alt-f", zwm_util_unfollow, "xterm"},
		{NULL, NULL, NULL},
	},

	.layouts = {
		{ tile, "tile", 0, "▤" },
		{ monocle, "monocle", 0, "□" },
		{ floating, "float", 1, "▭" },
		{ zen, "zen", 1, "◉" },
		{ grid, "grid", 0, "▦" },
		{ l_fullscreen, "fullscreen", 1, "■" },
		{NULL, NULL, 0},
	},
};


