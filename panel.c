#include "zwm.h"

Client *panel = NULL;
int oldy;

static void map(Client *c, void *priv)
{
	if(c->type == ZenDockWindow && !panel)
	{
		ZWM_DEBUG("panel found %s\n", c->name);
		panel = c;
		zwm_update_screen_geometry();
	}
}

static void configure(Client *c, void *priv)
{
	if(c == panel)
	{
		zwm_update_screen_geometry();
	}
}


static void unmap(Client *c, void *priv)
{
	if(c == panel)
	{
		panel = NULL;
		zwm_update_screen_geometry();
	}
}

static void rescreen(const char *layout, void *priv)
{
	int i;
	ZenGeom vscreen = {0,0,0,0};
	XWindowAttributes wa;
	if(panel) {
		XGetWindowAttributes(dpy, panel->win, &wa);
		panel->x = wa.x;
		panel->y = wa.y;
		panel->w = wa.width;
		panel->h = wa.height;

		for(i = 0; i< screen_count; i++){
			ZWM_DEBUG("before rescreen: %d, %d %d %d %d %d %d\n",i, panel->y, panel->h, screen[i].x, screen[i].y, screen[i].w, screen[i].h);
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

				if(panel->y+panel->h >= screen[i].y && panel->y <= screen[i].h) 
					screen[i].h -=  panel->h;
			}
		}
	} 

	for(i = 0; i< screen_count; i++){
		if(vscreen.x >= screen[i].x)
			vscreen.x = screen[i].x;
		if(vscreen.y >= screen[i].y)
			vscreen.y = screen[i].y;
		
		vscreen.w += screen[i].w ;

		if(vscreen.h < screen[i].h)
			vscreen.h = screen[i].h;

		ZWM_DEBUG( "Virtual Screen %d %d:  %d+%d+%dx%d\n", i,screen_count,
			       	 vscreen.x,  vscreen.y,
				vscreen.w , vscreen.h);
	}

	config.screen_x = vscreen.x;
	config.screen_y = vscreen.y;
	config.screen_w = vscreen.w;
	config.screen_h = vscreen.h;


	ZWM_DEBUG("rescreen: %d %d %d %d\n",screen[0].x, screen[0].y, screen[0].w, screen[0].h);
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
		zwm_update_screen_geometry();
	}
}

void zwm_panel_init(void)
{
	zwm_event_register(ZenClientUnmap, (ZenEFunc)unmap, NULL);
	zwm_event_register(ZenClientConfigure, (ZenEFunc)configure, NULL);
	zwm_event_register(ZenClientMap, (ZenEFunc)map, NULL);
	zwm_event_register(ZenScreenSize, (ZenEFunc)rescreen, NULL);
}

