#ifndef GAME_STAGES_H
#define GAME_STAGES_H

// ================================================================================================
// Battle stages for every map + battle music
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    // ================================================================================
    // BATTLE STAGES: every map has its own. Change `length` to make a stage longer or shorter
    // (240 = one screen, anything longer scrolls: the fighters can walk as far as the stage goes).
    // Platforms: { left edge, height of the surface, width }, width must be a multiple of 32.
    // Backgrounds: up to 3 layers, farthest first, { picture, speed % }: 25 = slow and far, 100 = keeps up with the camera.
    // ================================================================================

    constexpr battle::Platform world_platforms[]  = { { 96, 88, 64 }, { 416, 88, 96 } };
    constexpr battle::Platform world2_platforms[] = { { 160, 84, 96 } };
    constexpr battle::Platform house_platforms[]  = { { 80, 120, 64 } };

    constexpr battle::StageDef world_stage =
    {
        { 600, 128, world_platforms, 2, 0, 0 },
        { { &bn::regular_bg_items::battle_backdrop_16, 50 }, { &bn::regular_bg_items::layer_4_forest, 100, true } }, 2,
        nullptr, nullptr,                                  // floor/platform art: default grass + wood
        &bn::affine_bg_items::battle_top_grass,            // the 3D top
    };
    constexpr battle::StageDef world2_stage =
    {
        { 480, 128, world2_platforms, 1, 0, 0 },
        { { &bn::regular_bg_items::layer_2_mountains, 25 }, { &bn::regular_bg_items::layer_3_hills, 60 } }, 2,
        &bn::sprite_items::battle_floor_sand, &bn::sprite_items::battle_platform_sand,
    };
    constexpr battle::StageDef house_stage =
    {
        { 300, 128, house_platforms, 1, 0, 0 },
        {{ &bn::regular_bg_items::house_bg, 50}}, 1,
        &bn::sprite_items::battle_floor_house, &bn::sprite_items::battle_platform_house,
        &bn::affine_bg_items::battle_top_wood,
    };
    constexpr battle::StageDef world3_stage =
    {
        { 240, 128, house_platforms, 1, 0, 0},
        { { &bn::regular_bg_items::layer_2_mountains, 25 }, { &bn::regular_bg_items::layer_3_hills, 50 }, { &bn::regular_bg_items::layer_4_forest, 100 } }, 3,
    };

    const battle::StageDef& map_stage(int id)
    {
        switch(id)
        {
            case MAP_HOUSE:  return house_stage;
            case MAP_WORLD2: return world2_stage;
            case MAP_WORLD3: return world3_stage;
            default:         return world_stage;
        }
    }

    // the music of a battle on each map (until you have battle songs, it keeps the map's own)
    const MusicTrack& battle_music(int id)
    {
        switch(id)
        {
            case MAP_WORLD: return battle_track;
            default: return battle_track;        // later: return battle_track;
        }
    }
}

#endif
