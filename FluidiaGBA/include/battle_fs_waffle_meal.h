// ================================================================================================
// Final smash cutscene: Waffles' meal (kitchen)
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        // ---- FS_WAFFLE_MEAL cutscene: Waffles teleports around kitchen cooking -> throws meal to teammate ----
        void build_waffle_meal_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const int vic_def_idx = (_fs_victim >= 3 && _fight.f[_fs_victim].clone_of >= 0) ? _fight.f[_fs_victim].clone_of : _fs_victim;
            const FighterDef* vic_def = fighter_def(vic_def_idx);

            _fs_bg = bn::regular_bg_items::kitchen_fs_bg.create_bg(0, 0);
            _fs_bg->set_priority(3);

            _fs_atk_mat = bn::sprite_affine_mat_ptr::create();
            _fs_atk_mat->set_scale(1.5);
            _fs_atk_pal = bn::sprite_palette_ptr::create_new(atk_def->sprite->palette_item());
            _fs_atk = atk_def->sprite->create_sprite(-72, 18, sheet_frame(DIR_RIGHT, false));
            _fs_atk->set_bg_priority(0);
            _fs_atk->set_z_order(-2);
            _fs_atk->set_affine_mat(*_fs_atk_mat);
            _fs_atk->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_atk->set_palette(*_fs_atk_pal);

            _fs_vic_mat = bn::sprite_affine_mat_ptr::create();
            _fs_vic_mat->set_scale(1.5);
            _fs_vic_pal = bn::sprite_palette_ptr::create_new(vic_def->sprite->palette_item());
            if(_fs_victim >= 3) { _fs_vic_pal->set_fade(bn::color(4, 22, 31), 0.55); }
            else if(_fs_victim == 2) { _fs_vic_pal->set_fade(bn::color(6, 12, 31), 0.35); }

            _fs_vic = vic_def->sprite->create_sprite(76, 18, sheet_frame(DIR_LEFT, false));
            _fs_vic->set_bg_priority(0);
            _fs_vic->set_z_order(-1);
            _fs_vic->set_affine_mat(*_fs_vic_mat);
            _fs_vic->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_vic->set_palette(*_fs_vic_pal);

            _fs_beam_mat = bn::sprite_affine_mat_ptr::create();
            _fs_beam_mat->set_scale(1.5);
            _fs_impact = bn::sprite_items::mc_props.create_sprite(12, 2, 16);
            _fs_impact->set_bg_priority(0);
            _fs_impact->set_z_order(-3);
            _fs_impact->set_affine_mat(*_fs_beam_mat);
            _fs_impact->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_impact->set_visible(false);

            _fs_spark = bn::sprite_items::hit_spark.create_sprite(-72, 14, 0);
            _fs_spark->set_bg_priority(0);
            _fs_spark->set_z_order(-4);
            _fs_spark->set_visible(false);

            for(int i = 0; i < 2; ++i)
            {
                _fs_aura[i] = bn::sprite_items::fire_aura.create_sprite(12, 0, i);
                _fs_aura[i]->set_bg_priority(0);
                _fs_aura[i]->set_z_order(-4);
                _fs_aura[i]->set_visible(false);
            }

            text.set_center_alignment();
            text.generate(0, -60, "WAFFLE CARE!", _fs_banner);
            for(bn::sprite_ptr& sp : _fs_banner) { sp.set_bg_priority(0); sp.set_z_order(-10); }
        }

        void update_waffle_meal_fs(bn::sprite_text_generator& text)
        {
            (void)text;
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const int vic_def_idx = (_fs_victim >= 3 && _fight.f[_fs_victim].clone_of >= 0) ? _fight.f[_fs_victim].clone_of : _fs_victim;
            const FighterDef* vic_def = fighter_def(vic_def_idx);
            const int t = _fs_timer++;

            if(t <= 10) { set_flash(bn::fixed(10 - t) / 10); }
            if(t == 28) { _fs_banner.clear(); }

            if(t < 112)
            {
                static const int tp_x[8] = { -74, -28, 28, -22, 12, -62, 34, -14 };
                static const int tp_y[8] = {  18,  18, 18,  10, 12,  18, 18,  18 };
                static const int tp_d[8] = { DIR_RIGHT, DIR_RIGHT, DIR_LEFT, DIR_RIGHT, DIR_DOWN, DIR_RIGHT, DIR_LEFT, DIR_RIGHT };
                const int step = (t / 14) % 8;
                const int sub = t % 14;

                if(sub == 0 && t > 0) { events |= EV_SWING; }
                const int pose = sheet_frame(tp_d[step], true) + ((sub / 3) % 3);
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), pose);
                _fs_atk->set_position(tp_x[step], tp_y[step]);

                if(sub < 8)
                {
                    _fs_spark->set_visible(true);
                    _fs_spark->set_position(tp_x[step], tp_y[step] - 4);
                    _fs_spark->set_tiles(bn::sprite_items::hit_spark.tiles_item(), sub * 3 / 8);
                }
                else { _fs_spark->set_visible(false); }

                if(t >= 35)
                {
                    if(t == 35) { events |= EV_ABSORB; }
                    _fs_impact->set_visible(true);
                    _fs_impact->set_position(12, 2);
                    for(int i = 0; i < 2; ++i)
                    {
                        _fs_aura[i]->set_visible(true);
                        _fs_aura[i]->set_tiles(bn::sprite_items::fire_aura.tiles_item(), ((t / 4) + i) % 3);
                        _fs_aura[i]->set_position(12 + (i == 0 ? -8 : 8), 0 - (t % 5));
                    }
                }
                return;
            }

            if(t < 145)
            {
                if(t == 112)
                {
                    events |= EV_SHOT;
                    _fs_spark->set_visible(false);
                    for(int i = 0; i < 2; ++i) { _fs_aura[i]->set_visible(false); }
                    _fs_atk->set_tiles(atk_def->sprite->tiles_item(), sheet_frame(DIR_RIGHT, true) + 2);
                    _fs_atk->set_position(-14, 18);
                }
                const int dt = t - 112;
                const int mx = 12 + dt * 64 / 33;
                const int arc = (dt * (33 - dt)) / 10;
                const int my = 2 - arc;
                _fs_impact->set_visible(true);
                _fs_impact->set_position(mx, my);
                return;
            }

            if(t == 145)
            {
                _fs_impact->set_visible(false);
                events |= EV_ABSORB | EV_SMASH;
            }

            const int dt = t - 145;
            const int hop = (dt < 16) ? (dt < 8 ? dt * 2 : (16 - dt) * 2) : 0;
            _fs_vic->set_position(76, 18 - hop);
            _fs_vic->set_tiles(vic_def->sprite->tiles_item(), sheet_frame(DIR_LEFT, dt < 18));

            const bn::fixed glow = dt < 24 ? (bn::fixed(24 - dt) * bn::fixed(0.03)) : bn::fixed(0);
            _fs_vic_pal->set_fade(bn::color(10, 31, 18), glow);

            if(dt < 14)
            {
                _fs_spark->set_visible(true);
                _fs_spark->set_position(76, 10 - hop);
                _fs_spark->set_tiles(bn::sprite_items::hit_spark.tiles_item(), dt * 3 / 14);
            }
            else { _fs_spark->set_visible(false); }

            if(t >= 175) { set_flash(bn::fixed(t - 175) / 12); }
            if(t >= 187)
            {
                unload_fs_gfx();
                _phase = PH_FS_EXIT;
            }
        }
