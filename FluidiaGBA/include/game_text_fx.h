#ifndef GAME_TEXT_FX_H
#define GAME_TEXT_FX_H

// ================================================================================================
// enlarge(): makes sprite text bigger
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    void enlarge(bn::ivector<bn::sprite_ptr>& sprites, bn::fixed center_y, bn::fixed scale) // this is a pre-built function to enlarge da text
    {
        bn::sprite_affine_mat_ptr mat = bn::sprite_affine_mat_ptr::create();
        mat.set_scale(scale);

        for(bn::sprite_ptr& s : sprites)
        {
            s.set_affine_mat(mat);                                          // one shared transform
            // s.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);   // stops the edges getting clipped
            s.set_x(s.x() * scale);                                         // spread the letters out
            s.set_y(center_y + (s.y() - center_y) * scale);
        }
    }
}

#endif
