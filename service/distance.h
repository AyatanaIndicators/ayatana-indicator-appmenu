/*
Functions to calculate the distance between two strings.

Copyright 2011 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __DISTANCE_H__
#define __DISTANCE_H__

G_BEGIN_DECLS

guint calculate_distance (const gchar * needle, GStrv haystack, GStrv * matched);

G_END_DECLS

#endif /* __DISTANCE_H__ */
