// ================================================================================================
// Final smash cutscene: Umbreon's minecart
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        // ---- FS_UMB_MINECART cutscene: Enemy on minecart -> Pan left to Minecraft house -> Boom ----
        void build_umb_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* vic_def = fighter_def(_fs_victim);
            _fs_cam_x = 0;

            _fs_bg = bn::regular_bg_items::minecraft_fs_bg.create_bg(-136, 0);
            _fs_bg->set_priority(3);

            _fs_vic_mat = bn::sprite_affine_mat_ptr::create();
            _fs_vic_mat->set_scale(1.5);
            _fs_vic_pal = bn::sprite_palette_ptr::create_new(vic_def->sprite->palette_item());
            if(_fs_victim == 1)      { _fs_vic_pal->set_fade(bn::color(31, 6, 6), 0.35); }
            else if(_fs_victim == 2) { _fs_vic_pal->set_fade(bn::color(6, 12, 31), 0.35); }

            _fs_vic = vic_def->sprite->create_sprite(58, -8, safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_LEFT, false)));
            _fs_vic->set_bg_priority(0);
            _fs_vic->set_z_order(-2);
            _fs_vic->set_affine_mat(*_fs_vic_mat);
            _fs_vic->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_vic->set_palette(*_fs_vic_pal);

            _fs_cart_mat = bn::sprite_affine_mat_ptr::create();
            _fs_cart_mat->set_scale(1.5);
            _fs_cart = bn::sprite_ptr::create(
                bn::sprite_items::mc_rides.shape_size(),
                bn::sprite_items::mc_rides.tiles_item().create_tiles(2),
                bn::sprite_items::mc_props.palette_item().create_palette());
            _fs_cart->set_position(58, 14);
            _fs_cart->set_bg_priority(0);
            _fs_cart->set_z_order(-3);
            _fs_cart->set_affine_mat(*_fs_cart_mat);
            _fs_cart->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);

            _fs_beam_mat = bn::sprite_affine_mat_ptr::create();
            _fs_beam_mat->set_scale(1.85);
            _fs_impact = bn::sprite_items::fire_impact.create_sprite(-24, 6, 0);
            _fs_impact->set_bg_priority(0);
            _fs_impact->set_z_order(-4);
            _fs_impact->set_affine_mat(*_fs_beam_mat);
            _fs_impact->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_impact->set_visible(false);

            for(int i = 0; i < 4; ++i)
            {
                _fs_beam[i] = bn::sprite_items::explosion.create_sprite(-24, 6, 0);
                _fs_beam[i]->set_bg_priority(0);
                _fs_beam[i]->set_z_order(-5);
                _fs_beam[i]->set_visible(false);
            }

            text.set_center_alignment();
            text.generate(0, -58, "MINECART MAYHEM!", _fs_banner);
            for(bn::sprite_ptr& sp : _fs_banner) { sp.set_bg_priority(0); sp.set_z_order(-10); }
        }

        void update_umb_fs(bn::sprite_text_generator& text)
        {
            (void)text;
            const int t = _fs_timer++;

            if(t <= 10) { set_flash(bn::fixed(10 - t) / 10); }
            if(t == 28) { _fs_banner.clear(); }

            int wx = 96;
            if(t < 45)
            {
                _fs_cam_x = 0;
                wx = 450 - t * 85 / 45;
            }
            else if(t < 120)
            {
                _fs_cam_x = (t - 45) * 272 / 75;
                wx = 365 - (t - 45) * 195 / 75;
            }
            else
            {
                _fs_cam_x = 272;
                wx = (t < 138) ? (170 - (t - 120) * 74 / 18) : 96;
            }

            const int sx = wx - 392 + _fs_cam_x;
            if(t < 138)
            {
                const int jitter = (t % 4 < 2) ? -1 : 1;
                _fs_vic->set_visible(sx > -155 && sx < 155);
                _fs_vic->set_position(sx, -8 + jitter);
                _fs_cart->set_visible(sx > -155 && sx < 155);
                _fs_cart->set_position(sx, 14);
                _fs_cart->set_tiles(bn::sprite_items::mc_rides.tiles_item(), 2 + (t / 3) % 2);
            }
            else if(t == 138)
            {
                _fs_vic->set_visible(false);
                _fs_cart->set_visible(false);
                events |= EV_SWING;
            }

            int shake_x = 0, shake_y = 0;
            if(t >= 150)
            {
                if(t == 150) { events |= EV_BOOM | EV_SMASH; }
                if(t == 165) { events |= EV_BOOM | EV_HIT; }

                const int dt = t - 150;
                const int pwr = dt < 45 ? (7 - dt * 7 / 45) : 0;
                shake_x = (t % 2 == 0) ? pwr : -pwr;
                shake_y = (t % 4 < 2) ? pwr : -pwr;

                if(t <= 165) { set_flash(bn::fixed(165 - t) / 15); }

                if(dt < 45)
                {
                    _fs_beam_mat->set_scale(bn::fixed(1.7) + bn::fixed(dt % 4) * bn::fixed(0.08));
                    _fs_impact->set_visible(true);
                    _fs_impact->set_tiles(bn::sprite_items::fire_impact.tiles_item(), (dt / 3) % 3);
                    _fs_impact->set_position(-24 + shake_x, 6 + shake_y);

                    static const int bx[4] = { -52, 4, -24, -24 };
                    static const int by[4] = { 12, 12, -20, 22 };
                    for(int i = 0; i < 4; ++i)
                    {
                        const int e_age = (dt + i * 3) % 16;
                        _fs_beam[i]->set_visible(dt < 38);
                        _fs_beam[i]->set_tiles(bn::sprite_items::explosion.tiles_item(), (e_age / 4) % 4);
                        _fs_beam[i]->set_position(bx[i] + shake_x, by[i] + shake_y);
                    }
                }
                else
                {
                    _fs_impact->set_visible(false);
                    for(int i = 0; i < 4; ++i) { _fs_beam[i]->set_visible(false); }
                }
            }

            _fs_bg->set_position(-136 + _fs_cam_x + shake_x, shake_y);

            if(t >= 195) { set_flash(bn::fixed(t - 195) / 12); }
            if(t >= 207)
            {
                unload_fs_gfx();
                _phase = PH_FS_EXIT;
            }
        }
