#ifndef BATTLE_H
#define BATTLE_H

// ================================================================================================
// THE BATTLE SCREEN:
//
//   A = normal attacks: A (jab), A + direction on the ground (smashes), A in the air (aerials, + direction)
//   B = special attacks: B (water gun), B + left/right (aqua jet), B + up (aqua jet up), B + down (fox thief)
//   R = jump (press again in the air for a double jump)
//   LEFT / RIGHT = move  DOWN (held) = drop through a platform / fall faster
//   L (hold) = shield: a big defense boost that slowly fades the longer you hold it
//
//   The moves themselves are data in battle_moves.h. The rules are in battle_logic.h.
//
//   battle_ui.start(...)   when the screen is covered by the black blocks
//   battle_ui.update(...)  every frame while battle_ui.active()
//   battle_ui.release_blocks(blocks) when it finishes (the screen is covered again)
//
//   FILE MAP (battle.h used to be one giant file):
//     battle_types.h          FighterDef, StageDef, ParallaxLayer, EV_... flags
//     battle_anim.h           frame numbers + attack animations + projectile art (pick_frame...)
//     battle_helpers.h        format_int + reading the buttons
//     battle_view.h           FighterView (the sprites of one fighter)
//     battle.h                class Battle: the public API, the constants and all the data members
//     battle_update/scene/debug/fs_*/draw*.h  the member functions of class Battle, grouped by job. They are #included
//                             INSIDE the class body below, so they are not stand-alone headers.
//
//
// ================================================================================================

#include "bn_color.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_item.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_palettes.h"

#include "bn_sprite_items_black_block.h"
#include "bn_sprite_items_water_shot.h"
#include "bn_sprite_items_battle_floor.h"
#include "bn_sprite_items_battle_platform.h"
#include "bn_sprite_items_hp_bar.h"
#include "bn_sprite_items_shield.h"
#include "bn_sprite_items_explosion.h"
#include "bn_sprite_items_aqua_jet.h"
#include "bn_sprite_items_swipe.h"
#include "bn_sprite_items_hit_spark.h"
#include "bn_sprite_items_junk.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_tile.h"
#include "bn_bpp_mode.h"
#include "bn_compression_type.h"
#include "bn_sprite_items_mc_props.h"
#include "bn_sprite_items_mc_rides.h"
#include "bn_sprite_items_harueslice_sword.h"
#include "bn_sprite_items_sonic_slash.h"
#include "bn_sprite_items_blue_slash.h"
#include "bn_regular_bg_items_blue_dim_bg.h"
#include "bn_regular_bg_position_hbe_ptr.h"
#include "bn_regular_bg_items_minecraft_fs_bg.h"
#include "bn_regular_bg_items_kitchen_fs_bg.h"

#include "bn_display.h"
#include "bn_affine_bg_item.h"
#include "bn_affine_bg_ptr.h"
#include "bn_rect_window.h"
#include "bn_window.h"
#include "bn_affine_bg_pa_register_hbe_ptr.h"
#include "bn_affine_bg_dx_register_hbe_ptr.h"
#include "bn_affine_bg_dy_register_hbe_ptr.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_regular_bg_items_fs_bg.h"
#include "bn_regular_bg_items_ring_bg.h"
#include "bn_regular_bg_items_gym_lights_bg.h"
#include "bn_sprite_items_fire_beam.h"
#include "bn_sprite_items_fire_aura.h"
#include "bn_sprite_items_fire_impact.h"

#include "battle_logic.h"
#include "net_link.h"
#include "battle_types.h"
#include "battle_anim.h"
#include "battle_helpers.h"
#include "battle_view.h"


namespace battle
{
    // ============================================================================================
    // THE BATTLE
    // ============================================================================================

    namespace detail
    {
        BN_DATA_EWRAM int16_t floor_pa_buf[bn::display::height()] = {};
        BN_DATA_EWRAM int floor_dx_buf[bn::display::height()] = {};
        BN_DATA_EWRAM int floor_dy_buf[bn::display::height()] = {};
        BN_DATA_EWRAM bn::fixed fs_slice_x_buf[bn::display::height()];
        BN_DATA_EWRAM bn::fixed fs_slice_y_buf[bn::display::height()];
    }

    class Battle
    {
    public:
        unsigned events = 0;                       // EV_... flags of the frame that just ran

        bool active() const { return _phase != PH_OFF; }

        // +1 = the player won, -1 = the player lost (read it after active() turns false)
        int result() const { return _result; }
        void set_cpu_skill(int skill) { _fight.debug_ai_skill = skill; }
        void set_options(bool use_3d, bool use_tint) { _use_3d = use_3d; _use_tint = use_tint; }
        void start(const FighterDef& player, const FighterDef& enemy, const StageDef& stage,
                   bn::ivector<bn::sprite_ptr>& covering_blocks, bn::sprite_text_generator& text, unsigned seed,
                   const FighterDef* partner = nullptr, const Buff& buff = Buff(),
                   const bn::sprite_item* bystander = nullptr, bool debug_mode = false, net::LinkSession* net_session = nullptr, int net_local_idx = -1, const FighterDef* waffle_ref = nullptr)
        {
            (void)text;
            _stage = &stage;
            _player = &player;
            _enemy = &enemy;
            _partner = partner;
            _bystander_item = bystander;
            _waffle_def = waffle_ref ? waffle_ref : &player;
            _net = net_session;
            _local_idx = (_net && net_local_idx >= 0) ? net_local_idx : 0;
            _debug_enabled = _net ? false : debug_mode;
            init_net_sync_buffers();
            _by_rng = (seed * 2246822519u) | 1u;
            _player_stats = with_buff(player.stats, buff);         // (kept in this object: the fight only points at it)
            _enemy_stats = enemy.stats;
            if(partner && !_net) { _enemy_stats.max_hp *= 2; }     // 2v1 fairness: challenged NPC gets 2x HP
            _result = 0;
            events = 0;

            for(bn::sprite_ptr& block : covering_blocks) { _blocks.push_back(block); }
            covering_blocks.clear();
            _wipe = 0;
            move_blocks();

            _fight.start(stage.geom, _player_stats, _enemy_stats, seed, partner ? &partner->stats : nullptr);
            _fight.waffle_ref_stats = _waffle_def ? &_waffle_def->stats : nullptr;
            _fight.net_client = false;      // lockstep: every console simulates everything, identically

            // The stage itself is built on the NEXT frame (see PH_SETUP): the overworld was just thrown away, and
            // video memory is much happier if the new pictures are loaded a frame later instead of in the same one. YAY :3
            _phase = PH_SETUP;
        }

        #include "battle_update.h"

        // When active() turned false the screen is still covered. Take the blocks back to slide them away.
        void release_blocks(bn::ivector<bn::sprite_ptr>& out)
        {
            for(bn::sprite_ptr& block : _blocks) { out.push_back(block); }
            _blocks.clear();
        }

    private:
        enum Phase { PH_OFF, PH_SETUP, PH_OPENING, PH_READY, PH_FIGHT, PH_FS_ENTER, PH_FS_CUTSCENE, PH_FS_EXIT, PH_RESULT, PH_CLOSING , PH_DEBUG_MENU , PH_NET_SYNC };

        // ================================================================================
        // FINAL SMASH TABLE: add new finishers here, then add a case in build_fs_scene()
        // and update_fs_cutscene() below!
        // ================================================================================
        struct FinalSmashDef { int damage, kb_x, kb_y, stun, heal, def_bonus, def_frames; };

        static const FinalSmashDef& final_smash_def(int id)
        {
            static constexpr FinalSmashDef waffle_fire = { 55, fp(8.5), fp(-5.5), 45, 0, 0, 0 };
            static constexpr FinalSmashDef filip_drop  = { 58, fp(7.5), fp(-7.0), 48, 0, 0, 0 };
            static constexpr FinalSmashDef umb_cart    = { 56, fp(8.0), fp(-6.0), 48, 0, 0, 0 };
                        static constexpr FinalSmashDef waffle_meal = { 0,  0,       0,        0,  MEAL_HEAL, MEAL_DEF_BONUS, MEAL_DEF_FRAMES };
            static constexpr FinalSmashDef har_slice   = { 65, fp(9.0), fp(-6.5), 50, 0, 0, 0 };
            switch(id)
            {
                case FS_HARUE_SLICE:  return har_slice;
                case FS_FILIP_DROP:   return filip_drop;
                case FS_UMB_MINECART: return umb_cart;
                case FS_WAFFLE_MEAL:  return waffle_meal;
                default: return waffle_fire;
            }
        }

        static constexpr int FLOOR_TILES = 5;
        static constexpr int NET_DELAY = 3;           // 5 x 64 pixels: always enough to cover the screen while scrolling
        static constexpr int WIPE_SPEED = 10;           // same as the overworld transition
        static constexpr int WIPE_DISTANCE = 248;
        static constexpr int FLOOR_DEPTH = 64;     // was 40
        static constexpr int FLOOR_ROWS = 32;      // unchanged
        static constexpr int FLOOR_FOCAL = 100;    // was 60

        static constexpr int RESULT_FRAMES = 200;       // how long the result screen lasts
        static constexpr int KO_FLASH = 12;             // the white flash fades out over this many frames
        static constexpr int KO_FREEZE = 10;            // the world is frozen for this many frames after the knockout
        static constexpr int KO_SLOW_END = 70;          // slow motion until this frame...
        static constexpr int KO_SLOW_RATE = 3;          // ...where the world only moves every 3rd frame
        static constexpr int KO_BANNER = 40;            // the frame GAME! appears on

        struct Piece
        {
            bn::sprite_ptr sprite;
            int wx;                                     // where it is on the stage (its middle)
            int wy;
        };

        
        #include "battle_scene.h"

        #include "battle_debug.h"

        #include "battle_fs_core.h"

        #include "battle_fs_harue.h"

        #include "battle_fs_waffle_meal.h"

        #include "battle_fs_umbreon.h"

        #include "battle_fs_filip.h"

        #include "battle_fs_waffle.h"

        #include "battle_draw.h"

        #include "battle_draw_fighter.h"


        // ---------------------------------------------------------------------------- data

        int _phase = PH_OFF;
        int _timer = 0;
        int _banner_timer = 0;
        int _result = 0;
        bn::fixed _wipe = 0;
        bn::fixed _cam_x = 0;
        bn::fixed _shake = 0;

        const StageDef* _stage = nullptr;
        const FighterDef* _player = nullptr;
        const FighterDef* _enemy = nullptr;
        const FighterDef* _partner = nullptr;
        const FighterDef* _waffle_def = nullptr;
        FighterStats _player_stats = {};                // the player's stats WITH the buff on top
        FighterStats _enemy_stats = {};                 // enemy's stats (doubled HP in 2v1 battles)
        Fight _fight;

        bn::optional<bn::regular_bg_ptr> _bgs[3];
        bn::optional<bn::affine_bg_ptr> _floor_bg;
        bn::optional<bn::affine_bg_pa_register_hbe_ptr> _floor_pa_hbe;
        bn::optional<bn::affine_bg_dx_register_hbe_ptr> _floor_dx_hbe;
        bn::optional<bn::affine_bg_dy_register_hbe_ptr> _floor_dy_hbe;
        bn::vector<bn::sprite_ptr, FLOOR_TILES> _floor;
        bn::vector<Piece, 24> _pieces;
        bn::vector<bn::sprite_ptr, 8> _label[MAX_PROJECTILES];      // the letters of a shot that is shown as text
        bn::vector<bn::fixed, 8> _label_dx[MAX_PROJECTILES];        // where each letter sits relative to the middle of the shot
        int _label_id[MAX_PROJECTILES] = {};                        // which shot the letters belong to (set to -1 when a battle starts)
        const bn::sprite_item* _bystander_item = nullptr;   // a follower that can't fight (see start())
        bn::optional<bn::sprite_ptr> _bystander;
        int _by_x16 = 0, _by_dir = DIR_DOWN, _by_timer = 0, _by_step = 0, _by_anim = 0, _by_shown = -1;
        bool _by_walking = false;
        unsigned _by_rng = 1;
        bn::optional<FighterView> _views[MAX_FIGHTERS]; // 0 = P1, 1 = P2/foe, 2 = partner, 3..4 = water clones
        bn::optional<bn::sprite_ptr> _proj[MAX_PROJECTILES];
        int _proj_id[MAX_PROJECTILES] = {};
        bn::optional<bn::sprite_ptr> _fx[MAX_EFFECTS];
        int _fx_kind[MAX_EFFECTS] = {};
        bn::vector<bn::sprite_ptr, 12> _blocks;
        bn::vector<bn::sprite_ptr, 12> _banner;

        int _fs_id = FS_NONE;
        int _fs_attacker = 0;
        int _fs_victim = 1;
        int _fs_timer = 0;
        int _fs_cam_x = 0;
        int _fs_flash = 0;
        bn::optional<bn::regular_bg_ptr> _fs_bg;
        bn::optional<bn::regular_bg_position_hbe_ptr> _fs_slice_hbe_x;
        bn::optional<bn::regular_bg_position_hbe_ptr> _fs_slice_hbe_y;
        bn::optional<bn::regular_bg_ptr> _ko_bg;
        bool _ko_overlay_hidden = false;
        bn::optional<bn::sprite_ptr> _fs_atk;
        bn::optional<bn::sprite_ptr> _fs_vic;
        bn::optional<bn::sprite_ptr> _fs_vic_bot;
        bn::optional<bn::sprite_tiles_ptr> _fs_vic_top_tiles;
        bn::optional<bn::sprite_tiles_ptr> _fs_vic_bot_tiles;
        bn::optional<bn::sprite_affine_mat_ptr> _fs_atk_mat;
        bn::optional<bn::sprite_affine_mat_ptr> _fs_vic_mat;
        bn::optional<bn::sprite_affine_mat_ptr> _fs_beam_mat;
        bn::optional<bn::sprite_palette_ptr> _fs_atk_pal;
        bn::optional<bn::sprite_palette_ptr> _fs_vic_pal;
        bn::optional<bn::sprite_ptr> _fs_aura[2];
        bn::optional<bn::sprite_ptr> _fs_beam[5];
        bn::optional<bn::sprite_ptr> _fs_spark;
        bn::optional<bn::sprite_ptr> _fs_impact;
        bn::vector<bn::sprite_ptr, 12> _fs_banner;
        bool _debug_enabled = false;
        bool _use_3d = true;
        bool _use_tint = true;
        int _dbg_cursor = 0;
        bn::vector<bn::sprite_ptr, 6> _dbg_box;
        bn::vector<bn::sprite_ptr, 64> _dbg_text;
        bn::optional<bn::sprite_ptr> _table_sprites[MAX_FIGHTERS];
        bn::optional<bn::sprite_ptr> _chg_back;
        bn::optional<bn::sprite_ptr> _chg_fill;
        bn::optional<bn::sprite_affine_mat_ptr> _chg_mat;
        bn::optional<bn::sprite_ptr> _block_sprites[MAX_BLOCKS];
        bn::optional<bn::sprite_ptr> _fs_cart;
        bn::optional<bn::sprite_affine_mat_ptr> _fs_cart_mat;
        bn::optional<bn::sprite_ptr> _freeze_aura[2];
        net::LinkSession* _net = nullptr;
        int _local_idx = 0;
        int _net_frame = 0;
        int _net_sync_ticks = 0;
        int _net_ev_send = 0;
        int _net_ev_pkt = 0;
        int _net_fs_seq = 0;
        bool _desync_shown = false;
        bool _net_clone_prev[2] = {};
        int _net_clone_burst[2] = {};

        // Link battles, on frames where the sim is not running: keep talking to the peer
        void pump_net()
        {
            bn::link::send(_net->ls.next_packet(true));
            for(int i = 0; i < 4 && _net->poll(); ++i) {}
        }

        void init_net_sync_buffers()
        {
            _desync_shown = false;
            _net_frame = 0;
            _net_ev_send = 0;
            _net_ev_pkt = 0;
            _net_fs_seq = 0;
            if(_net) { _net->reset_battle_sync(); }
        }
    };
}

#endif
