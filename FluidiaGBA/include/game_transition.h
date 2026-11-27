#ifndef GAME_TRANSITION_H
#define GAME_TRANSITION_H

// ================================================================================================
// Screen transition: the black blocks that slide across the screen
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    // ================================================================================
    // SCREEN TRANSITION: a black block slides across the screen
    // ================================================================================

    // The block is a 4x3 grid of 64x64 black sprites = 256x192 pixels, a bit bigger than the 240x160 screen.
    constexpr int TRANSITION_SPEED = 10;   // pixels per frame (higher = faster)
    constexpr int BLOCK_COLS = 4;
    constexpr int BLOCK_ROWS = 3;

    // how far the block's center starts from the screen's center, so that it begins just off-screen
    int transition_distance(int dir)
    {
        return (dir == LEFT || dir == RIGHT) ? 248 : 176;     // (240 + 256) / 2   and   (160 + 192) / 2
    }

    void make_blocks(bn::vector<bn::sprite_ptr, 12>& blocks)
    {
        for(int row = 0; row < BLOCK_ROWS; ++row)
        {
            for(int col = 0; col < BLOCK_COLS; ++col)
            {
                bn::sprite_ptr s = bn::sprite_items::black_block.create_sprite(
                        bn::fixed(-96 + col * 64), bn::fixed(-64 + row * 64));
                s.set_bg_priority(0);        // in front of the backgrounds...
                s.set_z_order(-32000);       // ...and in front of every other sprite (lowest z order is drawn last)
                blocks.push_back(s);
            }
        }
    }

    // trans_pos is where the block is along its travel axis: it counts down from +distance to -distance,
    // and 0 means the block covers the whole screen. The facing direction decides which side it comes from:
    // facing RIGHT it enters from the right and slides left, facing LEFT it comes from the left, and so on.
    void move_blocks(bn::vector<bn::sprite_ptr, 12>& blocks, int dir, bn::fixed trans_pos)
    {
        bn::fixed off_x = 0;
        bn::fixed off_y = 0;
        switch(dir)
        {
            case RIGHT: off_x = trans_pos;  break;
            case LEFT:  off_x = -trans_pos; break;
            case DOWN:  off_y = trans_pos;  break;
            default:    off_y = -trans_pos; break;     // UP
        }

        for(int i = 0; i < blocks.size(); ++i)
        {
            const int col = i % BLOCK_COLS;
            const int row = i / BLOCK_COLS;
            blocks[i].set_position(bn::fixed(-96 + col * 64) + off_x, bn::fixed(-64 + row * 64) + off_y);
        }
    }
}

#endif
