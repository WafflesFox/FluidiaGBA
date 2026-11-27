#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

// ================================================================================================
// Music (chunked tracks + MusicPlayer) and sound effects (sfx)
//
// Part of main.cpp. Include it ONLY from main.cpp, after main.cpp's own #includes and in the order
// listed there: it uses everything main.cpp has declared before it.
// ================================================================================================

namespace
{
    constexpr int FRAME_US = 16743;   // one GBA frame is ~16.743 ms

    struct MusicTrack
    {
        const bn::sound_item* chunks;
        const int* chunk_us;            // length of each chunk in microseconds
        int count;
        bn::fixed volume;
    };

    template<int N>
    constexpr MusicTrack make_track(const bn::sound_item (&chunks)[N], const int (&lengths_us)[N], bn::fixed volume = 1)
    {
        return MusicTrack{ chunks, lengths_us, N, volume };
    }
    constexpr bn::sound_item title_chunks[] = { bn::sound_items::title_00, bn::sound_items::title_01, bn::sound_items::title_02, bn::sound_items::title_03, bn::sound_items::title_04 };
    constexpr bn::sound_item game_chunks[]  = { bn::sound_items::game_00, bn::sound_items::game_01, bn::sound_items::game_02, bn::sound_items::game_03, bn::sound_items::game_04 };
    constexpr bn::sound_item house_chunks[] = { bn::sound_items::house_00, bn::sound_items::house_01,bn::sound_items::house_02,bn::sound_items::house_03,bn::sound_items::house_04,bn::sound_items::house_05,bn::sound_items::house_06};
    constexpr bn::sound_item battle_chunks[]= { bn::sound_items::battle_00,bn::sound_items::battle_01,bn::sound_items::battle_02,bn::sound_items::battle_03,bn::sound_items::battle_04,bn::sound_items::battle_05};

    constexpr int title_us[] = { 18023764, 17998367, 17998367, 17998367, 15282358 };   // 5 chunks
    constexpr int game_us[] = { 18024807, 17998367, 17998367, 17998367, 15280090 };   // 5 chunks
    constexpr int house_us[] = { 18023500, 17998375, 17998312, 17998375, 17998375, 17998375, 6688375 };   // 7 chunks
    constexpr int battle_us[] = { 18023937, 18000000, 18000000, 18000000, 18000000, 2665750 };   // 6 chunks
    constexpr MusicTrack game_track = make_track(game_chunks, game_us);
    constexpr MusicTrack title_track = make_track(title_chunks, title_us);
    constexpr MusicTrack house_track = make_track(house_chunks, house_us);
    constexpr MusicTrack battle_track = make_track(battle_chunks, battle_us);

    struct MusicPlayer
    {
        const MusicTrack* track = nullptr;
        int chunk = 0;
        int elapsed_us = 0;
        bn::optional<bn::sound_handle> handle;

        void start_chunk(int carry_us)
        {
            handle = bn::sound::play_with_priority(100, track->chunks[chunk], track->volume);
            elapsed_us = carry_us;      // keep the leftover so rounding never accumulates into drift
        }

        void play(const MusicTrack& new_track)
        {
            if(track == &new_track) { return; }
            stop();
            track = &new_track;
            chunk = 0;
            start_chunk(0);
        }

        void update()                   // once per frame, same as before
        {
            if(!track) { return; }

            elapsed_us += FRAME_US;
            if(elapsed_us >= track->chunk_us[chunk])
            {
                const int carry = elapsed_us - track->chunk_us[chunk];
                chunk = (chunk + 1) % track->count;
                start_chunk(carry);
            }
        }

        void stop()
        {
            if(handle) { handle->stop(); handle.reset(); }
            track = nullptr;
        }
    };
    void sfx(int id){
        switch (id){
            case 1:
                bn::sound_items::pick.play();
                break;
            case 2:
                bn::sound_items::pick2.play();
                break;
            case 3:
                bn::sound_items::step.play();
                break;
            case 4:
                bn::sound_items::punch.play();
                break;
            case 5:
                bn::sound_items::punch_s.play();
                break;
            case 6:
                bn::sound_items::punch_h.play();
                break;
            case 7:
                bn::sound_items::swing.play();
                break;
            case 8:
                bn::sound_items::shieldhit.play();
                break;
            case 9:
                bn::sound_items::nar_go.play();
                break;
            case 10:
                bn::sound_items::gay.play();
                break;
            case 11:
                bn::sound_items::critical.play();
                break;
            case 12:
                bn::sound_items::shieldon.play();
                break;
            case 13:
                bn::sound_items::slam.play();
                break;
            case 14:
                bn::sound_items::slam2.play();
                break;
            case 15:
                bn::sound_items::yoink.play();
                break;
            case 16:
                bn::sound_items::grab.play();
                break;
            case 17:
                bn::sound_items::mining.play();
                break;
            case 18:
                bn::sound_items::final_smash.play();
                break;
            default:
                bn::sound_items::meow.play();
                break;
        }
    }
}

#endif
