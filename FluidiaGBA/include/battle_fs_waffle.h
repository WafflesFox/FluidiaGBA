// ================================================================================================
// Final smash cutscene: Waffles' fire (the default final smash)
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        void build_waffle_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const FighterDef* vic_def = fighter_def(_fs_victim);

            _fs_bg = bn::regular_bg_items::fs_bg.create_bg(0, 0);
            _fs_bg->set_priority(3);

            _fs_atk_mat = bn::sprite_affine_mat_ptr::create();
            _fs_atk_mat->set_scale(1.5);
            _fs_atk_pal = bn::sprite_palette_ptr::create_new(atk_def->sprite->palette_item());
            if(_fs_attacker == 1)      { _fs_atk_pal->set_fade(bn::color(31, 6, 6), 0.35); }
            else if(_fs_attacker == 2) { _fs_atk_pal->set_fade(bn::color(6, 12, 31), 0.35); }

            _fs_atk = atk_def->sprite->create_sprite(-55, 18, sheet_frame(DIR_RIGHT, false));
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

            _fs_vic = vic_def->sprite->create_sprite(-30, 18, safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_LEFT, false)));
            _fs_vic->set_bg_priority(0);
            _fs_vic->set_z_order(-1);
            _fs_vic->set_affine_mat(*_fs_vic_mat);
            _fs_vic->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_vic->set_palette(*_fs_vic_pal);

            _fs_spark = bn::sprite_items::hit_spark.create_sprite(-30, 8, 0);
            _fs_spark->set_bg_priority(0);
            _fs_spark->set_z_order(-3);

            for(int i = 0; i < 2; ++i)
            {
                _fs_aura[i] = bn::sprite_items::fire_aura.create_sprite(-35, 14, i);
                _fs_aura[i]->set_bg_priority(0);
                _fs_aura[i]->set_z_order(i == 0 ? 0 : -2);
                _fs_aura[i]->set_visible(false);
            }

            _fs_beam_mat = bn::sprite_affine_mat_ptr::create();
            _fs_beam_mat->set_scale(1.05, 1.5);
            for(int i = 0; i < 5; ++i)
            {
                _fs_beam[i] = bn::sprite_items::fire_beam.create_sprite(0, 12, 0);
                _fs_beam[i]->set_bg_priority(0);
                _fs_beam[i]->set_z_order(-2);
                _fs_beam[i]->set_affine_mat(*_fs_beam_mat);
                _fs_beam[i]->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                _fs_beam[i]->set_visible(false);
            }

            _fs_impact = bn::sprite_items::fire_impact.create_sprite(55, 12, 0);
            _fs_impact->set_bg_priority(0);
            _fs_impact->set_z_order(-3);
            _fs_impact->set_visible(false);
            (void)text;
        }

        void update_waffle_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const FighterDef* vic_def = fighter_def(_fs_victim);
            const int t = _fs_timer++;

            if(t <= 10) { set_flash(bn::fixed(10 - t) / 10); }

            if(t < 20)
            {
                _fs_bg->set_x(-t * 2);
                const int pose = safe_frame(*atk_def->sprite, attack_frame(ANIM_TAIL_UP, 1, DIR_RIGHT, t * 2), sheet_frame(DIR_RIGHT, true) + 2);
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), pose);
                const int hop = (t < 8) ? -t : -(16 - t);
                _fs_atk->set_position(-55, 18 + (hop < 0 ? hop : 0));

                if(t < 10)
                {
                    _fs_spark->set_visible(true);
                    _fs_spark->set_tiles(bn::sprite_items::hit_spark.tiles_item(), t * 3 / 10);
                }
                else { _fs_spark->set_visible(false); }

                const int vx = -30 + t * 11;
                const int vy = 18 - t * 6;
                _fs_vic->set_tiles(vic_def->sprite->tiles_item(), safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_LEFT, false)));
                _fs_vic->set_position(vx, vy);
                _fs_vic->set_visible(vx < 155 && vy > -105);
                return;
            }
            _fs_spark->set_visible(false);

            if(t < 39)
            {
                const int dt = t - 20;
                const int ax = -55 + dt * 20 / 18;
                const int arc = (dt * (18 - dt)) / 3;
                _fs_atk->set_position(ax, 18 - arc);
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), sheet_frame(DIR_RIGHT, true) + 1);
                _fs_vic->set_visible(false);
                _fs_bg->set_x(-t * 2);
                return;
            }

            if(t < 78)
            {
                if(t == 39)
                {
                    text.set_center_alignment();
                    text.generate(0, -58, "FIRE BLAST!", _fs_banner);
                    for(bn::sprite_ptr& s : _fs_banner) { s.set_bg_priority(0); s.set_z_order(-10); }
                }
                if(t == 47) { events |= EV_ABSORB; }
                const bn::fixed glow = bn::fixed(t - 39) / 38;
                _fs_atk_pal->set_fade(bn::color(31, 14, 0), bn::fixed(0.2) + glow * bn::fixed(0.55));
                const int pose = (t < 57) ? WAF_UP_JET : sheet_frame(DIR_RIGHT, true) + 1;
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), pose);
                const int jx = (t % 4 < 2) ? -1 : 1;
                _fs_atk->set_position(-35 + jx, 16);

                for(int i = 0; i < 2; ++i)
                {
                    _fs_aura[i]->set_visible(true);
                    _fs_aura[i]->set_tiles(bn::sprite_items::fire_aura.tiles_item(), ((t / 4) + i) % 3);
                    _fs_aura[i]->set_position(-35 + (i == 0 ? -6 : 6), 12 - (t % 6));
                }
                _fs_bg->set_x(-t * 2);
                return;
            }

            if(t == 78)
            {
                _fs_banner.clear();
                for(bn::optional<bn::sprite_ptr>& a : _fs_aura) { a->set_visible(false); }
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), sheet_frame(DIR_RIGHT, true) + 2);
                _fs_atk_pal->set_fade(bn::color(31, 18, 2), bn::fixed(0.65));
                events |= EV_BOOM;
            }

            if(t >= 113 && t < 148)
            {
                _fs_cam_x += 7;
                if(_fs_cam_x > 240) { _fs_cam_x = 240; }
            }

            const int shake_y = (t % 4 < 2) ? -2 : 2;
            const int shake_x = (t >= 143 && t < 198) ? ((t % 2 == 0) ? 3 : -3) : 0;

            _fs_bg->set_x(-(t * 6) - _fs_cam_x);
            _fs_atk->set_position(-35 - _fs_cam_x, 16 + shake_y);
            _fs_atk->set_visible(-35 - _fs_cam_x > -150);

            const int beam_reach = t < 90 ? (t - 78) * 28 : 340;
            const int beam_frame = (t / 2) % 3;
            const bn::fixed beam_sy = (t % 4 < 2) ? bn::fixed(1.45) : bn::fixed(1.75);
            _fs_beam_mat->set_scale(1.05, beam_sy);

            for(int i = 0; i < 5; ++i)
            {
                const int world_x = 22 + i * 64;
                const int screen_x = world_x - _fs_cam_x;
                const bool in_range = (i * 64 < beam_reach) && (screen_x > -160) && (screen_x < 160);
                _fs_beam[i]->set_visible(in_range);
                if(in_range)
                {
                    _fs_beam[i]->set_tiles(bn::sprite_items::fire_beam.tiles_item(), beam_frame);
                    _fs_beam[i]->set_position(screen_x, 12 + shake_y);
                }
            }

            const int vic_sx = 295 - _fs_cam_x + shake_x;
            const int vic_drop = (t < 143) ? -(143 - t) * 2 : 0;
            const int vic_sy = 16 + (vic_drop < -80 ? -80 : vic_drop) + shake_y;
            _fs_vic->set_position(vic_sx, vic_sy);
            _fs_vic->set_visible(vic_sx < 155);

            if(t >= 143 && t < 198)
            {
                if(t == 145 || t == 157 || t == 169 || t == 181) { events |= EV_HIT; }
                _fs_vic->set_tiles(vic_def->sprite->tiles_item(), safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_LEFT, false)));
                _fs_vic_pal->set_fade((t % 6 < 3) ? bn::color(31, 31, 31) : bn::color(31, 14, 0), 0.75);
                _fs_impact->set_visible(true);
                _fs_impact->set_tiles(bn::sprite_items::fire_impact.tiles_item(), (t / 3) % 3);
                _fs_impact->set_position(vic_sx - 8, 12 + shake_y);
            }
            else
            {
                _fs_impact->set_visible(false);
            }

            if(t >= 198)
            {
                set_flash(bn::fixed(t - 198) / 12);
            }
            if(t >= 210)
            {
                unload_fs_gfx();
                _phase = PH_FS_EXIT;
            }
        }
