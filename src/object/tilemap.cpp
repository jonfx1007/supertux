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

#include "object/tilemap.hpp"

#include "editor/editor.hpp"
#include "supertux/globals.hpp"
#include "supertux/sector.hpp"
#include "supertux/tile.hpp"
#include "supertux/tile_set.hpp"
#include "util/reader.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"
#include "video/drawing_context.hpp"
#include "video/surface.hpp"

TileMap::TileMap(const TileSet *new_tileset) :
  ExposedObject<TileMap, scripting::TileMap>(this),
  PathObject(),
  editor_active(true),
  tileset(new_tileset),
  tiles(),
  real_solid(false),
  effective_solid(false),
  speed_x(1),
  speed_y(1),
  width(0),
  height(0),
  z_pos(0),
  offset(Vector(0,0)),
  movement(0,0),
  drawing_effect(NO_EFFECT),
  alpha(1.0),
  current_alpha(1.0),
  remaining_fade_time(0),
  tint(1, 1, 1),
  current_tint(1, 1, 1),
  remaining_tint_fade_time(0),
  running(false),
  draw_target(DrawingTarget::COLORMAP),
  new_size_x(0),
  new_size_y(0),
  new_offset_x(0),
  new_offset_y(0),
  add_path(false)
{
}

TileMap::TileMap(const TileSet *tileset_, const ReaderMapping& reader) :
  GameObject(reader),
  ExposedObject<TileMap, scripting::TileMap>(this),
  PathObject(),
  editor_active(true),
  tileset(tileset_),
  tiles(),
  real_solid(false),
  effective_solid(false),
  speed_x(1),
  speed_y(1),
  width(-1),
  height(-1),
  z_pos(0),
  offset(Vector(0,0)),
  movement(Vector(0,0)),
  drawing_effect(NO_EFFECT),
  alpha(1.0),
  current_alpha(1.0),
  remaining_fade_time(0),
  tint(1, 1, 1),
  current_tint(1, 1, 1),
  remaining_tint_fade_time(0),
  running(false),
  draw_target(DrawingTarget::COLORMAP),
  new_size_x(0),
  new_size_y(0),
  new_offset_x(0),
  new_offset_y(0),
  add_path(false)
{
  assert(tileset);

  reader.get("solid",  real_solid);
  reader.get("speed",  speed_x);
  reader.get("speed-y", speed_y, speed_x);

  z_pos = reader_get_layer (reader, /* default = */ 0);

  if(real_solid && ((speed_x != 1) || (speed_y != 1))) {
    log_warning << "Speed of solid tilemap is not 1. fixing" << std::endl;
    speed_x = 1;
    speed_y = 1;
  }

  ReaderMapping path_mapping;
  if (reader.get("path", path_mapping)) {
    reader.get("running", running, false);
    path.reset(new Path());
    path->read(path_mapping);
    walker.reset(new PathWalker(path.get(), running));
    Vector v = path->get_base();
    set_offset(v);
  }

  std::string draw_target_s = "normal";
  reader.get("draw-target", draw_target_s);
  if (draw_target_s == "normal") draw_target = DrawingTarget::COLORMAP;
  if (draw_target_s == "lightmap") draw_target = DrawingTarget::LIGHTMAP;

  if (reader.get("alpha", alpha)) {
    current_alpha = alpha;
  }

  std::vector<float> vColor;
  if (reader.get("tint", vColor)) {
    current_tint = Color(vColor);
    tint = current_tint;
  }

  /* Initialize effective_solid based on real_solid and current_alpha. */
  effective_solid = real_solid;
  update_effective_solid ();

  reader.get("width", width);
  reader.get("height", height);
  if(width < 0 || height < 0) {
    //throw std::runtime_error("Invalid/No width/height specified in tilemap.");
    width = 0;
    height = 0;
    tiles.clear();
    resize(static_cast<int>(Sector::current()->get_width() / 32.0f),
           static_cast<int>(Sector::current()->get_height() / 32.0f));
    editor_active = false;
  } else {
    if(!reader.get("tiles", tiles))
      throw std::runtime_error("No tiles in tilemap.");

    if(int(tiles.size()) != width*height) {
      throw std::runtime_error("wrong number of tiles in tilemap.");
    }
  }

  bool empty = true;

  // make sure all tiles used on the tilemap are loaded and tilemap isn't empty
  for(const auto& tile : tiles) {
    if(tile != 0) {
      empty = false;
    }

    tileset->get(tile);
  }

  if(empty)
  {
    log_info << "Tilemap '" << name << "', z-pos '" << z_pos << "' is empty." << std::endl;
  }
}

TileMap::~TileMap()
{
}

void TileMap::float_channel(float target, float &current, float remaining_time, float elapsed_time)
{
  float amt = (target - current) / (remaining_time / elapsed_time);
  if (amt > 0) current = std::min(current + amt, target);
  if (amt < 0) current = std::max(current + amt, target);
}

void
TileMap::save(Writer& writer) {
  GameObject::save(writer);
  if (draw_target == DrawingTarget::LIGHTMAP) {
    writer.write("draw-target", "lightmap", false);
  } else {
    writer.write("draw-target", "normal", false);
  }
  writer.write("width", width);
  writer.write("height", height);
  writer.write("speed", speed_x);
  if(speed_y != speed_x) {
    writer.write("speed-y", speed_y);
  }
  writer.write("solid", real_solid);
  writer.write("z-pos", z_pos);
  if(alpha != 1) {
    writer.write("alpha", alpha);
  }
  writer.write("tint", tint.toVector());
  if(path) {
    path->save(writer);
  }
  writer.write("tiles", tiles);
}

ObjectSettings
TileMap::get_settings() {
  new_size_x = width;
  new_size_y = height;
  new_offset_x = 0;
  new_offset_y = 0;
  ObjectSettings result = GameObject::get_settings();
  result.options.push_back( ObjectOption(MN_TOGGLE, _("solid"), &real_solid));
  result.options.push_back( ObjectOption(MN_INTFIELD, _("resize offset x"), &new_offset_x));
  result.options.push_back( ObjectOption(MN_INTFIELD, _("resize offset y"), &new_offset_y));
  result.options.push_back( ObjectOption(MN_INTFIELD, _("width"), &new_size_x));
  result.options.push_back( ObjectOption(MN_INTFIELD, _("height"), &new_size_y));
  result.options.push_back( ObjectOption(MN_NUMFIELD, _("alpha"), &alpha));
  result.options.push_back( ObjectOption(MN_NUMFIELD, _("Speed x"), &speed_x));
  result.options.push_back( ObjectOption(MN_NUMFIELD, _("Speed y"), &speed_y));
  result.options.push_back( ObjectOption(MN_COLOR, _("tint"), &tint));
  result.options.push_back( ObjectOption(MN_INTFIELD, _("Z-pos"), &z_pos));

  ObjectOption draw_target_option(MN_STRINGSELECT, _("Draw target"), &draw_target);
  draw_target_option.select.push_back(_("normal"));
  draw_target_option.select.push_back(_("lightmap"));
  result.options.push_back(draw_target_option);

  add_path = walker.get() && path->is_valid();
  result.options.push_back( ObjectOption(MN_TOGGLE, _("Following path"), &add_path));

  if (walker.get() && path->is_valid()) {
    result.options.push_back( Path::get_mode_option(&path->mode) );
    result.options.push_back(ObjectOption(MN_TOGGLE, _("Running"), &running, "running"));
  }

  if (!editor_active) {
    result.options.push_back( ObjectOption(MN_REMOVE, "", NULL));
  }
  return result;
}

void
TileMap::after_editor_set() {
  if ((new_size_x != width || new_size_y != height ||
      new_offset_x || new_offset_y) &&
      new_size_x > 0 && new_size_y > 0) {
    resize(new_size_x, new_size_y, 0, new_offset_x, new_offset_y);
  }

  if (walker.get() && path->is_valid()) {
    if (!add_path) {
      path->nodes.clear();
    }
  } else {
    if (add_path) {
      path.reset(new Path(offset));
      walker.reset(new PathWalker(path.get(), running));
    }
  }
}

void
TileMap::update(float elapsed_time)
{
  // handle tilemap fading
  if (current_alpha != alpha) {
    remaining_fade_time = std::max(0.0f, remaining_fade_time - elapsed_time);
    if (remaining_fade_time == 0.0f) {
      current_alpha = alpha;
    } else {
      float_channel(alpha, current_alpha, remaining_fade_time, elapsed_time);
    }
    update_effective_solid ();
  }

  // handle tint fading
  if (current_tint.red != tint.red || current_tint.green != tint.green ||
      current_tint.blue != tint.blue || current_tint.alpha != tint.alpha) {

    remaining_tint_fade_time = std::max(0.0f, remaining_tint_fade_time - elapsed_time);
    if (remaining_tint_fade_time == 0.0f) {
      current_tint = tint;
    } else {
      float_channel(tint.red  , current_tint.red  , remaining_tint_fade_time, elapsed_time);
      float_channel(tint.green, current_tint.green, remaining_tint_fade_time, elapsed_time);
      float_channel(tint.blue , current_tint.blue , remaining_tint_fade_time, elapsed_time);
      float_channel(tint.alpha, current_tint.alpha, remaining_tint_fade_time, elapsed_time);
    }
  }

  movement = Vector(0,0);
  // if we have a path to follow, follow it
  if (walker.get()) {
    Vector v = walker->advance(elapsed_time);
    if (path->is_valid()) {
      movement = v - get_offset();
      set_offset(v);
    } else {
      set_offset(Vector(0, 0));
    }
  }
}

void
TileMap::draw(DrawingContext& context)
{
  // skip draw if current opacity is 0.0
  if (current_alpha == 0.0) return;

  context.push_transform();

  Canvas& canvas = context.get_canvas(draw_target);

  if(drawing_effect != 0) context.set_drawing_effect(drawing_effect);

  if (editor_active) {
    if(current_alpha != 1.0) {
      context.set_alpha(current_alpha);
    }
  } else {
    context.set_alpha(current_alpha/2);
  }

  /* Force the translation to be an integer so that the tiles appear sharper.
   * For consistency (i.e., to avoid 1-pixel gaps), this needs to be done even
   * for solid tilemaps that are guaranteed to have speed 1.
   * FIXME Force integer translation for all graphics, not just tilemaps. */
  float trans_x = roundf(context.get_translation().x);
  float trans_y = roundf(context.get_translation().y);
  bool normal_speed = editor_active && Editor::is_active();
  context.set_translation(Vector(static_cast<float>(static_cast<int>(trans_x * (normal_speed ? 1.0f : speed_x))),
                                 static_cast<float>(static_cast<int>(trans_y * (normal_speed ? 1.0f : speed_y)))));

  Rectf draw_rect = context.get_cliprect();
  Rect t_draw_rect = get_tiles_overlapping(draw_rect);
  Vector start = get_tile_position(t_draw_rect.left, t_draw_rect.top);

  Vector pos;
  int tx, ty;

  for(pos.x = start.x, tx = t_draw_rect.left; tx < t_draw_rect.right; pos.x += 32, ++tx) {
    for(pos.y = start.y, ty = t_draw_rect.top; ty < t_draw_rect.bottom; pos.y += 32, ++ty) {
      int index = ty*width + tx;
      assert (index >= 0);
      assert (index < (width * height));

      if (tiles[index] == 0) continue;
      const Tile* tile = tileset->get(tiles[index]);
      assert(tile != 0);
      tile->draw(canvas, pos, z_pos, current_tint);
    }
  }

  context.pop_transform();
}

void
TileMap::goto_node(int node_no)
{
  if (!walker.get()) return;
  walker->goto_node(node_no);
}

void
TileMap::start_moving()
{
  if (!walker.get()) return;
  walker->start_moving();
}

void
TileMap::stop_moving()
{
  if (!walker.get()) return;
  walker->stop_moving();
}

void
TileMap::set(int newwidth, int newheight, const std::vector<unsigned int>&newt,
             int new_z_pos, bool newsolid)
{
  if(int(newt.size()) != newwidth * newheight)
    throw std::runtime_error("Wrong tilecount count.");

  width  = newwidth;
  height = newheight;

  tiles.resize(newt.size());
  tiles = newt;

  if (new_z_pos > (LAYER_GUI - 100))
    z_pos = LAYER_GUI - 100;
  else
    z_pos  = new_z_pos;
  real_solid  = newsolid;
  update_effective_solid ();

  // make sure all tiles are loaded
  for(const auto& tile : tiles)
    tileset->get(tile);
}

void
TileMap::resize(int new_width, int new_height, int fill_id,
                int xoffset, int yoffset)
{
  if(new_width < width) {
    // remap tiles for new width
    for(int y = 0; y < height && y < new_height; ++y) {
      for(int x = 0; x < new_width; ++x) {
        tiles[y * new_width + x] = tiles[y * width + x];
      }
    }
  }

  tiles.resize(new_width * new_height, fill_id);

  if(new_width > width) {
    // remap tiles
    for(int y = std::min(height, new_height)-1; y >= 0; --y) {
      for(int x = new_width-1; x >= 0; --x) {
        if(x >= width) {
          tiles[y * new_width + x] = fill_id;
          continue;
        }

        tiles[y * new_width + x] = tiles[y * width + x];
      }
    }
  }

  height = new_height;
  width = new_width;

  //Apply offset
  if (xoffset || yoffset) {
    int X, Y;
    for(int y = 0; y < height; y++) {
      Y = (yoffset < 0) ? y : (height - y - 1);
      for(int x = 0; x < width; x++) {
        X = (xoffset < 0) ? x : (width - x - 1);
        if (Y - yoffset < 0 || Y - yoffset >= height ||
            X - xoffset < 0 || X - xoffset >= width) {
          tiles[Y * new_width + X] = fill_id;
        } else {
          tiles[Y * new_width + X] = tiles[(Y - yoffset) * width + X - xoffset];
        }
      }
    }
  }
}

void TileMap::resize(const Size& newsize, const Size& resize_offset) {
  resize(newsize.width, newsize.height, 0, resize_offset.width, resize_offset.height);
}

Rect
TileMap::get_tiles_overlapping(const Rectf &rect) const
{
  Rectf rect2 = rect;
  rect2.move(-offset);

  int t_left   = std::max(0     , int(floorf(rect2.get_left  () / 32)));
  int t_right  = std::min(width , int(ceilf (rect2.get_right () / 32)));
  int t_top    = std::max(0     , int(floorf(rect2.get_top   () / 32)));
  int t_bottom = std::min(height, int(ceilf (rect2.get_bottom() / 32)));
  return Rect(t_left, t_top, t_right, t_bottom);
}

void
TileMap::set_solid(bool solid)
{
  real_solid = solid;
  update_effective_solid ();
}

uint32_t
TileMap::get_tile_id(int x, int y) const
{
  if(x < 0 || x >= width || y < 0 || y >= height) {
    //log_warning << "tile outside tilemap requested" << std::endl;
    return 0;
  }

  return tiles[y*width + x];
}

const Tile*
TileMap::get_tile(int x, int y) const
{
  uint32_t id = get_tile_id(x, y);
  return tileset->get(id);
}

uint32_t
TileMap::get_tile_id_at(const Vector& pos) const
{
  Vector xy = (pos - offset) / 32;
  return get_tile_id(int(xy.x), int(xy.y));
}

const Tile*
TileMap::get_tile_at(const Vector& pos) const
{
  uint32_t id = get_tile_id_at(pos);
  return tileset->get(id);
}

void
TileMap::change(int x, int y, uint32_t newtile)
{
  assert(x >= 0 && x < width && y >= 0 && y < height);
  tiles[y*width + x] = newtile;
}

void
TileMap::change_at(const Vector& pos, uint32_t newtile)
{
  Vector xy = (pos - offset) / 32;
  change(int(xy.x), int(xy.y), newtile);
}

void
TileMap::change_all(uint32_t oldtile, uint32_t newtile)
{
  for (int x = 0; x < get_width(); x++) {
    for (int y = 0; y < get_height(); y++) {
      if (get_tile_id(x,y) != oldtile)
        continue;

      change(x,y,newtile);
    }
  }
}

void
TileMap::fade(float alpha_, float seconds)
{
  alpha = alpha_;
  remaining_fade_time = seconds;
}

void
TileMap::tint_fade(Color new_tint, float seconds)
{
  tint = new_tint;
  remaining_tint_fade_time = seconds;
}

void
TileMap::set_alpha(float alpha_)
{
  alpha = alpha_;
  current_alpha = alpha;
  remaining_fade_time = 0;
  update_effective_solid ();
}

float
TileMap::get_alpha() const
{
  return current_alpha;
}

void
TileMap::move_by(const Vector& shift)
{
  if (!path) {
    path.reset(new Path(offset));
    walker.reset(new PathWalker(path.get()));
    add_path = true;
  }
  path->move_by(shift);
  offset += shift;
}

/*
 * Private methods
 */
void
TileMap::update_effective_solid()
{
  if (!real_solid)
    effective_solid = false;
  else if (effective_solid && (current_alpha < .25))
    effective_solid = false;
  else if (!effective_solid && (current_alpha >= .75))
    effective_solid = true;
}

void
TileMap::set_tileset(const TileSet* new_tileset)
{
  tileset = new_tileset;
}

/* EOF */
