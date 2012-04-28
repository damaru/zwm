#include "zwm.h"
#include <limits.h>

struct {
#define _X(name) Atom name
#include "atoms.h"
} _atoms;
#include "atoms.h"
#undef _X

#define NATOMS ((int)(&_atoms.ZWM_LAST - &_atoms.ZWM_FIRST))

void
zwm_x11_atom_init(void) {
#define _X(name) _atoms.name = XInternAtom(dpy, #name, False)
#include "atoms.h"
#undef _X
#define _X(name) name = _atoms.name;
#include "atoms.h"
#undef _X

     XChangeProperty(dpy, root,
		     _NET_SUPPORTED, XA_ATOM, 32,
		     PropModeReplace, (unsigned char *) &_atoms,
		     NATOMS);
}

Bool
zwm_x11_atom_check(Window win, Atom bigatom, Atom smallatom){
    Atom real, *state;
    int format;
    unsigned char *data = NULL;
    ulong i, n, extra;
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
zwm_x11_atom_set(Window w, Atom a, Atom type, ulong *val,
    ulong nitems)
{
    return (XChangeProperty(dpy, w, a, type, 32, PropModeReplace,
        (unsigned char *)val, nitems) == Success);
}

ulong 
zwm_x11_atom_list(Window w, Atom a, Atom type, 
    ulong *ret, ulong nitems, ulong *left)
{
    Atom real_type;
    int i, real_format = 0;
    ulong items_read = 0;
    ulong bytes_left = 0;
    ulong *p;
    unsigned char *data;

    XGetWindowProperty(dpy, w, a,0L, nitems, False, type,
        &real_type, &real_format, &items_read, &bytes_left, &data);

    if (real_format == 32 && items_read) {
        p = (ulong *)data;
        for (i = 0; i < items_read; i++) *ret++ = *p++;
        XFree(data);
        if (left) *left = bytes_left;
        return items_read;
    } else {
	return 0;
    }
}

Bool
zwm_x11_atom_text(Window w, Atom atom, char *text, unsigned int size) 
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
	else if(name.encoding == UTF8_STRING)
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

