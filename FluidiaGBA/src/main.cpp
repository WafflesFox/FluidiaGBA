#define GAME_VER "0.5.2a"
#include "bn_core.h"
#include "bn_color.h"
#include "bn_bg_palettes.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "bn_string.h"
#include "common_variable_8x16_sprite_font.h"
#include "bn_sprite_items_player.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_regular_bg_ptr.h"
// Battle BG
#include "bn_regular_bg_items_layer_2_mountains.h"
#include "bn_regular_bg_items_layer_3_hills.h"
#include "bn_regular_bg_items_layer_4_forest.h"
#include "bn_regular_bg_items_beta_house_bg.h"
#include "bn_regular_bg_items_template_bg.h"

#include "bn_sound_items.h"
#include "bn_sound_item.h"
#include "bn_sound_handle.h"
#include "bn_sound.h"

//characters
#include "bn_sprite_items_waffle.h"
#include "bn_sprite_items_snep.h"
#include "bn_sprite_items_umbreon.h"
#include "bn_sprite_items_harue.h"
#include "bn_sprite_items_harueslice_sword.h"

#include "bn_sprite_items_black_block.h"   // solid black 64x64, used by the screen transition
#include "phone.h"
#include "battle.h"
#include "battle_moves.h"
#include "net_link.h"
#include "bn_sprite_palette_ptr.h"

// the maps: ground picture, the layer drawn in front of the player, and the collision grids
#include "bn_regular_bg_items_world.h"
#include "bn_regular_bg_items_world_top.h"
#include "bn_regular_bg_items_world2.h"
#include "bn_regular_bg_items_house_interior.h"
#include "bn_regular_bg_items_demo.h"
#include "bn_regular_bg_items_demo_top.h"
#include "world_map.h"              // namespace world
#include "world2_map.h"             // namespace world2
#include "house_interior_map.h"     // namespace house_interior
#include "demo_map.h"
#include "bn_sprite_items_battle_floor_house.h"
#include "bn_sprite_items_battle_platform_house.h"
#include "bn_sprite_items_battle_floor_sand.h"
#include "bn_sprite_items_battle_platform_sand.h"
#include "bn_affine_bg_items_battle_top_grass.h"
#include "bn_affine_bg_items_battle_top_wood.h"
#include "bn_affine_bg_items_battle_top_wood_wide.h"
#include "bn_regular_bg_items_battle_backdrop_16.h"

#include "game_common.h"
#include "game_debug_hud.h"
#include "game_audio.h"
#include "game_text_fx.h"
#include "game_maps.h"
#include "game_stages.h"
#include "game_map_gfx.h"
#include "game_transition.h"
#include "game_npc_data.h"
#include "game_fighters.h"
#include "game_npc.h"
#include "game_save.h"
#include "game_npc_ai.h"
#include "game_dialogue.h"
#include "game_title.h"
#include "game_char_select.h"
#include "game_play.h"

int main()
{
    bn::core::init();

    // while(true){ debugging crap, because GBA music is weird
    //     if(bn::keypad::l_pressed()) { bn::sound_items::title_20.play(); }
    //     if(bn::keypad::b_pressed()) { bn::sound_items::title_25.play(); }
    //     if(bn::keypad::r_pressed()) { bn::sound_items::title.play(); }
    //     bn::core::update();
    // }
    // bn::sound_items::beep.play();
    bn::bg_palettes::set_transparent_color(bn::color(10, 20, 31));  // the main bg

    bn::sprite_text_generator text(common::variable_8x16_sprite_font);

    title_screen(text);
    game_screen(text);
}
