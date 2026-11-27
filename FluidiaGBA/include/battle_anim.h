#ifndef BATTLE_ANIM_H
#define BATTLE_ANIM_H

// Which picture of a sprite sheet to show: frame numbers per fighter, attack animations, pick_frame(), projectile_art().

#include "bn_sprite_item.h"
#include "bn_sprite_tiles_item.h"
#include "bn_sprite_items_junk.h"
#include "bn_sprite_items_mc_props.h"
#include "bn_sprite_items_mc_rides.h"
#include "bn_sprite_items_water_shot.h"
#include "bn_sprite_items_sonic_slash.h"
#include "bn_sprite_items_explosion.h"
#include "battle_logic.h"

namespace battle
{
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
    inline int har_slash_step(int anim, int mf, bool& spin_left_half)
    {
        spin_left_half = false;
        switch(anim)
        {
            case ANIM_HAR_JAB:
                if(mf >= 0 && mf <= 8)   { return 2 - (mf / 3); }         // slash 1: top -> bottom
                if(mf >= 10 && mf <= 18) { return (mf - 10) / 3; }        // slash 2: bottom -> top (backswing)
                if(mf >= 20 && mf <= 28) { return 2 - ((mf - 20) / 3); }  // slash 3: top -> bottom
                return -1;
            case ANIM_HAR_FSMASH:
                if(mf >= 0 && mf < 10)   { return 2; }
                if(mf >= 10 && mf < 15)  { return 1; }
                if(mf >= 15 && mf < 21)  { return 0; }
                return -1;
            case ANIM_HAR_FS:
                if(mf >= 0 && mf < 5)    { return 2; }
                if(mf >= 5 && mf < 10)   { return 1; }
                if(mf >= 10 && mf < 16)  { return 0; }
                return -1;
            case ANIM_HAR_BSHOT:
                if(mf >= 0 && mf < 4)    { return 2; }
                if(mf >= 4 && mf < 8)    { return 1; }
                if(mf >= 8 && mf < 13)   { return 0; }
                return -1;
            case ANIM_HAR_USMASH:
                if(mf >= 0 && mf < 4)    { return 0; }                    // upward slash: bottom -> top
                if(mf >= 4 && mf < 8)    { return 1; }
                if(mf >= 8 && mf < 13)   { return 2; }
                return -1;
            case ANIM_HAR_DASH:
                if(mf >= 4 && mf < 9)    { return 2; }
                if(mf >= 9 && mf < 14)   { return 1; }
                if(mf >= 14 && mf < 19)  { return 0; }
                return -1;
            case ANIM_HAR_UPDOWN:
                if(mf >= 4 && mf < 8)    { return 0; }                    // upper slash: bottom -> top
                if(mf >= 8 && mf < 12)   { return 1; }
                if(mf >= 12 && mf < 17)  { return 2; }
                if(mf >= 24 && mf < 28)  { return 2; }                    // down slash: top -> bottom
                if(mf >= 28 && mf < 32)  { return 1; }
                if(mf >= 32 && mf < 37)  { return 0; }
                return -1;
            case ANIM_HAR_SPIN:
                if(mf >= 3 && mf < 12)   { return 2 - ((mf - 3) / 3); }
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
                return sheet_frame(DIR_DOWN, false);
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
            case ANIM_WAF_STEAL:
                return 34 + (side == DIR_LEFT ? 4 : 0);
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
}

#endif
