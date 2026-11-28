#ifndef BATTLE_TYPES_H
#define BATTLE_TYPES_H

// FighterDef, StageDef, ParallaxLayer and the EV_... sound-effect flags (the data main.cpp writes).

#include "bn_sprite_item.h"
#include "bn_regular_bg_item.h"
#include "bn_affine_bg_item.h"
#include "battle_logic.h"

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
        bool top_warps = true;
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
}

#endif
