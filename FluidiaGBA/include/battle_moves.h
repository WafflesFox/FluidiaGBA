#ifndef BATTLE_MOVES_H
#define BATTLE_MOVES_H

// ================================================================================================
// EVERY MOVE IN THE GAME LIVES HERE. To add a move:
//
//   1. write it below (copy one that is close to what you want)
//   2. put its address in a fighter's MoveSet (the bottom of this file)
//
// That is all: no other file has to change.
//
// A move is a timeline. Frame 0 is the frame after the button press, 60 frames = 1 second.
//
//   Move("name", total frames, pose)          total frames = startup + active + recovery
//     .hit(Hitbox()...)    a melee hitbox (up to 4 per move). Every hitbox hits ONCE per use.
//     .dash(Drive()...)    the fighter flies (aqua jet)
//     .fires(frame, Shot()...)             something flies away at that frame
//     .fires_on_landing(Shot()..., frame)  ...or appears when the fighter touches the ground
//     .absorbs(from, to)                   shots that touch the fighter in those frames are stolen
//     .swaps_when_holding(&other_move)     if a stolen shot is held, use THIS move instead
//     .throws_stored(frame, power%)        throws the stolen shot at that frame
//     .once_per_jump()                     can't be used twice before you land
//
//   Hitbox().frames(start, end)   active from `start` to `end - 1`
//            .at(x, y)            the middle of the box: x pixels in FRONT of you (negative = behind), y pixels above your feet
//            .size(w, h)          the box, in pixels (your own body is 14 x 26)
//            .dmg(n)              damage
//            .launch(kx, ky)      knockback speed: kx pushes away from you, ky negative = up, positive = down
//            .stun(n)             frames the victim can't act
//
// Use fp(2.5) for pixels per frame. Rough sizes: a jump goes up ~48 pixels, walking is 2 pixels per frame.
// ================================================================================================

#include "battle_logic.h"

namespace battle
{
    // ============================================================================================
    // B: THE SPECIAL MOVES
    // ============================================================================================

    // B: Water Gun. (Same numbers as the old single move: 8 frames of startup, 14 of recovery.)
    constexpr Move water_gun = Move("Water Gun", 22, ANIM_SHOOT)
        .fires(8, Shot().of(PROJ_WATER).dmg(2).fly(fp(4), 70).size(12, 8).spawn(12, 14).launch(fp(1), 0).stun(10));

    // B + left/right: Aqua Jet. Charges for 8 frames, then dashes ~80 pixels at high speed. It steers a little toward the
    // opponent if they are in front. It ends the moment it hits.
    constexpr Move aqua_jet = Move("Aqua Jet", 34, ANIM_JET_SIDE)
        .once_per_jump()
        .dash(Drive().frames(8, 23).speed(fp(5.5), 0).steer(30).stops_on_hit())
        .hit(Hitbox().frames(8, 23).at(2, 13).size(22, 22).dmg(7).launch(fp(3.0), fp(-2.5)).stun(20));

    // B + up: the same, but straight up. More damage than the sideways one.
    constexpr Move aqua_jet_up = Move("Aqua Jet Up", 38, ANIM_JET_UP)
        .once_per_jump()
        .dash(Drive().frames(6, 21).speed(0, fp(-5.0)).stops_on_hit())
        .hit(Hitbox().frames(6, 21).at(0, 16).size(20, 28).dmg(10).launch(fp(1.0), fp(-4.5)).stun(26));

    // B + down: Fox Thief. Stand still for a moment: any shot that touches you in frames 3-24 is stolen (no damage).
    // Press it AGAIN while holding a stolen shot and you throw it back, 30% stronger and faster.
    constexpr Move fox_thief_throw = Move("Fox Thief (throw)", 26, ANIM_SHOOT)
        .throws_stored(8, 130);

    constexpr Move fox_thief = Move("Fox Thief", 30, ANIM_STANCE)
        .absorbs(3, 25)
        .swaps_when_holding(&fox_thief_throw);

    // ============================================================================================
    // A ON THE GROUND
    // ============================================================================================

    // A: three slaps. The first two barely push, so the next one still reaches. The third launches.
    constexpr Move jab = Move("Slap Combo", 38, ANIM_SLAP)
        .hit(Hitbox().frames(4, 8).at(16, 14).size(16, 12).dmg(2).launch(fp(0.4), 0).stun(14))
        .hit(Hitbox().frames(14, 18).at(16, 14).size(16, 12).dmg(2).launch(fp(0.4), 0).stun(14))
        .hit(Hitbox().frames(24, 30).at(18, 14).size(20, 14).dmg(4).launch(fp(2.5), fp(-1.5)).stun(16));

    // A + left/right (forward smash): tail slam in front.
    constexpr Move tail_slam = Move("Tail Slam", 46, ANIM_TAIL)
        .hit(Hitbox().frames(14, 19).at(20, 14).size(30, 20).dmg(11).launch(fp(4.5), fp(-3.0)).stun(24));

    // A + up (up smash): tail slam above.
    constexpr Move tail_slam_up = Move("Tail Slam Up", 46, ANIM_TAIL_UP)
        .hit(Hitbox().frames(14, 19).at(2, 30).size(30, 28).dmg(10).launch(fp(1.0), fp(-5.0)).stun(26));

    // A + down (down smash): both fists slam down, one box on each side. Whoever is on a side gets launched that way.
    constexpr Move fist_slam = Move("Fist Slam", 44, ANIM_SLAM)
        .hit(Hitbox().frames(12, 17).at(20, 6).size(24, 14).dmg(9).launch(fp(3.0), fp(-3.5)).stun(22))
        .hit(Hitbox().frames(12, 17).at(-20, 6).size(24, 14).dmg(9).launch(fp(3.0), fp(-3.5)).stun(22));

    // ============================================================================================
    // A IN THE AIR
    // ============================================================================================

    // A: spin all the way around. Two hits: the first is light, the second launches.
    constexpr Move spin = Move("Spin", 32, ANIM_SPIN)
        .hit(Hitbox().frames(4, 12).at(0, 13).size(34, 24).dmg(3).launch(fp(1.0), fp(-1.5)).stun(12))
        .hit(Hitbox().frames(14, 22).at(0, 13).size(36, 26).dmg(4).launch(fp(2.8), fp(-2.0)).stun(16));

    // A + forward: a kick.
    constexpr Move kick = Move("Kick", 28, ANIM_KICK)
        .hit(Hitbox().frames(6, 12).at(18, 10).size(22, 12).dmg(6).launch(fp(3.5), fp(-1.5)).stun(18));

    // A + back: tail slap behind you (you keep facing forward).
    constexpr Move tail_slap_back = Move("Tail Slap Back", 26, ANIM_TAIL)
        .hit(Hitbox().frames(5, 11).at(-20, 14).size(26, 14).dmg(7).launch(fp(3.8), fp(-1.5)).stun(18));

    // A + up: tail slap above.
    constexpr Move tail_slap_up = Move("Tail Slap Up", 26, ANIM_TAIL_UP)
        .hit(Hitbox().frames(5, 11).at(2, 34).size(28, 16).dmg(6).launch(fp(0.5), fp(-4.0)).stun(20));

    // A + down: aqua jet straight DOWN. Hits (and spikes) whatever is below, and when you touch the ground there is an
    // explosion. The move then jumps to frame 52, so only 16 frames of recovery are left.
    constexpr Move aqua_dive = Move("Aqua Dive", 68, ANIM_JET_DOWN)
        .dash(Drive().frames(8, 52).speed(0, fp(6.0)))
        .hit(Hitbox().frames(8, 52).at(0, 4).size(18, 26).dmg(8).launch(fp(0.5), fp(3.0)).stun(18))
        .fires_on_landing(Shot().of(PROJ_EXPLOSION).dmg(9).fly(0, 10).size(40, 30).spawn(0, 12)
                              .launch(fp(3.5), fp(-4.0)).stun(22).steal_proof().lingers(), 52);

    // ============================================================================================
    // WHO KNOWS WHICH MOVES. A new character gets its own MoveSet (leave a slot out = no such move).
    // ============================================================================================

    constexpr MoveSet waffle_moveset = MoveSet()
        .set(SLOT_SPECIAL,      &water_gun)         // B
        .set(SLOT_SPECIAL_SIDE, &aqua_jet)          // B + left/right
        .set(SLOT_SPECIAL_UP,   &aqua_jet_up)       // B + up
        .set(SLOT_SPECIAL_DOWN, &fox_thief)         // B + down
        .set(SLOT_JAB,          &jab)               // A
        .set(SLOT_FSMASH,       &tail_slam)         // A + left/right
        .set(SLOT_USMASH,       &tail_slam_up)      // A + up
        .set(SLOT_DSMASH,       &fist_slam)         // A + down
        .set(SLOT_NAIR,         &spin)              // A in the air
        .set(SLOT_FAIR,         &kick)              // A in the air + forward
        .set(SLOT_BAIR,         &tail_slap_back)    // A in the air + back
        .set(SLOT_UAIR,         &tail_slap_up)      // A in the air + up
        .set(SLOT_DAIR,         &aqua_dive);        // A in the air + down
}

#endif
