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

namespace battle
{
    // ============================================================================================
    // DATA YOU WRITE (in main.cpp): fighters and stages
    // ============================================================================================

    // A fighter = a name, a sprite sheet (the same layout as the overworld sheet) and numbers (see FighterStats).
    struct FighterDef
    {
        const char* name;
        const bn::sprite_item* sprite;
        FighterStats stats;
    };

    // One scrolling background picture. speed_percent: 100 = moves as fast as the camera (the closest layer),
    // 25 = moves slowly (far away). The farthest layer goes first. (Backgrounds repeat sideways, so a picture
    // that tiles left-to-right looks best on long stages.)
    struct ParallaxLayer
    {
        const bn::regular_bg_item* item;
        int speed_percent;
        bool in_front = false;
    };

    // A stage = the shape of the ground (see StageGeom) + up to 3 background layers.
    struct StageDef
    {
        StageGeom geom;
        ParallaxLayer layers[3];
        int layer_count;
        const bn::sprite_item* floor_item = nullptr;      // nullptr = the default grass floor
        const bn::sprite_item* platform_item = nullptr;   // nullptr = the default wood platform
        const bn::affine_bg_item* top_item = nullptr;   // the picture on top of the 3D ground (nullptr = flat 2D stage)
    };

    // ---- things main.cpp can react to (sound effects) ----
    enum : unsigned
    {
        EV_SHOT    = 1 << 0,    // somebody fired
        EV_HIT     = 1 << 1,    // somebody got hit
        EV_BLOCKED = 1 << 2,    // ...and it landed on a shield
        EV_JUMP    = 1 << 3,
        EV_KO      = 1 << 4,
        EV_FIGHT   = 1 << 5,    // the fight just started
        EV_SWING   = 1 << 6,    // a melee move started (jab, smash, aerial...)
        EV_DASH    = 1 << 7,    // an aqua jet took off
        EV_ABSORB  = 1 << 8,    // fox thief stole a shot
        EV_BOOM    = 1 << 9,    // the dair explosion
        EV_SMASH   = 1 << 10,
        EV_FINAL_SMASH = 1 << 11,
        EV_MINING = 1 << 12,
        EV_GRAB = 1 << 13,
    };

    // ============================================================================================
    // WHICH PICTURE TO SHOW. You have no jump / attack / shield animations yet, so the walking and
    // running frames stand in for them. When you draw real ones, only this function needs to change.
    // The sheet is 4 directions x 3 frames, and the running frames come after the walking ones.
    // ============================================================================================

    constexpr int DIR_UP = 0, DIR_DOWN = 1, DIR_LEFT = 2, DIR_RIGHT = 3, SHIELD = 28, KO = 24, HURT = 25;
    //waffles specific animation
    constexpr int WAF_SIDE_JET = 30, WAF_UP_JET = 32, WAF_KICK = 51;
    constexpr int walk_frames[4] = { 0, 1, 0, 2 };

    constexpr int waff_jab[4] = {33,34,35,36};
    constexpr int waff_down_smash[8] = {41,42,43,44,44,45,46,47};
    constexpr int waff_up_smash[8] = {47,46,45,44,44,43,42,41};
    constexpr int waff_returntosender[2] = {33,34};
    constexpr int waff_left_returntosender = 4;
    constexpr int waff_jab_left_ofset = 4;
    constexpr int waff_tail_slam[7] = {48,49,50,51,52,53,54};
    constexpr int waff_tail_slam_left_offset = 7;

    // Filip (snep sheet). Everything is listed for facing RIGHT; the "offset" is added when he faces LEFT.
    constexpr int snep_basic_punch[5] = {30,31,32,33,34};
    constexpr int snep_basic_punch_offset = 5;
    constexpr int snep_counter[5] = {40,41,42,43,44};           // the counter stance
    constexpr int snep_counter_offset = 0;                      // 0 = the same frames 40-44 facing left too. If the sheet has a separate
                                                                // left version, put its offset here (careful: a frame that is not on the sheet crashes)
    constexpr int snep_grab[5] = {34,33,32,33,34};              // grabbing the attacker (facing left = +5 = 40,39,38,39,40)
    constexpr int snep_grab_offset = 5;
    constexpr int snep_throw_pose = 35;                         // the pose he finishes the spin on when he throws the shot back
    constexpr int snep_throw_pose_offset = 5;                   // (facing left = 40)

    constexpr int umb_mine[4] = {30,31,32,33};
    constexpr int umb_sword_slash[4] = {38,39,40,41};
    constexpr int umb_place[4] = {46,47,48,49};
    constexpr int umb_left_offset = 4;

    constexpr int har_slash[3]={30,31,32};
    constexpr int har_slash_offset = 3;

    constexpr int SPRITE_FEET = 12;                  // from the middle of the sprite down to the feet (as in the overworld)

    inline int sheet_frame(int dir, bool running)
    {
        return (running ? 12 : 0) + dir * 3;
    }
    // Plays pictures[0 .. count-1] ONCE, `ticks` game frames per picture, starting `begin` frames into the move.
    // Returns -1 when it isn't playing (before it starts, or after it ends unless hold_last is true).
    inline int play_once(const int* pictures, int count, int ticks, int begin, int move_frame, bool hold_last = false)
    {
        const int t = move_frame - begin;
        if(t < 0) { return -1; }
        if(t >= count * ticks) { return hold_last ? pictures[count - 1] : -1; }
        return pictures[t / ticks];
    }

    // The pose during a move. These are all stand-ins made from the frames you already have: moves that go up show
    // the "up" frames, moves that go down show the "down" frames, and the spin turns through all four directions.
    // Shared 1:1 slash sequence so Harue's hands and 64x64 sword play ONCE together
    inline int har_slash_step(int anim, int mf, bool& spin_left_half)
    {
        spin_left_half = false;
        switch(anim)
        {
            case ANIM_HAR_JAB:
                if(mf >= 0 && mf <= 8)   { return mf / 3; }
                if(mf >= 10 && mf <= 18) { return 2 - ((mf - 10) / 3); }
                if(mf >= 20 && mf <= 28) { return (mf - 20) / 3; }
                return -1;
            case ANIM_HAR_FSMASH:
                if(mf >= 0 && mf < 10)   { return 0; }
                if(mf >= 10 && mf < 15)  { return 1; }
                if(mf >= 15 && mf < 21)  { return 2; }
                return -1;
            case ANIM_HAR_FS:
                if(mf >= 0 && mf < 5)    { return 0; }
                if(mf >= 5 && mf < 10)   { return 1; }
                if(mf >= 10 && mf < 16)  { return 2; }
                return -1;
            case ANIM_HAR_USMASH:
            case ANIM_HAR_BSHOT:
                if(mf >= 0 && mf < 4)    { return 0; }
                if(mf >= 4 && mf < 8)    { return 1; }
                if(mf >= 8 && mf < 13)   { return 2; }
                return -1;
            case ANIM_HAR_DASH:
                if(mf >= 5 && mf < 9)    { return 0; }
                if(mf >= 9 && mf < 13)   { return 1; }
                if(mf >= 13 && mf < 18)  { return 2; }
                return -1;
            case ANIM_HAR_UPDOWN:
                if(mf >= 4 && mf < 8)    { return 0; }
                if(mf >= 8 && mf < 12)   { return 1; }
                if(mf >= 12 && mf < 17)  { return 2; }
                if(mf >= 24 && mf < 28)  { return 2; }
                if(mf >= 28 && mf < 32)  { return 1; }
                if(mf >= 32 && mf < 37)  { return 0; }
                return -1;
            case ANIM_HAR_SPIN:
                if(mf >= 3 && mf < 12)   { return (mf - 3) / 3; }
                if(mf >= 12 && mf < 21)  { spin_left_half = true; return (mf - 12) / 3; }
                return -1;
            default:
                return -1;
        }
    }

    inline int attack_frame(int anim, int facing, int side, int move_frame)
    {
        const int WAF_DIR_JET = facing > 0 ? WAF_SIDE_JET : WAF_SIDE_JET + 1;
        switch(anim)
        {
            case ANIM_SNEP_PUNCH:
            {
                const int pic = play_once(snep_basic_punch, 5, 4, 0, move_frame);
                return pic >= 0 ? pic + (side == DIR_LEFT ? snep_basic_punch_offset : 0) : sheet_frame(side, false);
            }
            case ANIM_SNEP_JAB:
            {
                if(move_frame >= 32) { return sheet_frame(side, false); }
                return snep_basic_punch[(move_frame % 10) * 5 / 10] + (side == DIR_LEFT ? snep_basic_punch_offset : 0);
            }
            case ANIM_SNEP_HEAVY:
            {
                const int off = side == DIR_LEFT ? snep_basic_punch_offset : 0;
                if(move_frame < 20) { return snep_basic_punch[0] + off; }
                if(move_frame < 34) { return snep_basic_punch[1] + off; }
                if(move_frame < 37) { return snep_basic_punch[3] + off; }
                if(move_frame < 50) { return snep_basic_punch[4] + off; }
                return sheet_frame(side, false);
            }
            case ANIM_SNEP_FLIP:
            {
                static const int order[4] = { DIR_RIGHT, DIR_UP, DIR_LEFT, DIR_DOWN };
                if(move_frame < 4 || move_frame >= 24) { return sheet_frame(side, false); }
                const int step = ((move_frame - 4) / 3) % 4;
                return sheet_frame(facing > 0 ? order[step] : order[(4 - step) % 4], false);
            }
            case ANIM_SNEP_DSMASH:
            {
                const int left = play_once(snep_basic_punch, 5, 3, 0, move_frame);
                if(left >= 0) { return left + snep_basic_punch_offset; }
                const int right = play_once(snep_basic_punch, 5, 3, 16, move_frame);
                return right >= 0 ? right : sheet_frame(side, false);
            }
            case ANIM_SNEP_COUNTER:
            {
                const int pic = play_once(snep_counter, 5, 4, 0, move_frame, true);
                return pic + (side == DIR_LEFT ? snep_counter_offset : 0);
            }
            case ANIM_SNEP_COUNTER_GRAB:
            {
                const int grab = play_once(snep_grab, 5, 6, 0, move_frame);
                if(grab >= 0) { return grab + (side == DIR_LEFT ? snep_grab_offset : 0); }
                const int punch = play_once(snep_basic_punch, 5, 3, 30, move_frame);
                if(punch >= 0) { return punch + (side == DIR_LEFT ? snep_basic_punch_offset : 0); }
                return sheet_frame(side, false);
            }
            case ANIM_SNEP_SPIN_THROW:
            {
                static const int order[4] = { DIR_DOWN, DIR_RIGHT, DIR_UP, DIR_LEFT };
                if(move_frame < 24) { return sheet_frame(order[(move_frame / 3) % 4], false); }
                return snep_throw_pose + (side == DIR_LEFT ? snep_throw_pose_offset : 0);
            }
            case ANIM_SNEP_THROW_UP:
                return sheet_frame(DIR_UP, true) + (move_frame >= 10 ? 2 : 0);
            case ANIM_SNEP_LEAP:
                if(move_frame < 6 || move_frame >= 30) { return sheet_frame(side, false); }
                return sheet_frame(side, true) + 1;
            case ANIM_SNEP_SCRATCH:
            {
                if(move_frame >= 50) { return sheet_frame(side, false); }
                return snep_basic_punch[(move_frame / 4) % 2 == 0 ? 2 : 3] + (side == DIR_LEFT ? snep_basic_punch_offset : 0);
            }
            case ANIM_SNEP_DAIR:
                return sheet_frame(side, true) + 1;
            case ANIM_SLAP:
                if(move_frame < 30) { return waff_jab[(move_frame % 10) * 4 / 10] + (side == DIR_LEFT ? waff_jab_left_ofset : 0); }
                return sheet_frame(side, false);
            case ANIM_TAIL:
            {
                const int pic = play_once(waff_tail_slam, 7, 4, 0, move_frame);
                return pic >= 0 ? pic + (side == DIR_LEFT ? waff_tail_slam_left_offset : 0) : sheet_frame(side, false);
            }
            case ANIM_RTS:
            {
                const int pic = play_once(waff_returntosender, 2, 3, 0, move_frame);
                return pic >= 0 ? pic + (side == DIR_LEFT ? waff_left_returntosender : 0) : sheet_frame(side, false);
            }
            case ANIM_KICK:     return side == DIR_RIGHT ? WAF_KICK : WAF_KICK + 7;
            case ANIM_SHOOT:    return sheet_frame(side, true) + 2;
            case ANIM_TAIL_UP:
            {
                const int pic = play_once(waff_up_smash, 8, 5, 0, move_frame);
                return pic >= 0 ? pic : sheet_frame(side, false);
            }
            case ANIM_JET_UP:   return WAF_UP_JET;
            case ANIM_SLAM:
            {
                const int pic = play_once(waff_down_smash, 8, 5, 0, move_frame);
                return pic >= 0 ? pic : sheet_frame(side, false);
            }
            case ANIM_JET_DOWN: return sheet_frame(DIR_DOWN, true) + 1;
            case ANIM_JET_SIDE: return WAF_DIR_JET;
            case ANIM_SPIN:
            {
                static const int order[4] = { DIR_DOWN, DIR_RIGHT, DIR_UP, DIR_LEFT };
                return sheet_frame(order[(move_frame / 3) % 4], false);
            }
            case ANIM_UMB_MINE:
            {
                const int pic = play_once(umb_mine, 4, 6, 0, move_frame);
                return pic >= 0 ? pic + (side == DIR_LEFT ? umb_left_offset : 0) : sheet_frame(side, false);
            }
            case ANIM_UMB_SLASH:
            {
                const int pic = play_once(umb_sword_slash, 4, 5, 0, move_frame);
                return pic >= 0 ? pic + (side == DIR_LEFT ? umb_left_offset : 0) : sheet_frame(side, false);
            }
            case ANIM_UMB_PLACE:
            {
                const int pic = play_once(umb_place, 4, 5, 0, move_frame);
                return pic >= 0 ? pic + (side == DIR_LEFT ? umb_left_offset : 0) : sheet_frame(side, false);
            }
            case ANIM_UMB_BOW:
                return sheet_frame(DIR_UP, true) + (move_frame >= 12 ? 2 : 0);
            case ANIM_UMB_THROW:
                return sheet_frame(side, true) + (move_frame >= 7 ? 2 : 0);
            case ANIM_WAF_CHARGE:
                return sheet_frame(DIR_UP, true) + ((move_frame / 4) % 2);
            case ANIM_HAR_JAB:
            case ANIM_HAR_FSMASH:
            case ANIM_HAR_FS:
            case ANIM_HAR_USMASH:
            case ANIM_HAR_DASH:
            case ANIM_HAR_BSHOT:
            case ANIM_HAR_UPDOWN:
            {
                bool left_half = false;
                const int idx = har_slash_step(anim, move_frame, left_half);
                if(idx >= 0) { return har_slash[idx] + (side == DIR_LEFT ? har_slash_offset : 0); }
                if(anim == ANIM_HAR_UPDOWN && move_frame >= 16 && move_frame < 24) { return sheet_frame(DIR_UP, true) + 1; }
                return sheet_frame(side, false);
            }
            case ANIM_HAR_SPIN:
            {
                static const int order[4] = { DIR_DOWN, DIR_RIGHT, DIR_UP, DIR_LEFT };
                return sheet_frame(order[(move_frame / 3) % 4], false);
            }
            case ANIM_HAR_REFLECT:
                return facing > 0 ? SHIELD : SHIELD + 1;
            case ANIM_HAR_CHARGE:
                return facing > 0 ? SHIELD : SHIELD + 1;
            case ANIM_STANCE:   return sheet_frame(DIR_DOWN, false);
            default:            return sheet_frame(side, true);
        }
    }

    // `anim` and `move_frame` only matter while attacking (state == ST_ATTACK)
    inline int pick_frame(int state, int facing, int step, int anim = ANIM_STAND, int move_frame = 0)
    {
        const int side = facing > 0 ? DIR_RIGHT : DIR_LEFT;
        const int shield_side = facing > 0 ? SHIELD : SHIELD+1;
        const int ko_side = facing > 0 ? KO : KO+1;
        const int hurt_side = facing > 0 ? HURT : HURT +2;
        switch(state)
        {
            case ST_WALK:     return sheet_frame(side, true) + walk_frames[step];     // the running frames
            case ST_AIR:      return sheet_frame(side, true) + 1;                     // placeholder for a jump animation
            case ST_ATTACK:   return attack_frame(anim, facing, side, move_frame);            // placeholders for the attack animations
            case ST_SHIELD:   return shield_side;
            case ST_HURT:     return hurt_side;
            case ST_GRABBED:  return hurt_side;
            case ST_KO:       return ko_side;
            default:          return sheet_frame(side, false);                        // standing (and flinching)
        }
    }

    inline int safe_frame(const bn::sprite_item& item, int frame, int fallback)
    {
        const int count = item.tiles_item().graphics_count();
        return (frame >= 0 && frame < count) ? frame : fallback;
    }

    // What a shot looks like: its sprite sheet, how many pictures it has, and how many game frames each is shown.
    // A new PROJ_... kind (battle_logic.h) needs a line here.
    struct ProjArt
    {
        const bn::sprite_item* item;
        int frames;
        int ticks;
        int first = 0;
    };

    inline ProjArt projectile_art(int kind)
    {
        switch(kind)
        {
            case PROJ_EXPLOSION: return ProjArt{ &bn::sprite_items::explosion, 4, 3 };
            case PROJ_JUNK_PENCIL: return ProjArt{ &bn::sprite_items::junk, 1, 1, 0 };
            case PROJ_JUNK_APPLE:  return ProjArt{ &bn::sprite_items::junk, 1, 1, 1 };
            case PROJ_JUNK_BAT:    return ProjArt{ &bn::sprite_items::junk, 1, 1, 2 };
            case PROJ_JUNK_LAPTOP: return ProjArt{ &bn::sprite_items::junk, 1, 1, 3 };
            case PROJ_JUNK_BALL:   return ProjArt{ &bn::sprite_items::junk, 1, 1, 4 };
            case PROJ_FIREBALL:    return ProjArt{ &bn::sprite_items::mc_props, 2, 4, 2 };
            case PROJ_BOOMERANG:   return ProjArt{ &bn::sprite_items::mc_props, 4, 3, 6 };
            case PROJ_ARROW:       return ProjArt{ &bn::sprite_items::mc_props, 1, 1, 10 };
            case PROJ_MINECART:    return ProjArt{ &bn::sprite_items::mc_rides, 2, 4, 2 };
            case PROJ_TNT:         return ProjArt{ &bn::sprite_items::mc_props, 1, 1, 4 };
            case PROJ_SONIC_SLASH: return ProjArt{ &bn::sprite_items::sonic_slash, 2, 3, 0 };
            default:             return ProjArt{ &bn::sprite_items::water_shot, 2, 5 };   // PROJ_WATER
        }
    }

    // ============================================================================================
    // SMALL HELPERS
    // ============================================================================================

    inline void format_int(int value, char* out)
    {
        char tmp[12];
        int t = 0, n = 0;
        if(value < 0) { out[n++] = '-'; value = -value; }
        if(value == 0) { tmp[t++] = '0'; }
        while(value > 0) { tmp[t++] = char('0' + value % 10); value /= 10; }
        while(t > 0) { out[n++] = tmp[--t]; }
        out[n] = 0;
    }

    // the player's buttons -> what the rules need
    inline Input read_player_input()
    {
        Input in;
        if(bn::keypad::left_held())  { in.move -= 1; }
        if(bn::keypad::right_held()) { in.move += 1; }
        in.up      = bn::keypad::up_held();
        in.down    = bn::keypad::down_held();
        in.jump    = bn::keypad::r_pressed();
        in.attack  = bn::keypad::a_pressed();
        in.special = bn::keypad::b_pressed();
        in.special_held = bn::keypad::b_held();
        in.shield  = bn::keypad::l_held();
        in.final_smash = bn::keypad::select_pressed();
        return in;
    }

    // Link battles: the RAW held buttons (the "pressed this frame" edges are derived later, identically on both consoles)
    inline unsigned read_player_held()
    {
        unsigned h = 0;
        if(bn::keypad::left_held())   { h |= net::Lockstep::H_LEFT; }
        if(bn::keypad::right_held())  { h |= net::Lockstep::H_RIGHT; }
        if(bn::keypad::up_held())     { h |= net::Lockstep::H_UP; }
        if(bn::keypad::down_held())   { h |= net::Lockstep::H_DOWN; }
        if(bn::keypad::l_held())      { h |= net::Lockstep::H_SHIELD; }
        if(bn::keypad::r_held())      { h |= net::Lockstep::H_JUMP; }
        if(bn::keypad::a_held())      { h |= net::Lockstep::H_ATTACK; }
        if(bn::keypad::b_held())      { h |= net::Lockstep::H_SPECIAL; }
        if(bn::keypad::select_held()) { h |= net::Lockstep::H_FS; }
        return h;
    }

    inline Input expand_held(unsigned h, unsigned prev)
    {
        Input in;
        if(h & net::Lockstep::H_LEFT)  { in.move -= 1; }
        if(h & net::Lockstep::H_RIGHT) { in.move += 1; }
        in.up      = (h & net::Lockstep::H_UP) != 0;
        in.down    = (h & net::Lockstep::H_DOWN) != 0;
        in.shield  = (h & net::Lockstep::H_SHIELD) != 0;
        const unsigned rising = h & ~prev;
        in.jump        = (rising & net::Lockstep::H_JUMP) != 0;
        in.attack      = (rising & net::Lockstep::H_ATTACK) != 0;
        in.special     = (rising & net::Lockstep::H_SPECIAL) != 0;
        in.special_held = (h & net::Lockstep::H_SPECIAL) != 0;
        in.final_smash = (rising & net::Lockstep::H_FS) != 0;
        return in;
    }

    // ============================================================================================
    // ONE FIGHTER ON SCREEN: the character, the shield bubble, the health bar and the name
    // ============================================================================================

    struct FighterView
    {
        bn::sprite_ptr sprite;
        bn::optional<bn::sprite_ptr> shield;
        bn::sprite_affine_mat_ptr shield_mat;
        bn::sprite_ptr hp_back;
        bn::sprite_ptr hp_fill;
        bn::sprite_affine_mat_ptr hp_mat;
        bn::vector<bn::sprite_ptr, 12> name;
        bn::vector<bn::sprite_ptr, 5> hp_text;
        bn::optional<bn::sprite_ptr> swipes[2];     // slash pictures where melee hitboxes are active
        bn::optional<bn::sprite_ptr> jet;           // the aqua jet bubble
        bn::optional<bn::sprite_ptr> held;          // the shot fox thief is holding above the head
        bn::optional<bn::sprite_ptr> pig;           // the pig Umbreon rides
        bn::optional<bn::sprite_ptr> sword;
        bn::optional<bn::sprite_ptr> slash_fx;
        bn::optional<bn::sprite_affine_mat_ptr> sword_mat;
        bn::vector<bn::sprite_ptr, 12> craft_text;
        int shown_mat = -1;
        int shown_tier = -1;
        int shown_dur = -1;
        int held_kind = -1;
        const bn::sprite_item* sheet = nullptr;     // the sprite sheet this character was made from
        int shown_frame = -1;
        int step = 0;
        int anim_timer = 0;
        bn::optional<bn::sprite_palette_ptr> pal;
        int shown_hp = -1;
        bool on_right = false;                     // the opponent's HUD is on the right
        int hud_dy = 0;                            // the partner's name and health bar sit this many pixels lower than the player's
        int tint_mode = 0;                           // 0 = none, 1 = red, 2 = blue, 3 = water cyan
        bool is_clone = false;

        // `partner` = a computer-controlled friend on the player's side: tinted blue, HUD on a second row, drawn behind the player.
        FighterView(const FighterDef& def, bool opponent, const char* label, bn::sprite_text_generator& text, bool partner = false, int tint_override = -1, bool clone = false) :
            sprite(def.sprite->create_sprite(0, 0, pick_frame(ST_IDLE, opponent ? -1 : 1, 0))),   // made at its real first frame: tiles are only loaded once
            shield_mat(bn::sprite_affine_mat_ptr::create()),
            hp_back(bn::sprite_items::hp_bar.create_sprite(0, 0, 0)),
            hp_fill(bn::sprite_items::hp_bar.create_sprite(0, 0, 1)),
            hp_mat(bn::sprite_affine_mat_ptr::create()),
            on_right(opponent),
            is_clone(clone)
        {
            sheet = def.sprite;
            shown_frame = pick_frame(ST_IDLE, opponent ? -1 : 1, 0);
            sprite.set_bg_priority(0);
            sprite.set_z_order(clone ? 2 : (partner ? 1 : (opponent ? 0 : -1)));
            hud_dy = partner ? 26 : 0;

            tint_mode = (tint_override >= 0) ? tint_override : (opponent ? 1 : (partner ? 2 : 0));
            pal = bn::sprite_palette_ptr::create_new(def.sprite->palette_item());
            if(tint_mode == 1)      { pal->set_fade(bn::color(31, 6, 6), 0.35); }
            else if(tint_mode == 2) { pal->set_fade(bn::color(6, 12, 31), 0.35); }
            else if(tint_mode == 3) { pal->set_fade(bn::color(4, 22, 31), 0.55); }
            sprite.set_palette(*pal);



            hp_back.set_bg_priority(0);
            hp_back.set_z_order(-10);
            hp_fill.set_bg_priority(0);
            hp_fill.set_z_order(-11);
            hp_fill.set_affine_mat(hp_mat);

            const int bar_center_x = opponent ? 96 : -96;             // the bars are 32 pixels wide
            hp_back.set_position(bar_center_x, -58 + hud_dy);
            hp_fill.set_position(bar_center_x, -58 + hud_dy);

            if(clone)
            {
                hp_back.set_visible(false);
                hp_fill.set_visible(false);
            }
            else if(opponent) { text.set_right_alignment(); text.generate(112, -72, label, name); }
            else              { text.set_left_alignment();  text.generate(-112, -72 + hud_dy, label, name); }
            text.set_center_alignment();

        }
    };

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
                    const bool knockout = _fight.f[0].hp <= 0 || _fight.f[1].hp <= 0;
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

        // ---------------------------------------------------------------------------- building the stage

        void build_stage_bgs()
        {
            const StageDef& stage = *_stage;
            for(int i = 0; i < stage.layer_count; ++i)
            {
                _bgs[i] = stage.layers[i].item->create_bg(0, 0);
                _bgs[i]->set_priority(stage.layers[i].in_front ? 0 : 3 - i);
            }
            if(stage.top_item)
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
                _views[0] = FighterView(*_player, false, _local_idx == 0 ? "YOU" : "P1", text, false, _local_idx == 0 ? 0 : 1);
                _views[1] = FighterView(*_enemy,  true,  _local_idx == 1 ? "YOU" : "P2", text, false, _local_idx == 1 ? 0 : 1);
            }
            else
            {
                _views[0] = FighterView(*_player, false, "YOU", text);
                _views[1] = FighterView(*_enemy, true, _enemy->name, text);
            }
            if(_partner) { _views[2] = FighterView(*_partner, false, _partner->name, text, true); }
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
            _fs_atk_mat.reset();
            _fs_vic_mat.reset();
            _fs_beam_mat.reset();
            _fs_atk_pal.reset();
            _fs_vic_pal.reset();
            _fs_bg.reset();
        }

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
                text.generate(-84, -32 + row * 15, line, _dbg_text);
            };

            put_line(0, "AI: ", ai_str);
            put_line(1, "MOVE: ", debug_slot_name(_fight.debug_force_slot));
            put_line(2, "CTRL: ", ctrl_name);
            put_line(3, "FILL MY SMASH", _fight.f[ctrl].smash >= SMASH_MAX ? " [FULL]" : " [A]");
            put_line(4, "HEAL BOTH", " [A]");
            put_line(5, "END BATTLE", " [A]");

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
            if(bn::keypad::up_pressed())   { _dbg_cursor = (_dbg_cursor + 5) % 6; redraw = true; }
            if(bn::keypad::down_pressed()) { _dbg_cursor = (_dbg_cursor + 1) % 6; redraw = true; }

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

            if(redraw) { draw_debug_menu(text); }
        }

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

        // ---- FS_HARUE_SLICE cutscene: Blue Dimension -> Pan Right -> Giant Blue Slash -> Screen Slice! ----
        void build_har_slice_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const FighterDef* vic_def = fighter_def(_fs_victim);
            _fs_cam_x = 0;

            _fs_bg = bn::regular_bg_items::blue_dim_bg.create_bg(136, 0);
            _fs_bg->set_priority(3);
            for(int r = 0; r < bn::display::height(); ++r)
            {
                detail::fs_slice_x_buf[r] = 0;
                detail::fs_slice_y_buf[r] = 0;
            }
            _fs_slice_hbe_x = bn::regular_bg_position_hbe_ptr::create_horizontal(*_fs_bg, detail::fs_slice_x_buf);
            _fs_slice_hbe_y = bn::regular_bg_position_hbe_ptr::create_vertical(*_fs_bg, detail::fs_slice_y_buf);

            _fs_atk_mat = bn::sprite_affine_mat_ptr::create();
            _fs_atk_mat->set_scale(1.5);
            _fs_atk_pal = bn::sprite_palette_ptr::create_new(atk_def->sprite->palette_item());
            _fs_atk_pal->set_fade(bn::color(4, 18, 31), 0.35);

            _fs_atk = atk_def->sprite->create_sprite(272, 18, har_slash[0] + har_slash_offset);
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

            _fs_vic = vic_def->sprite->create_sprite(-24, 18, safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_RIGHT, false)));
            _fs_vic->set_bg_priority(0);
            _fs_vic->set_z_order(-1);
            _fs_vic->set_affine_mat(*_fs_vic_mat);
            _fs_vic->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_vic->set_palette(*_fs_vic_pal);

            // Giant Blue Slash wave (2.4x scaled sonic_slash)
            _fs_beam_mat = bn::sprite_affine_mat_ptr::create();
            _fs_beam_mat->set_scale(2.4);
            _fs_beam_mat->set_horizontal_flip(true);
            _fs_beam[0] = bn::sprite_items::sonic_slash.create_sprite(0, 12, 0);
            _fs_beam[0]->set_bg_priority(0);
            _fs_beam[0]->set_z_order(-3);
            _fs_beam[0]->set_affine_mat(*_fs_beam_mat);
            _fs_beam[0]->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_beam[0]->set_visible(false);

            // Cut-line rift flashes along the diagonal screen slice
            for(int i = 1; i < 4; ++i)
            {
                _fs_beam[i] = bn::sprite_items::blue_slash.create_sprite(0, 0, 1);
                _fs_beam[i]->set_bg_priority(0);
                _fs_beam[i]->set_z_order(-5);
                _fs_beam[i]->set_visible(false);
            }

            _fs_impact = bn::sprite_items::explosion.create_sprite(-24, 14, 0);
            _fs_impact->set_bg_priority(0);
            _fs_impact->set_z_order(-4);
            _fs_impact->set_visible(false);

            // Harue's 64x64 sword in cutscene
            _fs_cart_mat = bn::sprite_affine_mat_ptr::create();
            _fs_cart_mat->set_scale(1.5);
            _fs_cart = bn::sprite_items::harueslice_sword.create_sprite(0, 18, 3);
            _fs_cart->set_bg_priority(0);
            _fs_cart->set_z_order(-2);
            _fs_cart->set_affine_mat(*_fs_cart_mat);
            _fs_cart->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_cart->set_visible(false);

            text.set_center_alignment();
            text.generate(0, -58, "DIMENSION SLASH!", _fs_banner);
            for(bn::sprite_ptr& sp : _fs_banner) { sp.set_bg_priority(0); sp.set_z_order(-10); }
        }

        void update_har_slice_fs(bn::sprite_text_generator& text)
        {
            (void)text;
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const FighterDef* vic_def = fighter_def(_fs_victim);
            const int t = _fs_timer++;

            if(t <= 10) { set_flash(bn::fixed(10 - t) / 10); }
            if(t == 24) { _fs_banner.clear(); }

            // 1. Pan camera right (25..62) to reveal Harue about to slash
            if(t >= 25 && t < 62)
            {
                _fs_cam_x = (t - 25) * 272 / 37;
                if(_fs_cam_x > 272) { _fs_cam_x = 272; }
            }
            // 2. Harue winds up and slashes (63..82), spawning giant blue slash at t=70
            int slash_wx = 250;
            if(t >= 70 && t < 125)
            {
                slash_wx = 250 - (t - 70) * 274 / 55;
            }
            // 3. Camera chases the giant blue slash back to Waffles (76..104)
            if(t >= 76 && t < 105)
            {
                _fs_cam_x = 272 - (t - 76) * 272 / 29;
                if(_fs_cam_x < 0) { _fs_cam_x = 0; }
            }
            else if(t >= 105)
            {
                _fs_cam_x = 0;
            }

            // Update Harue & his sword
            const int ax = 272 - _fs_cam_x;
            _fs_atk->set_visible(ax > -150 && ax < 150);
            _fs_atk->set_position(ax, 18);
            if(t >= 68 && t < 84)
            {
                const int st = (t - 68) / 5;
                const int sidx = (st > 2) ? 2 : st;
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), har_slash[sidx] + har_slash_offset);
                _fs_cart->set_visible(_fs_atk->visible());
                _fs_cart->set_position(ax, 18);
                _fs_cart->set_tiles(bn::sprite_items::harueslice_sword.tiles_item(), sidx);
                _fs_cart->set_horizontal_flip(true);
                
            }
            else
            {
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), (t >= 48 && t < 68) ? (har_slash[0] + har_slash_offset) : sheet_frame(DIR_LEFT, false));
                _fs_cart->set_visible(false);
            }
            if(t == 68) { events |= EV_SWING | EV_SHOT; }

            // Giant blue slash projectile heading toward victim
            if(t >= 70 && t < 125)
            {
                const int sx = slash_wx - _fs_cam_x;
                _fs_beam[0]->set_visible(sx > -160 && sx < 160);
                _fs_beam[0]->set_position(sx, 14);
                _fs_beam[0]->set_tiles(bn::sprite_items::sonic_slash.tiles_item(), (t / 3) % 2);
            }
            else
            {
                _fs_beam[0]->set_visible(false);
            }

            // Victim + Screen Slice Impact (125..174)
            int shake_x = 0;
            int shake_y = 0;
            bn::fixed slice_gap = 0;
            if(t >= 125)
            {
                if(t == 125) { events |= EV_BOOM | EV_SMASH; }
                if(t == 138) { events |= EV_BOOM | EV_HIT; }

                const int dt = t - 125;
                const int pwr = dt < 36 ? (6 - dt * 6 / 36) : 0;
                shake_x = (t % 2 == 0) ? pwr : -pwr;
                shake_y = (t % 4 < 2) ? pwr : -pwr;
                if(t <= 135) { set_flash(bn::fixed(135 - t) / 10); }

                slice_gap = dt < 22 ? bn::fixed(dt) * bn::fixed(1.6) : bn::fixed(35);

                // Explosion + diagonal slice rift flashes across the screen
                _fs_impact->set_visible(dt < 36);
                _fs_impact->set_tiles(bn::sprite_items::explosion.tiles_item(), (dt / 3) % 4);
                _fs_impact->set_position(-24 + shake_x, 14 + shake_y);

                for(int i = 1; i < 4; ++i)
                {
                    _fs_beam[i]->set_visible(dt < 26);
                    _fs_beam[i]->set_position(-80 + (i - 1) * 76, 36 - (i - 1) * 32);
                    _fs_beam[i]->set_tiles(bn::sprite_items::blue_slash.tiles_item(), (dt / 2 + i) % 3);
                }
                _fs_vic_pal->set_fade((t % 6 < 3) ? bn::color(31, 31, 31) : bn::color(4, 18, 31), 0.80);
            }

            const int vx = -24 - _fs_cam_x + shake_x - (slice_gap > 0 ? (slice_gap.integer() / 3) : 0);
            _fs_vic->set_visible(vx > -150 && vx < 150);
            _fs_vic->set_position(vx, 18 + shake_y + (slice_gap > 0 ? (slice_gap.integer() / 4) : 0));
            _fs_vic->set_tiles(vic_def->sprite->tiles_item(), safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_RIGHT, false)));

            _fs_bg->set_position(136 - _fs_cam_x + shake_x, shake_y);
            for(int r = 0; r < bn::display::height(); ++r)
            {
                if(slice_gap > 0)
                {
                    if(r < 78)
                    {
                        detail::fs_slice_x_buf[r] = -slice_gap;
                        detail::fs_slice_y_buf[r] = -slice_gap / 3;
                    }
                    else if(r > 82)
                    {
                        detail::fs_slice_x_buf[r] = slice_gap;
                        detail::fs_slice_y_buf[r] = slice_gap / 3;
                    }
                    else
                    {
                        detail::fs_slice_x_buf[r] = (r % 2 == 0) ? 120 : -120;
                        detail::fs_slice_y_buf[r] = 0;
                    }
                }
                else
                {
                    detail::fs_slice_x_buf[r] = 0;
                    detail::fs_slice_y_buf[r] = 0;
                }
            }
            _fs_slice_hbe_x->reload_deltas_ref();
            _fs_slice_hbe_y->reload_deltas_ref();

            if(t >= 172) { set_flash(bn::fixed(t - 172) / 12); }
            if(t >= 184)
            {
                unload_fs_gfx();
                _phase = PH_FS_EXIT;
            }
        }

        // ---- FS_WAFFLE_MEAL cutscene: Waffles teleports around kitchen cooking -> throws meal to teammate ----
        void build_waffle_meal_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const int vic_def_idx = (_fs_victim >= 3 && _fight.f[_fs_victim].clone_of >= 0) ? _fight.f[_fs_victim].clone_of : _fs_victim;
            const FighterDef* vic_def = fighter_def(vic_def_idx);

            _fs_bg = bn::regular_bg_items::kitchen_fs_bg.create_bg(0, 0);
            _fs_bg->set_priority(3);

            _fs_atk_mat = bn::sprite_affine_mat_ptr::create();
            _fs_atk_mat->set_scale(1.5);
            _fs_atk_pal = bn::sprite_palette_ptr::create_new(atk_def->sprite->palette_item());
            _fs_atk = atk_def->sprite->create_sprite(-72, 18, sheet_frame(DIR_RIGHT, false));
            _fs_atk->set_bg_priority(0);
            _fs_atk->set_z_order(-2);
            _fs_atk->set_affine_mat(*_fs_atk_mat);
            _fs_atk->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_atk->set_palette(*_fs_atk_pal);

            _fs_vic_mat = bn::sprite_affine_mat_ptr::create();
            _fs_vic_mat->set_scale(1.5);
            _fs_vic_pal = bn::sprite_palette_ptr::create_new(vic_def->sprite->palette_item());
            if(_fs_victim >= 3) { _fs_vic_pal->set_fade(bn::color(4, 22, 31), 0.55); }
            else if(_fs_victim == 2) { _fs_vic_pal->set_fade(bn::color(6, 12, 31), 0.35); }

            _fs_vic = vic_def->sprite->create_sprite(76, 18, sheet_frame(DIR_LEFT, false));
            _fs_vic->set_bg_priority(0);
            _fs_vic->set_z_order(-1);
            _fs_vic->set_affine_mat(*_fs_vic_mat);
            _fs_vic->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_vic->set_palette(*_fs_vic_pal);

            _fs_beam_mat = bn::sprite_affine_mat_ptr::create();
            _fs_beam_mat->set_scale(1.5);
            _fs_impact = bn::sprite_items::mc_props.create_sprite(12, 2, 16);
            _fs_impact->set_bg_priority(0);
            _fs_impact->set_z_order(-3);
            _fs_impact->set_affine_mat(*_fs_beam_mat);
            _fs_impact->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_impact->set_visible(false);

            _fs_spark = bn::sprite_items::hit_spark.create_sprite(-72, 14, 0);
            _fs_spark->set_bg_priority(0);
            _fs_spark->set_z_order(-4);
            _fs_spark->set_visible(false);

            for(int i = 0; i < 2; ++i)
            {
                _fs_aura[i] = bn::sprite_items::fire_aura.create_sprite(12, 0, i);
                _fs_aura[i]->set_bg_priority(0);
                _fs_aura[i]->set_z_order(-4);
                _fs_aura[i]->set_visible(false);
            }

            text.set_center_alignment();
            text.generate(0, -60, "CHEF WAFFLES!", _fs_banner);
            for(bn::sprite_ptr& sp : _fs_banner) { sp.set_bg_priority(0); sp.set_z_order(-10); }
        }

        void update_waffle_meal_fs(bn::sprite_text_generator& text)
        {
            (void)text;
            const FighterDef* atk_def = fighter_def(_fs_attacker);
            const int vic_def_idx = (_fs_victim >= 3 && _fight.f[_fs_victim].clone_of >= 0) ? _fight.f[_fs_victim].clone_of : _fs_victim;
            const FighterDef* vic_def = fighter_def(vic_def_idx);
            const int t = _fs_timer++;

            if(t <= 10) { set_flash(bn::fixed(10 - t) / 10); }
            if(t == 28) { _fs_banner.clear(); }

            if(t < 112)
            {
                static const int tp_x[8] = { -74, -28, 28, -22, 12, -62, 34, -14 };
                static const int tp_y[8] = {  18,  18, 18,  10, 12,  18, 18,  18 };
                static const int tp_d[8] = { DIR_RIGHT, DIR_RIGHT, DIR_LEFT, DIR_RIGHT, DIR_DOWN, DIR_RIGHT, DIR_LEFT, DIR_RIGHT };
                const int step = (t / 14) % 8;
                const int sub = t % 14;

                if(sub == 0 && t > 0) { events |= EV_SWING; }
                const int pose = sheet_frame(tp_d[step], true) + ((sub / 3) % 3);
                _fs_atk->set_tiles(atk_def->sprite->tiles_item(), pose);
                _fs_atk->set_position(tp_x[step], tp_y[step]);

                if(sub < 8)
                {
                    _fs_spark->set_visible(true);
                    _fs_spark->set_position(tp_x[step], tp_y[step] - 4);
                    _fs_spark->set_tiles(bn::sprite_items::hit_spark.tiles_item(), sub * 3 / 8);
                }
                else { _fs_spark->set_visible(false); }

                if(t >= 35)
                {
                    if(t == 35) { events |= EV_ABSORB; }
                    _fs_impact->set_visible(true);
                    _fs_impact->set_position(12, 2);
                    for(int i = 0; i < 2; ++i)
                    {
                        _fs_aura[i]->set_visible(true);
                        _fs_aura[i]->set_tiles(bn::sprite_items::fire_aura.tiles_item(), ((t / 4) + i) % 3);
                        _fs_aura[i]->set_position(12 + (i == 0 ? -8 : 8), 0 - (t % 5));
                    }
                }
                return;
            }

            if(t < 145)
            {
                if(t == 112)
                {
                    events |= EV_SHOT;
                    _fs_spark->set_visible(false);
                    for(int i = 0; i < 2; ++i) { _fs_aura[i]->set_visible(false); }
                    _fs_atk->set_tiles(atk_def->sprite->tiles_item(), sheet_frame(DIR_RIGHT, true) + 2);
                    _fs_atk->set_position(-14, 18);
                }
                const int dt = t - 112;
                const int mx = 12 + dt * 64 / 33;
                const int arc = (dt * (33 - dt)) / 10;
                const int my = 2 - arc;
                _fs_impact->set_visible(true);
                _fs_impact->set_position(mx, my);
                return;
            }

            if(t == 145)
            {
                _fs_impact->set_visible(false);
                events |= EV_ABSORB | EV_SMASH;
            }

            const int dt = t - 145;
            const int hop = (dt < 16) ? (dt < 8 ? dt * 2 : (16 - dt) * 2) : 0;
            _fs_vic->set_position(76, 18 - hop);
            _fs_vic->set_tiles(vic_def->sprite->tiles_item(), sheet_frame(DIR_LEFT, dt < 18));

            const bn::fixed glow = dt < 24 ? (bn::fixed(24 - dt) * bn::fixed(0.03)) : bn::fixed(0);
            _fs_vic_pal->set_fade(bn::color(10, 31, 18), glow);

            if(dt < 14)
            {
                _fs_spark->set_visible(true);
                _fs_spark->set_position(76, 10 - hop);
                _fs_spark->set_tiles(bn::sprite_items::hit_spark.tiles_item(), dt * 3 / 14);
            }
            else { _fs_spark->set_visible(false); }

            if(t >= 175) { set_flash(bn::fixed(t - 175) / 12); }
            if(t >= 187)
            {
                unload_fs_gfx();
                _phase = PH_FS_EXIT;
            }
        }

        // ---- FS_UMB_MINECART cutscene: Enemy on minecart -> Pan left to Minecraft house -> Boom ----
        void build_umb_fs(bn::sprite_text_generator& text)
        {
            const FighterDef* vic_def = fighter_def(_fs_victim);
            _fs_cam_x = 0;

            _fs_bg = bn::regular_bg_items::minecraft_fs_bg.create_bg(-136, 0);
            _fs_bg->set_priority(3);

            _fs_vic_mat = bn::sprite_affine_mat_ptr::create();
            _fs_vic_mat->set_scale(1.5);
            _fs_vic_pal = bn::sprite_palette_ptr::create_new(vic_def->sprite->palette_item());
            if(_fs_victim == 1)      { _fs_vic_pal->set_fade(bn::color(31, 6, 6), 0.35); }
            else if(_fs_victim == 2) { _fs_vic_pal->set_fade(bn::color(6, 12, 31), 0.35); }

            _fs_vic = vic_def->sprite->create_sprite(58, -8, safe_frame(*vic_def->sprite, pick_frame(ST_HURT, -1, 0), sheet_frame(DIR_LEFT, false)));
            _fs_vic->set_bg_priority(0);
            _fs_vic->set_z_order(-2);
            _fs_vic->set_affine_mat(*_fs_vic_mat);
            _fs_vic->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_vic->set_palette(*_fs_vic_pal);

            _fs_cart_mat = bn::sprite_affine_mat_ptr::create();
            _fs_cart_mat->set_scale(1.5);
            _fs_cart = bn::sprite_ptr::create(
                bn::sprite_items::mc_rides.shape_size(),
                bn::sprite_items::mc_rides.tiles_item().create_tiles(2),
                bn::sprite_items::mc_props.palette_item().create_palette());
            _fs_cart->set_position(58, 14);
            _fs_cart->set_bg_priority(0);
            _fs_cart->set_z_order(-3);
            _fs_cart->set_affine_mat(*_fs_cart_mat);
            _fs_cart->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);

            _fs_beam_mat = bn::sprite_affine_mat_ptr::create();
            _fs_beam_mat->set_scale(1.85);
            _fs_impact = bn::sprite_items::fire_impact.create_sprite(-24, 6, 0);
            _fs_impact->set_bg_priority(0);
            _fs_impact->set_z_order(-4);
            _fs_impact->set_affine_mat(*_fs_beam_mat);
            _fs_impact->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _fs_impact->set_visible(false);

            for(int i = 0; i < 4; ++i)
            {
                _fs_beam[i] = bn::sprite_items::explosion.create_sprite(-24, 6, 0);
                _fs_beam[i]->set_bg_priority(0);
                _fs_beam[i]->set_z_order(-5);
                _fs_beam[i]->set_visible(false);
            }

            text.set_center_alignment();
            text.generate(0, -58, "MINECART MAYHEM!", _fs_banner);
            for(bn::sprite_ptr& sp : _fs_banner) { sp.set_bg_priority(0); sp.set_z_order(-10); }
        }

        void update_umb_fs(bn::sprite_text_generator& text)
        {
            (void)text;
            const int t = _fs_timer++;

            if(t <= 10) { set_flash(bn::fixed(10 - t) / 10); }
            if(t == 28) { _fs_banner.clear(); }

            int wx = 96;
            if(t < 45)
            {
                _fs_cam_x = 0;
                wx = 450 - t * 85 / 45;
            }
            else if(t < 120)
            {
                _fs_cam_x = (t - 45) * 272 / 75;
                wx = 365 - (t - 45) * 195 / 75;
            }
            else
            {
                _fs_cam_x = 272;
                wx = (t < 138) ? (170 - (t - 120) * 74 / 18) : 96;
            }

            const int sx = wx - 392 + _fs_cam_x;
            if(t < 138)
            {
                const int jitter = (t % 4 < 2) ? -1 : 1;
                _fs_vic->set_visible(sx > -155 && sx < 155);
                _fs_vic->set_position(sx, -8 + jitter);
                _fs_cart->set_visible(sx > -155 && sx < 155);
                _fs_cart->set_position(sx, 14);
                _fs_cart->set_tiles(bn::sprite_items::mc_rides.tiles_item(), 2 + (t / 3) % 2);
            }
            else if(t == 138)
            {
                _fs_vic->set_visible(false);
                _fs_cart->set_visible(false);
                events |= EV_SWING;
            }

            int shake_x = 0, shake_y = 0;
            if(t >= 150)
            {
                if(t == 150) { events |= EV_BOOM | EV_SMASH; }
                if(t == 165) { events |= EV_BOOM | EV_HIT; }

                const int dt = t - 150;
                const int pwr = dt < 45 ? (7 - dt * 7 / 45) : 0;
                shake_x = (t % 2 == 0) ? pwr : -pwr;
                shake_y = (t % 4 < 2) ? pwr : -pwr;

                if(t <= 165) { set_flash(bn::fixed(165 - t) / 15); }

                if(dt < 45)
                {
                    _fs_beam_mat->set_scale(bn::fixed(1.7) + bn::fixed(dt % 4) * bn::fixed(0.08));
                    _fs_impact->set_visible(true);
                    _fs_impact->set_tiles(bn::sprite_items::fire_impact.tiles_item(), (dt / 3) % 3);
                    _fs_impact->set_position(-24 + shake_x, 6 + shake_y);

                    static const int bx[4] = { -52, 4, -24, -24 };
                    static const int by[4] = { 12, 12, -20, 22 };
                    for(int i = 0; i < 4; ++i)
                    {
                        const int e_age = (dt + i * 3) % 16;
                        _fs_beam[i]->set_visible(dt < 38);
                        _fs_beam[i]->set_tiles(bn::sprite_items::explosion.tiles_item(), (e_age / 4) % 4);
                        _fs_beam[i]->set_position(bx[i] + shake_x, by[i] + shake_y);
                    }
                }
                else
                {
                    _fs_impact->set_visible(false);
                    for(int i = 0; i < 4; ++i) { _fs_beam[i]->set_visible(false); }
                }
            }

            _fs_bg->set_position(-136 + _fs_cam_x + shake_x, shake_y);

            if(t >= 195) { set_flash(bn::fixed(t - 195) / 12); }
            if(t >= 207)
            {
                unload_fs_gfx();
                _phase = PH_FS_EXIT;
            }
        }

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

            int middle = (_fight.f[0].x16 + _fight.f[1].x16) / (2 * FP);
            if(middle < 120) { middle = 120; }
            if(middle > length - 120) { middle = length - 120; }
            return middle;
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
            if(f.state == ST_SHIELD || stealing)
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
                if(_fight.fs_freeze_timer > 0 && index == _fight.fs_freeze_user)
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
                }
                else if(v.tint_mode == 3)
                {
                    v.pal->set_fade(bn::color(4, 22, 31), bn::fixed(0.55));
                }
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
                    blue_idx = step;
                }
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