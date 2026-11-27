// ================================================================================================
// Building and tearing down the stage: backgrounds, floor, wipe blocks, banner, flash, unloading
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        // ---------------------------------------------------------------------------- building the stage

        void build_stage_bgs()
        {
            const StageDef& stage = *_stage;
            for(int i = 0; i < stage.layer_count; ++i)
            {
                _bgs[i] = stage.layers[i].item->create_bg(0, 0);
                _bgs[i]->set_priority(stage.layers[i].in_front ? 0 : 3 - i);
            }
            if(_use_3d && stage.top_item)
            {
                const int front = stage.geom.ground_y;
                const int horizon = front - FLOOR_DEPTH;

                for(int row = 0; row < bn::display::height(); ++row)
                {
                    int d = row - horizon;
                    if(d < 1) { d = 1; }
                    detail::floor_pa_buf[row] = int16_t(FLOOR_DEPTH * 256 / d);
                    detail::floor_dy_buf[row] = FLOOR_FOCAL * FLOOR_DEPTH * 256 / d;
                }

                _floor_bg = stage.top_item->create_bg(0, 0);
                _floor_bg->set_priority(1);
                _floor_bg->set_wrapping_enabled(true);

                update_floor_dx(_cam_x);
                _floor_pa_hbe = bn::affine_bg_pa_register_hbe_ptr::create(*_floor_bg, detail::floor_pa_buf);
                _floor_dx_hbe = bn::affine_bg_dx_register_hbe_ptr::create(*_floor_bg, detail::floor_dx_buf);
                _floor_dy_hbe = bn::affine_bg_dy_register_hbe_ptr::create(*_floor_bg, detail::floor_dy_buf);

                bn::rect_window win = bn::rect_window::internal();
                win.set_boundaries(front - FLOOR_ROWS - 80, -120, front - 80, 120);
                win.set_visible(true);
                bn::window::outside().set_show_bg(*_floor_bg, false);
            }
        }

        void build_scene(bn::sprite_text_generator& text)
        {
            const StageDef& stage = *_stage;
            const bn::sprite_item& floor_art    = stage.floor_item    ? *stage.floor_item    : bn::sprite_items::battle_floor;
            const bn::sprite_item& platform_art = stage.platform_item ? *stage.platform_item : bn::sprite_items::battle_platform;
                        build_stage_bgs();
            (void)stage;


            for(int i = 0; i < FLOOR_TILES; ++i)
            {
                // bn::sprite_ptr tile = bn::sprite_items::battle_floor.create_sprite(0, 0);
                bn::sprite_ptr tile  = floor_art.create_sprite(0, 0);
                tile.set_bg_priority(0);
                tile.set_z_order(10);
                _floor.push_back(tile);
            }

            for(int k = 0; k < stage.geom.platform_count; ++k)
            {
                const Platform& pl = stage.geom.platforms[k];
                for(int j = 0; j < pl.w / 32 && !_pieces.full(); ++j)  // platforms are built from 32-pixel pieces
                {
                    // bn::sprite_ptr piece = bn::sprite_items::battle_platform.create_sprite(0, 0);
                    bn::sprite_ptr piece = platform_art.create_sprite(0, 0); 
                    piece.set_bg_priority(0);
                    piece.set_z_order(10);
                    _pieces.push_back(Piece{ piece, pl.x + 16 + j * 32, pl.y + 4 });
                }
            }

            if(_net)
            {
                _views[0] = FighterView(*_player, false, _local_idx == 0 ? "YOU" : "P1", text, false, _use_tint ? (_local_idx == 0 ? 0 : 1) : 0);
                _views[1] = FighterView(*_enemy,  true,  _local_idx == 1 ? "YOU" : "P2", text, false, _use_tint ? (_local_idx == 1 ? 0 : 1) : 0);
            }
            else
            {
                _views[0] = FighterView(*_player, false, "YOU", text, false, 0);
                _views[1] = FighterView(*_enemy, true, _enemy->name, text, false, _use_tint ? 1 : 0);
            }
            if(_partner) { _views[2] = FighterView(*_partner, false, _partner->name, text, true, _use_tint ? 2 : 0); }
            if(_ko_overlay_hidden) { return; }

            for(int k = 0; k < MAX_PROJECTILES; ++k) { _label[k].clear(); _label_id[k] = -1; }
            for(int k = 0; k < MAX_PROJECTILES; ++k) { _proj_id[k] = -1; }

            if(_bystander_item)                                       // the follower that can't fight: it just hangs around
            {
                _bystander = _bystander_item->create_sprite(0, 0, sheet_frame(DIR_DOWN, false));
                _bystander->set_bg_priority(0);
                _bystander->set_z_order(3);                           // behind every fighter, in front of the floor
                _by_shown = sheet_frame(DIR_DOWN, false);
                _by_x16 = (_stage->geom.length / 2) * FP;
                _by_timer = 1;
            }

            _cam_x = camera_target();
            draw(text);
        }

        // ---------------------------------------------------------------------------- the wipe

        void make_blocks()
        {
            for(int row = 0; row < 3; ++row)
            {
                for(int col = 0; col < 4; ++col)
                {
                    bn::sprite_ptr block = bn::sprite_items::black_block.create_sprite(
                            bn::fixed(-96 + col * 64), bn::fixed(-64 + row * 64));
                    block.set_bg_priority(0);
                    block.set_z_order(-32000);
                    _blocks.push_back(block);
                }
            }
        }

        void move_blocks()
        {
            for(int i = 0; i < _blocks.size(); ++i)
            {
                _blocks[i].set_position(bn::fixed(-96 + (i % 4) * 64) + _wipe, bn::fixed(-64 + (i / 4) * 64));
            }
        }

        // ---------------------------------------------------------------------------- text banner

        void show_banner(bn::sprite_text_generator& text, const char* line)
        {
            _banner.clear();
            text.set_center_alignment();
            text.generate(0, -10, line, _banner);
            for(bn::sprite_ptr& s : _banner) { s.set_bg_priority(0); s.set_z_order(-20); }
        }

        // a flash over the whole screen: 1 = pure white, 0 = nothing
        void set_flash(bn::fixed intensity)
        {
            bn::bg_palettes::set_fade(bn::color(31, 31, 31), intensity);
            bn::sprite_palettes::set_fade(bn::color(31, 31, 31), intensity);
        }

        void collect_events()
        {
            const Events& e = _fight.ev;
            if(e.shot)    { events |= EV_SHOT; }
            if(e.swing)   { events |= EV_SWING; }
            if(e.dash)    { events |= EV_DASH; }
            if(e.absorb)  { events |= EV_ABSORB; }
            if(e.boom)    { events |= EV_BOOM; }
            if(e.hit)     { events |= EV_HIT; }
            if(e.hitsmash){ events |= EV_SMASH;}
            if(e.blocked) { events |= EV_BLOCKED; }
            if(e.jump)    { events |= EV_JUMP; }
            if(e.ko)      { events |= EV_KO; }
            if(e.mining)  { events |= EV_MINING;}
            if(e.grab)    { events |= EV_GRAB;}
            if(e.final_smash_start) { events |= EV_FINAL_SMASH; }
        }

        void unload_stage_bgs()
        {
            _floor_pa_hbe.reset();
            _floor_dx_hbe.reset();
            _floor_dy_hbe.reset();
            _floor_bg.reset();
            bn::rect_window::internal().set_visible(false);
            for(bn::optional<bn::regular_bg_ptr>& bg : _bgs) { bg.reset(); }
        }

        void set_ko_overlay_hidden(bool hidden)
        {
            _ko_overlay_hidden = hidden;
            for(bn::sprite_ptr& tile : _floor) { tile.set_visible(!hidden); }
            for(Piece& piece : _pieces) { piece.sprite.set_visible(!hidden); }
            if(_bystander) { _bystander->set_visible(!hidden); }
            if(_chg_back) { _chg_back->set_visible(!hidden); }
            if(_chg_fill) { _chg_fill->set_visible(!hidden); }
            for(int i = 0; i < MAX_FIGHTERS; ++i)
            {
                if(!_views[i]) { continue; }
                FighterView& v = *_views[i];
                v.hp_back.set_visible(!hidden && !v.is_clone);
                v.hp_fill.set_visible(!hidden && !v.is_clone && _fight.f[i].hp > 0);
                for(bn::sprite_ptr& sp : v.name) { sp.set_visible(!hidden); }
                for(bn::sprite_ptr& sp : v.hp_text) { sp.set_visible(!hidden); }
                for(bn::sprite_ptr& sp : v.craft_text) { sp.set_visible(!hidden); }
                if(hidden)
                {
                    v.shield.reset();
                    v.sword.reset();
                    v.slash_fx.reset();
                    v.jet.reset();
                    v.held.reset();
                    v.held_kind = -1;
                    v.pig.reset();
                    for(bn::optional<bn::sprite_ptr>& sw : v.swipes) { sw.reset(); }
                }
            }
            for(bn::optional<bn::sprite_ptr>& sp : _table_sprites) { if(sp) { sp->set_visible(!hidden); } }
            for(bn::optional<bn::sprite_ptr>& sp : _block_sprites) { if(sp) { sp->set_visible(!hidden); } }
            for(bn::optional<bn::sprite_ptr>& sp : _freeze_aura)  { if(sp) { sp->set_visible(!hidden); } }
            if(hidden)
            {
                for(int k = 0; k < MAX_PROJECTILES; ++k)
                {
                    _proj[k].reset();
                    _proj_id[k] = -1;
                    _label[k].clear();
                    _label_id[k] = -1;
                }
                for(int k = 0; k < MAX_EFFECTS; ++k) { _fx[k].reset(); _fx_kind[k] = -1; }
            }
        }

        void unload_stage_gfx()
        {
            _dbg_box.clear();
            _dbg_text.clear();
            for(int i = 0; i < MAX_FIGHTERS; ++i) { _views[i].reset(); }
            _chg_back.reset();
            _chg_fill.reset();
            _chg_mat.reset();
            for(bn::optional<bn::sprite_ptr>& sp : _table_sprites) { sp.reset(); }
            for(bn::optional<bn::sprite_ptr>& sp : _block_sprites) { sp.reset(); }
            for(bn::optional<bn::sprite_ptr>& sp : _freeze_aura)  { sp.reset(); }
            _bystander.reset();
            for(int k = 0; k < MAX_PROJECTILES; ++k) { _label[k].clear(); _label_id[k] = -1; }
            for(int k = 0; k < MAX_PROJECTILES; ++k) { _proj_id[k] = -1; }
            for(bn::optional<bn::sprite_ptr>& p : _proj) { p.reset(); }
            for(int k = 0; k < MAX_EFFECTS; ++k) { _fx[k].reset(); _fx_kind[k] = -1; }
            _floor.clear();
            _pieces.clear();
            _banner.clear();
            _ko_bg.reset();
            _ko_overlay_hidden = false;
            unload_stage_bgs();
        }

        const FighterDef* fighter_def(int idx) const
        {
            if(idx == 1) { return _enemy; }
            if(idx == 2 && _partner) { return _partner; }
            return _player;
        }

        void unload_fs_gfx()
        {
            _fs_slice_hbe_x.reset();
            _fs_slice_hbe_y.reset();
            _fs_banner.clear();
            _fs_spark.reset();
            _fs_impact.reset();
            _fs_cart.reset();
            _fs_cart_mat.reset();
            for(bn::optional<bn::sprite_ptr>& b : _fs_beam) { b.reset(); }
            for(bn::optional<bn::sprite_ptr>& a : _fs_aura) { a.reset(); }
            _fs_atk.reset();
            _fs_vic.reset();
            _fs_vic_bot.reset();
            _fs_vic_top_tiles.reset();
            _fs_vic_bot_tiles.reset();
            _fs_atk_mat.reset();
            _fs_vic_mat.reset();
            _fs_beam_mat.reset();
            _fs_atk_pal.reset();
            _fs_vic_pal.reset();
            _fs_bg.reset();
        }
