#ifndef BATTLE_MOVES_H
#define BATTLE_MOVES_H

// ================================================================================================
//  EVERY MOVE IN THE GAME LIVES IN THIS FILE
//
//  Jump to a fighter by searching for its banner:   ==== WAFFLES ====   ==== FILIP ====
//                                                   ==== UMBREON ====   ==== HARUE ====
//  Every fighter section has the same 3 parts:  1) its moves   2) its final smash   3) its BUTTON MAP
// ================================================================================================
//
//  HOW TO ADD A MOVE  (3 steps, no other file has to change)
//
//    1. COPY a move that is close to what you want, paste it ABOVE that fighter's "BUTTON MAP",
//       and give it a new name (the C++ name AND the "Name In Quotes", which is shown in the debug menu).
//    2. CHANGE THE NUMBERS (the cheat sheet below says what each one does).
//    3. PUT IT ON A BUTTON: in that fighter's BUTTON MAP, point a slot at it:   .set(SLOT_FAIR, &my_move)
//
//  Things that trip people up:
//    - A move can only point at (&) moves that are written ABOVE it in the file.
//    - There is ONE semicolon, at the very end of a move. No semicolons between the .hit(...) lines.
//    - A move has at most 4 hitboxes (MAX_HITBOXES in battle_logic.h; raise it if you need more).
//    - A slot you leave out of the BUTTON MAP simply means "this fighter has no such move".
//
// ------------------------------------------------------------------------------------------------
//  A MOVE IS A TIMELINE. Frame 0 = the frame after the button press. 60 frames = 1 second.
//
//      constexpr Move my_move = Move("My Move", 40, ANIM_SLAP)    // shown name, TOTAL frames, pose
//          .hit(Hitbox().frames(8, 14)                            // WHEN:      active on frames 8..13
//                       .at(20, 14).size(24, 18)                  // WHERE:     20px in front, 14px up. Box is 24 x 18
//                       .dmg(8).launch(fp(3.0), fp(-2.0))         // HOW HARD:  damage, knockback (kx, ky)
//                       .stun(20));                               //            victim can't act for 20 frames
//
//  Total frames = startup + active + recovery. Frames 0..7 above are startup, 8..13 are the hit, and
//  frames 14..39 are recovery: the fighter is stuck until frame 40. A long recovery = a risky move.
//
//  Every hitbox in this file is written in the same 3 lines:  WHEN / WHERE / HOW HARD  (+ extras at the end).
//
// ------------------------------------------------------------------------------------------------
//  NUMBERS THAT FEEL RIGHT (taken from the moves below; Waffles has 300 HP, Filip 500, Harue 690)
//
//     damage      2-4 light jab hit    5-9 normal    10-14 strong smash    18-25 huge, with a long wind-up
//     launch kx   0.4 barely pushes    1-2 small     3-4 normal            5-6 sends them flying
//     launch ky   0 sideways           -1.5 small pop   -3 normal   -5 almost straight up   (+ = spikes down)
//     stun        10-14 light          16-24 normal  26-34 heavy
//     where       y = 14 is the middle of the body, y = 30+ is above the head, x = 16-28 is arm's reach
//     your body   14 wide and 26 tall (a little different per fighter)
//     fp(2.5)     means 2.5 pixels per frame. Walking is ~2, a jump goes up ~48 pixels.
//
// ------------------------------------------------------------------------------------------------
//  THE BUTTONS (SLOTS). Each fighter's BUTTON MAP says which move sits in which slot.
//
//     B                   SLOT_SPECIAL         A on the ground          SLOT_JAB
//     B + left/right      SLOT_SPECIAL_SIDE    A + left/right           SLOT_FSMASH
//     B + up              SLOT_SPECIAL_UP      A + up                   SLOT_USMASH
//     B + down            SLOT_SPECIAL_DOWN    A + down                 SLOT_DSMASH
//
//     A in the air        SLOT_NAIR            A in the air + forward   SLOT_FAIR
//     A in the air + up   SLOT_UAIR            A in the air + back      SLOT_BAIR
//     A in the air + down SLOT_DAIR
//
//  The Final Smash is not a slot: it is the "fs_start" move you hand to the fighter in game_fighters.h.
//
// ------------------------------------------------------------------------------------------------
//  CHEAT SHEET: everything you can chain onto  Move(...)   Hitbox()   Shot()   Drive()
//
//  --- Move("name", total frames, ANIM_pose) ---------------------------------------------------
//    .hit(Hitbox()...)                       a melee hitbox (max 4). Every hitbox hits ONCE per use.
//    .dash(Drive()...)                       the fighter flies for a while (aqua jet). .dash2() = a second one
//    .fires(frame, Shot()...)                something flies away at that frame
//    .fires_random(frame, list, how_many)    ...a random one out of a list of Shots (see Insult, Throw Junk)
//    .fires_on_landing(Shot()..., frame)     ...appears when the fighter touches the ground, then jumps to `frame`
//    .once_per_jump()                        can't be used twice before you land (so up-moves can't fly away)
//    .with_anim(ANIM_x)                      the same move with another pose (copy a move, change the picture)
//    .leaps(frame, vx, vy)                   thrown off the ground at that frame, a jump you can't steer
//    .ends_on_landing(frame)                 a downward dash that touches the ground jumps to this frame
//    .super_armor(from, to, max_dmg)         hits of at most max_dmg damage don't flinch him in those frames
//
//  --- catching things (Fox Thief, Counter, Blade Reflect) -------------------------------------
//    .absorbs(from, to)                      enemy shots that touch the fighter in those frames are stolen
//    .reflects(from, to)                     shots are bounced back (Harue)
//    .swaps_when_holding(&other_move)        if a stolen shot is held, use THAT move instead of this one
//    .throws_stored(frame, power%)           throws the stolen shot at that frame (130 = 30% stronger)
//    .orbits_held()                          the stolen shot spins around the fighter until it is thrown
//    .after_absorb(&move)                    a stolen shot starts `move` at once
//    .counters_melee(from, to, &move)        a melee hit in those frames is caught: attacker grabbed, `move` starts
//    .after_grab(&move)                      a hitbox with .grabs() connected: `move` takes over
//
//  --- the special tricks (Umbreon + Harue + final smashes) ------------------------------------
//    .mine_mat(f) .craft_weapon(f) .place_block(f) .place_tnt(f) .mount_pig(f)      Umbreon (f = frame it happens)
//    .charge_clone()  .charge_power()  .summon_har_clone()                          Waffles / Harue charge moves
//    .starts_final_smash(FS_x)               if this move hits, the final smash starts
//    .in_stage_final_smash(FS_x)             ...one that plays in the stage instead of a cutscene (Harue)
//    .powered_final_smash(FS_x)              ...a different one while Harue is powered up
//    .ally_fs(FS_x)                          ...the version used when it hits a teammate
//
//  --- Hitbox() --------------------------------------------------------------------------------
//    .frames(start, end)       active on frames start .. end-1
//    .at(x, y)                 the middle of the box: x pixels IN FRONT of you (negative = behind), y above your feet
//    .size(w, h)               the box, in pixels
//    .dmg(n)                   damage
//    .launch(kx, ky)           knockback speed: kx pushes away from you; ky negative = up, positive = down
//    .stun(n)                  frames the victim can't act
//    extras:  .left_right()    x is the real left/right of the SCREEN, not "in front of you"
//             .grabs()         grab instead of damage (pair it with .after_grab on the move)
//             .keeps_grab()    damage only, a held victim stays held (the scratches)
//             .bounces(speed)  the attacker bounces off the victim (pogo)
//             .weapon()        damage and knockback scale with the Minecraft weapon (and wear it down)
//             .hits_teammates()  also hits your own team
//             .delayed_boom(dmg) plants an explosion on the victim (only while Harue is powered up)
//
//  --- Shot() (anything that flies) ------------------------------------------------------------
//    .of(PROJ_x)               which picture/kind it is (see the list below)
//    .dmg(n)  .launch(kx, ky)  .stun(n)    same as a hitbox
//    .fly(speed, frames)       flies straight. Range = speed x frames. speed 0 = stays where it appears
//    .arc(vx, vy, gravity, frames)   a thrown object that flies in an arc instead
//    .size(w, h)               its box
//    .spawn(x, y)              where it appears: x in front of you, y above your feet
//    .steal_proof()            Fox Thief can't steal it       .lingers()   stays after it hit (explosions)
//    .explodes(dmg)            explodes when it hits or lands  .labeled("TEXT")   drawn as text (no sprite yet)
//    .fan(vx)                  fires three: straight and one to each side      .boomerang(accel)  comes back
//    .weapon()                 scaled by the Minecraft weapon  .hurts_everyone()  hits your own team too
//    .heals_ally(hp)           heals a teammate it touches     .starts_final_smash(FS_x)
//
//  --- Drive() (the fighter flies) -------------------------------------------------------------
//    .frames(start, end)       .speed(vx, vy)   vx points where he faces, vy negative = up
//    .steer(percent)           0..100: steers up/down toward the opponent    .stops_on_hit()
//
//  --- lists you can pick from (they live in battle_logic.h) -----------------------------------
//    PROJ_ (what a Shot looks like):
//      WATER EXPLOSION JUNK_PENCIL JUNK_APPLE JUNK_BAT JUNK_LAPTOP JUNK_BALL
//      FIREBALL BOOMERANG ARROW MINECART TNT SONIC_SLASH
//    ANIM_ (which pose a move uses; these are stand-ins until each move gets its own sprites):
//      STAND SLAP TAIL TAIL_UP KICK SPIN SLAM SHOOT JET_SIDE JET_UP JET_DOWN STANCE RTS
//      WAF_CHARGE WAF_STEAL      SNEP_JAB SNEP_HEAVY SNEP_FLIP SNEP_DSMASH SNEP_COUNTER SNEP_COUNTER_GRAB
//      SNEP_SPIN_THROW SNEP_THROW_UP SNEP_LEAP SNEP_SCRATCH SNEP_DAIR SNEP_PUNCH
//      UMB_MINE UMB_SLASH UMB_PLACE UMB_BOW UMB_THROW      HAR_JAB HAR_FSMASH HAR_USMASH HAR_SPIN
//      HAR_CHARGE HAR_BSHOT HAR_DASH HAR_UPDOWN HAR_REFLECT HAR_FS      (all written ANIM_xxx in code)
// ================================================================================================

#include "battle_logic.h"

namespace battle
{

    // ============================================================================================
    // ==== WAFFLES ====
    // ============================================================================================

    // ---- B: Water Gun ----------------------------------------------------------------------
    // 8 frames of startup, 14 of recovery. The water also heals a teammate it touches.
    constexpr Move water_gun = Move("Water Gun", 22, ANIM_SHOOT)
        .fires(8, Shot().of(PROJ_WATER)
                        .dmg(2).fly(fp(4), 70)
                        .size(12, 8).spawn(12, 14)
                        .launch(fp(0), 0).stun(0)
                        .heals_ally(2));

    // ---- B + left/right: Aqua Jet ----------------------------------------------------------
    // Charges for 8 frames, then dashes ~80 pixels at high speed. It steers a little toward the
    // opponent if they are in front. It ends the moment it hits.
    constexpr Move aqua_jet = Move("Aqua Jet", 34, ANIM_JET_SIDE)
        .once_per_jump()
        .dash(Drive().frames(8, 23).speed(fp(5.5), 0).steer(30).stops_on_hit())
        .hit(Hitbox().frames(8, 23)
                     .at(2, 13).size(22, 22)
                     .dmg(7).launch(fp(3.0), fp(-2.5)).stun(20));

    // ---- B + up: Water Clone ---------------------------------------------------------------
    // HOLD B + Up: a gauge charges. At full charge a Water Clone of the enemy appears
    // (10% HP, 50% damage).
    constexpr Move waffle_up_clone = Move("Water Clone", 1000, ANIM_WAF_CHARGE)
        .charge_clone();

    // (Not on a button right now: an Aqua Jet that goes straight up, with more damage than the sideways one.
    //  To use it, change the SLOT_SPECIAL_UP line in waffle_moveset to &aqua_jet_up.)
    constexpr Move aqua_jet_up = Move("Aqua Jet Up", 38, ANIM_JET_UP)
        .once_per_jump()
        .dash(Drive().frames(6, 21).speed(0, fp(-5.0)).stops_on_hit())
        .hit(Hitbox().frames(6, 21)
                     .at(0, 16).size(20, 28)
                     .dmg(10).launch(fp(1.0), fp(-4.5)).stun(26));

    // ---- B + down: Fox Thief ---------------------------------------------------------------
    // Stand still for a moment: any shot that touches you in frames 3-24 is stolen (no damage).
    // Press it AGAIN while holding a stolen shot and you throw it back, 30% stronger and faster.
    // (The throw is written first, because Fox Thief points at it.)
    constexpr Move fox_thief_throw = Move("Fox Thief (throw)", 26, ANIM_RTS)
        .throws_stored(8, 130);

    constexpr Move fox_thief = Move("Fox Thief", 30, ANIM_WAF_STEAL)
        .absorbs(3, 25)
        .swaps_when_holding(&fox_thief_throw);

    // ---- A: Slap Combo ---------------------------------------------------------------------
    // Three slaps. The first two barely push, so the next one still reaches. The third launches.
    constexpr Move jab = Move("Slap Combo", 38, ANIM_SLAP)
        .hit(Hitbox().frames(4, 8)
                     .at(16, 14).size(16, 12)
                     .dmg(2).launch(fp(0.4), 0).stun(14))
        .hit(Hitbox().frames(14, 18)
                     .at(16, 14).size(16, 12)
                     .dmg(2).launch(fp(0.4), 0).stun(14))
        .hit(Hitbox().frames(24, 30)
                     .at(18, 14).size(20, 14)
                     .dmg(4).launch(fp(2.5), fp(-1.5)).stun(16));

    // ---- A + left/right: Tail Slam (forward smash) -----------------------------------------
    constexpr Move tail_slam = Move("Tail Slam", 46, ANIM_TAIL)
        .hit(Hitbox().frames(14, 19)
                     .at(20, 14).size(30, 20)
                     .dmg(11).launch(fp(4.5), fp(-3.0)).stun(24));

    // ---- A + up: Tail Slam Up (up smash) ---------------------------------------------------
    constexpr Move tail_slam_up = Move("Tail Slam Up", 46, ANIM_TAIL_UP)
        .hit(Hitbox().frames(14, 19)
                     .at(2, 30).size(30, 28)
                     .dmg(10).launch(fp(1.0), fp(-5.0)).stun(26));

    // ---- A + down: Fist Slam (down smash) --------------------------------------------------
    // Both fists slam down, one box on each side. Whoever is on a side gets launched that way.
    constexpr Move fist_slam = Move("Fist Slam", 44, ANIM_SLAM)
        .hit(Hitbox().frames(12, 17)
                     .at(20, 6).size(24, 14)
                     .dmg(9).launch(fp(3.0), fp(-3.5)).stun(22))
        .hit(Hitbox().frames(12, 17)
                     .at(-20, 6).size(24, 14)
                     .dmg(9).launch(fp(3.0), fp(-3.5)).stun(22));

    // ---- A in the air: Spin (neutral air) --------------------------------------------------
    // Spins all the way around. Two hits: the first is light, the second launches.
    constexpr Move spin = Move("Spin", 32, ANIM_SPIN)
        .hit(Hitbox().frames(4, 12)
                     .at(0, 13).size(34, 24)
                     .dmg(3).launch(fp(1.0), fp(-1.5)).stun(12))
        .hit(Hitbox().frames(14, 22)
                     .at(0, 13).size(36, 26)
                     .dmg(4).launch(fp(2.8), fp(-2.0)).stun(16));

    // ---- A in the air + forward: Kick ------------------------------------------------------
    constexpr Move kick = Move("Kick", 28, ANIM_KICK)
        .hit(Hitbox().frames(6, 12)
                     .at(18, 10).size(22, 12)
                     .dmg(6).launch(fp(3.5), fp(-1.5)).stun(18));

    // ---- A in the air + back: Tail Slap Back (you keep facing forward) ---------------------
    constexpr Move tail_slap_back = Move("Tail Slap Back", 26, ANIM_TAIL)
        .hit(Hitbox().frames(5, 11)
                     .at(-20, 14).size(26, 14)
                     .dmg(7).launch(fp(3.8), fp(-1.5)).stun(18));

    // ---- A in the air + up: Tail Slap Up ---------------------------------------------------
    constexpr Move tail_slap_up = Move("Tail Slap Up", 26, ANIM_TAIL_UP)
        .hit(Hitbox().frames(5, 11)
                     .at(2, 34).size(28, 16)
                     .dmg(6).launch(fp(0.5), fp(-4.0)).stun(20));

    // ---- A in the air + down: Aqua Dive ----------------------------------------------------
    // An aqua jet straight DOWN. Hits (and spikes) whatever is below, and when you touch the ground
    // there is an explosion. The move then jumps to frame 52, so only 16 frames of recovery are left.
    constexpr Move aqua_dive = Move("Aqua Dive", 68, ANIM_JET_DOWN)
        .dash(Drive().frames(8, 52).speed(0, fp(6.0)))
        .hit(Hitbox().frames(8, 52)
                     .at(0, 4).size(18, 26)
                     .dmg(8).launch(fp(0.5), fp(3.0)).stun(18))
        .fires_on_landing(Shot().of(PROJ_EXPLOSION)
                                .dmg(9).fly(0, 10)
                                .size(40, 30).spawn(0, 12)
                                .launch(fp(3.5), fp(-4.0)).stun(22)
                                .steal_proof().lingers(),
                          52);

    // ---- FINAL SMASH: Fire Uppercut --------------------------------------------------------
    // A lunging uppercut. If it hits, Fire Blast starts! (If it hits a teammate: the Meal one.)
    constexpr Move waffle_fs_start = Move("Fire Uppercut", 32, ANIM_TAIL_UP)
        .dash(Drive().frames(4, 13).speed(fp(3.8), fp(-1.2)).stops_on_hit())
        .hit(Hitbox().frames(5, 14)
                     .at(16, 16).size(24, 28)
                     .dmg(6).launch(0, fp(-3.5)).stun(22)
                     .hits_teammates())
        .starts_final_smash(FS_WAFFLE_FIRE)
        .ally_fs(FS_WAFFLE_MEAL);

    // ---- WAFFLES: BUTTON MAP ---------------------------------------------------------------
    constexpr MoveSet waffle_moveset = MoveSet()
        .set(SLOT_SPECIAL,      &water_gun)         // B
        .set(SLOT_SPECIAL_SIDE, &aqua_jet)          // B + left/right
        .set(SLOT_SPECIAL_UP,   &waffle_up_clone)   // B + up
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
    // ==== FILIP (called "snep" in the code) ====
    // ============================================================================================

    // ---- B + down: Counter -----------------------------------------------------------------
    // Stand ready (frames 4-29). A MELEE hit in that time is caught: he grabs the attacker, then punches
    // them away. A SHOT that touches him in that time is caught too: he spins with it and throws it back,
    // twice as hard and twice as fast. (The two follow-up moves are written first, Counter points at them.)
    constexpr Move counter_punch = Move("Counter Punch", 58, ANIM_SNEP_COUNTER_GRAB)
        .hit(Hitbox().frames(36, 42)
                     .at(16, 14).size(28, 22)
                     .dmg(18).launch(fp(5.5), fp(-3.5)).stun(34));

    constexpr Move counter_return = Move("Counter Return", 44, ANIM_SNEP_SPIN_THROW)
        .throws_stored(24, 200)
        .orbits_held();

    constexpr Move counter = Move("Counter", 46, ANIM_SNEP_COUNTER)
        .absorbs(4, 30)
        .after_absorb(&counter_return)
        .counters_melee(4, 30, &counter_punch);

    // ---- B: Insult -------------------------------------------------------------------------
    // A random word flies out. No damage and no knockback, but the victim is stunned.
    // To add a word: add a line to the list AND raise the 6 in .fires_random(8, words, 6).
    constexpr Shot words[] =
    {
        Shot().dmg(0).fly(fp(1.0), 60).size(22, 10).spawn(12, 16).stun(30).labeled("LOSER"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(30).labeled("NOOB"),
        Shot().dmg(0).fly(fp(1.0), 60).size(22, 10).spawn(12, 16).stun(35).labeled("TRASH"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(40).labeled("IDIOT"),
        Shot().dmg(0).fly(fp(1.0), 60).size(18, 10).spawn(12, 16).stun(50).labeled("BOZO"),
        Shot().dmg(0).fly(fp(1.0), 60).size(24, 10).spawn(12, 16).stun(60).labeled("CRINGE"),
    };
    constexpr Move insult = Move("Insult", 26, ANIM_SHOOT)
        .fires_random(8, words, 6);

    // ---- B + left/right: Pounce ------------------------------------------------------------
    // Leaps forward. If it connects he grabs on and scratches 3 times, then lets go. If it misses
    // there is a long recovery (the move is 72 frames, he lands around frame 25).
    // (The scratches are written first, Pounce points at them.)
    constexpr Move scratch_flurry = Move("Scratch Flurry", 58, ANIM_SNEP_SCRATCH)
        .hit(Hitbox().frames(8, 11)
                     .at(16, 14).size(24, 22)
                     .dmg(3).stun(10).keeps_grab())
        .hit(Hitbox().frames(20, 23)
                     .at(16, 14).size(24, 22)
                     .dmg(3).stun(10).keeps_grab())
        .hit(Hitbox().frames(32, 35)
                     .at(16, 14).size(24, 22)
                     .dmg(3).stun(10).keeps_grab())
        .hit(Hitbox().frames(46, 49)                  // letting go
                     .at(16, 14).size(24, 22)
                     .dmg(2).launch(fp(3.0), fp(-1.5)).stun(18));

    constexpr Move pounce = Move("Pounce", 72, ANIM_SNEP_LEAP)
        .once_per_jump()
        .leaps(6, fp(4.2), -fp(4.0))
        .hit(Hitbox().frames(8, 30)
                     .at(12, 14).size(22, 26)
                     .grabs())
        .after_grab(&scratch_flurry);

    // ---- B + up: Throw Junk ----------------------------------------------------------------
    // A random object flies up and forward in an arc. Heavier objects hit harder.
    // The laptop explodes when it touches somebody (or the floor).
    // (No sprites for these yet: they are drawn as text/stand-ins.)
    // To add an object: add a line AND raise the 5 in .fires_random(10, junk, 5).
    constexpr Shot junk[] =
    {
        Shot().dmg(3).arc(fp(2.2), -fp(4.2), 3, 90).size(12, 5).spawn(6, 24).launch(fp(1.2), fp(-1.0)).stun(12).of(PROJ_JUNK_PENCIL),
        Shot().dmg(4).arc(fp(1.8), -fp(4.4), 3, 90).size(9, 9).spawn(6, 24).launch(fp(1.5), fp(-1.5)).stun(14).of(PROJ_JUNK_APPLE),
        Shot().dmg(8).arc(fp(1.5), -fp(4.6), 4, 90).size(18, 6).spawn(6, 24).launch(fp(3.5), fp(-2.5)).stun(18).of(PROJ_JUNK_BAT),
        Shot().dmg(3).arc(fp(1.6), -fp(4.6), 4, 90).size(14, 10).spawn(6, 24).launch(fp(1.0), fp(-1.0)).stun(10).explodes(9).of(PROJ_JUNK_LAPTOP),
        Shot().dmg(25).arc(fp(1.2), -fp(4.2), 4, 90).size(12, 12).spawn(6, 24).launch(fp(4.0), fp(-3.0)).stun(24).of(PROJ_JUNK_BALL),
    };
    constexpr Move throw_junk = Move("Throw Junk", 40, ANIM_SNEP_THROW_UP)
        .fires_random(10, junk, 5);

    // ---- A: Jab ----------------------------------------------------------------------------
    // The same slap combo as Waffles, with Filip's picture. (This is why Waffles' section has to stay above.)
    constexpr Move snep_jab = jab.with_anim(ANIM_SNEP_JAB);

    // ---- A + left/right: Heavy Punch -------------------------------------------------------
    // A long wind-up, then a huge hit. Super armor from frame 8 until the punch comes out: hits of
    // 12 damage or less don't flinch him, a harder hit breaks the armor.
    constexpr Move heavy_punch = Move("Heavy Punch", 76, ANIM_SNEP_HEAVY)
        .super_armor(8, 36, 12)
        .hit(Hitbox().frames(34, 40)
                     .at(20, 14).size(32, 22)
                     .dmg(24).launch(fp(6.0), fp(-3.0)).stun(32));

    // ---- A + up: Backflip ------------------------------------------------------------------
    // Flips up and backwards, kicking on the way up.
    constexpr Move backflip = Move("Backflip", 44, ANIM_SNEP_FLIP)
        .dash(Drive().frames(4, 20).speed(-fp(1.2), -fp(3.4)))
        .hit(Hitbox().frames(6, 12)
                     .at(14, 20).size(22, 20)
                     .dmg(7).launch(fp(1.5), fp(-3.0)).stun(18))
        .hit(Hitbox().frames(11, 18)
                     .at(4, 40).size(26, 22)
                     .dmg(9).launch(fp(0.8), fp(-5.0)).stun(24));

    // ---- A + down: Left-Right Punch --------------------------------------------------------
    // Punches LEFT first, then RIGHT (the real left and right of the screen, whichever way he faces).
    constexpr Move double_punch = Move("Left-Right Punch", 46, ANIM_SNEP_DSMASH)
        .hit(Hitbox().frames(8, 13)
                     .at(-20, 14).size(26, 20)
                     .dmg(8).launch(fp(2.5), fp(-1.5)).stun(18)
                     .left_right())
        .hit(Hitbox().frames(24, 29)
                     .at(20, 14).size(26, 20)
                     .dmg(10).launch(fp(4.0), fp(-2.5)).stun(22)
                     .left_right());

    // ---- A in the air + down: Pogo Kick ----------------------------------------------------
    // Dives down and a little forward. Hitting somebody bounces him back up, so he can kick again
    // right away. Touching the ground ends the dive (a short landing lag).
    constexpr Move pogo_kick = Move("Pogo Kick", 40, ANIM_SNEP_DAIR)
        .dash(Drive().frames(6, 34).speed(fp(1.4), fp(4.8)))
        .hit(Hitbox().frames(6, 34)
                     .at(4, 4).size(16, 20)
                     .dmg(6).launch(fp(1.0), fp(2.5)).stun(16)
                     .bounces(fp(4.4)))
        .ends_on_landing(28);

    // ---- FINAL SMASH: Ring Drop ------------------------------------------------------------
    // Dashes forward. If it hits, Ring Drop starts!
    constexpr Move filip_fs_start = Move("Ring Drop", 34, ANIM_SNEP_LEAP)
        .dash(Drive().frames(4, 14).speed(fp(5.2), 0).stops_on_hit())
        .hit(Hitbox().frames(5, 14)
                     .at(14, 14).size(24, 24)
                     .dmg(6).launch(fp(2.0), fp(-2.5)).stun(22))
        .starts_final_smash(FS_FILIP_DROP);

    // ---- FILIP: BUTTON MAP -----------------------------------------------------------------
    // (No aerials except the down one yet: a slot you leave out = no such move.)
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


    // ============================================================================================
    // ==== UMBREON (Minecraft) ====
    // ============================================================================================
    // Umbreon starts with a crafting table, mines materials and crafts weapons. The moves marked
    // .weapon() get stronger with the weapon he holds (wood, stone, iron, diamond).

    // ---- B in the air: Place Block ---------------------------------------------------------
    // Places a block beneath his feet.
    constexpr Move umb_place_block = Move("Place Block", 18, ANIM_UMB_PLACE)
        .place_block(4);

    // ---- B on the crafting table: Craft Weapon ---------------------------------------------
    // With enough materials: Iron (1 mat) or Diamond (2 mats) weapon. In the air it turns into Place Block.
    constexpr Move umb_craft = Move("Craft Weapon", 24, ANIM_UMB_PLACE)
        .craft_weapon(10)
        .swaps_when_holding(&umb_place_block);

    // ---- B on the ground: Mine -------------------------------------------------------------
    // +1 material with a close pickaxe swing. On the crafting table it turns into Craft Weapon.
    // (Written last of the three, because it points at Craft Weapon, which points at Place Block.)
    constexpr Move umb_mine_move = Move("Mine", 24, ANIM_UMB_MINE)
        .mine_mat(12)
        .hit(Hitbox().frames(8, 14)
                     .at(14, 12).size(16, 14)
                     .dmg(4).launch(fp(1.2), fp(-1.0)).stun(12))
        .swaps_when_holding(&umb_craft);

    // ---- B + left/right: Fox Ride ----------------------------------------------------------
    // Rides a pig with a carrot fishing rod. Jump any time to get off; knocked off if the pig dies.
    constexpr Move umb_pig_ride = Move("Fox Ride", 10, ANIM_STAND)
        .mount_pig(4);

    // ---- B + up: Fireball Trio -------------------------------------------------------------
    // Shoots three fireballs upwards and diagonally.
    constexpr Move umb_fireballs = Move("Fireball Trio", 28, ANIM_UMB_THROW)
        .fires(8, Shot().of(PROJ_FIREBALL)
                        .dmg(3).arc(0, -fp(3.6), 0, 48).fan(fp(2.4))
                        .size(12, 12).spawn(0, 24)
                        .launch(fp(1.5), fp(-3.2)).stun(16));

    // ---- B + down: Place TNT ---------------------------------------------------------------
    // Explodes when touched by anyone, even Umbreon.
    constexpr Move umb_tnt = Move("Place TNT", 24, ANIM_UMB_PLACE)
        .place_tnt(10);

    // ---- A: Boomerang ----------------------------------------------------------------------
    constexpr Move umb_boomerang = Move("Boomerang", 26, ANIM_UMB_THROW)
        .fires(7, Shot().of(PROJ_BOOMERANG)
                        .dmg(4).fly(fp(4.5), 90).boomerang(2)
                        .size(14, 12).spawn(12, 14)
                        .launch(fp(1.8), fp(-1.8)).stun(14)
                        .weapon());

    // ---- A + left/right: Sword Slash -------------------------------------------------------
    // Swings the sword with full force.
    constexpr Move umb_sword_full = Move("Sword Slash", 38, ANIM_UMB_SLASH)
        .hit(Hitbox().frames(10, 17)
                     .at(20, 14).size(30, 22)
                     .dmg(12).launch(fp(4.8), fp(-3.2)).stun(24)
                     .weapon());

    // ---- A + up: Bow Shot ------------------------------------------------------------------
    // Shoots the bow upwards.
    constexpr Move umb_bow_up = Move("Bow Shot", 30, ANIM_UMB_BOW)
        .fires(12, Shot().of(PROJ_ARROW)
                         .dmg(8).arc(0, -fp(5.4), 2, 95)
                         .size(8, 16).spawn(0, 26)
                         .launch(fp(0.5), fp(-4.0)).stun(20)
                         .weapon());

    // ---- A + down: Pickaxe Smash -----------------------------------------------------------
    // Smashes both sides with the pickaxe.
    constexpr Move umb_pickaxe_smash = Move("Pickaxe Smash", 42, ANIM_UMB_MINE)
        .hit(Hitbox().frames(10, 16)
                     .at(20, 10).size(24, 18)
                     .dmg(6).launch(fp(3.6), fp(-3.2)).stun(22)
                     .weapon())
        .hit(Hitbox().frames(10, 16)
                     .at(-20, 10).size(24, 18)
                     .dmg(5).launch(fp(3.6), fp(-3.2)).stun(22)
                     .weapon());

    // ---- A in the air (every direction): Air Slash -----------------------------------------
    // A simple sword slash. All five aerial slots use this one move for now.
    constexpr Move umb_air_slash = Move("Air Slash", 24, ANIM_UMB_SLASH)
        .hit(Hitbox().frames(5, 13)
                     .at(16, 14).size(26, 20)
                     .dmg(3).launch(fp(2.6), fp(-1.6)).stun(16)
                     .weapon());

    // ---- FINAL SMASH: Minecart Throw -------------------------------------------------------
    // Throws a minecart. If it hits: the Minecraft House explosion cutscene!
    constexpr Move umb_fs_start = Move("Minecart Throw", 32, ANIM_UMB_THROW)
        .fires(8, Shot().of(PROJ_MINECART)
                        .dmg(6).fly(fp(5.0), 48)
                        .size(26, 16).spawn(16, 10)
                        .launch(fp(2.0), fp(-2.0)).stun(20)
                        .steal_proof().starts_final_smash(FS_UMB_MINECART));

    // ---- UMBREON: BUTTON MAP ---------------------------------------------------------------
    constexpr MoveSet umb_moveset = MoveSet()
        .set(SLOT_SPECIAL,      &umb_mine_move)     // B
        .set(SLOT_SPECIAL_SIDE, &umb_pig_ride)      // B + left/right
        .set(SLOT_SPECIAL_UP,   &umb_fireballs)     // B + up
        .set(SLOT_SPECIAL_DOWN, &umb_tnt)           // B + down
        .set(SLOT_JAB,          &umb_boomerang)     // A
        .set(SLOT_FSMASH,       &umb_sword_full)    // A + left/right
        .set(SLOT_USMASH,       &umb_bow_up)        // A + up
        .set(SLOT_DSMASH,       &umb_pickaxe_smash) // A + down
        .set(SLOT_NAIR,         &umb_air_slash)     // A in the air
        .set(SLOT_FAIR,         &umb_air_slash)     // A in the air + forward
        .set(SLOT_BAIR,         &umb_air_slash)     // A in the air + back
        .set(SLOT_UAIR,         &umb_air_slash)     // A in the air + up
        .set(SLOT_DAIR,         &umb_air_slash);    // A in the air + down


    // ============================================================================================
    // ==== HARUE ====
    // ============================================================================================
    // Harue has a Power-Up: charge it with a held B, and for 20 seconds he is faster and hits harder
    // (+35% speed, +30% damage), and some moves get extra effects.

    // ---- B: Sonic Slash (tap) / Focus Charge (hold) ----------------------------------------
    // Tap: a fast blue slash projectile (low damage, strong knockback).
    // Hold: charges the Power-Up gauge. (Sonic Slash is written first, Focus Charge points at it.)
    constexpr Move har_sonic_boom = Move("Sonic Slash", 30, ANIM_HAR_BSHOT)
        .fires(4, Shot().of(PROJ_SONIC_SLASH)
                        .dmg(3).fly(fp(2.0), 40)
                        .size(16, 22).spawn(16, 14)
                        .launch(fp(2.2), fp(-2.4)).stun(20));

    constexpr Move har_power_charge = Move("Focus Charge", 1000, ANIM_HAR_CHARGE)
        .charge_power()
        .swaps_when_holding(&har_sonic_boom);

    // ---- B + left/right: Flash Slash -------------------------------------------------------
    // Dash and slash. When powered up it plants a 2-second delayed explosion on the victim.
    constexpr Move har_dash_slash = Move("Flash Slash", 50, ANIM_HAR_DASH)
        .once_per_jump()
        .dash(Drive().frames(10, 22).speed(fp(8.5), 0))
        .hit(Hitbox().frames(10, 22)
                     .at(12, 14).size(28, 22)
                     .dmg(8).launch(0, 0).stun(18)
                     .delayed_boom(16));

    // ---- B + up: Sky Splitter --------------------------------------------------------------
    // An upper slash into the air, then a heavy downward slam slash (huge damage, huge end lag).
    constexpr Move har_up_down_slash = Move("Sky Splitter", 66, ANIM_HAR_UPDOWN)
        .once_per_jump()
        .dash(Drive().frames(4, 16).speed(fp(1.0), -fp(5.2)))
        .dash2(Drive().frames(24, 36).speed(fp(1.2), fp(6.2)))
        .hit(Hitbox().frames(5, 16)
                     .at(10, 22).size(24, 28)
                     .dmg(7).launch(fp(0.5), -fp(5.4)).stun(20))
        .hit(Hitbox().frames(25, 36)
                     .at(14, 10).size(28, 28)
                     .dmg(18).launch(fp(3.8), fp(4.4)).stun(28))
        .ends_on_landing(40);

    // ---- B + down: Blade Reflect / Shadow Twin ---------------------------------------------
    // Normal: Blade Reflect, a shield stance that reflects projectiles only.
    // While powered up: Shadow Twin, which uses up the Power-Up and summons a clone
    // (Harue or Waffles, 10% HP and 50% damage).
    // (Shadow Twin is written first, Blade Reflect points at it.)
    constexpr Move har_summon_clone = Move("Shadow Family", 22, ANIM_HAR_CHARGE)
        .summon_har_clone();

    constexpr Move har_reflect = Move("Blade Reflect", 30, ANIM_HAR_REFLECT)
        .reflects(3, 25)
        .swaps_when_holding(&har_summon_clone);

    // ---- A: Triple Slash -------------------------------------------------------------------
    // Three slashes back and forth.
    constexpr Move har_jab = Move("Triple Slash", 36, ANIM_HAR_JAB)
        .hit(Hitbox().frames(4, 8)
                     .at(18, 14).size(22, 18)
                     .dmg(3).launch(fp(0.4), 0).stun(14))
        .hit(Hitbox().frames(13, 17)
                     .at(18, 14).size(22, 18)
                     .dmg(3).launch(fp(0.4), 0).stun(14))
        .hit(Hitbox().frames(22, 28)
                     .at(22, 14).size(26, 20)
                     .dmg(5).launch(fp(3.2), fp(-1.8)).stun(18));

    // ---- A + left/right: Great Cleave ------------------------------------------------------
    // A huge slash with massive range.
    constexpr Move har_huge_slash = Move("Great Cleave", 44, ANIM_HAR_FSMASH)
        .hit(Hitbox().frames(12, 19)
                     .at(28, 14).size(44, 28)
                     .dmg(14).launch(fp(5.2), fp(-3.2)).stun(26));

    // ---- A + up: Rising Crescent -----------------------------------------------------------
    // One slash that sends the opponent upwards.
    constexpr Move har_up_slash = Move("Rising Crescent", 34, ANIM_HAR_USMASH)
        .hit(Hitbox().frames(6, 13)
                     .at(12, 26).size(30, 32)
                     .dmg(11).launch(fp(0.6), fp(-5.6)).stun(26));

    // ---- A + down: Whirlwind Slash ---------------------------------------------------------
    // A spinning slash that hits both sides (the real right of the screen first, then the left).
    constexpr Move har_spin_slash = Move("Whirlwind Slash", 38, ANIM_HAR_SPIN)
        .hit(Hitbox().frames(5, 11)
                     .at(18, 14).size(26, 20)
                     .dmg(5).launch(fp(2.4), fp(-2.0)).stun(16)
                     .left_right())
        .hit(Hitbox().frames(13, 19)
                     .at(-18, 14).size(26, 20)
                     .dmg(6).launch(fp(3.0), fp(-2.2)).stun(18)
                     .left_right());

    // ---- A in the air (every direction): Sword Cyclone -------------------------------------
    // A spin attack with the sword. All five aerial slots use this one move for now.
    constexpr Move har_air_spin = Move("Sword Cyclone", 30, ANIM_HAR_SPIN)
        .hit(Hitbox().frames(4, 18)
                     .at(0, 14).size(38, 28)
                     .dmg(8).launch(fp(3.2), fp(-2.2)).stun(18));

    // ---- FINAL SMASH: Twin Tempest ---------------------------------------------------------
    // A huge lunge slash. If it hits: the in-stage clone combo, ending with the double down-slash
    // explosion. While powered up the finish is different (the Slice one).
    constexpr Move har_fs_start = Move("Twin Tempest", 34, ANIM_HAR_FS)
        .dash(Drive().frames(4, 15).speed(fp(4.8), 0).stops_on_hit())
        .hit(Hitbox().frames(5, 16)
                     .at(22, 14).size(36, 28)
                     .dmg(6).launch(0, fp(-2.0)).stun(24))
        .in_stage_final_smash(FS_HARUE_COMBO)
        .powered_final_smash(FS_HARUE_SLICE);

    // ---- HARUE: BUTTON MAP -----------------------------------------------------------------
    constexpr MoveSet har_moveset = MoveSet()
        .set(SLOT_SPECIAL,      &har_power_charge)  // B
        .set(SLOT_SPECIAL_SIDE, &har_dash_slash)    // B + left/right
        .set(SLOT_SPECIAL_UP,   &har_up_down_slash) // B + up
        .set(SLOT_SPECIAL_DOWN, &har_reflect)       // B + down
        .set(SLOT_JAB,          &har_jab)           // A
        .set(SLOT_FSMASH,       &har_huge_slash)    // A + left/right
        .set(SLOT_USMASH,       &har_up_slash)      // A + up
        .set(SLOT_DSMASH,       &har_spin_slash)    // A + down
        .set(SLOT_NAIR,         &har_air_spin)      // A in the air
        .set(SLOT_FAIR,         &har_air_spin)      // A in the air + forward
        .set(SLOT_BAIR,         &har_air_spin)      // A in the air + back
        .set(SLOT_UAIR,         &har_air_spin)      // A in the air + up
        .set(SLOT_DAIR,         &har_air_spin);     // A in the air + down

}

#endif
