#include "bn_core.h"
#include "bn_color.h"
#include "bn_bg_palettes.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "common_variable_8x16_sprite_font.h"
#include "bn_sprite_items_player.h"
#include "bn_sprite_affine_mat_ptr.h"   // add with the other includes
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_items_layer_2_mountains.h"
#include "bn_regular_bg_items_layer_3_hills.h"
#include "bn_regular_bg_items_layer_4_forest.h"
#include "bn_sound_items.h"
#include "bn_sound_item.h"
#include "bn_sound_handle.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"



namespace
{
    constexpr int music_chunks = 4;   // how many chunks

    bn::sound_item music_chunk(int index)
    {
        switch(index)
        {
            case 0:  return bn::sound_items::title_00;
            case 1:  return bn::sound_items::title_01;
            case 2:  return bn::sound_items::title_02;
            default: return bn::sound_items::title_03;
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

        text.set_center_alignment();
        text.generate(0, -20, "FLUIDIA", title);
        enlarge(title, -20, 2);

        text.generate(0, 40, "PRESS START", prompt);

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

    void game_screen()
    {
        bn::sprite_ptr player = bn::sprite_items::player.create_sprite(0, -40); // hey, its me :3
        bn::fixed x = 0;
        bn::fixed y = -40;
        bn::fixed vy = 0;
        const bn::fixed gravity = 0.15;
        const bn::fixed floor_y = 64;

        while(true)
        {
            if(bn::keypad::left_held())  { x -= 1; }
            if(bn::keypad::right_held()) { x += 1; }
            if(bn::keypad::a_pressed() && y >= floor_y) { vy = -4; }

            vy += gravity;
            y += vy;
            if(y > floor_y) { y = floor_y; vy = 0; }

            player.set_position(x, y);
            bn::core::update();
        }
    }
}

int main()
{
    bn::core::init();

    // while(true){ debugging crap
    //     if(bn::keypad::l_pressed()) { bn::sound_items::title_20.play(); }
    //     if(bn::keypad::b_pressed()) { bn::sound_items::title_25.play(); }
    //     if(bn::keypad::r_pressed()) { bn::sound_items::title.play(); }
    //     bn::core::update();
    // }
    // bn::sound_items::beep.play();
    bn::bg_palettes::set_transparent_color(bn::color(10, 20, 31));  // the main bg

    bn::sprite_text_generator text(common::variable_8x16_sprite_font);

    title_screen(text);
    game_screen();
}
