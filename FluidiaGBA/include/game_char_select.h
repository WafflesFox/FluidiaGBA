#ifndef GAME_CHAR_SELECT_H
#define GAME_CHAR_SELECT_H

// ================================================================================================
// The character select screen (single player and link)
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    bool char_select_screen(bn::sprite_text_generator& text, MusicPlayer& music, net::LinkSession& net_session,
                            int current_map, bn::vector<bn::sprite_ptr, 12>& blocks, int trans_dir,
                            int& out_p1_pick, int& out_p2_pick)
    {
        bn::regular_bg_ptr mountains = bn::regular_bg_items::layer_2_mountains.create_bg(0, 0);
        bn::regular_bg_ptr hills     = bn::regular_bg_items::layer_3_hills.create_bg(0, 0);
        bn::regular_bg_ptr forest    = bn::regular_bg_items::layer_4_forest.create_bg(0, 0);
        mountains.set_priority(3);
        hills.set_priority(2);
        forest.set_priority(1);

        bn::vector<bn::sprite_ptr, 4> panel_bg;
        for(int i = 0; i < 4; ++i)
        {
            bn::sprite_ptr b = bn::sprite_items::black_block.create_sprite(bn::fixed(-96 + i * 64), 64);
            b.set_bg_priority(0);
            b.set_z_order(5);
            panel_bg.push_back(b);
        }

        const int cols = playable_count <= 4 ? playable_count : 4;
        const int total_rows = (playable_count + cols - 1) / cols;
        const int visible_rows = total_rows <= 2 ? total_rows : 2;
        const int grid_y0 = visible_rows == 1 ? -34 : -44;
        const int cell_w = 48;
        const int cell_h = 30;

        int local_pick = 0;
        int remote_pick = 0;
        bool local_ready = false;
        bool remote_ready = false;
        bool peer_in_cs = false;
        int scroll_row = 0;

        bn::vector<bn::sprite_ptr, 8> card_sprites;
        bn::optional<bn::sprite_ptr> p1_prev;
        bn::optional<bn::sprite_ptr> p2_prev;
        bn::sprite_palette_ptr p1_pal = bn::sprite_palette_ptr::create_new(playable_fighters[0]->sprite->palette_item());
        bn::sprite_palette_ptr p2_pal = bn::sprite_palette_ptr::create_new(playable_fighters[0]->sprite->palette_item());

        bn::vector<bn::sprite_ptr, 12> header_text;
        bn::vector<bn::sprite_ptr, 12> cursor_text;
        bn::vector<bn::sprite_ptr, 32> status_text;

        text.set_bg_priority(0);
        text.set_center_alignment();
        text.generate(0, -68, "SELECT FIGHTER", header_text);
        for(bn::sprite_ptr& sp : header_text) { sp.set_z_order(-10); }

        auto rebuild_cards = [&]()
        {
            card_sprites.clear();
            for(int r = 0; r < visible_rows; ++r)
            {
                const int row = scroll_row + r;
                if(row >= total_rows) { break; }
                int row_cols = playable_count - row * cols;
                if(row_cols > cols) { row_cols = cols; }
                const int cy = grid_y0 + r * cell_h;
                for(int c = 0; c < row_cols; ++c)
                {
                    const int idx = row * cols + c;
                    const int cx = (2 * c - (row_cols - 1)) * cell_w / 2;
                    bn::sprite_ptr sp = playable_fighters[idx]->sprite->create_sprite(cx, cy, first_frame(DOWN, false));
                    sp.set_bg_priority(0);
                    sp.set_z_order(-1);
                    card_sprites.push_back(sp);
                }
            }
        };

        auto card_pos = [&](int idx, int& out_x, int& out_y) -> bool
        {
            const int row = idx / cols;
            const int col = idx % cols;
            if(row < scroll_row || row >= scroll_row + visible_rows) { return false; }
            int row_cols = playable_count - row * cols;
            if(row_cols > cols) { row_cols = cols; }
            out_x = (2 * col - (row_cols - 1)) * cell_w / 2;
            out_y = grid_y0 + (row - scroll_row) * cell_h;
            return true;
        };

        int shown_p1 = -1;
        int shown_p2 = -1;
        bool shown_p1_rdy = false;
        bool shown_p2_rdy = false;
        bool shown_peer = false;

        auto refresh_ui = [&]()
        {
            const int p1_idx = (net_session.my_id == 0) ? local_pick : remote_pick;
            const int p2_idx = (net_session.my_id == 1) ? local_pick : remote_pick;
            const bool p1_rdy = (net_session.my_id == 0) ? local_ready : remote_ready;
            const bool p2_rdy = (net_session.my_id == 1) ? local_ready : remote_ready;

            cursor_text.clear();
            text.set_center_alignment();
            int cx = 0, cy = 0;
            if(card_pos(p1_idx, cx, cy)) { text.generate(cx - 8, cy - 16, "P1", cursor_text); }
            if(card_pos(p2_idx, cx, cy)) { text.generate(cx + 8, cy + 16, "P2", cursor_text); }
            for(bn::sprite_ptr& sp : cursor_text) { sp.set_z_order(-12); }

            if(p1_idx != shown_p1 || !p1_prev)
            {
                shown_p1 = p1_idx;
                p1_pal = bn::sprite_palette_ptr::create_new(playable_fighters[p1_idx]->sprite->palette_item());
                p1_prev = playable_fighters[p1_idx]->sprite->create_sprite(-76, 22, first_frame(RIGHT, false));
                p1_prev->set_bg_priority(0);
                p1_prev->set_z_order(-2);
                p1_prev->set_palette(p1_pal);
            }
            if(p2_idx != shown_p2 || !p2_prev)
            {
                shown_p2 = p2_idx;
                p2_pal = bn::sprite_palette_ptr::create_new(playable_fighters[p2_idx]->sprite->palette_item());
                p2_prev = playable_fighters[p2_idx]->sprite->create_sprite(76, 22, first_frame(LEFT, false));
                p2_prev->set_bg_priority(0);
                p2_prev->set_z_order(-2);
                p2_prev->set_palette(p2_pal);
            }

            p1_pal.set_fade(p1_rdy ? bn::color(31, 28, 10) : bn::color(0, 0, 0), p1_rdy ? bn::fixed(0.35) : bn::fixed(0));
            p2_pal.set_fade(p2_rdy ? bn::color(10, 26, 31) : bn::color(0, 0, 0), p2_rdy ? bn::fixed(0.35) : bn::fixed(0));

            status_text.clear();
            text.set_center_alignment();
            text.generate(-76, 46, playable_fighters[p1_idx]->name, status_text);
            text.generate(-76, 60, p1_rdy ? "READY!" : "PICKING", status_text);
            text.generate(76, 46, playable_fighters[p2_idx]->name, status_text);
            text.generate(76, 60, p2_rdy ? "READY!" : "PICKING", status_text);
            text.generate(0, 22, "VS", status_text);

            const char* mid_msg = !peer_in_cs ? "SYNCING..." :
                                  ((p1_rdy && p2_rdy) ? "STARTING!" :
                                  (local_ready ? "WAIT (B:BACK)" : "A:READY"));
            text.generate(0, 70, mid_msg, status_text);
            for(bn::sprite_ptr& sp : status_text) { sp.set_z_order(-10); }

            shown_p1_rdy = p1_rdy;
            shown_p2_rdy = p2_rdy;
            shown_peer = peer_in_cs;
        };

        rebuild_cards();
        refresh_ui();

        bn::fixed cam = 0;
        bn::fixed cam_step = 0.25;
        int anim_tick = 0;
        int frame_count = 0;
        int both_ready_ticks = 0;

        // wipe_state: 0 = opening wipe away, 1 = selecting, 2 = closing wipe into battle
        int wipe_state = 0;
        bn::fixed wipe_pos = 0;
        bool result_ok = true;

        while(true)
        {
            if(!net_session.connected)
            {
                result_ok = false;
                if(wipe_state != 2)
                {
                    make_blocks(blocks);
                    wipe_pos = transition_distance(trans_dir);
                    move_blocks(blocks, trans_dir, wipe_pos);
                    wipe_state = 2;
                }
            }

            if(wipe_state == 0)
            {
                wipe_pos -= TRANSITION_SPEED;
                if(wipe_pos <= -transition_distance(trans_dir))
                {
                    blocks.clear();
                    wipe_state = 1;
                }
                else { move_blocks(blocks, trans_dir, wipe_pos); }
            }
            else if(wipe_state == 2)
            {
                wipe_pos -= TRANSITION_SPEED;
                if(wipe_pos <= 0)
                {
                    move_blocks(blocks, trans_dir, 0);
                    break;
                }
                move_blocks(blocks, trans_dir, wipe_pos);
            }

            if(wipe_state == 1 && peer_in_cs)
            {
                if(!local_ready)
                {
                    int next_pick = local_pick;
                    if(bn::keypad::left_pressed()  && local_pick > 0)                  { --next_pick; }
                    if(bn::keypad::right_pressed() && local_pick + 1 < playable_count) { ++next_pick; }
                    if(bn::keypad::up_pressed()    && local_pick - cols >= 0)          { next_pick -= cols; }
                    if(bn::keypad::down_pressed()  && local_pick + cols < playable_count) { next_pick += cols; }

                    if(next_pick != local_pick)
                    {
                        sfx(2);
                        local_pick = next_pick;
                        const int row = local_pick / cols;
                        bool scrolled = false;
                        if(row < scroll_row) { scroll_row = row; scrolled = true; }
                        else if(row >= scroll_row + visible_rows) { scroll_row = row - visible_rows + 1; scrolled = true; }
                        if(scrolled) { rebuild_cards(); }
                    }

                    if(bn::keypad::a_pressed())
                    {
                        sfx(12);
                        local_ready = true;
                    }
                }
                else if(bn::keypad::b_pressed() && !remote_ready)
                {
                    sfx(2);
                    local_ready = false;
                }
            }

            // Exchange selection & ready flags deterministically over the link session
            net_session.local_invite = net::INV_BUSY;
            net_session.update_overworld(16 + local_pick * 16, local_ready ? 777 : 333, current_map,
                                         local_ready ? 1 : 0, local_ready, false);

            ++frame_count;
            if(!peer_in_cs && frame_count >= 18 && net_session.has_remote_pos && !net_session.remote_moving)
            {
                const bool valid_unready = !net_session.remote_running && net_session.remote_y >= 310 && net_session.remote_y <= 355;
                const bool valid_ready   =  net_session.remote_running && net_session.remote_y >= 755 && net_session.remote_y <= 795;
                if(valid_unready || valid_ready) { peer_in_cs = true; }
            }

            if(peer_in_cs && net_session.has_remote_pos)
            {
                int rp = (net_session.remote_x - 8) / 16;
                if(rp < 0) { rp = 0; }
                if(rp >= playable_count) { rp = playable_count - 1; }
                remote_pick = rp;
                remote_ready = net_session.remote_running && net_session.remote_y >= 750;
            }

            const int p1_idx = (net_session.my_id == 0) ? local_pick : remote_pick;
            const int p2_idx = (net_session.my_id == 1) ? local_pick : remote_pick;
            const bool p1_rdy = (net_session.my_id == 0) ? local_ready : remote_ready;
            const bool p2_rdy = (net_session.my_id == 1) ? local_ready : remote_ready;

            if(p1_idx != shown_p1 || p2_idx != shown_p2 || p1_rdy != shown_p1_rdy ||
               p2_rdy != shown_p2_rdy || peer_in_cs != shown_peer)
            {
                refresh_ui();
            }

            if(wipe_state == 1 && peer_in_cs && local_ready && remote_ready)
            {
                if(both_ready_ticks == 0) { sfx(9); }
                if(++both_ready_ticks >= 25)
                {
                    make_blocks(blocks);
                    wipe_pos = transition_distance(trans_dir);
                    move_blocks(blocks, trans_dir, wipe_pos);
                    wipe_state = 2;
                }
            }
            else
            {
                both_ready_ticks = 0;
            }

            // Animate previews & parallax BG
            ++anim_tick;
            const int step_p1 = walk_cycle[(anim_tick / (p1_rdy ? 5 : 10)) % 4];
            const int step_p2 = walk_cycle[(anim_tick / (p2_rdy ? 5 : 10)) % 4];
            if(p1_prev) { p1_prev->set_tiles(playable_fighters[p1_idx]->sprite->tiles_item(), first_frame(RIGHT, p1_rdy) + step_p1); }
            if(p2_prev) { p2_prev->set_tiles(playable_fighters[p2_idx]->sprite->tiles_item(), first_frame(LEFT, p2_rdy) + step_p2); }

            cam += cam_step;
            if(cam > 40 || cam < -40) { cam_step = -cam_step; }
            mountains.set_x(-cam * bn::fixed(0.25));
            hills.set_x(-cam * bn::fixed(0.5));
            forest.set_x(-cam);

            music.update();
            bn::core::update();
        }

        out_p1_pick = (net_session.my_id == 0) ? local_pick : remote_pick;
        out_p2_pick = (net_session.my_id == 1) ? local_pick : remote_pick;
        text.set_center_alignment();
        return result_ok;
    }
}

#endif
