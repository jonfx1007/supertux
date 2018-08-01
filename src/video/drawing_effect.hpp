//  SuperTux
//  Copyright (C) 2016 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SUPERTUX_VIDEO_DRAWING_EFFECT_HPP
#define HEADER_SUPERTUX_VIDEO_DRAWING_EFFECT_HPP

/** bitset for drawing effects */
enum {
  /** Don't apply anything */
  NO_EFFECT = 0,

  /** Draw the Surface upside down */
  VERTICAL_FLIP = (1<<1),

  /** Draw the Surface from left to down */
  HORIZONTAL_FLIP = (1<<2),

  NUM_EFFECTS
};

typedef unsigned int DrawingEffect;

#endif

/* EOF */
