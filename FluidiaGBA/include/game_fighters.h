#ifndef GAME_FIGHTERS_H
#define GAME_FIGHTERS_H

// ================================================================================================
// Fighting data: the FighterDefs (Waffles, Filip, Umbreon, Harue) and the playable list
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    // ---- FIGHTING DATA ----
    // Your character and the Waffles NPC both use waffle_fighter, so they have the same moves.
    // To give another NPC a fight: make a FighterDef for them (copy this one) and put its address
    // at the end of their line in npc_defs. NPCs without one can never start a battle.
    // constexpr battle::Move water_shot_move =
    // {
    //     4,                       // damage
    //     8, 14,                   // startup and recovery, in frames (60 frames = 1 second)
    //     battle::PROJ_WATER,
    //     battle::fp(4), 70,     // shot speed (pixels per frame) and how long it lives (frames): range = 245 pixels
    //     12, 8,                   // hitbox width and height
    //     12, 14,                  // it appears this far in front of you and this far above your feet
    // };
    // constexpr battle::Move waffle_moves[] = { water_shot_move };         // moves[0] is the B button

    constexpr battle::FighterDef waffle_fighter =
    {
        "Waffles",
        &bn::sprite_items::waffle,                                       // the overworld sprite sheet is reused
        {
            300,                                                         // max HP
            battle::fp(2.0), battle::fp(0.6), battle::fp(0.2), battle::fp(0.35),   // walk speed, ground accel, air accel, friction
            88, 88, 2,                                                   // jump power, double jump power, jumps (2 = a double jump)
            5, 112, 144,                                                 // gravity, fall speed, fast-fall speed
            14, 26,                                                      // hurtbox width and height, in pixels
            0,                                                           // defense: % of damage that is ALWAYS removed
            60,                                                          // computer skill 0..100 (only used when the computer is this fighter)
            &battle::waffle_moveset,                                     // the moveset
            100, 100, 100,
            &battle::waffle_fs_start,
        },
    };
    constexpr battle::FighterDef snep_fighter =
    {
        "Filip",
        &bn::sprite_items::snep,
        {
            500,
            battle::fp(1.5), battle::fp(0.5),battle::fp(0.5),battle::fp(0.2),
            90,88,2,
            7,140,190,
            14,28,
            10,
            70,
            &battle::snep_moveset,
            100, 100, 100,
            &battle::filip_fs_start,
        }
    };
    constexpr battle::FighterDef umb_fighter =
    {
        "Umbreon",
        &bn::sprite_items::umbreon,
        {
            360,
            battle::fp(1.3), battle::fp(0.7),battle::fp(0.5),battle::fp(0.4),
            70,70,2,
            7,100,120,
            14,26,
            0,
            80,
            &battle::umb_moveset,
            100, 100, 100,
            &battle::umb_fs_start,
            true,
        }
    };
    constexpr battle::FighterDef har_fighter =
    {
        "Harue",
        &bn::sprite_items::harue,
        {
            690,
            battle::fp(2.1), battle::fp(0.65), battle::fp(0.28), battle::fp(0.35),
            100, 0, 1,
            7, 125, 165,
            15, 28,
            5,
            75,
            &battle::har_moveset,
            100, 100, 100,
            &battle::har_fs_start,
        }
    };

    // Add any new playable fighters here — the Character Select screen adapts automatically!
    constexpr const battle::FighterDef* playable_fighters[] =
    {
        &waffle_fighter,
        &snep_fighter,
        &umb_fighter,
        &har_fighter,
    };
    constexpr int playable_count = sizeof(playable_fighters) / sizeof(playable_fighters[0]);
}

#endif
