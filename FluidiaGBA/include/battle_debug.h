// ================================================================================================
// The debug menu (slot names, stepping, drawing and updating the menu)
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        const char* debug_slot_name(int slot) const
        {
            const FighterStats& s = *_fight.f[_fight.debug_ai_target].stats;
            if(slot == SLOT_COUNT)     { return s.final_smash_move ? s.final_smash_move->name : "NONE"; }
            if(slot == SLOT_COUNT + 1) { return "Place Block"; }
            if(slot == SLOT_COUNT + 2) { return "Craft Weapon"; }
            if(s.moves && s.moves->slot[slot]) { return s.moves->slot[slot]->name; }
            return "NONE";
        }

        bool debug_slot_valid(const FighterStats& s, int slot) const
        {
            if(slot == SLOT_COUNT)     { return s.final_smash_move != nullptr; }
            if(slot == SLOT_COUNT + 1) { return s.spawns_crafting_table; }
            if(slot == SLOT_COUNT + 2) { return s.spawns_crafting_table; }
            if(!has_move(s, slot))     { return false; }
            const Move* m = s.moves->slot[slot];
            for(int prev = 0; prev < slot; ++prev)
            {
                if(s.moves->slot[prev] == m) { return false; }
            }
            return true;
        }

        void step_debug_slot(int dir)
        {
            const FighterStats& s = *_fight.f[_fight.debug_ai_target].stats;
            const int total = SLOT_COUNT + 3;
            for(int k = 0; k < total; ++k)
            {
                _fight.debug_force_slot = (_fight.debug_force_slot + dir + total) % total;
                if(debug_slot_valid(s, _fight.debug_force_slot)) { return; }
            }
        }

        void draw_debug_menu(bn::sprite_text_generator& text)
        {
            _dbg_text.clear();
            text.set_center_alignment();
            text.generate(0, -52, "DEBUG MENU", _dbg_text);

            const char* ai_str = _fight.debug_ai_mode == DBG_AI_IDLE ? "OFF" :
                                 (_fight.debug_ai_mode == DBG_AI_FORCE ? "FORCE" : "NORMAL");
            const int ctrl = (_fight.debug_ai_target == 0) ? 1 : 0;
            const char* ctrl_name = fighter_def(ctrl)->name;

            text.set_left_alignment();
            char line[32];
            auto put_line = [&](int row, const char* label, const char* val)
            {
                int pos = 0;
                line[pos++] = (row == _dbg_cursor) ? '>' : ' ';
                line[pos++] = ' ';
                for(int i = 0; label[i] && pos < 29; ++i) { line[pos++] = label[i]; }
                if(val)
                {
                    for(int i = 0; val[i] && pos < 29; ++i) { line[pos++] = val[i]; }
                }
                line[pos] = 0;
                text.generate(-84, -38 + row * 14, line, _dbg_text);
            };

            put_line(0, "AI: ", ai_str);
            put_line(1, "MOVE: ", debug_slot_name(_fight.debug_force_slot));
            put_line(2, "CTRL: ", ctrl_name);
            put_line(3, "FILL MY SMASH", _fight.f[ctrl].smash >= SMASH_MAX ? " [FULL]" : " [A]");
            put_line(4, "HEAL BOTH", " [A]");
            put_line(5, "END BATTLE", " [A]");
            int sk = _fight.debug_ai_skill >= 0 ? _fight.debug_ai_skill : _fight.f[_fight.debug_ai_target].stats->ai_skill;
            char sk_buf[6];
            format_int(sk, sk_buf);
            put_line(6, "CPU SKILL: ", sk_buf);

            for(bn::sprite_ptr& sp : _dbg_text) { sp.set_bg_priority(0); sp.set_z_order(-26); }
            text.set_center_alignment();
        }

        void open_debug_menu(bn::sprite_text_generator& text)
        {
            _dbg_cursor = 0;
            if(!debug_slot_valid(*_fight.f[_fight.debug_ai_target].stats, _fight.debug_force_slot))
            {
                step_debug_slot(1);
            }
            _dbg_box.clear();
            for(int r = 0; r < 2; ++r)
            {
                for(int c = 0; c < 3; ++c)
                {
                    bn::sprite_ptr b = bn::sprite_items::black_block.create_sprite(-64 + c * 64, -32 + r * 64);
                    b.set_bg_priority(0);
                    b.set_z_order(-25);
                    _dbg_box.push_back(b);
                }
            }
            draw_debug_menu(text);
        }

        void close_debug_menu()
        {
            _dbg_box.clear();
            _dbg_text.clear();
            _phase = PH_FIGHT;
        }

        void update_debug_menu(bn::sprite_text_generator& text)
        {
            if(bn::keypad::start_pressed() || bn::keypad::b_pressed())
            {
                close_debug_menu();
                return;
            }
            bool redraw = false;
            if(bn::keypad::up_pressed())   { _dbg_cursor = (_dbg_cursor + 6) % 7; redraw = true; }
            if(bn::keypad::down_pressed()) { _dbg_cursor = (_dbg_cursor + 1) % 7; redraw = true; }

            const int lr = (bn::keypad::right_pressed() ? 1 : 0) - (bn::keypad::left_pressed() ? 1 : 0);
            const bool ap = bn::keypad::a_pressed();
            const int ctrl = (_fight.debug_ai_target == 0) ? 1 : 0;

            if(_dbg_cursor == 0 && (lr != 0 || ap))
            {
                const int dir = lr != 0 ? lr : 1;
                _fight.debug_ai_mode = (_fight.debug_ai_mode + dir + 3) % 3;
                _fight.debug_cd = 0;
                redraw = true;
            }
            else if(_dbg_cursor == 1 && (lr != 0 || ap))
            {
                step_debug_slot(lr != 0 ? lr : 1);
                _fight.debug_ai_mode = DBG_AI_FORCE;
                _fight.debug_cd = 0;
                redraw = true;
            }
            else if(_dbg_cursor == 2 && (lr != 0 || ap))
            {
                _fight.debug_ai_target = 1 - _fight.debug_ai_target;
                _fight.debug_cd = 0;
                if(!debug_slot_valid(*_fight.f[_fight.debug_ai_target].stats, _fight.debug_force_slot))
                {
                    step_debug_slot(1);
                }
                redraw = true;
            }
            else if(_dbg_cursor == 3 && ap)
            {
                _fight.f[ctrl].smash = SMASH_MAX;
                redraw = true;
            }
            else if(_dbg_cursor == 4 && ap)
            {
                for(int i = 0; i < _fight.count; ++i)
                {
                    _fight.f[i].hp = _fight.f[i].stats->max_hp;
                }
                redraw = true;
            }
            else if(_dbg_cursor == 5 && ap)
            {
                close_debug_menu();
                _fs_flash = 0;
                set_flash(0);
                _banner.clear();
                _result = 0;
                make_blocks();
                _wipe = WIPE_DISTANCE;
                move_blocks();
                _phase = PH_CLOSING;
                return;
            }
            else if(_dbg_cursor == 6 && (lr != 0 || ap))
            {
                int cur_sk = _fight.debug_ai_skill >= 0 ? _fight.debug_ai_skill : _fight.f[_fight.debug_ai_target].stats->ai_skill;
                cur_sk += (lr != 0 ? lr * 10 : 10);
                if(cur_sk > 100) { cur_sk = 0; }
                if(cur_sk < 0)   { cur_sk = 100; }
                _fight.debug_ai_skill = cur_sk;
                redraw = true;
            }

            if(redraw) { draw_debug_menu(text); }
        }
