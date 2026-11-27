#ifndef GAME_COMMON_H
#define GAME_COMMON_H

// ================================================================================================
// Shared basics: the walk cycle, directions, first_frame(), clamp_fixed()
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    // constexpr int music_chunks = 5;   // how many chunks
    constexpr int walk_cycle[4] = {0,1,0,2}; // the animation >w<
    enum Dir {UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3};

    int first_frame(int dir, bool running)
    {
        return (running ? 12 : 0) + dir * 3;
    }

    bn::fixed clamp_fixed(bn::fixed v, bn::fixed lo, bn::fixed hi)
    {
        if(v < lo) { return lo; }
        if(v > hi) { return hi; }
        return v;
    }
}

#endif
