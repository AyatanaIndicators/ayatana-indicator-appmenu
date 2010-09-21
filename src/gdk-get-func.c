/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-2007 Peter Mattis, Spencer Kimball,
 * Josh MacDonald, Ryan Lortie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
/*
#ifdef HAVE_XKB
#include <X11/XKBlib.h>
#endif

#include <netinet/in.h>
#include <unistd.h>
*/
#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <gdk/gdkwindow.h>

#include "MwmUtil.h"
/*
#include "gdkwindowimpl.h"
#include "gdkasync.h"
#include "gdkinputprivate.h"
#include "gdkdisplay-x11.h"
#include "gdkprivate-x11.h"
#include "gdkregion.h"
#include "gdkinternals.h"
#include "gdkwindow-x11.h"
#include "gdkalias.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <X11/extensions/shape.h>

#ifdef HAVE_XCOMPOSITE
#include <X11/extensions/Xcomposite.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif
*/

#define WINDOW_IS_TOPLEVEL_OR_FOREIGN(window) \
  (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD &&   \
     GDK_WINDOW_TYPE (window) != GDK_WINDOW_OFFSCREEN)

static MotifWmHints *
gdk_xid_get_mwm_hints (Window window)
{
  GdkDisplay *display;
  Atom hints_atom = None;
  guchar *data;
  Atom type;
  gint format;
  gulong nitems;
  gulong bytes_after;
  
  display = gdk_display_get_default ();
  
  hints_atom = gdk_x11_get_xatom_by_name_for_display (display, _XA_MOTIF_WM_HINTS);

  gdk_error_trap_push ();
  XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), window,
		      hints_atom, 0, sizeof (MotifWmHints)/sizeof (long),
		      False, AnyPropertyType, &type, &format, &nitems,
		      &bytes_after, &data);
  gdk_flush ();
  gdk_error_trap_pop ();

  if (type == None)
    return NULL;
  
  return (MotifWmHints *)data;
}

/**
 * gdk_window_get_functions:
 * @window: The toplevel #X11Window to get the functions for
 * @functions: The window functions will be written here
 *
 * Returns the functions set on the GdkWindow with #gdk_window_set_functions
 * Returns: TRUE if the window has functions set, FALSE otherwise.
 **/
gboolean
egg_xid_get_functions (Window window,
                       GdkWMFunction *functions)
{
  MotifWmHints *hints;
  gboolean result = FALSE;

  hints = gdk_xid_get_mwm_hints (window);
  
  if (hints)
    {
      if (hints->flags & MWM_HINTS_FUNCTIONS)
        {
          if (functions)
            *functions = hints->functions;
          result = TRUE;
        }
      
      XFree (hints);
    }

  return result;
}
