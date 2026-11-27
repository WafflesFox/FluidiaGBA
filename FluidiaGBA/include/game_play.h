#ifndef GAME_PLAY_H
#define GAME_PLAY_H

// ================================================================================================
// game_screen(): the overworld game loop (walking, NPCs, battles, link)
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    void game_screen(bn::sprite_text_generator& text)
    {
        int current_map = MAP_HOUSE;
        bn::optional<bn::regular_bg_ptr> ground_bg = make_ground(current_map);
        bn::optional<bn::regular_bg_ptr> top_bg = make_top(current_map);
        ground_bg->set_priority(2);                       // ground: behind the player (backshot the tree)
        if(top_bg) { top_bg->set_priority(0); }           // roofs and canopies: in front of the player

        // the player's position in WORLD pixels (the middle of the first world)
        bn::fixed x = 7 * T;
        bn::fixed y = 11 * T;

        bn::vector<bn::sprite_ptr, 32> text_sprites;
        bn::vector<bn::sprite_ptr, 32> pause_text_sprites;
        text.set_center_alignment();
        text.generate(-65,70, map_name(current_map), text_sprites);

        // int chunk = 0;
        // bn::sound_handle music = gameMusic_chunk(chunk).play();
        MusicPlayer music;
        music.play(map_music(current_map));
        // bn::sprite_ptr player = bn::sprite_items::player.create_sprite(0, -40);
        // bn::fixed vy = 0;
        // const bn::fixed gravity = 0.15;
        // const bn::fixed floor_y = 64;
        int dir = DOWN;
        int step = 0;
        int timer = 0;
        int shown = first_frame(dir, false);
        bn::sprite_ptr waffle = bn::sprite_items::waffle.create_sprite(0, 0, shown);
        waffle.set_bg_priority(1); // frontshot the tree (between the ground and the roofs)
        bool paused = true;

        const int feet = 12;   // check the ground at the players feet, not the middle of the sprite
        
        init_npc_saves();
        bn::vector<Npc, 8> npcs;              // the NPCs of the CURRENT map only (rebuilt on every map change)
        spawn_npcs(npcs, current_map, x.integer(), y.integer());

        Conversation talk;                    // the dialogue box: who we talk to, which node, which answer is picked
        static phone::Phone phone_ui;     // static: it is big, so it lives in normal memory, not on the small stack
        static battle::Battle battle_ui;
        bool debug_mode = false;
        bn::vector<bn::sprite_ptr, 8> debug_text_sprites;
        net::LinkSession net_session;
        bn::sprite_ptr remote_waffle = bn::sprite_items::waffle.create_sprite(0, 0, first_frame(DOWN, false));
        bn::sprite_palette_ptr remote_pal = bn::sprite_palette_ptr::create_new(bn::sprite_items::waffle.palette_item());
        remote_pal.set_fade(bn::color(6, 18, 31), 0.35);
        remote_waffle.set_palette(remote_pal);
        remote_waffle.set_bg_priority(1);
        remote_waffle.set_visible(false);
        bn::fixed remote_draw_x = 0;
        bn::fixed remote_draw_y = 0;
        int remote_step = 0;
        int remote_anim_timer = 0;
        int remote_shown = first_frame(DOWN, false);

        bn::vector<bn::sprite_ptr, 12> conn_text_sprites;
        int conn_banner_timer = 0;

        enum NetPrompt { NP_NONE, NP_CONFIRM_REQ, NP_WAIT_REPLY, NP_INCOMING_REQ, NP_DECLINED };
        int net_prompt = NP_NONE;
        int net_choice = 0;
        int net_prompt_timer = 0;
        int decline_send_timer = 0;
        bool net_battle_pending = false;
        bn::vector<bn::sprite_ptr, 4> net_box;
        bn::vector<bn::sprite_ptr, 32> net_prompt_text;
        bn::vector<bn::sprite_ptr, 24> net_prompt_choices;

        auto close_net_prompt = [&]()
        {
            net_prompt = NP_NONE;
            net_box.clear();
            net_prompt_text.clear();
            net_prompt_choices.clear();
        };

        auto draw_net_choices = [&](const char* a, const char* b)
        {
            net_prompt_choices.clear();
            text.set_center_alignment();
            bn::string<20> l0(net_choice == 0 ? "> " : "  "); l0.append(a);
            bn::string<20> l1(net_choice == 1 ? "> " : "  "); l1.append(b);
            text.generate(-60, 72, l0, net_prompt_choices);
            text.generate(60, 72, l1, net_prompt_choices);
        };

        auto open_net_box = [&](const char* title, const char* body, const char* c0, const char* c1)
        {
            net_box.clear();
            net_prompt_text.clear();
            net_prompt_choices.clear();
            for(int i = 0; i < 4; ++i)
            {
                bn::sprite_ptr b = bn::sprite_items::black_block.create_sprite(bn::fixed(-96 + i * 64), 64);
                b.set_bg_priority(0);
                net_box.push_back(b);
            }
            text.set_center_alignment();
            text.generate(-80, 40, title, net_prompt_text);
            text.generate(0, 56, body, net_prompt_text);
            net_choice = 0;
            if(c0 && c1) { draw_net_choices(c0, c1); }
        };

        // screen transition state
        bn::vector<bn::sprite_ptr, 12> blocks;   // the black block
        int trans_phase = 0;                     // 0 = none, 1 = block closing over the screen, 2 = block moving away
        int trans_warp = 0;                      // which warp of the current map we are going through
        int trans_dir = DOWN;                    // the way the player was facing when it started
        bn::fixed trans_pos = 0;                 // 0 = the block covers the whole screen
        int battle_npc = -1;                     // >= 0: the wipe that is closing is the start of a battle against this NPC
        // bool snepTalked = false;
        // DEBUGGING THE TEXT BOX
        // bn::vector<bn::sprite_ptr, 8> test_sprites;
        // text.generate(0, -40, "TEST", test_sprites);

        while(true)
        {
            // ---- the phone: L opens it, and the world freezes while it is open ----
            // ---- the battle: while it runs, the world is hidden and frozen ----
            if(battle_ui.active())
            {
                battle_ui.update(text);

                const unsigned ev = battle_ui.events;                   // sound effects: change these numbers to your own sounds
                if(ev & battle::EV_SHOT) { sfx(7); }
                if(ev & battle::EV_HIT)  { sfx(4); }
                if(ev & battle::EV_KO)   { sfx(6); sfx(11);}
                if(ev & battle::EV_BLOCKED) {sfx(8);}
                if(ev & battle::EV_FIGHT) {sfx(9);}
                if(ev & battle::EV_SWING) {sfx(7);}
                if(ev & battle::EV_ABSORB) {sfx(15);}
                if(ev & battle::EV_BOOM) {sfx(13);}
                if(ev & battle::EV_FINAL_SMASH){sfx(18);}
                if(ev & battle::EV_SMASH){sfx(5);}
                if(ev & battle::EV_GRAB){sfx(16);}
                if(ev & battle::EV_MINING){sfx(17);}

                if(!battle_ui.active())                                 // it just ended: the black blocks cover the screen again
                {
                    music.update();
                    bn::core::update();                                 // flush battle BGs from VRAM
                    ground_bg = make_ground(current_map);               // bring the overworld back
                    top_bg = make_top(current_map);
                    ground_bg->set_priority(2);
                    if(top_bg) { top_bg->set_priority(0); }
                    waffle.set_visible(true);
                    net_session.local_invite = net::INV_NONE;
                    spawn_npcs(npcs, current_map, x.integer(), y.integer());   // the NPCs come back (followers next to you)
                    text.set_center_alignment();
                    text.generate(-65,70, map_name(current_map), text_sprites);
                    if(debug_mode) { text.set_right_alignment(); text.generate(112, -70, "DEBUG", debug_text_sprites); text.set_center_alignment(); }
                    music.stop();
                    bn::sound::stop_all();                  // don't rely on the handle after a busy battle
                    music.play(map_music(current_map));
                    battle_ui.release_blocks(blocks);                   // take the blocks back and slide them away, like after a warp
                    trans_phase = 2;
                    trans_dir = RIGHT;
                    trans_pos = 0;
                }
            }
            else if(phone_ui.active())
            {
                phone_ui.update(text);
            }
            else if(!paused && trans_phase == 0 && !talk.active() &&
                    bn::keypad::select_held() && bn::keypad::l_held() && bn::keypad::r_held() &&
                    (bn::keypad::select_pressed() || bn::keypad::l_pressed() || bn::keypad::r_pressed()))
            {
                debug_mode = !debug_mode;
                sfx(debug_mode ? 12 : 2);
                debug_text_sprites.clear();
                if(debug_mode)
                {
                    text.set_right_alignment();
                    text.generate(112, -70, "DEBUG", debug_text_sprites);
                    text.set_center_alignment();
                }
            }
            else if(bn::keypad::l_pressed() && !bn::keypad::select_held() && !bn::keypad::r_held() && !paused && trans_phase == 0 && !talk.active())
            {
                phone_ui.open(text);
            }
            
            if(bn::keypad::start_pressed() && !phone_ui.active() && !battle_ui.active())
            {
                paused = !paused;

                if(paused)
                {
                    sfx(5);
                    text.generate(0, 0, "PAUSED", pause_text_sprites);
                }
                else
                {
                    pause_text_sprites.clear();
                }
            }
            if (!paused && !phone_ui.active() && !battle_ui.active())
            {
                bn::fixed dx = 0;
                bn::fixed dy = 0;
                if(bn::keypad::left_held())  { dx -= 1; }
                if(bn::keypad::right_held()) { dx += 1; }
                if(bn::keypad::up_held())    { dy -= 1; }
                if(bn::keypad::down_held())  { dy += 1; }
                // if(bn::keypad::r_pressed()) {sfx(8); sfx(4);}
                if(trans_phase != 0 || talk.active() || net_prompt != NP_NONE) { dx = 0; dy = 0; }   // the player stands still while the screen wipes and talking

                const bool moving = dx != 0 || dy != 0;
                const bool running = moving && bn::keypad::b_held();
                const bn::fixed speed = running ? 1.5 : 1;

                if(moving)
                {
                    if(dx < 0)      { dir = LEFT;  }
                    else if(dx > 0) { dir = RIGHT; }
                    else if(dy < 0) { dir = UP;    }
                    else            { dir = DOWN;  }
                    
                    // NPC collissions
                    bn::fixed next_x = x + dx * speed;
                    if(map_walkable(current_map, next_x.integer(), (y + feet).integer()) &&
                       !blocked_by_npc(npcs, x.integer(), y.integer(), next_x.integer(), y.integer())) { x = next_x; }
                    bn::fixed next_y = y + dy * speed;
                    if(map_walkable(current_map, x.integer(), (next_y + feet).integer()) &&
                       !blocked_by_npc(npcs, x.integer(), y.integer(), x.integer(), next_y.integer())) { y = next_y; }

                    if(++timer >= (running ? 5 : 8))   // lower number = faster animation
                    {
                        timer = 0;
                        step = (step + 1) % 4;
                    }
                }
                else
                {
                    step = 0;
                    timer = 0;
                }

                const int wanted = first_frame(dir, running) + walk_cycle[step];
                if(wanted != shown)    // only touch the graphics when the frame actually changes
                {
                    shown = wanted;
                    waffle.set_tiles(bn::sprite_items::waffle.tiles_item(), shown);
                }
                
                // ---- NPCs: wander or follow (they freeze while you talk) ----
                if(!talk.active())
                {
                    for(Npc& n : npcs)
                    {
                        update_npc(n, npcs, x.integer(), y.integer());
                    }
                }

                // ---- link battle prompts & talking ----
                if(net_prompt != NP_NONE)
                {
                    if(net_prompt == NP_CONFIRM_REQ)
                    {
                        if(bn::keypad::left_pressed() && net_choice > 0)  { sfx(2); net_choice = 0; draw_net_choices("Yes", "No"); }
                        if(bn::keypad::right_pressed() && net_choice < 1) { sfx(2); net_choice = 1; draw_net_choices("Yes", "No"); }
                        if(bn::keypad::b_pressed()) { sfx(2); close_net_prompt(); }
                        else if(bn::keypad::a_pressed())
                        {
                            sfx(1);
                            if(net_choice == 0 && net_session.connected)
                            {
                                net_session.local_invite = net::INV_REQUEST;
                                net_prompt = NP_WAIT_REPLY;
                                open_net_box("LINK BATTLE", "Waiting for reply... (B:Cancel)", nullptr, nullptr);
                            }
                            else { close_net_prompt(); }
                        }
                    }
                    else if(net_prompt == NP_WAIT_REPLY)
                    {
                        if(bn::keypad::b_pressed() || !net_session.connected)
                        {
                            sfx(2);
                            net_session.local_invite = net::INV_NONE;
                            close_net_prompt();
                        }
                        else if(net_session.remote_invite == net::INV_REQUEST && net_session.my_id == 1)
                        {
                            net_session.local_invite = net::INV_ACCEPT;
                            close_net_prompt();
                            net_battle_pending = true;
                            trans_phase = 1;
                            trans_dir = dir;
                            trans_pos = transition_distance(dir);
                            make_blocks(blocks);
                        }
                        else if(net_session.remote_invite == net::INV_ACCEPT)
                        {
                            sfx(12);
                            net_session.local_invite = net::INV_BUSY;
                            close_net_prompt();
                            net_battle_pending = true;
                            trans_phase = 1;
                            trans_dir = dir;
                            trans_pos = transition_distance(dir);
                            make_blocks(blocks);
                        }
                        else if(net_session.remote_invite == net::INV_DECLINE)
                        {
                            sfx(2);
                            net_session.local_invite = net::INV_NONE;
                            net_prompt = NP_DECLINED;
                            net_prompt_timer = 60;
                            open_net_box("LINK BATTLE", "Battle request declined.", nullptr, nullptr);
                        }
                    }
                    else if(net_prompt == NP_INCOMING_REQ)
                    {
                        if(!net_session.connected || net_session.remote_invite == net::INV_NONE)
                        {
                            close_net_prompt();
                        }
                        else
                        {
                            if(bn::keypad::left_pressed() && net_choice > 0)  { sfx(2); net_choice = 0; draw_net_choices("Accept", "Decline"); }
                            if(bn::keypad::right_pressed() && net_choice < 1) { sfx(2); net_choice = 1; draw_net_choices("Accept", "Decline"); }
                            if(bn::keypad::b_pressed())
                            {
                                sfx(2);
                                net_session.local_invite = net::INV_DECLINE;
                                decline_send_timer = 35;
                                close_net_prompt();
                            }
                            else if(bn::keypad::a_pressed())
                            {
                                sfx(1);
                                if(net_choice == 0)
                                {
                                    net_session.local_invite = net::INV_ACCEPT;
                                    close_net_prompt();
                                    net_battle_pending = true;
                                    trans_phase = 1;
                                    trans_dir = dir;
                                    trans_pos = transition_distance(dir);
                                    make_blocks(blocks);
                                }
                                else
                                {
                                    net_session.local_invite = net::INV_DECLINE;
                                    decline_send_timer = 35;
                                    close_net_prompt();
                                }
                            }
                        }
                    }
                    else if(net_prompt == NP_DECLINED)
                    {
                        if(--net_prompt_timer <= 0 || bn::keypad::a_pressed() || bn::keypad::b_pressed()) { close_net_prompt(); }
                    }
                }
                else if(talk.active())
                {
                    update_conversation(talk, text);
                    if(!talk.active() && talk.battle_npc >= 0)
                    {
                        battle_npc = talk.battle_npc;
                        talk.battle_npc = -1;
                        trans_phase = 1;
                        trans_dir = dir;
                        trans_pos = transition_distance(dir);
                        make_blocks(blocks);
                    }
                }
                else if(trans_phase == 0 && net_session.connected && net_session.remote_invite == net::INV_REQUEST && decline_send_timer == 0)
                {
                    sfx(12);
                    net_prompt = NP_INCOMING_REQ;
                    open_net_box("LINK BATTLE", "Other player wants to battle!", "Accept", "Decline");
                }
                else if(trans_phase == 0 && bn::keypad::a_pressed())
                {
                    sfx(1);
                    const bool near_remote = net_session.connected && net_session.has_remote_pos &&
                                             net_session.remote_map == current_map &&
                                             abs_int(x.integer() - net_session.remote_x) < 28 &&
                                             abs_int(y.integer() - net_session.remote_y) < 28;
                    Npc* target = npc_in_reach(npcs, x.integer(), y.integer());
                    if(near_remote && (!target || (abs_int(x.integer() - net_session.remote_x) + abs_int(y.integer() - net_session.remote_y)) <=
                                                  (abs_int(x.integer() - target->x.integer()) + abs_int(y.integer() - target->y.integer()))))
                    {
                        net_prompt = NP_CONFIRM_REQ;
                        open_net_box("LINK BATTLE", "Request a battle?", "Yes", "No");
                    }
                    else if(target)
                    {
                        start_conversation(talk, *target, x.integer(), y.integer(), text);
                    }
                }

                // ---- warps: did the player's feet step into one? ----
                int warp_count = 0;
                const Warp* warps = map_warps(current_map, warp_count);
                if(trans_phase == 0)
                {
                    for(int i = 0; i < warp_count; ++i)
                    {
                        const Warp& w = warps[i];
                        const int fx = x.integer();
                        const int fy = (y + feet).integer();

                        if(fx >= w.x && fx < w.x + w.w && fy >= w.y && fy < w.y + w.h)
                        {
                            trans_phase = 1;                          // start closing the screen
                            trans_warp = i;
                            trans_dir = dir;
                            trans_pos = transition_distance(dir);
                            make_blocks(blocks);
                            break;
                        }
                    }
                }

                // ---- the transition itself ----
                if(trans_phase != 0)
                {
                    trans_pos -= TRANSITION_SPEED;

                    if(trans_phase == 1 && trans_pos <= 0 && net_battle_pending)
                    {
                        trans_pos = 0;
                        net_battle_pending = false;

                        ground_bg.reset();
                        top_bg.reset();
                        waffle.set_visible(false);
                        remote_waffle.set_visible(false);
                        park_npcs(npcs);
                        text_sprites.clear();
                        debug_text_sprites.clear();
                        conn_text_sprites.clear();
                        conn_banner_timer = 0;
                        music.update();
                        bn::core::update(); // free VRAM before creating Character Select BGs

                        int p1_pick = 0;
                        int p2_pick = 0;
                        const bool cs_ok = char_select_screen(text, music, net_session, current_map, blocks, trans_dir, p1_pick, p2_pick);
                        music.update();
                        bn::core::update(); // free Character Select BGs from VRAM

                        bool synced = false;
                        int btl_map = current_map;
                        unsigned btl_seed = unsigned(random_int(500) + 1);

                        if(cs_ok)
                        {
                            net_session.reset_battle_sync();
                            net_session.local_invite = net::INV_BUSY;

                            if(net_session.my_id == 0)
                            {
                                const int pkt = (net::PKT_BTL_SEED << 12) | ((btl_seed & 0x1FF) << 3) | (btl_map & 7);
                                for(int w = 0; w < 90; ++w)
                                {
                                    bn::link::send(pkt);
                                    net_session.poll();
                                    if(net_session.peer_ready) { synced = true; break; }
                                    music.update();
                                    bn::core::update();
                                }
                            }
                            else
                            {
                                for(int w = 0; w < 90; ++w)
                                {
                                    net_session.poll();
                                    if(net_session.got_seed)
                                    {
                                        btl_map = net_session.synced_map;
                                        btl_seed = net_session.synced_seed;
                                        bn::link::send((net::PKT_BTL_READY << 12) | 1);
                                        synced = true;
                                        break;
                                    }
                                    music.update();
                                    bn::core::update();
                                }
                            }
                        }

                        if(synced)
                        {
                            music.play(battle_music(btl_map));
                            battle_ui.set_options(phone_ui.settings().battle_3d, phone_ui.settings().battle_tint);
                            battle_ui.start(*playable_fighters[p1_pick], *playable_fighters[p2_pick], map_stage(btl_map),
                                            blocks, text, btl_seed, nullptr, battle::Buff(), nullptr, false,
                                            &net_session, net_session.my_id, &waffle_fighter);
                            trans_phase = 0;
                        }
                        else
                        {
                            ground_bg = make_ground(current_map);
                            top_bg = make_top(current_map);
                            ground_bg->set_priority(2);
                            if(top_bg) { top_bg->set_priority(0); }
                            waffle.set_visible(true);
                            spawn_npcs(npcs, current_map, x.integer(), y.integer());
                            text.set_center_alignment();
                            text.generate(-65, 70, map_name(current_map), text_sprites);
                            if(debug_mode)
                            {
                                text.set_right_alignment();
                                text.generate(112, -70, "DEBUG", debug_text_sprites);
                                text.set_center_alignment();
                            }
                            net_session.local_invite = net::INV_NONE;
                            trans_phase = 2;
                        }
                    }
                    else if(trans_phase == 1 && trans_pos <= 0 && battle_npc >= 0)
                    {
                        trans_pos = 0;                                // the screen is fully covered: this wipe was for a battle
                        const NpcDef& foe = npc_defs[battle_npc];

                        const int companion = pick_companion(npcs, battle_npc);        // a follower tags along (before park_npcs empties the list)
                        const battle::FighterDef* partner = nullptr;                    // can fight: joins your side
                        battle::Buff buff;                                              // can't fight: gives a bonus from its traits...
                        const bn::sprite_item* bystander = nullptr;                     // ...and wanders around the stage while you fight
                        if(companion >= 0)
                        {
                            const NpcDef& friend_def = npc_defs[companion];
                            if(friend_def.fight) { partner = friend_def.fight; }
                            else                 { buff = buff_from_traits(friend_def.traits); bystander = friend_def.sprite_item; }
                        }

                        ground_bg.reset();                            // the battle needs those background slots
                        top_bg.reset();
                        waffle.set_visible(false);                    // hide the overworld
                        park_npcs(npcs);                              // free the NPC sprites too: the battle needs their video memory
                        text_sprites.clear();
                        debug_text_sprites.clear();
                        music.play(battle_music(current_map));
                        remote_waffle.set_visible(false);
                        conn_text_sprites.clear();
                        conn_banner_timer = 0;
                        int cpu_skill = foe.fight->stats.ai_skill;
                        if(debug_mode)
                        {
                            bn::vector<bn::sprite_ptr, 48> dbg_lvl_text;
                            auto redraw_cpu_prompt = [&]()
                            {
                                dbg_lvl_text.clear();
                                text.set_center_alignment();
                                bn::string<32> hdr("CPU LEVEL FOR ");
                                hdr.append(foe.fight->name);
                                char nbuf[8];
                                int v = cpu_skill, t = 0, p = 0;
                                char tmp[8];
                                if(v == 0) { tmp[t++] = '0'; }
                                while(v > 0) { tmp[t++] = char('0' + v % 10); v /= 10; }
                                while(t > 0) { nbuf[p++] = tmp[--t]; }
                                nbuf[p] = 0;
                                bn::string<20> val("<  ");
                                val.append(nbuf);
                                val.append("  >");
                                text.generate(0, -34, "DEBUG MODE", dbg_lvl_text);
                                text.generate(0, -12, hdr, dbg_lvl_text);
                                text.generate(0, 12, val, dbg_lvl_text);
                                text.generate(0, 40, "L/R:+-10  U/D:+-1  A:GO", dbg_lvl_text);
                                for(bn::sprite_ptr& sp : dbg_lvl_text) { sp.set_bg_priority(0); sp.set_z_order(-32005); }
                            };
                            redraw_cpu_prompt();
                            while(!bn::keypad::a_pressed() && !bn::keypad::start_pressed())
                            {
                                int delta = 0;
                                if(bn::keypad::left_pressed())  { delta -= 10; }
                                if(bn::keypad::right_pressed()) { delta += 10; }
                                if(bn::keypad::down_pressed())  { delta -= 1; }
                                if(bn::keypad::up_pressed())    { delta += 1; }
                                if(delta != 0)
                                {
                                    sfx(2);
                                    cpu_skill += delta;
                                    if(cpu_skill < 0) { cpu_skill = 0; }
                                    if(cpu_skill > 100) { cpu_skill = 100; }
                                    redraw_cpu_prompt();
                                }
                                music.update();
                                bn::core::update();
                            }
                            sfx(1);
                            dbg_lvl_text.clear();
                        }
                        battle_ui.set_options(phone_ui.settings().battle_3d, phone_ui.settings().battle_tint);
                        battle_ui.start(waffle_fighter, *foe.fight, map_stage(current_map), blocks, text, unsigned(random_int(60000) + 1), partner, buff, bystander, debug_mode, nullptr, -1, &waffle_fighter);
                        if(debug_mode) { battle_ui.set_cpu_skill(cpu_skill); }
                        battle_npc = -1;
                        trans_phase = 0;                              // the battle owns the black blocks now
                    }
                    else if(trans_phase == 1 && trans_pos <= 0)
                    {
                        trans_pos = 0;                                // the screen is fully covered: swap the maps now
                        const Warp& w = warps[trans_warp];
                        current_map = w.dest_map;
                        x = w.dest_x;
                        y = w.dest_y;

                        ground_bg.reset();                            // free the old map first
                        top_bg.reset();
                        music.update();
                        bn::core::update();
                        ground_bg = make_ground(current_map);
                        top_bg = make_top(current_map);
                        ground_bg->set_priority(2);
                        if(top_bg) { top_bg->set_priority(0); }

                        end_conversation(talk);                       // (safety) never keep pointing at an NPC that is about to vanish
                        spawn_npcs(npcs, current_map, x.integer(), y.integer());   // old NPCs are stored + freed, new ones created, followers tag along
                        music.play(map_music(current_map));

                        text_sprites.clear();                         // update the name in the corner
                        text.generate(-65,70, map_name(current_map), text_sprites);

                        trans_phase = 2;                              // now slide the block away
                    }
                    else if(trans_phase == 2 && trans_pos <= -transition_distance(trans_dir))
                    {
                        blocks.clear();                               // the block has left the screen
                        trans_phase = 0;
                    }

                    if(trans_phase != 0)
                    {
                        move_blocks(blocks, trans_dir, trans_pos);
                    }
                }

                if(!battle_ui.active())       // (the overworld is gone on the frame a battle starts)
                {
                // ---- camera: follows the player, stops at the edges of the current map ----
                const int map_w = map_width(current_map);
                const int map_h = map_height(current_map);
                const bn::fixed cam_x = clamp_fixed(x, 120, map_w - 120);
                const bn::fixed cam_y = clamp_fixed(y, 80, map_h - 80);

                const bn::fixed bg_x = bn::fixed(map_w / 2) - cam_x;
                const bn::fixed bg_y = bn::fixed(map_h / 2) - cam_y;
                ground_bg->set_position(bg_x, bg_y);
                if(top_bg) { top_bg->set_position(bg_x, bg_y); }

                // Draw CHaracters
                waffle.set_position(x - cam_x, y - cam_y);
                for(Npc& n : npcs)
                {
                    n.sprite.set_visible(true);
                    n.sprite.set_position(n.x - cam_x, n.y - cam_y);
                    n.sprite.set_z_order(-n.y.integer());         // lower on the map = drawn in front
                }

                // depth sorting: whoever is lower on the map is drawn in front
                waffle.set_z_order(-y.integer());

                if(net_session.connected && net_session.has_remote_pos && net_session.remote_map == current_map)
                {
                    if(abs_int(net_session.remote_x - remote_draw_x.integer()) > 48 ||
                       abs_int(net_session.remote_y - remote_draw_y.integer()) > 48)
                    {
                        remote_draw_x = net_session.remote_x;
                        remote_draw_y = net_session.remote_y;
                    }
                    else
                    {
                        remote_draw_x += (bn::fixed(net_session.remote_x) - remote_draw_x) / 3;
                        remote_draw_y += (bn::fixed(net_session.remote_y) - remote_draw_y) / 3;
                    }
                    if(net_session.remote_moving)
                    {
                        if(++remote_anim_timer >= (net_session.remote_running ? 5 : 8))
                        {
                            remote_anim_timer = 0;
                            remote_step = (remote_step + 1) % 4;
                        }
                    }
                    else { remote_step = 0; remote_anim_timer = 0; }
                    const int rwanted = first_frame(net_session.remote_dir, net_session.remote_running) + walk_cycle[remote_step];
                    if(rwanted != remote_shown)
                    {
                        remote_shown = rwanted;
                        remote_waffle.set_tiles(bn::sprite_items::waffle.tiles_item(), rwanted);
                    }
                    remote_waffle.set_visible(true);
                    remote_waffle.set_position(remote_draw_x - cam_x, remote_draw_y - cam_y);
                    remote_waffle.set_z_order(-remote_draw_y.integer());
                }
                else { remote_waffle.set_visible(false); }
                }
            }

            // if(!music.active())   // this chunk finished, so start the next one
            // {
            //     chunk = (chunk + 1) % music_chunks;
            //     music = gameMusic_chunk(chunk).play();
            // }
            if(!battle_ui.active())
            {
                if(decline_send_timer > 0 && --decline_send_timer == 0 && net_session.local_invite == net::INV_DECLINE)
                {
                    net_session.local_invite = net::INV_NONE;
                }
                const bool busy_ow = paused || phone_ui.active() || talk.active() || trans_phase != 0;
                if(busy_ow && net_session.local_invite == net::INV_NONE) { net_session.local_invite = net::INV_BUSY; }
                else if(!busy_ow && net_session.local_invite == net::INV_BUSY) { net_session.local_invite = net::INV_NONE; }

                const bool ow_moving = !busy_ow && net_prompt == NP_NONE &&
                                       (bn::keypad::left_held() || bn::keypad::right_held() || bn::keypad::up_held() || bn::keypad::down_held());
                const bool ow_running = ow_moving && bn::keypad::b_held();
                net_session.update_overworld(x.integer(), y.integer(), current_map, dir, ow_running, ow_moving);

                if(net_session.just_connected)
                {
                    sfx(12);
                    conn_text_sprites.clear();
                    text.set_left_alignment();
                    text.generate(-112, -70, net_session.my_id == 0 ? "CONNECTED P1" : "CONNECTED P2", conn_text_sprites);
                    text.set_center_alignment();
                    conn_banner_timer = 180;
                }
                if(conn_banner_timer > 0 && --conn_banner_timer == 0) { conn_text_sprites.clear(); }
            }
            music.update();
            bn::core::update();
        }
        music.stop();
    }
}

#endif
