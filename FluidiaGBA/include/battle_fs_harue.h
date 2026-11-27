// ================================================================================================
// Final smash cutscene: Harue's Dimension Slash
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        // ---- FS_HARUE_SLICE cutscene: Blue Dimension -> Pan Right -> Giant Blue Slash -> Screen Slice! ----
        void build_har_slice_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const FighterDef* vic_def = fighter_def(_fs_victim);
            _fs_cam_x = 0;

            _fs_bg = bn::regular_bg_items::blue_dim_bg.create_bg(136, 0);
            _fs_bg->set_priority(3);
            for(int r = 0; r < bn::display::height(); ++r)
            {
                detail::fs_slice_x_buf[r] = 0;
                detail::fs_slice_y_buf[r] = 0;
            }
            _fs_slice_hbe_x = bn::regular_bg_position_hbe_ptr::create_horizontal(*_fs_bg, detail::fs_slice_x_buf);
            _fs_slice_hbe_y = bn::regular_bg_position_hbe_ptr::create_vertical(*_fs_bg, detail::fs_slice_y_buf);

            _fs_atk_mat = bn::sprite_affine_mat_ptr::create();
            _fs_atk_mat->set_scale(1.5);
            _fs_atk_pal = bn::sprite_palette_ptr::create_new(atk_def->sprite->palette_item());
            _fs_atk_pal->set_fade(bn::color(4, 18, 31), 0.35);

            _fs_atk = atk_def->sprite->create_sprite(272, 18, har_slash[0] + har_slash_offset);
            _fs_atk->set_bg_priority(0);
            _fs_atk->set_z_order(-1);
            _fs_atk->set_affine_mat(*_fs_atk_mat);
            _fs_atk->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_atk->set_palette(*_fs_atk_pal);

            _fs_vic_mat = bn::sprite_affine_mat_ptr::create();
            _fs_vic_mat->set_scale(1.5);
            _fs_vic_pal = bn::sprite_palette_ptr::create_new(vic_def->sprite->palette_item());
            if(_fs_victim == 1)      { _fs_vic_pal->set_fade(bn::color(31, 6, 6), 0.35); }
            else if(_fs_victim == 2) { _fs_vic_pal->set_fade(bn::color(6, 12, 31), 0.35); }

            _fs_vic = vic_def->sprite->create_sprite(-24, 2, safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_RIGHT, false)));
            _fs_vic->set_bg_priority(0);
            _fs_vic->set_z_order(-1);
            _fs_vic->set_affine_mat(*_fs_vic_mat);
            _fs_vic->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_vic->set_palette(*_fs_vic_pal);

            const int vic_hurt = safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_RIGHT, false));
            const int tiles_per_frame = vic_def->sprite->tiles_item().tiles_count_per_graphic();
            const bn::bpp_mode bpp = vic_def->sprite->palette_item().bpp();

            if(bpp == bn::bpp_mode::BPP_4 && tiles_per_frame == 16 && vic_def->sprite->tiles_item().compression() == bn::compression_type::NONE)
            {
                _fs_vic_top_tiles = bn::sprite_tiles_ptr::allocate(16, bn::bpp_mode::BPP_4);
                _fs_vic_bot_tiles = bn::sprite_tiles_ptr::allocate(16, bn::bpp_mode::BPP_4);
                bn::span<bn::tile> top_v = *_fs_vic_top_tiles->vram();
                bn::span<bn::tile> bot_v = *_fs_vic_bot_tiles->vram();
                const bn::tile* src = vic_def->sprite->tiles_item().tiles_ref().data() + vic_hurt * 16;

                for(int ty = 0; ty < 4; ++ty)
                {
                    for(int tx = 0; tx < 4; ++tx)
                    {
                        const int tidx = ty * 4 + tx;
                        bn::tile t_top = src[tidx];
                        bn::tile t_bot = src[tidx];
                        for(int ly = 0; ly < 8; ++ly)
                        {
                            const int py = ty * 8 + ly;
                            uint32_t row_top = t_top.data[ly];
                            uint32_t row_bot = t_bot.data[ly];
                            for(int lx = 0; lx < 8; ++lx)
                            {
                                const int px = tx * 8 + lx;
                                const int cut_py = 17 - (px - 16) / 3;
                                const uint32_t mask = 0xFu << (lx * 4);
                                if(py <= cut_py) { row_bot &= ~mask; }
                                else             { row_top &= ~mask; }
                            }
                            t_top.data[ly] = row_top;
                            t_bot.data[ly] = row_bot;
                        }
                        top_v[tidx] = t_top;
                        bot_v[tidx] = t_bot;
                    }
                }
                _fs_vic_bot = bn::sprite_ptr::create(vic_def->sprite->shape_size(), *_fs_vic_bot_tiles, *_fs_vic_pal);
                _fs_vic_bot->set_bg_priority(0);
                _fs_vic_bot->set_z_order(-1);
                _fs_vic_bot->set_affine_mat(*_fs_vic_mat);
                _fs_vic_bot->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                _fs_vic_bot->set_visible(false);
            }

            // Giant Blue Slash wave (2.4x scaled sonic_slash)
            _fs_beam_mat = bn::sprite_affine_mat_ptr::create();
            _fs_beam_mat->set_scale(2.4);
            _fs_beam[0] = bn::sprite_items::sonic_slash.create_sprite(0, 12, 2);
            _fs_beam[0]->set_bg_priority(0);
            _fs_beam[0]->set_z_order(-3);
            _fs_beam[0]->set_affine_mat(*_fs_beam_mat);
            _fs_beam[0]->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_beam[0]->set_visible(false);

            // Cut-line rift flashes along the diagonal screen slice
            for(int i = 1; i < 4; ++i)
            {
                _fs_beam[i] = bn::sprite_items::blue_slash.create_sprite(0, 0, 1);
                _fs_beam[i]->set_bg_priority(0);
                _fs_beam[i]->set_z_order(-5);
                _fs_beam[i]->set_visible(false);
            }

            _fs_impact = bn::sprite_items::explosion.create_sprite(-24, 14, 0);
            _fs_impact->set_bg_priority(0);
            _fs_impact->set_z_order(-4);
            _fs_impact->set_visible(false);

            // Harue's 64x64 sword in cutscene
            _fs_cart_mat = bn::sprite_affine_mat_ptr::create();
            _fs_cart_mat->set_scale(1.5);
            _fs_cart = bn::sprite_items::harueslice_sword.create_sprite(0, 18, 3);
            _fs_cart->set_bg_priority(0);
            _fs_cart->set_z_order(-2);
            _fs_cart->set_affine_mat(*_fs_cart_mat);
            _fs_cart->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_cart->set_visible(false);

            text.set_center_alignment();
            text.generate(0, -58, "DIMENSION SLASH!", _fs_banner);
            for(bn::sprite_ptr& sp : _fs_banner) { sp.set_bg_priority(0); sp.set_z_order(-10); }
        }

        void update_har_slice_fs(bn::sprite_text_generator& text)
        {
            (void)text;
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const FighterDef* vic_def = fighter_def(_fs_victim);
            const int t = _fs_timer++;

            if(t <= 10) { set_flash(bn::fixed(10 - t) / 10); }
            if(t == 24) { _fs_banner.clear(); }

            // 1. Pan camera right (25..62) to reveal Harue about to slash
            if(t >= 25 && t < 62)
            {
                _fs_cam_x = (t - 25) * 272 / 37;
                if(_fs_cam_x > 272) { _fs_cam_x = 272; }
            }
            // 2. Harue winds up and slashes (63..82), spawning giant blue slash at t=70
            int slash_wx = 250;
            if(t >= 70 && t < 125)
            {
                slash_wx = 250 - (t - 70) * 274 / 55;
            }
            // 3. Camera chases the giant blue slash back to Waffles (76..104)
            if(t >= 76 && t < 105)
            {
                _fs_cam_x = 272 - (t - 76) * 272 / 29;
                if(_fs_cam_x < 0) { _fs_cam_x = 0; }
            }
            else if(t >= 105)
            {
                _fs_cam_x = 0;
            }

            // Update Harue & his sword
            const int ax = 272 - _fs_cam_x;
            _fs_atk->set_visible(ax > -150 && ax < 150);
            _fs_atk->set_position(ax, 18);
            if(t >= 68 && t < 84)
            {
                const int st = (t - 68) / 5;
                const int sidx = (st > 2) ? 2 : st;
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), har_slash[sidx] + har_slash_offset);
                _fs_cart->set_visible(_fs_atk->visible());
                _fs_cart->set_position(ax, 18);
                _fs_cart->set_tiles(bn::sprite_items::harueslice_sword.tiles_item(), 2 - sidx);
                _fs_cart_mat->set_horizontal_flip(true);
                
            }
            else
            {
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), (t >= 48 && t < 68) ? (har_slash[0] + har_slash_offset) : sheet_frame(DIR_LEFT, false));
                _fs_cart->set_visible(false);
            }
            if(t == 68) { events |= EV_SWING | EV_SHOT; }

            // Giant blue slash projectile heading toward victim
            if(t >= 70 && t < 125)
            {
                const int sx = slash_wx - _fs_cam_x;
                _fs_beam[0]->set_visible(sx > -160 && sx < 160);
                _fs_beam[0]->set_position(sx, 2);
                _fs_beam[0]->set_tiles(bn::sprite_items::sonic_slash.tiles_item(), 2 + ((t / 3) % 2));
            }
            else
            {
                _fs_beam[0]->set_visible(false);
            }

            // Victim + Screen Slice Impact (125..174)
            int shake_x = 0;
            int shake_y = 0;
            bn::fixed slice_gap = 0;
            if(t >= 125)
            {
                if(t == 125) { events |= EV_BOOM | EV_SMASH; }
                if(t == 138) { events |= EV_BOOM | EV_HIT; }

                const int dt = t - 125;
                const int pwr = dt < 36 ? (6 - dt * 6 / 36) : 0;
                shake_x = (t % 2 == 0) ? pwr : -pwr;
                shake_y = (t % 4 < 2) ? pwr : -pwr;
                if(t <= 135) { set_flash(bn::fixed(135 - t) / 10); }

                slice_gap = dt < 22 ? bn::fixed(dt) * bn::fixed(1.6) : bn::fixed(35);

                // Explosion + diagonal slice rift flashes across the screen
                _fs_impact->set_visible(dt < 36);
                _fs_impact->set_tiles(bn::sprite_items::explosion.tiles_item(), (dt / 3) % 4);
                _fs_impact->set_position(-24 + shake_x, 2 + shake_y);

                for(int i = 1; i < 4; ++i)
                {
                    _fs_beam[i]->set_visible(dt < 26);
                    _fs_beam[i]->set_position(-80 + (i - 1) * 76, 36 - (i - 1) * 32);
                    _fs_beam[i]->set_tiles(bn::sprite_items::blue_slash.tiles_item(), (dt / 2 + i) % 3);
                }
                _fs_vic_pal->set_fade((t % 6 < 3) ? bn::color(31, 31, 31) : bn::color(4, 18, 31), 0.80);
            }

            const int vx_base = -24 - _fs_cam_x + shake_x;
            const bool vis = vx_base > -150 && vx_base < 150;
            if(slice_gap > 0 && _fs_vic_top_tiles && _fs_vic_bot)
            {
                const int gx = slice_gap.integer() / 2;
                const int gy = slice_gap.integer() / 3;
                _fs_vic->set_tiles(*_fs_vic_top_tiles);
                _fs_vic->set_visible(vis);
                _fs_vic->set_position(vx_base - gx, 2 + shake_y - gy);
                _fs_vic_bot->set_visible(vis);
                _fs_vic_bot->set_position(vx_base + gx, 2 + shake_y + gy);
            }
            else
            {
                _fs_vic->set_visible(vis);
                _fs_vic->set_position(vx_base, 2 + shake_y);
                _fs_vic->set_tiles(vic_def->sprite->tiles_item(), safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_RIGHT, false)));
                if(_fs_vic_bot) { _fs_vic_bot->set_visible(false); }
            }

            _fs_bg->set_position(136 - _fs_cam_x + shake_x, shake_y);
            for(int r = 0; r < bn::display::height(); ++r)
            {
                if(slice_gap > 0)
                {
                    if(r < 78)
                    {
                        detail::fs_slice_x_buf[r] = -slice_gap;
                        detail::fs_slice_y_buf[r] = -slice_gap / 3;
                    }
                    else if(r > 82)
                    {
                        detail::fs_slice_x_buf[r] = slice_gap;
                        detail::fs_slice_y_buf[r] = slice_gap / 3;
                    }
                    else
                    {
                        detail::fs_slice_x_buf[r] = (r % 2 == 0) ? 120 : -120;
                        detail::fs_slice_y_buf[r] = 0;
                    }
                }
                else
                {
                    detail::fs_slice_x_buf[r] = 0;
                    detail::fs_slice_y_buf[r] = 0;
                }
            }
            _fs_slice_hbe_x->reload_deltas_ref();
            _fs_slice_hbe_y->reload_deltas_ref();

            if(t >= 172) { set_flash(bn::fixed(t - 172) / 12); }
            if(t >= 184)
            {
                unload_fs_gfx();
                _phase = PH_FS_EXIT;
            }
        }
