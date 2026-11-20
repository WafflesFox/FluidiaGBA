#include "bn_core.h"
#include "bn_color.h"
#include "bn_bg_palettes.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "common_variable_8x16_sprite_font.h"
#include "bn_sprite_items_player.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_items_layer_2_mountains.h"
#include "bn_regular_bg_items_layer_3_hills.h"
#include "bn_regular_bg_items_layer_4_forest.h"
#include "bn_sound_items.h"
#include "bn_sound_item.h"
#include "bn_sound_handle.h"
#include "bn_sprite_items_waffle.h"
#include "bn_sprite_items_black_block.h"   // solid black 64x64, used by the screen transition

// the maps: ground picture, the layer drawn in front of the player, and the collision grids
#include "bn_regular_bg_items_world.h"
#include "bn_regular_bg_items_world_top.h"
#include "bn_regular_bg_items_world2.h"
#include "bn_regular_bg_items_house_interior.h"
#include "world_map.h"              // namespace world
#include "world2_map.h"             // namespace world2
#include "house_interior_map.h"     // namespace house_interior



namespace
{
    constexpr int music_chunks = 5;   // how many chunks
    constexpr int walk_cycle[4] = {0,1,0,2}; // the animation >w<
    enum Dir {UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3};

    int first_frame(int dir, bool running)
    {
        return (running ? 12 : 0) + dir * 3;
    }

    bn::fixed clamp_fixed(bn::fixed v, bn::fixed lo, bn::fixed hi)
    {
        if(v < lo) { return lo; }
        if(v > hi) { return hi; }
        return v;
    }

    bn::sound_item music_chunk(int index) // title music
    {
        switch(index)
        {
            case 0:  return bn::sound_items::title_00;
            case 1:  return bn::sound_items::title_01;
            case 2:  return bn::sound_items::title_02;
            case 3: return bn::sound_items::title_03;
            default: return bn::sound_items::title_04;
        }
    }
    bn::sound_item gameMusic_chunk(int index) // game music
    {
        switch(index){
            case 0: return bn::sound_items::game_00;
            case 1: return bn::sound_items::game_01;
            case 2: return bn::sound_items::game_02;
            case 3: return bn::sound_items::game_03;
            default: return bn::sound_items::game_04;
        }
    }

    void enlarge(bn::ivector<bn::sprite_ptr>& sprites, bn::fixed center_y, bn::fixed scale) // this function was helped made by Claude AI
    {
        bn::sprite_affine_mat_ptr mat = bn::sprite_affine_mat_ptr::create();
        mat.set_scale(scale);

        for(bn::sprite_ptr& s : sprites)
        {
            s.set_affine_mat(mat);                                          // one shared transform
            // s.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);   // stops the edges getting clipped
            s.set_x(s.x() * scale);                                         // spread the letters out
            s.set_y(center_y + (s.y() - center_y) * scale);
        }
    }

    // ================================================================================
    // MAPS AND WARPS
    // ================================================================================

    constexpr int T = 16;           // one map tile, in pixels

    enum MapId { MAP_WORLD, MAP_WORLD2, MAP_HOUSE };

    // A warp is a rectangle on a map, in world pixels. When the player's FEET step inside it,
    // the game loads dest_map and puts the player's center at (dest_x, dest_y) there.
    // Always arrive OUTSIDE the destination's own warp rectangles, or you bounce straight back.
    struct Warp { int x, y, w, h; int dest_map; int dest_x, dest_y; };

    constexpr Warp world_warps[] =
    {
        // the house door (tile 41,41) -> inside the house, standing just above its door mat
        { 41 * T, 41 * T, T, T,                 MAP_HOUSE,  7 * T + 8, 11 * T },
        // the east edge of the world (a gap in the trees on rows 32-33) -> west side of world2
        { world::pixel_w - 8, 32 * T, 8, 2 * T, MAP_WORLD2, 3 * T,     32 * T + 8 },
    };

    constexpr Warp world2_warps[] =
    {
        // the west edge -> back to the east side of the first world
        { 0, 32 * T, 8, 2 * T,                  MAP_WORLD,  world::pixel_w - 3 * T, 32 * T + 8 },
    };

    constexpr Warp house_warps[] =
    {
        // the door mat (tile 7,12) -> outside, just below the house door
        { 7 * T, 12 * T, T, T,                  MAP_WORLD,  41 * T + 8, 42 * T },
    };

    // everything that differs per map lives in these small functions

    int map_width(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_interior::pixel_w;
            case MAP_WORLD2: return world2::pixel_w;
            default:         return world::pixel_w;
        }
    }

    int map_height(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_interior::pixel_h;
            case MAP_WORLD2: return world2::pixel_h;
            default:         return world::pixel_h;
        }
    }

    bool map_walkable(int id, int px, int py)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_interior::is_walkable(px, py);
            case MAP_WORLD2: return world2::is_walkable(px, py);
            default:         return world::is_walkable(px, py);
        }
    }

    // the name shown in the corner (rename these to whatever you like)
    const char* map_name(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return "Home";
            case MAP_WORLD2: return "Second World";
            default:         return "Grassy Plains";
        }
    }

    bn::regular_bg_ptr make_ground(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return bn::regular_bg_items::house_interior.create_bg(0, 0);
            case MAP_WORLD2: return bn::regular_bg_items::world2.create_bg(0, 0);
            default:         return bn::regular_bg_items::world.create_bg(0, 0);
        }
    }

    // the layer drawn in front of the player (roofs, canopies); not every map has one
    bn::optional<bn::regular_bg_ptr> make_top(int id)
    {
        if(id == MAP_WORLD)
        {
            return bn::regular_bg_items::world_top.create_bg(0, 0);
        }
        return bn::optional<bn::regular_bg_ptr>();
    }

    const Warp* map_warps(int id, int& count)
    {
        switch(id)
        {
            case MAP_HOUSE:
                count = sizeof(house_warps) / sizeof(house_warps[0]);
                return house_warps;
            case MAP_WORLD2:
                count = sizeof(world2_warps) / sizeof(world2_warps[0]);
                return world2_warps;
            default:
                count = sizeof(world_warps) / sizeof(world_warps[0]);
                return world_warps;
        }
    }

    // ================================================================================
    // SCREEN TRANSITION: a black block slides across the screen
    // ================================================================================

    // The block is a 4x3 grid of 64x64 black sprites = 256x192 pixels, a bit bigger than the 240x160 screen.
    constexpr int TRANSITION_SPEED = 10;   // pixels per frame (higher = faster)
    constexpr int BLOCK_COLS = 4;
    constexpr int BLOCK_ROWS = 3;

    // how far the block's center starts from the screen's center, so that it begins just off-screen
    int transition_distance(int dir)
    {
        return (dir == LEFT || dir == RIGHT) ? 248 : 176;     // (240 + 256) / 2   and   (160 + 192) / 2
    }

    void make_blocks(bn::vector<bn::sprite_ptr, 12>& blocks)
    {
        for(int row = 0; row < BLOCK_ROWS; ++row)
        {
            for(int col = 0; col < BLOCK_COLS; ++col)
            {
                bn::sprite_ptr s = bn::sprite_items::black_block.create_sprite(
                        bn::fixed(-96 + col * 64), bn::fixed(-64 + row * 64));
                s.set_bg_priority(0);        // in front of the backgrounds...
                s.set_z_order(-32000);       // ...and in front of every other sprite (lowest z order is drawn last)
                blocks.push_back(s);
            }
        }
    }

    // trans_pos is where the block is along its travel axis: it counts down from +distance to -distance,
    // and 0 means the block covers the whole screen. The facing direction decides which side it comes from:
    // facing RIGHT it enters from the right and slides left, facing LEFT it comes from the left, and so on.
    void move_blocks(bn::vector<bn::sprite_ptr, 12>& blocks, int dir, bn::fixed trans_pos)
    {
        bn::fixed off_x = 0;
        bn::fixed off_y = 0;
        switch(dir)
        {
            case RIGHT: off_x = trans_pos;  break;
            case LEFT:  off_x = -trans_pos; break;
            case DOWN:  off_y = trans_pos;  break;
            default:    off_y = -trans_pos; break;     // UP
        }

        for(int i = 0; i < blocks.size(); ++i)
        {
            const int col = i % BLOCK_COLS;
            const int row = i / BLOCK_COLS;
            blocks[i].set_position(bn::fixed(-96 + col * 64) + off_x, bn::fixed(-64 + row * 64) + off_y);
        }
    }

    // ================================================================================
    // SCREENS
    // ================================================================================

    void title_screen(bn::sprite_text_generator& text)
    {
        bn::regular_bg_ptr mountains = bn::regular_bg_items::layer_2_mountains.create_bg(0, 0);
        bn::regular_bg_ptr hills     = bn::regular_bg_items::layer_3_hills.create_bg(0, 0);
        bn::regular_bg_ptr forest    = bn::regular_bg_items::layer_4_forest.create_bg(0, 0);
        mountains.set_priority(3);   // Layers, 3 = furthest back
        hills.set_priority(2);
        forest.set_priority(1);
        text.set_bg_priority(0);

        bn::fixed camera = 0;
        bn::fixed step = 0.25;
        const bn::fixed mountains_speed = 0.25;
        const bn::fixed hills_speed = 0.5;

        bn::vector<bn::sprite_ptr, 32> title;
        bn::vector<bn::sprite_ptr, 32> prompt;
        bn::vector<bn::sprite_ptr, 32> text_sprites;

        text.set_center_alignment();
        text.generate(0, -20, "FLUIDIA", title);
        enlarge(title, -20, 2);

        text.generate(0, 40, "PRESS START", prompt);
        text.generate(-65,70, "Prototype v0.1.3", text_sprites);
        text.generate(70,70, "By Waffles Fox", text_sprites);

        bool visible = true;
        int frame = 0;

        // if the music chunk finishes, another will play (wow)
        int chunk = 0;
        bn::sound_handle music = music_chunk(chunk).play();

        while(!bn::keypad::start_pressed())
        {
            if(++frame == 30)
            {
                frame = 0;
                visible = !visible;
                for(bn::sprite_ptr& s : prompt) { s.set_visible(visible); }
            }

            camera += step;
            if(camera > 40 || camera < -40) { step = -step; }   // camera control woooo

            mountains.set_x(-camera * mountains_speed);
            hills.set_x(-camera * hills_speed);
            forest.set_x(-camera);
            if(!music.active())   // this chunk finished, so start the next one
            {
                chunk = (chunk + 1) % music_chunks;
                music = music_chunk(chunk).play();
            }

            bn::core::update();
        }
        music.stop();
        // everything gets destroyed in this bounds
    }

    void game_screen(bn::sprite_text_generator& text)
    {
        int current_map = MAP_WORLD;
        bn::optional<bn::regular_bg_ptr> ground_bg = make_ground(current_map);
        bn::optional<bn::regular_bg_ptr> top_bg = make_top(current_map);
        ground_bg->set_priority(2);                       // ground: behind the player (backshot the tree)
        if(top_bg) { top_bg->set_priority(0); }           // roofs and canopies: in front of the player

        // the player's position in WORLD pixels (the middle of the first world)
        bn::fixed x = world::pixel_w / 2;
        bn::fixed y = world::pixel_h / 2;

        bn::vector<bn::sprite_ptr, 32> text_sprites;
        bn::vector<bn::sprite_ptr, 32> pause_text_sprites;
        text.set_center_alignment();
        text.generate(-65,70, map_name(current_map), text_sprites);

        int chunk = 0;
        bn::sound_handle music = gameMusic_chunk(chunk).play();
        // bn::sprite_ptr player = bn::sprite_items::player.create_sprite(0, -40);
        // bn::fixed vy = 0;
        // const bn::fixed gravity = 0.15;
        // const bn::fixed floor_y = 64;
        int dir = DOWN;
        int step = 0;
        int timer = 0;
        int shown = first_frame(dir, false);
        bn::sprite_ptr waffle = bn::sprite_items::waffle.create_sprite(0, 0, shown);
        waffle.set_bg_priority(1); // frontshot the tree (between the ground and the roofs)
        bool paused = true;

        const int feet = 12;   // check the ground at the players feet, not the middle of the sprite

        // screen transition state
        bn::vector<bn::sprite_ptr, 12> blocks;   // the black block
        int trans_phase = 0;                     // 0 = none, 1 = block closing over the screen, 2 = block moving away
        int trans_warp = 0;                      // which warp of the current map we are going through
        int trans_dir = DOWN;                    // the way the player was facing when it started
        bn::fixed trans_pos = 0;                 // 0 = the block covers the whole screen

        while(true)
        {
            if(bn::keypad::start_pressed())
            {
                paused = !paused;

                if(paused)
                {
                    text.generate(0, 0, "PAUSED", pause_text_sprites);
                }
                else
                {
                    pause_text_sprites.clear();
                }
            }
            if (!paused)
            {
                bn::fixed dx = 0;
                bn::fixed dy = 0;
                if(bn::keypad::left_held())  { dx -= 1; }
                if(bn::keypad::right_held()) { dx += 1; }
                if(bn::keypad::up_held())    { dy -= 1; }
                if(bn::keypad::down_held())  { dy += 1; }
                if(trans_phase != 0) { dx = 0; dy = 0; }   // the player stands still while the screen wipes

                const bool moving = dx != 0 || dy != 0;
                const bool running = moving && bn::keypad::b_held();
                const bn::fixed speed = running ? 1.5 : 1;

                if(moving)
                {
                    if(dx < 0)      { dir = LEFT;  }
                    else if(dx > 0) { dir = RIGHT; }
                    else if(dy < 0) { dir = UP;    }
                    else            { dir = DOWN;  }

                    // each axis separately, so the player slides along walls instead of sticking
                    // (collision asks the CURRENT map)
                    bn::fixed next_x = x + dx * speed;
                    if(map_walkable(current_map, next_x.integer(), (y + feet).integer())) { x = next_x; }
                    bn::fixed next_y = y + dy * speed;
                    if(map_walkable(current_map, x.integer(), (next_y + feet).integer())) { y = next_y; }

                    if(++timer >= (running ? 5 : 8))   // lower number = faster animation
                    {
                        timer = 0;
                        step = (step + 1) % 4;
                    }
                }
                else
                {
                    step = 0;
                    timer = 0;
                }

                const int wanted = first_frame(dir, running) + walk_cycle[step];
                if(wanted != shown)    // only touch the graphics when the frame actually changes
                {
                    shown = wanted;
                    waffle.set_tiles(bn::sprite_items::waffle.tiles_item(), shown);
                }

                // ---- warps: did the player's feet step into one? ----
                int warp_count = 0;
                const Warp* warps = map_warps(current_map, warp_count);
                if(trans_phase == 0)
                {
                    for(int i = 0; i < warp_count; ++i)
                    {
                        const Warp& w = warps[i];
                        const int fx = x.integer();
                        const int fy = (y + feet).integer();

                        if(fx >= w.x && fx < w.x + w.w && fy >= w.y && fy < w.y + w.h)
                        {
                            trans_phase = 1;                          // start closing the screen
                            trans_warp = i;
                            trans_dir = dir;
                            trans_pos = transition_distance(dir);
                            make_blocks(blocks);
                            break;
                        }
                    }
                }

                // ---- the transition itself ----
                if(trans_phase != 0)
                {
                    trans_pos -= TRANSITION_SPEED;

                    if(trans_phase == 1 && trans_pos <= 0)
                    {
                        trans_pos = 0;                                // the screen is fully covered: swap the maps now
                        const Warp& w = warps[trans_warp];
                        current_map = w.dest_map;
                        x = w.dest_x;
                        y = w.dest_y;

                        ground_bg.reset();                            // free the old map first
                        top_bg.reset();
                        ground_bg = make_ground(current_map);
                        top_bg = make_top(current_map);
                        ground_bg->set_priority(2);
                        if(top_bg) { top_bg->set_priority(0); }

                        text_sprites.clear();                         // update the name in the corner
                        text.generate(-65,70, map_name(current_map), text_sprites);

                        trans_phase = 2;                              // now slide the block away
                    }
                    else if(trans_phase == 2 && trans_pos <= -transition_distance(trans_dir))
                    {
                        blocks.clear();                               // the block has left the screen
                        trans_phase = 0;
                    }

                    if(trans_phase != 0)
                    {
                        move_blocks(blocks, trans_dir, trans_pos);
                    }
                }

                // ---- camera: follows the player, stops at the edges of the current map ----
                const int map_w = map_width(current_map);
                const int map_h = map_height(current_map);
                const bn::fixed cam_x = clamp_fixed(x, 120, map_w - 120);
                const bn::fixed cam_y = clamp_fixed(y, 80, map_h - 80);

                const bn::fixed bg_x = bn::fixed(map_w / 2) - cam_x;
                const bn::fixed bg_y = bn::fixed(map_h / 2) - cam_y;
                ground_bg->set_position(bg_x, bg_y);
                if(top_bg) { top_bg->set_position(bg_x, bg_y); }

                waffle.set_position(x - cam_x, y - cam_y);
            }

            if(!music.active())   // this chunk finished, so start the next one
            {
                chunk = (chunk + 1) % music_chunks;
                music = gameMusic_chunk(chunk).play();
            }
            bn::core::update();
        }
        music.stop();
    }
}

int main()
{
    bn::core::init();

    // while(true){ debugging crap, because GBA music is weird
    //     if(bn::keypad::l_pressed()) { bn::sound_items::title_20.play(); }
    //     if(bn::keypad::b_pressed()) { bn::sound_items::title_25.play(); }
    //     if(bn::keypad::r_pressed()) { bn::sound_items::title.play(); }
    //     bn::core::update();
    // }
    // bn::sound_items::beep.play();
    bn::bg_palettes::set_transparent_color(bn::color(10, 20, 31));  // the main bg

    bn::sprite_text_generator text(common::variable_8x16_sprite_font);

    title_screen(text);
    game_screen(text);
}