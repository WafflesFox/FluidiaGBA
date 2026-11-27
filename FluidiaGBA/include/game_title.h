#ifndef GAME_TITLE_H
#define GAME_TITLE_H

// ================================================================================================
// The title screen
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
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
        text.generate(-65,70, "version " GAME_VER, text_sprites);
        text.generate(70,70, "By Waffles Fox", text_sprites);

        bool visible = true;
        int frame = 0;

        // if the music chunk finishes, another will play (wow)
        // int chunk = 0;
        // bn::sound_handle music = music_chunk(chunk).play();
        MusicPlayer music; music.play(title_track);

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
            // if(!music.active())   // this chunk finished, so start the next one
            // {
            //     chunk = (chunk + 1) % music_chunks;
            //     music = music_chunk(chunk).play();
            // }
            music.update();

            bn::core::update();
        }
        music.stop();
        sfx(1);
        // everything gets destroyed in this bounds
    }
}

#endif
