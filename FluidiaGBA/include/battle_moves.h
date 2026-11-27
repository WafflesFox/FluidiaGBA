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
        .fires(8, Shot().of(PROJ_WATER).dmg(2).fly(fp(4), 70).size(12, 8).spawn(12, 14).launch(fp(0), 0).stun(0).heals_ally(2));

    // B + left/right: Aqua Jet. Charges for 8 frames, then dashes ~80 pixels at high speed. It steers a little toward the
    // opponent if they are in front. It ends the moment it hits.
    constexpr Move aqua_jet = Move("Aqua Jet", 34, ANIM_JET_SIDE)
        .once_per_jump()
        .dash(Drive().frames(8, 23).speed(fp(5.5), 0).steer(30).stops_on_hit())
        .hit(Hitbox().frames(8, 23).at(2, 13).size(22, 22).dmg(7).launch(fp(3.0), fp(-2.5)).stun(20));

    // B + up: the same, but straight up. More damage than the sideways one.
    // Hold B + Up: charges a gauge; at full charge spawns a Water Clone of the enemy (10% HP, 50% dmg)
    constexpr Move waffle_up_clone = Move("Water Clone", 1000, ANIM_WAF_CHARGE)
        .charge_clone();

    constexpr Move aqua_jet_up = Move("Aqua Jet Up", 38, ANIM_JET_UP)
        .once_per_jump()
        .dash(Drive().frames(6, 21).speed(0, fp(-5.0)).stops_on_hit())
        .hit(Hitbox().frames(6, 21).at(0, 16).size(20, 28).dmg(10).launch(fp(1.0), fp(-4.5)).stun(26));

    // B + down: Fox Thief. Stand still for a moment: any shot that touches you in frames 3-24 is stolen (no damage).
    // Press it AGAIN while holding a stolen shot and you throw it back, 30% stronger and faster.
    constexpr Move fox_thief_throw = Move("Fox Thief (throw)", 26, ANIM_RTS)
        .throws_stored(8, 130);

    constexpr Move fox_thief = Move("Fox Thief", 30, ANIM_WAF_STEAL)
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

    // SELECT (when Final Smash is ready): lunging uppercut. If it hits, starts Fire Blast!
    constexpr Move waffle_fs_start = Move("Fire Uppercut", 32, ANIM_TAIL_UP)
        .dash(Drive().frames(4, 13).speed(fp(3.8), fp(-1.2)).stops_on_hit())
        .hit(Hitbox().frames(5, 14).at(16, 16).size(24, 28).dmg(6).launch(0, fp(-3.5)).stun(22).hits_teammates())
        .starts_final_smash(FS_WAFFLE_FIRE)
        .ally_fs(FS_WAFFLE_MEAL);

    constexpr MoveSet waffle_moveset = MoveSet()
        .set(SLOT_SPECIAL,      &water_gun)         // B
        .set(SLOT_SPECIAL_SIDE, &aqua_jet)          // B + left/right
        .set(SLOT_SPECIAL_UP,   &waffle_up_clone)       // B + up
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
        Shot().dmg(0).fly(fp(1.0), 60).size(22, 10).spawn(12, 16).stun(30).labeled("LOSER"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(30).labeled("NOOB"),
        Shot().dmg(0).fly(fp(1.0), 60).size(22, 10).spawn(12, 16).stun(35).labeled("TRASH"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(40).labeled("IDIOT"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(50).labeled("FAGGOT"),
        Shot().dmg(0).fly(fp(1.0), 60).size(24, 10).spawn(12, 16).stun(60).labeled("CRINGE"),
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
        Shot().dmg(3).arc(fp(2.2), -fp(4.2), 3, 90).size(12, 5).spawn(6, 24).launch(fp(1.2), fp(-1.0)).stun(12).of(PROJ_JUNK_PENCIL), // no sprites yet, im too lazy
        Shot().dmg(4).arc(fp(1.8), -fp(4.4), 3, 90).size(9, 9).spawn(6, 24).launch(fp(1.5), fp(-1.5)).stun(14).of(PROJ_JUNK_APPLE),
        Shot().dmg(8).arc(fp(1.5), -fp(4.6), 4, 90).size(18, 6).spawn(6, 24).launch(fp(3.5), fp(-2.5)).stun(18).of(PROJ_JUNK_BAT),
        Shot().dmg(3).arc(fp(1.6), -fp(4.6), 4, 90).size(14, 10).spawn(6, 24).launch(fp(1.0), fp(-1.0)).stun(10).explodes(9).of(PROJ_JUNK_LAPTOP),
        Shot().dmg(25).arc(fp(1.2), -fp(4.2), 4, 90).size(12, 12).spawn(6, 24).launch(fp(4.0), fp(-3.0)).stun(24).of(PROJ_JUNK_BALL),
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

    // SELECT (when Final Smash is ready): dashes forward. If it hits, starts Ring Drop!
    constexpr Move filip_fs_start = Move("Ring Drop", 34, ANIM_SNEP_LEAP)
        .dash(Drive().frames(4, 14).speed(fp(5.2), 0).stops_on_hit())
        .hit(Hitbox().frames(5, 14).at(14, 14).size(24, 24).dmg(6).launch(fp(2.0), fp(-2.5)).stun(22))
        .starts_final_smash(FS_FILIP_DROP);

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

    // ============================================================================================
    // UMBREON
    // ============================================================================================

    // B in air: place block beneath feet
    constexpr Move umb_place_block = Move("Place Block", 18, ANIM_UMB_PLACE)
        .place_block(4);

    // B on crafting table (with materials): craft Iron (1 mat) or Diamond (2 mats) weapon
    constexpr Move umb_craft = Move("Craft Weapon", 24, ANIM_UMB_PLACE)
        .craft_weapon(10)
        .swaps_when_holding(&umb_place_block);

    // B on ground: mine for materials (+1 material, close pickaxe swing)
    constexpr Move umb_mine_move = Move("Mine", 24, ANIM_UMB_MINE)
        .mine_mat(12)
        .hit(Hitbox().frames(8, 14).at(14, 12).size(16, 14).dmg(4).launch(fp(1.2), fp(-1.0)).stun(12))
        .swaps_when_holding(&umb_craft);

    // Side B: ride on a pig with carrot fishing rod (jump anytime to get off; knocked off if pig dies)
    constexpr Move umb_pig_ride = Move("Fox Ride", 10, ANIM_STAND)
        .mount_pig(4);

    // Up B: shoot three fireballs upwards and diagonally
    constexpr Move umb_fireballs = Move("Fireball Trio", 28, ANIM_UMB_THROW)
        .fires(8, Shot().of(PROJ_FIREBALL).dmg(3).arc(0, -fp(3.6), 0, 48).fan(fp(2.4))
                        .size(12, 12).spawn(0, 24).launch(fp(1.5), fp(-3.2)).stun(16));

    // Down B: place TNT (explodes when touched by anyone, even Umbreon)
    constexpr Move umb_tnt = Move("Place TNT", 24, ANIM_UMB_PLACE)
        .place_tnt(10);

    // A: throw boomerang
    constexpr Move umb_boomerang = Move("Boomerang", 26, ANIM_UMB_THROW)
        .fires(7, Shot().of(PROJ_BOOMERANG).dmg(4).fly(fp(4.5), 90).boomerang(2)
                        .size(14, 12).spawn(12, 14).launch(fp(1.8), fp(-1.8)).stun(14).weapon());

    // Side A: swing sword with full force
    constexpr Move umb_sword_full = Move("Sword Slash", 38, ANIM_UMB_SLASH)
        .hit(Hitbox().frames(10, 17).at(20, 14).size(30, 22).dmg(12).launch(fp(4.8), fp(-3.2)).stun(24).weapon());

    // Up A: shoot bow upwards
    constexpr Move umb_bow_up = Move("Bow Shot", 30, ANIM_UMB_BOW)
        .fires(12, Shot().of(PROJ_ARROW).dmg(8).arc(0, -fp(5.4), 2, 95)
                        .size(8, 16).spawn(0, 26).launch(fp(0.5), fp(-4.0)).stun(20).weapon());

    // Down A: smash both sides with pickaxe
    constexpr Move umb_pickaxe_smash = Move("Pickaxe Smash", 42, ANIM_UMB_MINE)
        .hit(Hitbox().frames(10, 16).at(20, 10).size(24, 18).dmg(6).launch(fp(3.6), fp(-3.2)).stun(22).weapon())
        .hit(Hitbox().frames(10, 16).at(-20, 10).size(24, 18).dmg(5).launch(fp(3.6), fp(-3.2)).stun(22).weapon());

    // A in air: simple sword slash
    constexpr Move umb_air_slash = Move("Air Slash", 24, ANIM_UMB_SLASH)
        .hit(Hitbox().frames(5, 13).at(16, 14).size(26, 20).dmg(3).launch(fp(2.6), fp(-1.6)).stun(16).weapon());

    // Final Smash: throw a minecart -> Minecraft House explosion cutscene!
    constexpr Move umb_fs_start = Move("Minecart Throw", 32, ANIM_UMB_THROW)
        .fires(8, Shot().of(PROJ_MINECART).dmg(6).fly(fp(5.0), 48).size(26, 16).spawn(16, 10)
                        .launch(fp(2.0), fp(-2.0)).stun(20).steal_proof().starts_final_smash(FS_UMB_MINECART));

    constexpr MoveSet umb_moveset = MoveSet()
        .set(SLOT_SPECIAL,      &umb_mine_move)
        .set(SLOT_SPECIAL_SIDE, &umb_pig_ride)
        .set(SLOT_SPECIAL_UP,   &umb_fireballs)
        .set(SLOT_SPECIAL_DOWN, &umb_tnt)
        .set(SLOT_JAB,          &umb_boomerang)
        .set(SLOT_FSMASH,       &umb_sword_full)
        .set(SLOT_USMASH,       &umb_bow_up)
        .set(SLOT_DSMASH,       &umb_pickaxe_smash)
        .set(SLOT_NAIR,         &umb_air_slash)
        .set(SLOT_FAIR,         &umb_air_slash)
        .set(SLOT_BAIR,         &umb_air_slash)
        .set(SLOT_UAIR,         &umb_air_slash)
        .set(SLOT_DAIR,         &umb_air_slash);
    // ============================================================================================
    // HARUE
    // ============================================================================================

    // B (tap): fast blue sonic boom slash projectile (low dmg, strong knockback)
    constexpr Move har_sonic_boom = Move("Sonic Slash", 22, ANIM_HAR_BSHOT)
        .fires(4, Shot().of(PROJ_SONIC_SLASH).dmg(3).fly(fp(7.2), 40)
                        .size(16, 22).spawn(16, 14).launch(fp(5.2), fp(-2.4)).stun(20));

    // B (hold): charges Power-Up gauge; at 100% boosts speed +35% and damage +30% for 20s
    constexpr Move har_power_charge = Move("Focus Charge", 1000, ANIM_HAR_CHARGE)
        .charge_power()
        .swaps_when_holding(&har_sonic_boom);

    // Side + B: Dash & Slash; when powered up plants a 2-second delayed explosion on hit
    constexpr Move har_dash_slash = Move("Flash Slash", 32, ANIM_HAR_DASH)
        .once_per_jump()
        .dash(Drive().frames(4, 18).speed(fp(8.5), 0))
        .hit(Hitbox().frames(4, 18).at(12, 14).size(28, 22).dmg(8).launch(0, 0).stun(18).delayed_boom(16));

    // Up + B: Upper slash into the air, then heavy downward slam slash (huge damage, huge end lag)
    constexpr Move har_up_down_slash = Move("Sky Splitter", 66, ANIM_HAR_UPDOWN)
        .once_per_jump()
        .dash(Drive().frames(4, 16).speed(fp(1.0), -fp(5.2)))
        .dash2(Drive().frames(24, 36).speed(fp(1.2), fp(6.2)))
        .hit(Hitbox().frames(5, 16).at(10, 22).size(24, 28).dmg(7).launch(fp(0.5), -fp(5.4)).stun(20))
        .hit(Hitbox().frames(25, 36).at(14, 10).size(28, 28).dmg(18).launch(fp(3.8), fp(4.4)).stun(28))
        .ends_on_landing(40);

    // Down + B (when powered up): consumes Power-Up and summons a 10% HP / 50% dmg Harue or Waffles clone
    constexpr Move har_summon_clone = Move("Shadow Twin", 22, ANIM_HAR_CHARGE)
        .summon_har_clone();

    // Down + B (normal): projectile-only Reflect using shield stance
    constexpr Move har_reflect = Move("Blade Reflect", 30, ANIM_HAR_REFLECT)
        .reflects(3, 25)
        .swaps_when_holding(&har_summon_clone);

    // A: 3 slashes back and forth
    constexpr Move har_jab = Move("Triple Slash", 36, ANIM_HAR_JAB)
        .hit(Hitbox().frames(4, 8).at(18, 14).size(22, 18).dmg(3).launch(fp(0.4), 0).stun(14))
        .hit(Hitbox().frames(13, 17).at(18, 14).size(22, 18).dmg(3).launch(fp(0.4), 0).stun(14))
        .hit(Hitbox().frames(22, 28).at(22, 14).size(26, 20).dmg(5).launch(fp(3.2), fp(-1.8)).stun(18));

    // Side + A: Huge slash with massive range
    constexpr Move har_huge_slash = Move("Great Cleave", 44, ANIM_HAR_FSMASH)
        .hit(Hitbox().frames(12, 19).at(28, 14).size(44, 28).dmg(14).launch(fp(5.2), fp(-3.2)).stun(26));

    // Up + A: One slash sending opponent upwards
    constexpr Move har_up_slash = Move("Rising Crescent", 34, ANIM_HAR_USMASH)
        .hit(Hitbox().frames(6, 13).at(12, 26).size(30, 32).dmg(11).launch(fp(0.6), fp(-5.6)).stun(26));

    // Down + A: Spinning slash (hits both sides)
    constexpr Move har_spin_slash = Move("Whirlwind Slash", 38, ANIM_HAR_SPIN)
        .hit(Hitbox().frames(5, 11).at(18, 14).size(26, 20).dmg(5).launch(fp(2.4), fp(-2.0)).stun(16).left_right())
        .hit(Hitbox().frames(13, 19).at(-18, 14).size(26, 20).dmg(6).launch(fp(3.0), fp(-2.2)).stun(18).left_right());

    // Air + A: Spin attack with a sword
    constexpr Move har_air_spin = Move("Sword Cyclone", 30, ANIM_HAR_SPIN)
        .hit(Hitbox().frames(4, 18).at(0, 14).size(38, 28).dmg(8).launch(fp(3.2), fp(-2.2)).stun(18));

    // Final Smash: huge lunge slash -> in-stage clone combo + double down-slash explosion
    constexpr Move har_fs_start = Move("Twin Tempest", 34, ANIM_HAR_FS)
        .dash(Drive().frames(4, 15).speed(fp(4.8), 0).stops_on_hit())
        .hit(Hitbox().frames(5, 16).at(22, 14).size(36, 28).dmg(6).launch(0, fp(-2.0)).stun(24))
        .in_stage_final_smash(FS_HARUE_COMBO)
        .powered_final_smash(FS_HARUE_SLICE);

    constexpr MoveSet har_moveset = MoveSet()
        .set(SLOT_SPECIAL,      &har_power_charge)
        .set(SLOT_SPECIAL_SIDE, &har_dash_slash)
        .set(SLOT_SPECIAL_UP,   &har_up_down_slash)
        .set(SLOT_SPECIAL_DOWN, &har_reflect)
        .set(SLOT_JAB,          &har_jab)
        .set(SLOT_FSMASH,       &har_huge_slash)
        .set(SLOT_USMASH,       &har_up_slash)
        .set(SLOT_DSMASH,       &har_spin_slash)
        .set(SLOT_NAIR,         &har_air_spin)
        .set(SLOT_FAIR,         &har_air_spin)
        .set(SLOT_BAIR,         &har_air_spin)
        .set(SLOT_UAIR,         &har_air_spin)
        .set(SLOT_DAIR,         &har_air_spin);

}

#endif
