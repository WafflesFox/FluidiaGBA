// ================================================================================================
// Drawing the whole battle each frame: camera, stage, effects, projectiles, HUD, bystander
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        // ---------------------------------------------------------------------------- drawing

        // the camera sits between the two fighters, and stops at the ends of the stage
        bn::fixed camera_target() const
        {
            const int length = _stage->geom.length;
            if(length <= 240) { return length / 2; }
            if(_fight.fs_combo_active)
            {
                if(_fight.fs_combo_tick >= 33 && _fight.fs_combo_tick <= 92)
                {
                    int tx = _fight.fs_combo_home_x;
                    if(tx < 120) { tx = 120; }
                    if(tx > length - 120) { tx = length - 120; }
                    return tx;
                }
                return length / 2;
            }
            if(_fight.fs_freeze_timer > 0 && _fight.fs_freeze_user >= 0)
            {
                int tx = _fight.f[_fight.fs_freeze_user].x16 / FP;
                if(tx < 120) { tx = 120; }
                if(tx > length - 120) { tx = length - 120; }
                return tx;
            }

            int focus = _net ? _local_idx : ((_debug_enabled && _fight.debug_ai_target == 0) ? 1 : 0);
            if(_fight.f[focus].hp <= 0 && _fight.f[2].active && !_fight.f[2].is_clone && _fight.f[2].hp > 0)
            {
                focus = 2;
            }
            const int foe = _fight.non_clone_foe_of(focus);
            const int px = _fight.f[focus].x16 / FP;
            const int ex = _fight.f[foe].x16 / FP;
            int tx = (abs_i(px - ex) <= 200) ? ((px + ex) / 2) : px;
            if(tx < 120) { tx = 120; }
            if(tx > length - 120) { tx = length - 120; }
            return tx;
        }

        void draw(bn::sprite_text_generator& text)
        {
            _cam_x += (camera_target() - _cam_x) / 6;                   // a smooth follow
            const bn::fixed cam = _cam_x + _shake;                      // (_shake is only used by the KO screen)

            // parallax: far layers move slowly, near layers keep up with the camera
            for(int i = 0; i < _stage->layer_count; ++i)
            {
                if(_bgs[i]) { _bgs[i]->set_x(-(cam - 120) * _stage->layers[i].speed_percent / 100); }
            }
            if(_floor_bg)
            {
                update_floor_dx(cam);
                _floor_dx_hbe->reload_values_ref();                   // only dx changes every frame
            }

            // the floor: five tiles that hop along so there is always floor under the screen
            const int left_edge = cam.integer() - 120;
            const int first_tile = (left_edge >= 0 ? left_edge / 64 : -((-left_edge + 63) / 64)) * 64;
            for(int i = 0; i < _floor.size(); ++i)
            {
                _floor[i].set_position(bn::fixed(first_tile + i * 64 + 32) - cam, _stage->geom.ground_y + 16 - 80);
            }

            for(Piece& piece : _pieces)
            {
                piece.sprite.set_position(bn::fixed(piece.wx) - cam, piece.wy - 80);
            }

            draw_bystander(cam);

            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                const Fighter& f = _fight.f[i];
                if(f.active && f.has_table)
                {
                    if(!_table_sprites[i])
                    {
                        _table_sprites[i] = bn::sprite_items::mc_props.create_sprite(0, 0, 0);
                        _table_sprites[i]->set_bg_priority(0);
                        _table_sprites[i]->set_z_order(8);
                    }
                    _table_sprites[i]->set_position(bn::fixed(f.table_x16 / FP) - cam, (f.table_y16 / FP) - 8 - 80);
                }
                else { _table_sprites[i].reset(); }
            }

            for(int k = 0; k < MAX_BLOCKS; ++k)
            {
                const PlacedBlock& b = _fight.blocks[k];
                if(b.active)
                {
                    if(!_block_sprites[k])
                    {
                        _block_sprites[k] = bn::sprite_items::mc_props.create_sprite(0, 0, 1);
                        _block_sprites[k]->set_bg_priority(0);
                        _block_sprites[k]->set_z_order(9);
                    }
                    _block_sprites[k]->set_position(bn::fixed(b.x + b.w / 2) - cam, b.y + 8 - 80);
                    _block_sprites[k]->set_visible(b.life > 45 || (b.life / 4) % 2 == 0);
                }
                else { _block_sprites[k].reset(); }
            }

            for(int i = 3; i < MAX_FIGHTERS; ++i)
            {
                const Fighter& cf = _fight.f[i];
                if(cf.active && cf.state != ST_KO)
                {
                    if(!_views[i] && cf.clone_of >= 0)
                    {
                        const FighterDef* cdef = (cf.clone_of == 100 && _waffle_def) ? _waffle_def : fighter_def(cf.clone_of);
                        const int ctint = (cf.clone_of == 100) ? 3 : 2;
                        _views[i] = FighterView(*cdef, Fight::team_of(i) == 1, "", text, false, ctint, true);
                    }
                }
                else { _views[i].reset(); }
            }

            bool show_chg = false;
            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                if(_views[i] && _fight.f[i].active)
                {
                    draw_fighter(i, *_views[i], _fight.f[i], cam, text);
                    const Fighter& f = _fight.f[i];
                    if(!show_chg && !f.is_clone && f.state == ST_ATTACK && f.move && (f.move->charges_clone || (f.move->charges_power && f.move_frame >= 10)) && !_ko_overlay_hidden)
                    {
                        show_chg = true;
                        if(!_chg_back)
                        {
                            _chg_mat = bn::sprite_affine_mat_ptr::create();
                            _chg_back = bn::sprite_items::hp_bar.create_sprite(0, 0, 0);
                            _chg_back->set_bg_priority(0);
                            _chg_back->set_z_order(-9);
                            _chg_fill = bn::sprite_items::hp_bar.create_sprite(0, 0, 1);
                            _chg_fill->set_bg_priority(0);
                            _chg_fill->set_z_order(-10);
                            _chg_fill->set_affine_mat(*_chg_mat);
                        }
                        const bn::fixed cx = bn::fixed(f.x16 / FP) - cam;
                        const bn::fixed cy = (f.y16 / FP) - f.stats->body_h - 10 - 80;
                        const int cur_chg = f.move->charges_power ? f.har_charge : f.clone_charge;
                        const int max_chg = f.move->charges_power ? HAR_CHARGE_MAX : CLONE_CHARGE_MAX;
                        bn::fixed ratio = bn::fixed(cur_chg) / max_chg;
                        if(ratio < bn::fixed(0.04)) { ratio = bn::fixed(0.04); }
                        if(ratio > 1) { ratio = 1; }
                        _chg_mat->set_horizontal_scale(ratio);
                        _chg_back->set_position(cx, cy);
                        _chg_fill->set_position(cx - 16 + bn::fixed(16) * ratio, cy);
                    }
                }
            }
            if(!show_chg) { _chg_back.reset(); _chg_fill.reset(); _chg_mat.reset(); }

            if(_fight.fs_freeze_timer > 0 && _fight.fs_freeze_user >= 0 && !_ko_overlay_hidden)
            {
                const Fighter& u = _fight.f[_fight.fs_freeze_user];
                const int ux = u.x16 / FP;
                const int uy = u.y16 / FP;
                const int elapsed = FS_FREEZE_FRAMES - _fight.fs_freeze_timer;
                for(int i = 0; i < 2; ++i)
                {
                    const int frame = (elapsed / 3 + i) % 3;
                    if(!_freeze_aura[i])
                    {
                        _freeze_aura[i] = bn::sprite_items::fire_aura.create_sprite(0, 0, frame);
                        _freeze_aura[i]->set_bg_priority(0);
                        _freeze_aura[i]->set_z_order(i == 0 ? 1 : -2);
                    }
                    _freeze_aura[i]->set_tiles(bn::sprite_items::fire_aura.tiles_item(), frame);
                    _freeze_aura[i]->set_position(bn::fixed(ux + (i == 0 ? -7 : 7)) - cam, uy - 14 - 80);
                }
            }
            else
            {
                _freeze_aura[0].reset();
                _freeze_aura[1].reset();
            }

            for(int k = 0; k < MAX_PROJECTILES; ++k)
            {
                const Projectile& p = _fight.proj[k];
                if(!p.active) { _proj[k].reset(); _proj_id[k] = -1; _label[k].clear(); _label_id[k] = -1; continue; }

                if(p.shot.label)                                          // Filip's words and thrown objects: drawn as text for now
                {
                    _proj[k].reset();
                    if(_label_id[k] != p.id)                              // a new one: make its letters once (centered on 0, 0)
                    {
                        _label_id[k] = p.id;
                        _label[k].clear();
                        _label_dx[k].clear();
                        text.set_center_alignment();
                        text.generate(0, 0, p.shot.label, _label[k]);
                        for(bn::sprite_ptr& letter : _label[k])
                        {
                            _label_dx[k].push_back(letter.x());
                            letter.set_bg_priority(0);
                            letter.set_z_order(-2);
                        }
                    }
                    for(int j = 0; j < _label[k].size(); ++j)
                    {
                        _label[k][j].set_position(_label_dx[k][j] + (bn::fixed(p.x16 / FP) - cam), (p.y16 / FP) - 80);
                    }
                    continue;
                }

                const ProjArt art = projectile_art(p.shot.kind);
                if(!_proj[k] || _proj_id[k] != p.id)
                {
                    _proj_id[k] = p.id;
                    if(art.item == &bn::sprite_items::mc_rides)
                    {
                        _proj[k] = bn::sprite_ptr::create(
                            bn::sprite_items::mc_rides.shape_size(),
                            bn::sprite_items::mc_rides.tiles_item().create_tiles(art.first),
                            bn::sprite_items::mc_props.palette_item().create_palette());
                    }
                    else
                    {
                        _proj[k] = art.item->create_sprite(0, 0, art.first);
                    }
                    _proj[k]->set_bg_priority(0);
                    _proj[k]->set_z_order(-2);
                }
                _proj[k]->set_position(bn::fixed(p.x16 / FP) - cam, (p.y16 / FP) - 80);
                _proj[k]->set_horizontal_flip(p.vx < 0);
                const int art_frame = (p.shot.kind == PROJ_ARROW) ? (p.vy > 0 ? 1 : 0)
                                    : (p.shot.kind == PROJ_TNT)   ? ((p.vx == 0 && p.age >= TNT_ARM_FRAMES && (p.age % 24 < 6)) ? 1 : 0)
                                    : (p.age / art.ticks) % art.frames;
                _proj[k]->set_tiles(art.item->tiles_item(), art.first + art_frame);
            }

            // hit sparks & harmless dust puffs
            for(int k = 0; k < MAX_EFFECTS; ++k)
            {
                const Effect& e = _fight.fx[k];
                if(!e.active) { _fx[k].reset(); _fx_kind[k] = -1; continue; }

                const bool is_dust = (e.kind == FX_DUST);
                const bn::sprite_item& item = is_dust ? bn::sprite_items::mc_props : bn::sprite_items::hit_spark;
                const int first = is_dust ? 12 : 0;
                const int total = is_dust ? 4 : 3;
                int frame = e.age * total / e.life;
                if(frame >= total) { frame = total - 1; }
                if(!_fx[k] || _fx_kind[k] != e.kind)
                {
                    _fx_kind[k] = e.kind;
                    _fx[k] = item.create_sprite(0, 0, first + frame);
                    _fx[k]->set_bg_priority(0);
                    _fx[k]->set_z_order(-5);
                }
                _fx[k]->set_position(bn::fixed(e.x16 / FP) - cam, (e.y16 / FP) - 80);
                _fx[k]->set_tiles(item.tiles_item(), first + frame);
            }
        }
        // fills the per-row start positions for the camera: the camera is the picture position that sits at the screen centre
        void update_floor_dx(bn::fixed cam)
        {
            const int front = _stage->geom.ground_y;
            const int cam_texels = cam.data() >> 4;               // 12 fractional bits -> the 8 the hardware uses
            for(int row = front - FLOOR_ROWS; row < front; ++row)
            {
                detail::floor_dx_buf[row] = cam_texels - detail::floor_pa_buf[row] * 120;   // 120 = half the screen width
            }
        }

        // The follower that can't fight wanders left and right behind the fighters: a few steps, a rest, a few steps...
        // (it only decorates the stage: nothing can hit it and it can't hit anything)
        void draw_bystander(bn::fixed cam)
        {
            if(!_bystander) { return; }

            if(--_by_timer <= 0)                                      // decide what to do next
            {
                _by_rng ^= _by_rng << 13;
                _by_rng ^= _by_rng >> 17;
                _by_rng ^= _by_rng << 5;
                _by_walking = (_by_rng % 100) < 70;
                if(_by_walking) { _by_dir = ((_by_rng >> 8) & 1) ? DIR_RIGHT : DIR_LEFT; }
                _by_timer = 40 + int((_by_rng >> 12) % 70);
            }

            if(_by_walking)
            {
                _by_x16 += (_by_dir == DIR_RIGHT ? 1 : -1) * (FP / 2);            // half a pixel per frame
                const int min_x16 = 16 * FP;
                const int max_x16 = (_stage->geom.length - 16) * FP;
                if(_by_x16 < min_x16) { _by_x16 = min_x16; _by_dir = DIR_RIGHT; }
                if(_by_x16 > max_x16) { _by_x16 = max_x16; _by_dir = DIR_LEFT; }
                if(++_by_anim >= 8) { _by_anim = 0; _by_step = (_by_step + 1) % 4; }
            }
            else { _by_anim = 0; _by_step = 0; }

            const int frame = sheet_frame(_by_dir, false) + (_by_walking ? walk_frames[_by_step] : 0);
            if(frame != _by_shown)
            {
                _by_shown = frame;
                _bystander->set_tiles(_bystander_item->tiles_item(), frame);
            }
            _bystander->set_position(bn::fixed(_by_x16 / FP) - cam, _stage->geom.ground_y - SPRITE_FEET - 80);       // feet on the ground, same line as the fighters
        }
