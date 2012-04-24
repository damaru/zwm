#include "zwm.h"

static Client *panel = NULL;
static int oldy;

static void map(Client *c, void *priv)
{
	if(c->type == ZenDockWindow && !panel)
	{
		panel = c;
		zwm_update_screen_geometry(False);
	}
}

static void configure(Client *c, void *priv)
{
	if(c == panel)
	{
		zwm_update_screen_geometry(False);
	}
}

static void unmap(Client *c, void *priv)
{
	if(c == panel)
	{
		panel = NULL;
		zwm_update_screen_geometry(False);
	}
}

static void rescreen(const char *layout, void *priv)
{
	int i;
	XWindowAttributes wa;
	if(panel) {
		XGetWindowAttributes(dpy, panel->win, &wa);
		panel->x = wa.x;
		panel->y = wa.y;
		panel->w = wa.width;
		panel->h = wa.height;
		for(i = 0; i< screen_count; i++){
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

void zwm_panel_toggle(const char *args)
{
	if(panel){
		if(panel->y >= 0 && panel->y <= (screen[0].y + screen[0].h))
		{
			oldy = panel->y;
			zwm_client_moveresize(panel, panel->x, 10000, panel->w, panel->h);
		} else {
			zwm_client_moveresize(panel, panel->x, oldy, panel->w, panel->h);
		}
		zwm_update_screen_geometry(False);
		zwm_layout_dirty();
	}
}

void zwm_panel_init(void)
{
	zwm_event_register(ZenClientUnmap, (ZenEFunc)unmap, NULL);
	zwm_event_register(ZenClientConfigure, (ZenEFunc)configure, NULL);
	zwm_event_register(ZenClientMap, (ZenEFunc)map, NULL);
	zwm_event_register(ZenScreenSize, (ZenEFunc)rescreen, NULL);
}

