#include "zwm.h"

static Client *panel = NULL;
static void panel_map(Client *c, void *priv);
static void panel_configure(Client *c, void *priv);
static void panel_unmap(Client *c, void *priv);
static void panel_rescan(const char *layout, void *priv);
static int oldy;

void zwm_panel_init(void)
{
	zwm_event_register(ZwmClientUnmap, (ZwmEFunc)panel_unmap, NULL);
	zwm_event_register(ZwmClientConfigure, (ZwmEFunc)panel_configure, NULL);
	zwm_event_register(ZwmClientMap, (ZwmEFunc)panel_map, NULL);
	zwm_event_register(ZwmScreenSize, (ZwmEFunc)panel_rescan, NULL);
}

void zwm_panel_hide(void)
{
	if(panel){
		if(panel->y >= 0 && panel->y <= (screen[0].y + screen[0].h))
		{
			oldy = panel->y;
			zwm_client_moveresize(panel, panel->x, 10000, panel->w, panel->h);
		}
		zwm_screen_rescan(False);
		zwm_layout_dirty();
	}
}

void zwm_panel_show(void)
{
	if(panel){
		zwm_client_moveresize(panel, panel->x, oldy, panel->w, panel->h);
		zwm_screen_rescan(False);
//		zwm_layout_dirty();
	}
}

void zwm_panel_toggle(void)
{
	if(panel){
		if(panel->y >= 0 && panel->y <= (screen[0].y + screen[0].h))
		{
			oldy = panel->y;
			zwm_client_moveresize(panel, panel->x, 10000, panel->w, panel->h);
		} else {
			zwm_client_moveresize(panel, panel->x, oldy, panel->w, panel->h);
		}
		zwm_screen_rescan(False);
		zwm_layout_dirty();
	}
}

static void panel_map(Client *c, void *priv) {
	if(c->type == ZwmDockWindow && !panel)
	{
		panel = c;
		zwm_screen_rescan(False);
	}
}

static void panel_configure(Client *c, void *priv) {
	if(c == panel)
	{
		zwm_screen_rescan(False);
	}
}

static void panel_unmap(Client *c, void *priv) {
	if(c == panel)
	{
		panel = NULL;
		zwm_screen_rescan(False);
	}
}

static void panel_rescan(const char *layout, void *priv) {
	int i;
	XWindowAttributes wa;
	if(panel) {
		XGetWindowAttributes(dpy, panel->win, &wa);
		panel->x = wa.x;
		panel->y = wa.y;
		panel->w = wa.width;
		panel->h = wa.height;
		for(i = 0; i< config.screen_count; i++){
			if( panel->h == screen[i].h)
			{
				if(panel->x+panel->w < screen[i].w/2) {
					screen[i].x = panel->x + panel->w;
				}
				screen[i].w -= panel->w;
			} else {
				if(panel->y < screen[i].h/2)
				{
					screen[i].y = panel->y + panel->h;
				}

				if(panel->y+panel->h >= screen[0].y && panel->y <= screen[0].h) 
					screen[i].h -=  panel->h;
			}
		}
	} 
	zwm_layout_arrange();
}
