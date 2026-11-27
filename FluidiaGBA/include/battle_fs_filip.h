// ================================================================================================
// Final smash cutscene: Filip's ring drop
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        // ---- FS_FILIP_DROP cutscene: Ring -> Fence -> POV Lights -> Ring Explosion ----
        void build_filip_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const FighterDef* vic_def = fighter_def(_fs_victim);

            _fs_bg = bn::regular_bg_items::ring_bg.create_bg(136, 0);
            _fs_bg->set_priority(3);

            _fs_atk_mat = bn::sprite_affine_mat_ptr::create();
            _fs_atk_mat->set_scale(1.5);
            _fs_atk_pal = bn::sprite_palette_ptr::create_new(atk_def->sprite->palette_item());
            if(_fs_attacker == 1)      { _fs_atk_pal->set_fade(bn::color(31, 6, 6), 0.35); }
            else if(_fs_attacker == 2) { _fs_atk_pal->set_fade(bn::color(6, 12, 31), 0.35); }

            _fs_atk = atk_def->sprite->create_sprite(272, -34, sheet_frame(DIR_LEFT, false));
            _fs_atk->set_bg_priority(0);
            _fs_atk->set_z_order(-2);
            _fs_atk->set_affine_mat(*_fs_atk_mat);
            _fs_atk->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_atk->set_palette(*_fs_atk_pal);

            _fs_vic_mat = bn::sprite_affine_mat_ptr::create();
            _fs_vic_mat->set_scale(1.5);
            _fs_vic_pal = bn::sprite_palette_ptr::create_new(vic_def->sprite->palette_item());
            if(_fs_victim == 1)      { _fs_vic_pal->set_fade(bn::color(31, 6, 6), 0.35); }
            else if(_fs_victim == 2) { _fs_vic_pal->set_fade(bn::color(6, 12, 31), 0.35); }

            _fs_vic = vic_def->sprite->create_sprite(0, -70, safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_LEFT, false)));
            _fs_vic->set_bg_priority(0);
            _fs_vic->set_z_order(-1);
            _fs_vic->set_affine_mat(*_fs_vic_mat);
            _fs_vic->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_vic->set_palette(*_fs_vic_pal);

            _fs_beam_mat = bn::sprite_affine_mat_ptr::create();
            _fs_beam_mat->set_scale(1.85);
            _fs_impact = bn::sprite_items::fire_impact.create_sprite(0, 26, 0);
            _fs_impact->set_bg_priority(0);
            _fs_impact->set_z_order(-3);
            _fs_impact->set_affine_mat(*_fs_beam_mat);
            _fs_impact->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_impact->set_visible(false);

            for(int i = 0; i < 3; ++i)
            {
                _fs_beam[i] = bn::sprite_items::explosion.create_sprite(0, 26, 0);
                _fs_beam[i]->set_bg_priority(0);
                _fs_beam[i]->set_z_order(-4);
                _fs_beam[i]->set_visible(false);
            }

            text.set_center_alignment();
            text.generate(0, -58, "RING DROP!", _fs_banner);
            for(bn::sprite_ptr& sp : _fs_banner) { sp.set_bg_priority(0); sp.set_z_order(-10); }
        }

        void update_filip_fs(bn::sprite_text_generator& text)
        {
            (void)text;
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const FighterDef* vic_def = fighter_def(_fs_victim);
            const int t = _fs_timer++;

            // ---- Scene 1 (0..68): Victim lands in ring -> camera pans to Filip on fence -> jumps high ----
            if(t < 69)
            {
                if(t <= 10) { set_flash(bn::fixed(10 - t) / 10); }
                if(t == 14) { events |= EV_HIT; _fs_banner.clear(); }

                if(t >= 18 && t < 49)
                {
                    _fs_cam_x += 9;
                    if(_fs_cam_x > 272) { _fs_cam_x = 272; }
                }
                _fs_bg->set_x(136 - _fs_cam_x);

                int vy = 32;
                if(t < 14)      { vy = -70 + t * 102 / 14; }
                else if(t < 20) { vy = 32 - (t < 17 ? (t - 14) * 3 : (20 - t) * 3); }
                const int vx = 0 - _fs_cam_x;
                _fs_vic->set_position(vx, vy);
                _fs_vic->set_visible(vx > -155);

                const int ax = 272 - _fs_cam_x;
                int ay = -34;
                int apose = sheet_frame(DIR_LEFT, false);
                if(t >= 49 && t < 55)
                {
                    ay = -32;
                }
                else if(t >= 55)
                {
                    if(t == 55) { events |= EV_JUMP; }
                    ay = -34 - (t - 55) * 8;
                    apose = sheet_frame(DIR_LEFT, true) + 1;
                }
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), apose);
                _fs_atk->set_position(ax, ay);
                _fs_atk->set_visible(ax < 155 && ay > -105);
                return;
            }

            if(t == 69)
            {
                _fs_bg.reset();
                _fs_vic->set_visible(false);
                _fs_atk->set_visible(false);
                set_flash(bn::fixed(0.65));
                return;
            }

            // ---- Scene 2 (70..123): Victim's POV looking up at boxing lights; Filip slides top -> bottom ----
            if(t == 70)
            {
                _fs_bg = bn::regular_bg_items::gym_lights_bg.create_bg(0, 0);
                _fs_bg->set_priority(3);
            }
            if(t >= 70 && t < 124)
            {
                if(t <= 78) { set_flash(bn::fixed(78 - t) * bn::fixed(0.65) / 8); }
                if(t == 84) { events |= EV_SWING; }

                const int dt = t - 70;
                const int sy = -92 + dt * 188 / 53;
                const bn::fixed sc = bn::fixed(0.75) + bn::fixed(dt) * bn::fixed(1.15) / 53;
                _fs_atk_mat->set_scale(sc);
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), sheet_frame(DIR_DOWN, true) + 1);
                _fs_atk->set_position(0, sy);
                _fs_atk->set_visible(true);
                _fs_bg->set_position(0, (t % 4 < 2) ? 0 : 1);
                return;
            }

            if(t == 124)
            {
                _fs_bg.reset();
                _fs_atk->set_visible(false);
                set_flash(bn::fixed(0.65));
                return;
            }

            // ---- Scene 3 (125..195): Filip lands on victim in ring -> MASSIVE EXPLOSION ----
            if(t == 125)
            {
                _fs_cam_x = 0;
                _fs_bg = bn::regular_bg_items::ring_bg.create_bg(136, 0);
                _fs_bg->set_priority(3);
                _fs_atk_mat->set_scale(1.5);
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), sheet_frame(DIR_DOWN, true) + 1);
                _fs_vic->set_tiles(vic_def->sprite->tiles_item(), safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_LEFT, false)));
                _fs_vic->set_visible(true);
            }

            int shake_x = 0, shake_y = 0;
            if(t < 137)
            {
                if(t <= 132) { set_flash(bn::fixed(132 - t) * bn::fixed(0.65) / 7); }
                const int ay = -86 + (t - 125) * 116 / 12;
                _fs_atk->set_position(0, ay);
                _fs_atk->set_visible(true);
                _fs_vic->set_position(0, 32);
            }
            else
            {
                if(t == 137) { events |= EV_BOOM | EV_SMASH; }
                if(t == 149) { events |= EV_BOOM | EV_HIT; }

                const int dt = t - 137;
                const int pwr = dt < 45 ? (7 - dt * 7 / 45) : 0;
                shake_x = (t % 2 == 0) ? pwr : -pwr;
                shake_y = (t % 4 < 2) ? pwr : -pwr;

                if(t <= 152) { set_flash(bn::fixed(152 - t) / 15); }

                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), sheet_frame(DIR_DOWN, false));
                _fs_atk->set_position(shake_x, 26 + shake_y);
                _fs_vic->set_position(shake_x, 32 + shake_y);
                _fs_vic_pal->set_fade((t % 6 < 3) ? bn::color(31, 31, 31) : bn::color(31, 14, 0), 0.75);

                if(dt < 40)
                {
                    const bn::fixed imp_scale = bn::fixed(1.65) + bn::fixed(dt % 4) * bn::fixed(0.08);
                    _fs_beam_mat->set_scale(imp_scale);
                    _fs_impact->set_visible(true);
                    _fs_impact->set_tiles(bn::sprite_items::fire_impact.tiles_item(), (dt / 3) % 3);
                    _fs_impact->set_position(shake_x, 22 + shake_y);

                    static const int bx[3] = { -28, 28, 0 };
                    static const int by[3] = { 28, 28, -4 };
                    for(int i = 0; i < 3; ++i)
                    {
                        const int e_age = (dt + i * 3) % 16;
                        _fs_beam[i]->set_visible(dt < 34);
                        _fs_beam[i]->set_tiles(bn::sprite_items::explosion.tiles_item(), (e_age / 4) % 4);
                        _fs_beam[i]->set_position(bx[i] + shake_x, by[i] + shake_y);
                    }
                }
                else
                {
                    _fs_impact->set_visible(false);
                    for(int i = 0; i < 3; ++i) { _fs_beam[i]->set_visible(false); }
                }
            }
            _fs_bg->set_position(136 + shake_x, shake_y);

            if(t >= 183)
            {
                set_flash(bn::fixed(t - 183) / 12);
            }
            if(t >= 195)
            {
                unload_fs_gfx();
                _phase = PH_FS_EXIT;
            }
        }
