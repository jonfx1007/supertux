//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#include "object/background.hpp"

#include <iostream>
#include "math/sizef.hpp"
#include "supertux/globals.hpp"
#include "supertux/object_factory.hpp"
#include "supertux/sector.hpp"
#include "util/reader.hpp"

Background::Background() :
  layer(LAYER_BACKGROUND0),
  imagefile_top(),
  imagefile(),
  imagefile_bottom(),
  pos(),
  speed(),
  speed_y(),
  scroll_speed(),
  scroll_offset(),
  image_top(),
  image(),
  image_bottom()
{
}

Background::Background(const Reader& reader) :
  layer(LAYER_BACKGROUND0),
  imagefile_top(),
  imagefile(),
  imagefile_bottom(),
  pos(),
  speed(),
  speed_y(),
  scroll_speed(),
  scroll_offset(),
  image_top(),
  image(),
  image_bottom()
{
  // read position, defaults to (0,0)
  float px = 0;
  float py = 0;
  reader.get("x", px);
  reader.get("y", py);
  this->pos = Vector(px,py);

  speed = 1.0;
  speed_y = 1.0;

  reader.get("scroll-offset-x", scroll_offset.x);
  reader.get("scroll-offset-y", scroll_offset.y);

  reader.get("scroll-speed-x", scroll_speed.x);
  reader.get("scroll-speed-y", scroll_speed.y);

  reader.get("layer", layer);
  if(!reader.get("image", imagefile) || !reader.get("speed", speed))
    throw std::runtime_error("Must specify image and speed for background");

  set_image(imagefile, speed);
  if (!reader.get("speed-y", speed_y))
  {
    speed_y = speed;
  }

  if (reader.get("image-top", imagefile_top)) {
    image_top = Surface::create(imagefile_top);
  }
  if (reader.get("image-bottom", imagefile_bottom)) {
    image_bottom = Surface::create(imagefile_bottom);
  }
}

Background::~Background()
{
}

void
Background::update(float delta)
{
  scroll_offset += scroll_speed * delta;
}

void
Background::set_image(const std::string& name, float speed)
{
  this->imagefile = name;
  this->speed = speed;

  image = Surface::create(name);
}

void
Background::draw_image(DrawingContext& context, const Vector& pos)
{
  Sizef level(Sector::current()->get_width(),
              Sector::current()->get_height());

  // FIXME: Implement proper clipping here
  int start_x = -level.width  / image->get_width()  / 2;
  int end_x   =  level.width  / image->get_width()  / 2;
  int start_y = -level.height / image->get_height() / 2;
  int end_y   =  level.height / image->get_height() / 2;

  for(int y = start_y; y <= end_y; ++y)
    for(int x = start_x; x <= end_x; ++x)
    {
      Vector p(pos.x + x * image->get_width()  - image->get_width()/2, 
               pos.y + y * image->get_height() - image->get_height()/2);

      if (image_top.get() != NULL && (y < 0))
      {
        context.draw_surface(image_top.get(), p, layer);
      }
      else if (image_bottom.get() != NULL && (y > 0))
      {
        context.draw_surface(image_bottom.get(), p, layer);
      }
      else
      {
        context.draw_surface(image.get(), p, layer);
      }
    }
}

void
Background::draw(DrawingContext& context)
{
  if(image.get() == NULL)
    return;

  Sizef level_size(Sector::current()->get_width(),
                   Sector::current()->get_height());
  Sizef screen(SCREEN_WIDTH, SCREEN_HEIGHT);
  Sizef translation_range = level_size - screen;
  Vector center_offset(context.get_translation().x - translation_range.width  / 2.0f, 
                       context.get_translation().y - translation_range.height / 2.0f);

  // FIXME: We are not handling 'pos'
  draw_image(context, Vector(level_size.width / 2.0f, level_size.height / 2.0f) + center_offset * (1.0f - speed));
}

/* EOF */
