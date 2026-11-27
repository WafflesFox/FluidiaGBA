// ================================================================================================
// Battle::update(): the frame-by-frame state machine (setup, opening, fight, final smash, result, closing...)
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        // one frame. `text` is the game's text generator.
        void update(bn::sprite_text_generator& text)
        {
            events = 0;

            switch(_phase)
            {
                case PH_SETUP:
                    build_scene(text);
                    _phase = PH_OPENING;
                    break;

                case PH_OPENING:
                    _wipe -= WIPE_SPEED;                                // the blocks slide off to the left
                    if(_wipe <= -WIPE_DISTANCE)
                    {
                        _blocks.clear();
                        _phase = PH_READY;
                        _timer = 60;
                        show_banner(text, "READY?");
                    }
                    else { move_blocks(); }
                    break;

                case PH_READY:
                    if(--_timer <= 0)
                    {
                        if(_net)
                        {
                            _net->in_battle = true;
                            init_net_sync_buffers();
                            _net_sync_ticks = 0;
                            _phase = PH_NET_SYNC;
                        }
                        else
                        {
                            show_banner(text, "GO!");
                            _banner_timer = 45;
                            _phase = PH_FIGHT;
                            events |= EV_FIGHT;
                        }
                    }
                    break;

                case PH_NET_SYNC:
                    bn::link::send((net::PKT_BTL_READY << 12) | 1);
                    _net->poll();
                    if(_net->peer_ready || ++_net_sync_ticks >= 90)
                    {
                        bn::link::send((net::PKT_BTL_READY << 12) | 1);
                        show_banner(text, "GO!");
                        _banner_timer = 45;
                        _phase = PH_FIGHT;
                        events |= EV_FIGHT;
                    }
                    break;

                case PH_FIGHT:
                {
                    if(_debug_enabled && bn::keypad::start_pressed())
                    {
                        open_debug_menu(text);
                        _phase = PH_DEBUG_MENU;
                        return;
                    }
                    Input in[MAX_FIGHTERS] = {};
                    bool stepped = true;               // false on a frame where the sim did not advance (waiting for the peer)
                    if(_net)
                    {
                        net::Lockstep& ls = _net->ls;
                        ls.schedule(read_player_held());
                        bn::link::send(ls.next_packet());
                        for(int i = 0; i < 4 && _net->poll(); ++i) {}

                        unsigned held[2];
                        unsigned before[2];
                        if(ls.try_step(_local_idx, held, before))
                        {
                            in[0] = expand_held(held[0], before[0]);        // player 0 = host's fighter on BOTH consoles
                            in[1] = expand_held(held[1], before[1]);
                            for(int i = 2; i < MAX_FIGHTERS; ++i)
                            {
                                if(_fight.f[i].active) { in[i] = _fight.think(i); }
                            }
                            _fight.step(in);

                            if(ls.is_checkpoint())
                            {
                                bn::link::send((net::PKT_BTL_HASH << 12) | ls.record_hash(_fight.state_hash()));
                            }
                        }
                        else
                        {
                            stepped = false;
                            if(ls.timed_out())
                            {
                                // the peer is gone (cable pulled / console off): leave through the result screen
                                _result = -1;
                                _phase = PH_RESULT;
                                _timer = RESULT_FRAMES;
                            }
                        }

                        if(ls.desync && !_desync_shown)
                        {
                            _desync_shown = true;
                            show_banner(text, "DESYNC");
                            _banner_timer = 240;
                        }
                    }
                    else
                    {
                        const int ctrl = (_debug_enabled && _fight.debug_ai_target == 0) ? 1 : 0;
                        in[ctrl] = read_player_input();
                        for(int i = 0; i < MAX_FIGHTERS; ++i)
                        {
                            if(i != ctrl && _fight.f[i].active) { in[i] = _fight.think(i); }
                        }
                        _fight.step(in);
                    }
                    if(stepped) { collect_events(); }

                    if(_fight.fs_combo_active)
                    {
                        const int ct = _fight.fs_combo_tick;
                        const bool split_boom = (ct >= 75 && ct <= 86);
                        const bool end_boom   = (ct >= 232 && ct <= 246);
                        _shake = split_boom ? ((ct % 2 == 0) ? 3 : -3) : (end_boom ? ((ct % 2 == 0) ? 5 : -5) : 0);
                        if(split_boom)     { set_flash(bn::fixed(86 - ct) / 11); }
                        else if(end_boom)  { set_flash(bn::fixed(246 - ct) / 14); }
                        else
                        {
                            set_flash(0);
                            bn::bg_palettes::set_fade(bn::color(0, 0, 0), bn::fixed(0.60));
                        }
                    }
                    else if(_fight.fs_freeze_timer > 0)
                    {
                        const int elapsed = FS_FREEZE_FRAMES - _fight.fs_freeze_timer;
                        _shake = (elapsed < 10) ? ((elapsed % 2 == 0) ? 2 : -2) : 0;
                        if(elapsed < 10)
                        {
                            set_flash(bn::fixed(10 - elapsed) * bn::fixed(0.09));
                        }
                        else
                        {
                            set_flash(0);
                            bn::bg_palettes::set_fade(bn::color(0, 0, 0), bn::fixed(0.72));
                        }
                    }
                    else
                    {
                        _shake = 0;
                        if(_fs_flash > 0)
                        {
                            --_fs_flash;
                            set_flash(bn::fixed(_fs_flash) / 12);
                        }
                        else
                        {
                            bool any_smash_ready = false;
                            for(int i = 0; i < _fight.count; ++i)
                            {
                                if(_fight.f[i].stats->final_smash_move != nullptr && _fight.f[i].smash >= SMASH_MAX && _fight.f[i].state != ST_KO)
                                {
                                    any_smash_ready = true;
                                }
                            }
                            bn::bg_palettes::set_fade(bn::color(0, 0, 0), any_smash_ready ? bn::fixed(0.42) : bn::fixed(0));
                        }
                    }

                    if(_fight.pending_final_smash >= 0)
                    {
                        _fs_attacker = _fight.pending_final_smash;
                        _fs_victim = _fight.pending_final_smash_victim >= 0 ? _fight.pending_final_smash_victim : _fight.foe_of(_fs_attacker);
                        _fs_id = _fight.pending_final_smash_id;
                        _fight.pending_final_smash = -1;
                        _fight.pending_final_smash_victim = -1;
                        _fight.pending_final_smash_id = FS_NONE;
                        _fs_timer = 0;
                        _fs_cam_x = 0;
                        _fs_flash = 0;
                        _shake = 0;
                        unload_stage_gfx();
                        set_flash(1);
                        events |= EV_FINAL_SMASH;
                        _phase = PH_FS_ENTER;
                        return;
                    }

                    if(_banner_timer > 0 && --_banner_timer == 0) { _banner.clear(); }

                    if(_fight.winner >= 0)
                    {
                        _fs_flash = 0;
                        set_flash(0);
                        const int me_idx = _net ? _local_idx : ((_debug_enabled && _fight.debug_ai_target == 0) ? 1 : 0);
                        _result = (_fight.winner == me_idx) ? 1 : -1;
                        _phase = PH_RESULT;
                        _timer = RESULT_FRAMES;
                    }
                    break;
                }

                case PH_DEBUG_MENU:
                    update_debug_menu(text);
                    return;

                case PH_FS_ENTER:
                    if(_net) { pump_net(); }
                    build_fs_scene(text);
                    _phase = PH_FS_CUTSCENE;
                    return;

                case PH_FS_CUTSCENE:
                    if(_net) { pump_net(); }
                    update_fs_cutscene(text);
                    return;

                case PH_FS_EXIT:
                {
                    build_scene(text);
                    const FinalSmashDef& fs = final_smash_def(_fs_id);
                    _fight.apply_final_smash_hit(_fs_attacker, _fs_victim, fs.damage, fs.kb_x, fs.kb_y, fs.stun, fs.heal, fs.def_bonus, fs.def_frames);
                    collect_events();
                    if(_fight.winner >= 0)
                    {
                        const int me_idx = _net ? _local_idx : ((_debug_enabled && _fight.debug_ai_target == 0) ? 1 : 0);
                        _result = (_fight.winner == me_idx) ? 1 : -1;
                        _phase = PH_RESULT;
                        _timer = RESULT_FRAMES;
                    }
                    else
                    {
                        _fs_flash = 12;
                        if(_net) { _net->clear_cutscene_pending(); }
                        _phase = PH_FIGHT;
                    }
                    break;
                }

                case PH_RESULT:
                {
                    if(_net) { pump_net(); }
                    const bool knockout = !_fight.team_alive(0) || !_fight.team_alive(1);
                    const int t = RESULT_FRAMES - _timer;
                    const bool frozen = knockout && t < KO_FREEZE;
                    const bool slow = knockout && t < KO_SLOW_END;

                    if(knockout && t == 0)
                    {
                        unload_stage_bgs();
                        set_ko_overlay_hidden(true);
                    }
                    else if(knockout && t == 1 && !_ko_bg)
                    {
                        _ko_bg = bn::regular_bg_items::fs_bg.create_bg(0, 0);
                        _ko_bg->set_priority(3);
                    }

                    if(!frozen && (!slow || t % KO_SLOW_RATE == 0))
                    {
                        const Input in[MAX_FIGHTERS] = {};
                        _fight.step(in);
                    }

                    _shake = 0;
                    if(slow)
                    {
                        const int power = 6 - t * 6 / KO_SLOW_END;
                        _shake = (t % 2 == 0) ? power : -power;
                    }

                    if(_ko_bg)
                    {
                        _ko_bg->set_position(-t * 6, _shake);
                    }

                    if(knockout && t == KO_BANNER - 1)
                    {
                        _ko_bg.reset();
                        set_flash(bn::fixed(0.5));
                    }
                    else if(knockout && t == KO_BANNER)
                    {
                        set_ko_overlay_hidden(false);
                        build_stage_bgs();
                    }

                    if(knockout && t <= KO_FLASH) { set_flash(bn::fixed(KO_FLASH - t) / KO_FLASH); }
                    else if(knockout && t >= KO_BANNER && t <= KO_BANNER + 6) { set_flash(bn::fixed(KO_BANNER + 6 - t) / 12); }
                    else if(!knockout && t == 0) { set_flash(0); }

                    if(t == (knockout ? KO_BANNER : 0)) { show_banner(text, _result > 0 ? "GAME!" : "GAY!"); }

                    if(--_timer <= 0)
                    {
                        _ko_bg.reset();
                        set_ko_overlay_hidden(false);
                        _shake = 0;
                        set_flash(0);
                        _banner.clear();
                        make_blocks();
                        _wipe = WIPE_DISTANCE;
                        move_blocks();
                        _phase = PH_CLOSING;
                    }
                    break;
                }
                case PH_CLOSING:
                    _wipe -= WIPE_SPEED;
                    if(_wipe <= 0)
                    {
                        _wipe = 0;
                        move_blocks();
                        finish();                                       // the screen is covered: throw the stage away
                        return;
                    }
                    move_blocks();
                    break;

                default:
                    return;
            }

            draw(text);
        }
