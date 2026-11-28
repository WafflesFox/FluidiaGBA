// ================================================================================================
// Drawing one fighter and the extra pictures around its moves (jet, swipes, held shot)
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        void draw_fighter(int index, FighterView& v, const Fighter& f, bn::fixed cam, bn::sprite_text_generator& text)
        {
            const FighterStats& s = *f.stats;
            const int fx = f.x16 / FP;
            const int fy = f.y16 / FP;

            const int ride_lift = f.on_pig ? 8 : 0;
            v.sprite.set_position(bn::fixed(fx) - cam, fy - SPRITE_FEET - ride_lift - 80);
            if(f.on_pig)
            {
                if(!v.pig)
                {
                    v.pig = bn::sprite_ptr::create(
                        bn::sprite_items::mc_rides.shape_size(),
                        bn::sprite_items::mc_rides.tiles_item().create_tiles(0),
                        bn::sprite_items::mc_props.palette_item().create_palette());
                    v.pig->set_bg_priority(0);
                    v.pig->set_z_order(0);
                }
                v.pig->set_tiles(bn::sprite_items::mc_rides.tiles_item(), (_fight.frame / 5) % 2);
                v.pig->set_position(bn::fixed(fx) - cam, fy - 8 - 80);
                v.pig->set_horizontal_flip(f.facing < 0);
            }
            else { v.pig.reset(); }

            // animation: the walk cycle advances while walking
            if(f.state == ST_WALK)
            {
                if(++v.anim_timer >= 5) { v.anim_timer = 0; v.step = (v.step + 1) % 4; }
            }
            else { v.step = 0; v.anim_timer = 0; }

            const int side = f.facing > 0 ? DIR_RIGHT : DIR_LEFT;
            int anim = f.move ? f.move->anim : ANIM_STAND;
            if(f.stats->spawns_crafting_table && f.weapon_tier == 0 &&
               (anim == ANIM_UMB_MINE || anim == ANIM_UMB_SLASH))
            {
                anim = ANIM_UMB_PLACE;
            }
            const int raw_frame = f.on_pig ? sheet_frame(side, false)
                                           : pick_frame(f.state, f.facing, v.step, anim, f.move_frame);
            const int fallback = sheet_frame(side, f.state == ST_ATTACK) + (f.state == ST_ATTACK ? 2 : 0);
            const int frame = safe_frame(*v.sheet, raw_frame, fallback);
            if(frame != v.shown_frame)                                  // only touch the graphics when the frame changes
            {
                v.shown_frame = frame;
                v.sprite.set_tiles(v.sheet->tiles_item(), frame);
            }
            v.sprite.set_visible(!(f.state == ST_HURT && (f.timer / 2) % 2 == 0));   // blink while flinching

            // the shield bubble shrinks as its energy (= its defense boost) runs down
            // (fox thief borrows the same bubble while it is ready to steal a shot)
            const Move* move = f.state == ST_ATTACK ? f.move : nullptr;
            const bool stealing = move && ((move->absorb_end > move->absorb_start && f.move_frame >= move->absorb_start && f.move_frame < move->absorb_end) ||
                                          (move->reflect_end > move->reflect_start && f.move_frame >= move->reflect_start && f.move_frame < move->reflect_end));
            if(f.state == ST_SHIELD)
            {
                if(!v.shield)
                {
                    v.shield = bn::sprite_items::shield.create_sprite(0, 0);
                    v.shield->set_bg_priority(0);
                    v.shield->set_z_order(-3);
                    v.shield->set_affine_mat(v.shield_mat);
                }
                const bn::fixed scale = (f.state == ST_SHIELD)
                    ? (bn::fixed(0.45) + bn::fixed(0.55) * f.shield / SHIELD_MAX)
                    : bn::fixed(0.9);
                v.shield_mat.set_scale(scale);
                v.shield->set_position(bn::fixed(fx) - cam, fy - s.body_h / 2 - 80);
            }
            else { v.shield.reset(); }
            

            draw_move_effects(index, v, f, move, cam);

            // the health bar: the green part is squeezed from the right
            if(!v.is_clone && f.hp != v.shown_hp)
            {
                v.shown_hp = f.hp;

                bn::fixed ratio = bn::fixed(f.hp) / s.max_hp;
                if(ratio < bn::fixed(0.02)) { ratio = bn::fixed(0.02); }
                v.hp_mat.set_horizontal_scale(ratio);
                const int left = v.on_right ? 80 : -112;
                v.hp_fill.set_x(bn::fixed(left) + bn::fixed(16) * ratio);
                v.hp_fill.set_visible(f.hp > 0);

                v.hp_text.clear();
                char number[8];
                format_int(f.hp, number);
                if(v.on_right) { text.set_right_alignment(); text.generate(72, -58, number, v.hp_text); }
                else           { text.set_left_alignment();  text.generate(-72, -58 + v.hud_dy, number, v.hp_text); }
                text.set_center_alignment();
            }

            if(f.stats->spawns_crafting_table &&
               (f.materials != v.shown_mat || f.weapon_tier != v.shown_tier || f.weapon_dur != v.shown_dur))
            {
                v.shown_mat = f.materials;
                v.shown_tier = f.weapon_tier;
                v.shown_dur = f.weapon_dur;
                v.craft_text.clear();
                char buf[20];
                int pos = 0;
                buf[pos++] = 'M';
                buf[pos++] = ':';
                if(f.materials >= 10) { buf[pos++] = char('0' + f.materials / 10); }
                buf[pos++] = char('0' + f.materials % 10);
                if(f.weapon_tier > 0)
                {
                    const char* tag = (f.weapon_tier >= 4) ? " DM:" :
                                      (f.weapon_tier == 3) ? " IR:" :
                                      (f.weapon_tier == 2) ? " ST:" : " WD:";
                    for(int k = 0; k < 4; ++k) { buf[pos++] = tag[k]; }
                    if(f.weapon_dur >= 10) { buf[pos++] = char('0' + f.weapon_dur / 10); }
                    buf[pos++] = char('0' + f.weapon_dur % 10);
                }
                buf[pos] = 0;
                if(v.on_right) { text.set_right_alignment(); text.generate(112, -46, buf, v.craft_text); }
                else           { text.set_left_alignment();  text.generate(-112, -46 + v.hud_dy, buf, v.craft_text); }
                text.set_center_alignment();
            }

            if(v.pal)
            {
                if(v.tint_mode == 3)                       // shadow clone: purple, pulsing
                {
                    const int pulse = _fight.frame % 24;
                    const int tri = pulse < 12 ? pulse : 24 - pulse;
                    v.pal->set_fade(bn::color(28, 16, 31), bn::fixed(tri) * bn::fixed(0.03));   // 0 .. 0.36
                }
                else if(v.tint_mode == 4)                  // water clone: steady cyan, nothing to do
                {
                }
                else if(_fight.fs_freeze_timer > 0 && index == _fight.fs_freeze_user)
                {
                    const int pulse = _fight.fs_freeze_timer % 8;
                    const int tri = pulse < 4 ? pulse : 8 - pulse;
                    v.pal->set_fade(bn::color(31, 26, 10), bn::fixed(0.45) + bn::fixed(tri) * bn::fixed(0.08));
                }
                else if(f.stats->final_smash_move != nullptr && f.smash >= SMASH_MAX && f.state != ST_KO)
                {
                    const int pulse = _fight.frame % 24;
                    const int tri = pulse < 12 ? pulse : 24 - pulse;
                    const bn::fixed glow = bn::fixed(0.20) + bn::fixed(tri) * bn::fixed(0.045);
                    v.pal->set_fade(bn::color(31, 22, 4), glow);
                }
                else if(f.har_power_timer > 0)
                {
                    const int pulse = _fight.frame % 16;
                    const int tri = pulse < 8 ? pulse : 16 - pulse;
                    v.pal->set_fade(bn::color(4, 16, 31), bn::fixed(0.20) + bn::fixed(tri) * bn::fixed(0.06));
                }else if(stealing)
                {
                    // Fast blue pulse while Fox Thief / Counter / Reflect window is active
                    const int pulse = _fight.frame % 8;
                    const int tri = pulse < 4 ? pulse : 8 - pulse;
                    v.pal->set_fade(bn::color(6, 22, 31), bn::fixed(0.30) + bn::fixed(tri) * bn::fixed(0.11));
                }
                else if(v.tint_mode == 1)
                {
                    v.pal->set_fade(bn::color(31, 6, 6), bn::fixed(0.35));
                }
                else if(f.state == ST_ATTACK && f.move && f.move->charges_clone)
                {
                    const int pulse = _fight.frame % 12;
                    const int tri = pulse < 6 ? pulse : 12 - pulse;
                    v.pal->set_fade(bn::color(6, 24, 31), bn::fixed(0.25) + bn::fixed(tri) * bn::fixed(0.07));
                }// else if(f.state == ST_ATTACK && f.move && f.state == stealing)
                // {
                //     const int pulse = _fight.frame % 12;
                //     const int tri = pulse < 6 ? pulse : 12 - pulse;
                //     v.pal->set_fade(bn::color(6, 24, 31), bn::fixed(0.25) + bn::fixed(tri) * bn::fixed(0.07));
                // }
                // else if(v.tint_mode == 3)
                // {
                //     v.pal->set_fade(bn::color(4, 22, 31), bn::fixed(0.55));
                // }
                else if(f.bomb_timer > 0 && (f.bomb_timer < 40 ? (_fight.frame % 6 < 3) : (_fight.frame % 16 < 4)))
                {
                    v.pal->set_fade(bn::color(31, 10, 2), bn::fixed(0.65));
                }
                else if(f.def_buff_timer > 0)
                {
                    const int pulse = _fight.frame % 20;
                    const int tri = pulse < 10 ? pulse : 20 - pulse;
                    v.pal->set_fade(bn::color(10, 30, 18), bn::fixed(0.20) + bn::fixed(tri) * bn::fixed(0.03));
                }
                else if(v.tint_mode == 2)
                {
                    v.pal->set_fade(bn::color(6, 12, 31), bn::fixed(0.35));
                }
                else
                {
                    v.pal->set_fade(bn::color(0, 0, 0), 0);
                }
            }
        }

        // The extra pictures around a move: the aqua jet bubble, the swipes where melee hitboxes are, and the shot
        // fox thief is holding. They are made when needed and thrown away when not.
        void draw_move_effects(int index, FighterView& v, const Fighter& f, const Move* move, bn::fixed cam)
        {
            const FighterStats& s = *f.stats;
            const int fx = f.x16 / FP;
            const int fy = f.y16 / FP;

            // the aqua jet: a bubble behind the character while it flies (the sheet has side / up / down pictures)
            if(move && f.driving)
            {
                const int base = move->drive.vy < 0 ? 2 : (move->drive.vy > 0 ? 4 : 0);
                const int frame = base + (f.move_frame / 2) % 2;
                if(!v.jet)
                {
                    v.jet = bn::sprite_items::aqua_jet.create_sprite(0, 0, frame);
                    v.jet->set_bg_priority(0);
                    v.jet->set_z_order(1);
                }
                v.jet->set_tiles(bn::sprite_items::aqua_jet.tiles_item(), frame);
                v.jet->set_position(bn::fixed(fx) - cam, fy - s.body_h / 2 - 80);
                v.jet->set_horizontal_flip(f.facing < 0);
            }
            else { v.jet.reset(); }

            // Harue 64x64 sword overlay + blue slashing effect
            int sword_idx = -1;
            int blue_idx = -1;
            bool sword_left = (f.facing < 0);
            if(move)
            {
                bool spin_left_half = false;
                const int step = har_slash_step(move->anim, f.move_frame, spin_left_half);
                if(step >= 0)
                {
                    sword_left = (move->anim == ANIM_HAR_SPIN) ? spin_left_half : (f.facing < 0);
                    sword_idx = step;
                    blue_idx = 2 - step;
                }
                else
                {
                    const int hold = har_hold_step(move->anim, f.move_frame);
                    if(hold >= 0) { sword_idx = hold; }      // sword stays, no blue slash
                }
                // else if(move->anim == ANIM_HAR_DASH && f.move_frame >= 19) { sword_idx = 0; }   // sword only, no blue slash
            }
            if(sword_idx >= 0)
            {
                if(!v.sword)
                {
                    v.sword = bn::sprite_items::harueslice_sword.create_sprite(0, 0, sword_idx);
                    v.sword->set_bg_priority(0);
                    v.sword->set_z_order(-4);
                }
                v.sword->set_tiles(bn::sprite_items::harueslice_sword.tiles_item(), sword_idx);
                v.sword->set_position(bn::fixed(fx) - cam, fy - SPRITE_FEET - 80);
                v.sword->set_horizontal_flip(sword_left);

                if(blue_idx >= 0)
                {
                    if(!v.slash_fx)
                    {
                        v.slash_fx = bn::sprite_items::blue_slash.create_sprite(0, 0, blue_idx);
                        v.slash_fx->set_bg_priority(0);
                        v.slash_fx->set_z_order(-3);
                    }
                    v.slash_fx->set_tiles(bn::sprite_items::blue_slash.tiles_item(), blue_idx);
                    v.slash_fx->set_position(bn::fixed(fx) + (sword_left ? -8 : 8) - cam, fy - SPRITE_FEET - 80);
                    v.slash_fx->set_horizontal_flip(sword_left);
                }
                else { v.slash_fx.reset(); }
            }
            else { v.sword.reset(); v.sword_mat.reset(); v.slash_fx.reset(); }

            // swipes: until you draw real attack animations, a little slash shows where each melee hitbox is
            int used = 0;
            if(move && move->drive.end <= move->drive.start && sword_idx < 0)
            {
                for(int k = 0; k < move->hit_count && used < 2; ++k)
                {
                    Rect box;
                    if(!_fight.hitbox_now(index, k, box)) { continue; }

                    const Hitbox& h = move->hits[k];
                    const int length = h.end > h.start ? h.end - h.start : 1;
                    const int frame = (f.move_frame - h.start) * 3 / length;            // 0..2 over the hitbox's life
                    bn::optional<bn::sprite_ptr>& swipe = v.swipes[used];
                    if(!swipe)
                    {
                        swipe = bn::sprite_items::swipe.create_sprite(0, 0, frame);
                        swipe->set_bg_priority(0);
                        swipe->set_z_order(-4);
                    }
                    swipe->set_tiles(bn::sprite_items::swipe.tiles_item(), frame);
                    swipe->set_position(bn::fixed(box.x) - cam, box.y - 80);
                    swipe->set_horizontal_flip(box.x < fx || (box.x == fx && f.facing < 0));
                    ++used;
                }
            }
            for(int k = used; k < 2; ++k) { v.swipes[k].reset(); }

            // the shot fox thief is holding floats above the head until it is thrown
            if(f.has_stored)
            {
                const ProjArt art = projectile_art(f.stored.kind);
                if(!v.held || v.held_kind != f.stored.kind)
                {
                    v.held_kind = f.stored.kind;
                    if(art.item == &bn::sprite_items::mc_rides)
                    {
                        v.held = bn::sprite_ptr::create(
                            bn::sprite_items::mc_rides.shape_size(),
                            bn::sprite_items::mc_rides.tiles_item().create_tiles(art.first),
                            bn::sprite_items::mc_props.palette_item().create_palette());
                    }
                    else
                    {
                        v.held = art.item->create_sprite(0, 0, art.first);
                    }
                    v.held->set_bg_priority(0);
                    v.held->set_z_order(-6);
                }
                if(move && (move->flags & MF_ORBIT_HELD))                 // Filip's counter: the shot spins around him
                {
                    static const int orbit_x[8] = { 16, 11, 0, -11, -16, -11, 0, 11 };
                    static const int orbit_y[8] = { 0, -11, -16, -11, 0, 11, 16, 11 };
                    const int o = (f.move_frame / 3) % 8;
                    v.held->set_position(bn::fixed(fx + orbit_x[o]) - cam, fy - s.body_h / 2 + orbit_y[o] - 80);
                }
                else { v.held->set_position(bn::fixed(fx) - cam, fy - s.body_h - 12 - 80); }
                v.held->set_horizontal_flip(f.facing < 0);
            }
            else { v.held.reset(); v.held_kind = -1; }
        }
