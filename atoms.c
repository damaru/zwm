#include "zwm.h"

#define INIT_ATOM(name) Atom name
#include "atoms.h"
#undef INIT_ATOM

#define NATOMS (((char*)&ZWM_LAST - (char*)&ZWM_FIRST))

void
zwm_init_atoms(void) {
#define INIT_ATOM(name) name = XInternAtom(dpy, #name, False)
#include "atoms.h"
#undef INIT_ATOM

     XChangeProperty(dpy, root,
		     _NET_SUPPORTED, XA_ATOM, 32,
		     PropModeReplace, (unsigned char *) &WM_NAME,
		     NATOMS);
}

Atom
zwm_x11_get_window_type(Window win)
{
    Atom real, *atoms;
    int format;
    unsigned char *data = NULL;
    unsigned long i,  n, extra;
    Atom ret = 0;
    if(XGetWindowProperty(dpy, win, _NET_WM_WINDOW_TYPE, 0L, LONG_MAX, False,
                          XA_ATOM, &real, &format, &n, &extra,
                          (unsigned char **) &data) == Success && data){
        Atom *atoms = (Atom *) data;
	for(i = 0; i < n; i++){
		if (atoms[i] == _NET_WM_WINDOW_TYPE_DND ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_DESKTOP ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_DOCK ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_TOOLBAR ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_MENU ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_DIALOG ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_NORMAL ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_UTILITY ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_SPLASH ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_DROPDOWN_MENU ||
	            atoms[i] == _NET_WM_WINDOW_TYPE_TOOLTIP) {
			ret = atoms[i];
			break;
		}
	}
	free(data);
    }
    return ret;
}

Bool
zwm_x11_check_atom(Window win, Atom bigatom, Atom smallatom){
    Atom real, *state;
    int format;
    unsigned char *data = NULL;
    unsigned long i, n, extra;
    if(XGetWindowProperty(dpy, win, bigatom, 0L, LONG_MAX, False,
                          XA_ATOM, &real, &format, &n, &extra,
                          (unsigned char **) &data) == Success && data){
        state = (Atom *) data;
        for(i = 0; i < n; i++){
            if(state[i] == smallatom) {
		    free(data);
                        return True;
	    }
        }
	free(data);
    }
    return False;
}

Bool
zwm_x11_set_atoms(Window w, Atom a, Atom type, unsigned long *val,
    unsigned long nitems)
{
    return (XChangeProperty(dpy, w, a, type, 32, PropModeReplace,
        (unsigned char *)val, nitems) == Success);
}

unsigned long 
zwm_x11_get_atoms(Window w, Atom a, Atom type, unsigned long off,
    unsigned long *ret, unsigned long nitems, unsigned long *left)
{
    Atom real_type;
    int i, real_format = 0;
    unsigned long items_read = 0;
    unsigned long bytes_left = 0;
    unsigned long *p;
    unsigned char *data;

    XGetWindowProperty(dpy, w, a, off, nitems, False, type,
        &real_type, &real_format, &items_read, &bytes_left, &data);

    if (real_format == 32 && items_read) {
        p = (unsigned long *)data;
        for (i = 0; i < items_read; i++) *ret++ = *p++;
        XFree(data);
        if (left) *left = bytes_left;
        return items_read;
    } else {
	return 0;
    }
}

Bool
zwm_x11_append_atoms(Window w, Atom a, Atom type, unsigned long *val,
    unsigned long nitems)
{
    return (XChangeProperty(dpy, w, a, type, 32, PropModeAppend,
        (unsigned char *)val, nitems) == Success);
}

Bool
zwm_x11_get_text_property(Window w, Atom atom, char *text, unsigned int size) 
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if(!text || size == 0)
		return False;
	text[0] = '\0';
	XGetTextProperty(dpy, w, &name, atom);
	if(!name.nitems)
		return False;
	if(name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if(XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success
		&& n > 0 && *list)
		{
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return True;
}

