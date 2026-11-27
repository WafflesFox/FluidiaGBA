// ================================================================================================
// finish() and the final-smash dispatchers build_fs_scene() / update_fs_cutscene()
//
// NOT a normal header: these are member functions of `class Battle` (battle.h), and battle.h #includes
// this file INSIDE the class body. Do not include it anywhere else.
// ================================================================================================

        void finish()
        {
            if(_net) { _net->in_battle = false; }
            unload_stage_gfx();
            unload_fs_gfx();
            set_flash(0);
            _phase = PH_OFF;
        }

        void build_fs_scene(bn::sprite_text_generator& text)
        {
            switch(_fs_id)
            {
                case FS_FILIP_DROP:   build_filip_fs(text); break;
                case FS_UMB_MINECART: build_umb_fs(text); break;
                case FS_WAFFLE_MEAL:  build_waffle_meal_fs(text); break;
                case FS_HARUE_SLICE:  build_har_slice_fs(text); break;
                default: build_waffle_fs(text); break;
            }
        }

        void update_fs_cutscene(bn::sprite_text_generator& text)
        {
            switch(_fs_id)
            {
                case FS_FILIP_DROP:   update_filip_fs(text); break;
                case FS_UMB_MINECART: update_umb_fs(text); break;
                case FS_WAFFLE_MEAL:  update_waffle_meal_fs(text); break;
                case FS_HARUE_SLICE:  update_har_slice_fs(text); break;
                default: update_waffle_fs(text); break;
            }
        }
