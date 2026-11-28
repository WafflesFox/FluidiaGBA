#ifndef BATTLE_VIEW_H
#define BATTLE_VIEW_H

// FighterView: the sprites of one fighter on screen (character, shield bubble, health bar, name).

#include "bn_color.h"
#include "bn_optional.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_items_hp_bar.h"
#include "battle_logic.h"
#include "battle_types.h"
#include "battle_anim.h"

namespace battle
{
    // ============================================================================================
    // ONE FIGHTER ON SCREEN: the character, the shield bubble, the health bar and the name
    // ============================================================================================

    struct FighterView
    {
        bn::sprite_ptr sprite;
        bn::optional<bn::sprite_ptr> shield;
        bn::sprite_affine_mat_ptr shield_mat;
        bn::sprite_ptr hp_back;
        bn::sprite_ptr hp_fill;
        bn::sprite_affine_mat_ptr hp_mat;
        bn::vector<bn::sprite_ptr, 12> name;
        bn::vector<bn::sprite_ptr, 5> hp_text;
        bn::optional<bn::sprite_ptr> swipes[2];     // slash pictures where melee hitboxes are active
        bn::optional<bn::sprite_ptr> jet;           // the aqua jet bubble
        bn::optional<bn::sprite_ptr> held;          // the shot fox thief is holding above the head
        bn::optional<bn::sprite_ptr> pig;           // the pig Umbreon rides
        bn::optional<bn::sprite_ptr> sword;
        bn::optional<bn::sprite_ptr> slash_fx;
        bn::optional<bn::sprite_affine_mat_ptr> sword_mat;
        bn::vector<bn::sprite_ptr, 12> craft_text;
        int shown_mat = -1;
        int shown_tier = -1;
        int shown_dur = -1;
        int held_kind = -1;
        const bn::sprite_item* sheet = nullptr;     // the sprite sheet this character was made from
        int shown_frame = -1;
        int step = 0;
        int anim_timer = 0;
        bn::optional<bn::sprite_palette_ptr> pal;
        int shown_hp = -1;
        bool on_right = false;                     // the opponent's HUD is on the right
        int hud_dy = 0;                            // the partner's name and health bar sit this many pixels lower than the player's
        int tint_mode = 0;                           // 0 = none, 1 = red, 2 = blue, 3 = water cyan
        bool is_clone = false;

        // `partner` = a computer-controlled friend on the player's side: tinted blue, HUD on a second row, drawn behind the player.
        FighterView(const FighterDef& def, bool opponent, const char* label, bn::sprite_text_generator& text, bool partner = false, int tint_override = -1, bool clone = false) :
            sprite(def.sprite->create_sprite(0, 0, pick_frame(ST_IDLE, opponent ? -1 : 1, 0))),   // made at its real first frame: tiles are only loaded once
            shield_mat(bn::sprite_affine_mat_ptr::create()),
            hp_back(bn::sprite_items::hp_bar.create_sprite(0, 0, 0)),
            hp_fill(bn::sprite_items::hp_bar.create_sprite(0, 0, 1)),
            hp_mat(bn::sprite_affine_mat_ptr::create()),
            on_right(opponent),
            is_clone(clone)
        {
            sheet = def.sprite;
            shown_frame = pick_frame(ST_IDLE, opponent ? -1 : 1, 0);
            sprite.set_bg_priority(0);
            sprite.set_z_order(clone ? 2 : (partner ? 1 : (opponent ? 0 : -1)));
            hud_dy = partner ? 26 : 0;
            
            tint_mode = (tint_override >= 0) ? tint_override : (opponent ? 1 : (partner ? 2 : 0));
            pal = bn::sprite_palette_ptr::create_new(def.sprite->palette_item());
            if(tint_mode == 1)      { pal->set_fade(bn::color(31, 6, 6), 0.35); }
            else if(tint_mode == 2) { pal->set_fade(bn::color(6, 12, 31), 0.35); }
            else if(tint_mode == 3) { tint_palette(*pal, bn::color(20, 4, 28)); }   // shadow clone: purple
            else if(tint_mode == 4) { tint_palette(*pal, bn::color(4, 22, 31)); }   // water clone: cyan
            sprite.set_palette(*pal);

            hp_back.set_bg_priority(0);
            hp_back.set_z_order(-10);
            hp_fill.set_bg_priority(0);
            hp_fill.set_z_order(-11);
            hp_fill.set_affine_mat(hp_mat);

            const int bar_center_x = opponent ? 96 : -96;             // the bars are 32 pixels wide
            hp_back.set_position(bar_center_x, -58 + hud_dy);
            hp_fill.set_position(bar_center_x, -58 + hud_dy);

            if(clone)
            {
                hp_back.set_visible(false);
                hp_fill.set_visible(false);
            }
            else if(opponent) { text.set_right_alignment(); text.generate(112, -72, label, name); }
            else              { text.set_left_alignment();  text.generate(-112, -72 + hud_dy, label, name); }
            text.set_center_alignment();

        }
    };
}

#endif
