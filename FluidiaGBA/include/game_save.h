// game_save.h  (include it from main.cpp after game_npc.h, before game_play.h)
#include "bn_sram.h"

namespace
{
    constexpr uint32_t SAVE_MAGIC   = 0x464C5544;   // "FLUD": tells us SRAM holds OUR data
    constexpr uint32_t SAVE_VERSION = 1;            // bump it when you change SaveData

    struct SaveData
    {
        uint32_t magic;
        uint32_t version;
        int map;
        int player_x, player_y;
        NpcSave npcs[npc_count];       // where each NPC is, who follows you, dialogue progress
        uint32_t checksum;             // catches corrupted / half-written saves
    };

    uint32_t save_checksum(const SaveData& d)
    {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&d);
        uint32_t sum = 0;
        for(int i = 0; i < int(sizeof(SaveData) - sizeof(uint32_t)); ++i) { sum = sum * 31 + p[i]; }
        return sum;
    }

    void save_game(int map, int px, int py)
    {
        SaveData d = {};
        d.magic = SAVE_MAGIC;
        d.version = SAVE_VERSION;
        d.map = map;
        d.player_x = px;
        d.player_y = py;
        for(int i = 0; i < npc_count; ++i) { d.npcs[i] = npc_save[i]; }
        d.checksum = save_checksum(d);
        bn::sram::write(d);
    }

    bool load_game(int& map, int& px, int& py)     // false = no valid save (first boot)
    {
        SaveData d;
        bn::sram::read(d);
        if(d.magic != SAVE_MAGIC || d.version != SAVE_VERSION || d.checksum != save_checksum(d)) { return false; }
        map = d.map;
        px = d.player_x;
        py = d.player_y;
        for(int i = 0; i < npc_count; ++i) { npc_save[i] = d.npcs[i]; }
        return true;
    }
}
