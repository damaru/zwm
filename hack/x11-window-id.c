//
//  Copyright(c) 2004, The Rasterman
//  http://www.mail-archive.com/devel@xfree86.org/msg05806.html

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <sys/utsname.h>
#include <string.h>

/* prototypes */
static void __e_hack_set_properties(Display *display, Window window);

/* dlopened xlib so we can find the symbols in the real xlib to call them */
static void *lib_xlib = NULL;
static Atom a_zwm_insert = 0;
static Atom a_client_leader = 0;

static void unset_env(Display *display, Window w)
{
	const char *env;
	Atom real, *state;
	int format;
	unsigned char *data = NULL;
	ulong i, n, extra;
	if (!a_client_leader)
		a_client_leader   = XInternAtom(display, "WM_CLIENT_LEADER", False);

	if(XGetWindowProperty(display, w, a_client_leader, 0L, 32, False,
				XA_ATOM, &real, &format, &n, &extra,
				(unsigned char **) &data) == Success && data){
		unsetenv("ZWM_INSERT");
	}
}

int XMapWindow(Display *display, Window w)
{
   static int (*func) ( Display *display, Window w);
   if (!lib_xlib) lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
   if (!func) func = dlsym (lib_xlib, "XMapWindow");

   unset_env(display, w);

   return func(display, w);
}


/* the function that actually sets the properties on toplevel window */
static void
__e_hack_set_properties(Display *display, Window window)
{
   static Atom a_launch_id = 0;
   static Atom a_launch_path = 0;
   static Atom a_user_id = 0;
   static Atom a_process_id = 0;
   static Atom net_process_id = 0;
   static Atom a_p_process_id = 0;
   static Atom a_machine_name = 0;
   static Atom a_user_name = 0;
   char *env = NULL;

   if (!a_zwm_insert)    a_zwm_insert    = XInternAtom(display, "_ZWM_INSERT", False);
   if (!a_launch_id)    a_launch_id    = XInternAtom(display, "_E_HACK_LAUNCH_ID", False);
   if (!a_launch_path)  a_launch_path  = XInternAtom(display, "_E_HACK_LAUNCH_PATH", False);
   if (!a_user_id)      a_user_id      = XInternAtom(display, "_E_HACK_USER_ID", False);
   if (!a_process_id)   a_process_id   = XInternAtom(display, "_E_HACK_PROCESS_ID", False);
   if (!net_process_id)   net_process_id   = XInternAtom(display, "_NET_WM_PID", False);
   if (!a_p_process_id) a_p_process_id = XInternAtom(display, "_E_HACK_PARENT_PROCESS_ID", False);
   if (!a_machine_name) a_machine_name = XInternAtom(display, "_E_HACK_MACHINE_NAME", False);
   if (!a_user_name)    a_user_name    = XInternAtom(display, "_E_HACK_USER_NAME", False);

   if ((env = getenv("ZWM_INSERT"))){
      XChangeProperty(display, window, a_zwm_insert, XA_STRING, 8, PropModeReplace, env, strlen(env));
      unset_env(display, window);
   }

   if ((env = getenv("E_HACK_LAUNCH_ID")))
      XChangeProperty(display, window, a_launch_id, XA_STRING, 8, PropModeReplace, env, strlen(env));
   if ((env = getenv("E_HACK_LAUNCH_PATH")))
      XChangeProperty(display, window, a_launch_path, XA_STRING, 8, PropModeReplace, env, strlen(env));
     {
        uid_t uid;
        pid_t pid, ppid;
        struct utsname ubuf;
        char buf[4096];
        
        uid = getuid();
        pid = getpid();
        ppid = getppid();

        snprintf(buf, sizeof(buf), "%i", uid);
        XChangeProperty(display, window, a_user_id, XA_STRING, 8, PropModeReplace, buf, strlen(buf));
        snprintf(buf, sizeof(buf), "%i", pid);
        XChangeProperty(display, window, a_process_id, XA_STRING, 8, PropModeReplace, buf, strlen(buf));
        XChangeProperty(display, window, net_process_id, XA_CARDINAL, 32, PropModeReplace, (const unsigned char *)&pid, 1);
        snprintf(buf, sizeof(buf), "%i", ppid);
        XChangeProperty(display, window, a_p_process_id, XA_STRING, 8, PropModeReplace, buf, strlen(buf));
        if (!uname(&ubuf))
          {
             snprintf(buf, sizeof(buf), "%s", ubuf.nodename);
             XChangeProperty(display, window, a_machine_name, XA_STRING, 8, PropModeReplace, buf, strlen(buf));
          }
        else
           XChangeProperty(display, window, a_machine_name, XA_STRING, 8, PropModeReplace, " ", 1);
     }
   if ((env = getenv("USER")))
      XChangeProperty(display, window, a_user_name, XA_STRING, 8, PropModeReplace, env, strlen(env));
}

/* XCreateWindow intercept hack */
Window
XCreateWindow(
              Display *display,
              Window parent,
              int x, int y,
              unsigned int width, unsigned int height,
              unsigned int border_width,
              int depth,
              unsigned int class,
              Visual *visual,
              unsigned long valuemask,
              XSetWindowAttributes *attributes
              )
{
   static Window (*func)
      (
       Display *display,
       Window parent,
       int x, int y,
       unsigned int width, unsigned int height,
       unsigned int border_width,
       int depth,
       unsigned int class,
       Visual *visual,
       unsigned long valuemask,
       XSetWindowAttributes *attributes
       ) = NULL;
   int i;

   /* find the real Xlib and the real X function */
   if (!lib_xlib) lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
   if (!func) func = dlsym (lib_xlib, "XCreateWindow");

   /* multihead screen handling loop */
   for (i = 0; i < ScreenCount(display); i++)
     {
        /* if the window is created as a toplevel window */
        if (parent == RootWindow(display, i))
          {
             Window window;
             
             /* create it */
             window = (*func) (display, parent, x, y, width, height, 
                                border_width, depth, class, visual, valuemask, 
                                attributes);
             /* set properties */
             __e_hack_set_properties(display, window);
             /* return it */
             return window;
          }
     }
   /* normal child window - create as usual */
   return (*func) (display, parent, x, y, width, height, border_width, depth,
                   class, visual, valuemask, attributes);
}

/* XCreateSimpleWindow intercept hack */
Window
XCreateSimpleWindow(
                    Display *display,
                    Window parent,
                    int x, int y,
                    unsigned int width, unsigned int height,
                    unsigned int border_width,
                    unsigned long border,
                    unsigned long background
                    )
{
   static Window (*func)
      (
       Display *display,
       Window parent,
       int x, int y,
       unsigned int width, unsigned int height,
       unsigned int border_width,
       unsigned long border,
       unsigned long background
       ) = NULL;
   int i;
   
   /* find the real Xlib and the real X function */
   if (!lib_xlib) lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
   if (!func) func = dlsym (lib_xlib, "XCreateSimpleWindow");
   
   /* multihead screen handling loop */
   for (i = 0; i < ScreenCount(display); i++)
     {
        /* if the window is created as a toplevel window */
        if (parent == RootWindow(display, i))
          {
             Window window;
             
             /* create it */
             window = (*func) (display, parent, x, y, width, height, 
                                border_width, border, background);
             /* set properties */
             __e_hack_set_properties(display, window);
             /* return it */
             return window;
          }
     }
   /* normal child window - create as usual */
   return (*func) (display, parent, x, y, width, height, 
                   border_width, border, background);
}



/* XReparentWindow intercept hack */
int
XReparentWindow(
                Display *display,
                Window window,
                Window parent,
                int x, int y
                )
{
   static int (*func)
      (
       Display *display,
       Window window,
       Window parent,
       int x, int y
       ) = NULL;
   int i;
   
   /* find the real Xlib and the real X function */
   if (!lib_xlib) lib_xlib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
   if (!func) func = dlsym (lib_xlib, "XReparentWindow");

  unsetenv("ZWM_INSERT");
   
   /* multihead screen handling loop */
   for (i = 0; i < ScreenCount(display); i++)
     {
        /* if the window is created as a toplevel window */
        if (parent == RootWindow(display, i))
          {
             /* set properties */
             __e_hack_set_properties(display, window);
             /* reparent it */
             return (*func) (display, window, parent, x, y);
          }
     }
   /* normal child window reparenting - reparent as usual */
   return (*func) (display, window, parent, x, y);
}
