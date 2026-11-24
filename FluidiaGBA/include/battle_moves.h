#ifndef BATTLE_MOVES_H
#define BATTLE_MOVES_H

// ================================================================================================
// EVERY MOVE IN THE GAME LIVES HERE. To add a move:
//
//   1. write it below (copy one that is close to what you want)
//   2. put its address in a fighter's MoveSet (the bottom of this file)
//
// That is all: no other file has to change >__>
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
        .fires(8, Shot().of(PROJ_WATER).dmg(2).fly(fp(4), 70).size(12, 8).spawn(12, 14).launch(fp(0), 0).stun(0));

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
    constexpr Move fox_thief_throw = Move("Fox Thief (throw)", 26, ANIM_RTS)
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

    // ============================================================================================
    // The extra building blocks used herw :
    //     .leaps(frame, vx, vy)            thrown off the ground at that frame (a jump you can't steer)
    //     .after_grab(&move)               a hitbox with .grabs() connected: this move takes over (the victim is held)
    //     Hitbox().keeps_grab()            damage only, the held victim stays held
    //     .super_armor(from, to, max)      hits of at most `max` damage don't flinch or launch in those frames
    //     .counters_melee(from, to, &move) a melee hit in those frames is caught: the attacker is grabbed, `move` starts
    //     .after_absorb(&move)             a stolen shot starts `move` at once
    //     Hitbox().bounces(speed)          the attacker bounces off the victim (pogo)
    //     .ends_on_landing(frame)          a downward dash that touches the ground jumps to this frame
    //     Hitbox().left_right()            x is the real left/right of the screen, not "in front of you"
    //     Shot().arc(vx, vy, gravity, life)  a thrown object that flies in an arc;  .explodes(dmg)  .labeled("TEXT")
    // ============================================================================================

    // ---- B + down: COUNTER ----
    // Stand ready (frames 4-29). A MELEE hit in that time is caught: he grabs the attacker, then punches them away.
    constexpr Move counter_punch = Move("Counter Punch", 58, ANIM_SNEP_COUNTER_GRAB)
        .hit(Hitbox().frames(36, 42).at(16, 14).size(28, 22).dmg(18).launch(fp(5.5), fp(-3.5)).stun(34));

    // A SHOT that touches him in that time is caught too: he spins with it and throws it back, twice as hard and twice as fast.
    constexpr Move counter_return = Move("Counter Return", 44, ANIM_SNEP_SPIN_THROW)
        .throws_stored(24, 200)
        .orbits_held();

    constexpr Move counter = Move("Counter", 46, ANIM_SNEP_COUNTER)
        .absorbs(4, 30)
        .after_absorb(&counter_return)
        .counters_melee(4, 30, &counter_punch);

    // ---- B: INSULT. A random word flies out. No damage and no knockback, but the victim is stunned. ----
    constexpr Shot words[] =
    {
        Shot().dmg(0).fly(fp(1.0), 60).size(22, 10).spawn(12, 16).stun(50).labeled("LOSER"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(50).labeled("NOOB"),
        Shot().dmg(0).fly(fp(1.0), 60).size(22, 10).spawn(12, 16).stun(50).labeled("TRASH"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(50).labeled("IDIOT"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(50).labeled("FAGGOT"),
        Shot().dmg(0).fly(fp(1.0), 60).size(24, 10).spawn(12, 16).stun(50).labeled("CRINGE"),
    };
    constexpr Move insult = Move("Insult", 26, ANIM_SHOOT)
        .fires_random(8, words, 6);

    // ---- B + left/right: POUNCE. Leaps forward. If it connects he grabs on and scratches 3 times, then lets go.
    //      If it misses there is a long recovery (the move is 72 frames, he lands around frame 25). ----
    constexpr Move scratch_flurry = Move("Scratch Flurry", 58, ANIM_SNEP_SCRATCH)
        .hit(Hitbox().frames(8, 11).at(16, 14).size(24, 22).dmg(3).stun(10).keeps_grab())
        .hit(Hitbox().frames(20, 23).at(16, 14).size(24, 22).dmg(3).stun(10).keeps_grab())
        .hit(Hitbox().frames(32, 35).at(16, 14).size(24, 22).dmg(3).stun(10).keeps_grab())
        .hit(Hitbox().frames(46, 49).at(16, 14).size(24, 22).dmg(2).launch(fp(3.0), fp(-1.5)).stun(18));   // letting go

    constexpr Move pounce = Move("Pounce", 72, ANIM_SNEP_LEAP)
        .once_per_jump()
        .leaps(6, fp(4.2), -fp(4.0))
        .hit(Hitbox().frames(8, 30).at(12, 14).size(22, 26).grabs())
        .after_grab(&scratch_flurry);

    // ---- B + up: THROW JUNK. A random object flies up and forward in an arc. Heavier objects hit harder.
    //      The laptop explodes when it touches somebody (or the floor). ---- mmm explosion
    constexpr Shot junk[] =
    {
        Shot().dmg(3).arc(fp(2.2), -fp(4.2), 3, 90).size(12, 5).spawn(6, 24).launch(fp(1.2), fp(-1.0)).stun(12).labeled("PENCIL"), // no sprites yet, im too lazy
        Shot().dmg(4).arc(fp(1.8), -fp(4.4), 3, 90).size(9, 9).spawn(6, 24).launch(fp(1.5), fp(-1.5)).stun(14).labeled("APPLE"),
        Shot().dmg(8).arc(fp(1.5), -fp(4.6), 4, 90).size(18, 6).spawn(6, 24).launch(fp(3.5), fp(-2.5)).stun(18).labeled("BAT"),
        Shot().dmg(3).arc(fp(1.6), -fp(4.6), 4, 90).size(14, 10).spawn(6, 24).launch(fp(1.0), fp(-1.0)).stun(10).explodes(9).labeled("LAPTOP"),
        Shot().dmg(12).arc(fp(1.2), -fp(4.2), 4, 90).size(12, 12).spawn(6, 24).launch(fp(4.0), fp(-3.0)).stun(24).labeled("BALL"),
    };
    constexpr Move throw_junk = Move("Throw Junk", 40, ANIM_SNEP_THROW_UP)
        .fires_random(10, junk, 5);

    constexpr Move snep_jab = jab.with_anim(ANIM_SNEP_JAB);

    // ---- A + left/right: HEAVY PUNCH. A long wind-up, then a huge hit. Super armor from frame 8 until the punch comes
    //      out: hits of 12 damage or less don't flinch him, a harder hit breaks the armor. ----
    constexpr Move heavy_punch = Move("Heavy Punch", 76, ANIM_SNEP_HEAVY)
        .super_armor(8, 36, 12)
        .hit(Hitbox().frames(34, 40).at(20, 14).size(32, 22).dmg(24).launch(fp(6.0), fp(-3.0)).stun(32));

    // ---- A + up: BACKFLIP. Flips up and backwards, kicking on the way up. ----
    constexpr Move backflip = Move("Backflip", 44, ANIM_SNEP_FLIP)
        .dash(Drive().frames(4, 20).speed(-fp(1.2), -fp(3.4)))
        .hit(Hitbox().frames(6, 12).at(14, 20).size(22, 20).dmg(7).launch(fp(1.5), fp(-3.0)).stun(18))
        .hit(Hitbox().frames(11, 18).at(4, 40).size(26, 22).dmg(9).launch(fp(0.8), fp(-5.0)).stun(24));

    // ---- A + down: punches LEFT first, then RIGHT (the real left and right of the screen, whichever way he faces) ----
    constexpr Move double_punch = Move("Left-Right Punch", 46, ANIM_SNEP_DSMASH)
        .hit(Hitbox().frames(8, 13).at(-20, 14).size(26, 20).dmg(8).launch(fp(2.5), fp(-1.5)).stun(18).left_right())
        .hit(Hitbox().frames(24, 29).at(20, 14).size(26, 20).dmg(10).launch(fp(4.0), fp(-2.5)).stun(22).left_right());

    //      A + down in the air: POGO KICK. Dives down and a little forward. Hitting somebody bounces him back up,
    //      so he can kick again right away. Touching the ground ends the dive (a short landing lag)
    constexpr Move pogo_kick = Move("Pogo Kick", 40, ANIM_SNEP_DAIR)
        .dash(Drive().frames(6, 34).speed(fp(1.4), fp(4.8)))
        .hit(Hitbox().frames(6, 34).at(4, 4).size(16, 20).dmg(6).launch(fp(1.0), fp(2.5)).stun(16).bounces(fp(4.4)))
        .ends_on_landing(28);

    constexpr MoveSet snep_moveset = MoveSet()
        .set(SLOT_SPECIAL,      &insult)            // B
        .set(SLOT_SPECIAL_SIDE, &pounce)            // B + left/right
        .set(SLOT_SPECIAL_UP,   &throw_junk)        // B + up
        .set(SLOT_SPECIAL_DOWN, &counter)           // B + down
        .set(SLOT_JAB,          &snep_jab)          // A
        .set(SLOT_FSMASH,       &heavy_punch)       // A + left/right
        .set(SLOT_USMASH,       &backflip)          // A + up
        .set(SLOT_DSMASH,       &double_punch)      // A + down
        .set(SLOT_DAIR,         &pogo_kick);        // A in the air + down
        // (no aerials for the other directions yet: leave a slot out = no such move)
}

#endif
