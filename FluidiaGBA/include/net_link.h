#ifndef NET_LINK_H
#define NET_LINK_H

#include "bn_link.h"
#include "bn_link_state.h"
#include "bn_link_player.h"
#include "bn_optional.h"
#include "battle_logic.h"

namespace net
{
    // 15-bit safe Link packets (0x1000 .. 0x7FFF)
    // Overworld uses types 1..4 & 6; active Battle uses types 3..7 (gated by LinkSession::in_battle)
    enum PacketType
    {
        PKT_OW_X          = 1,  // payload: world x in pixels (0..4095)
        PKT_OW_Y          = 2,  // payload: world y in pixels (0..4095)
        PKT_OW_META       = 3,  // (out of battle) bits 0..2: map, 3..4: dir, 5: run, 6: move, 7..9: invite
        PKT_BTL_SEED      = 4,  // (out of battle) bits 0..2: stage_map, bits 3..11: seed
        PKT_BTL_SYNC_POS  = 3,  // (in battle) bits 10..11: sub (0:P0x, 1:P0y+256, 2:P1x, 3:P1y+256)
        PKT_BTL_SYNC_STAT = 4,  // (in battle) bits 10..11: sub (0:P0hp, 1:P1hp, 2:P0smash, 3:P1smash)
        PKT_BTL_INPUT     = 5,  // bits 0..1: move, 2:up, 3:dn, 4:shld, 5..8: edge toggles (jmp,atk,spc,fs)
        PKT_BTL_READY     = 6,  // payload: 1 when ready
        PKT_BTL_EVENT     = 7,  // bits 8..11: kind (1=FinalSmash, 2=Winner)
    };

    enum InviteState
    {
        INV_NONE    = 0,
        INV_REQUEST = 1,
        INV_ACCEPT  = 2,
        INV_DECLINE = 3,
        INV_BUSY    = 4,
    };

    struct LinkSession
    {
        bool connected = false;
        bool just_connected = false;
        bool in_battle = false;
        int my_id = 0;
        int disconnect_timer = 0;
        int send_tick = 0;

        // Remote overworld state
        int remote_x = 0;
        int remote_y = 0;
        int remote_map = 0;
        int remote_dir = 1;
        bool remote_running = false;
        bool remote_moving = false;
        int remote_invite = INV_NONE;
        bool has_remote_pos = false;

        // Local invite state
        int local_invite = INV_NONE;

        // Battle handshake
        bool got_seed = false;
        int synced_map = 0;
        unsigned synced_seed = 1;
        bool peer_ready = false;

        // Edge-toggled battle input (never drops or double-fires jumps/attacks!)
        bool loc_jmp_tog = false;
        bool loc_atk_tog = false;
        bool loc_spc_tog = false;
        bool loc_fs_tog  = false;

        bool rem_jmp_tog = false;
        bool rem_atk_tog = false;
        bool rem_spc_tog = false;
        bool rem_fs_tog  = false;

        bool pend_jmp = false;
        bool pend_atk = false;
        bool pend_spc = false;
        bool pend_fs  = false;
        battle::Input rem_held = {};

        // Host -> Client periodic state snapshot
        bool has_sync_x[2]     = {};
        bool has_sync_y[2]     = {};
        bool has_sync_hp[2]    = {};
        bool has_sync_smash[2] = {};
        int sync_x[2]          = {};
        int sync_y[2]          = {};
        int sync_hp[2]         = {};
        int sync_smash[2]      = {};

        // Authoritative battle events from Host
        bool got_fs_event = false;
        int ev_fs_atk = 0;
        int ev_fs_vic = 1;
        int ev_fs_id  = 0;
        int last_rx_fs_seq = -1;

        bool got_win_event = false;
        int ev_winner = -1;

        static int clamp12(int v)
        {
            if(v < 0) { return 0; }
            if(v > 4095) { return 4095; }
            return v;
        }

        static int clamp10(int v)
        {
            if(v < 0) { return 0; }
            if(v > 1023) { return 1023; }
            return v;
        }

        void record_local_input(const battle::Input& cur)
        {
            if(cur.jump)        { loc_jmp_tog = !loc_jmp_tog; }
            if(cur.attack)      { loc_atk_tog = !loc_atk_tog; }
            if(cur.special)     { loc_spc_tog = !loc_spc_tog; }
            if(cur.final_smash) { loc_fs_tog  = !loc_fs_tog;  }
        }

        int pack_local_input(const battle::Input& cur) const
        {
            int bits = 0;
            if(cur.move < 0)    { bits |= 1; }
            else if(cur.move > 0) { bits |= 2; }
            if(cur.up)          { bits |= 1 << 2; }
            if(cur.down)        { bits |= 1 << 3; }
            if(cur.shield)      { bits |= 1 << 4; }
            if(loc_jmp_tog)     { bits |= 1 << 5; }
            if(loc_atk_tog)     { bits |= 1 << 6; }
            if(loc_spc_tog)     { bits |= 1 << 7; }
            if(loc_fs_tog)      { bits |= 1 << 8; }
            return (PKT_BTL_INPUT << 12) | bits;
        }

        battle::Input consume_remote_input()
        {
            battle::Input out = rem_held;
            out.jump        = pend_jmp; pend_jmp = false;
            out.attack      = pend_atk; pend_atk = false;
            out.special     = pend_spc; pend_spc = false;
            out.final_smash = pend_fs;  pend_fs  = false;
            return out;
        }

        void handle_packet(int pkt)
        {
            const int type = (pkt >> 12) & 7;
            const int payload = pkt & 0x0FFF;

            if(!in_battle)
            {
                switch(type)
                {
                    case PKT_OW_X:
                        remote_x = payload;
                        break;
                    case PKT_OW_Y:
                        remote_y = payload;
                        has_remote_pos = true;
                        break;
                    case PKT_OW_META:
                        remote_map = payload & 7;
                        remote_dir = (payload >> 3) & 3;
                        remote_running = ((payload >> 5) & 1) != 0;
                        remote_moving  = ((payload >> 6) & 1) != 0;
                        remote_invite  = (payload >> 7) & 7;
                        break;
                    case PKT_BTL_SEED:
                        synced_map = payload & 7;
                        synced_seed = unsigned((payload >> 3) & 0x1FF);
                        if(synced_seed == 0) { synced_seed = 1; }
                        got_seed = true;
                        break;
                    case PKT_BTL_READY:
                        peer_ready = true;
                        break;
                    default:
                        break;
                }
                return;
            }

            switch(type)
            {
                case PKT_BTL_INPUT:
                {
                    const int mv = payload & 3;
                    rem_held.move   = (mv == 1) ? -1 : (mv == 2 ? 1 : 0);
                    rem_held.up     = (payload & (1 << 2)) != 0;
                    rem_held.down   = (payload & (1 << 3)) != 0;
                    rem_held.shield = (payload & (1 << 4)) != 0;

                    const bool j = (payload & (1 << 5)) != 0;
                    const bool a = (payload & (1 << 6)) != 0;
                    const bool s = (payload & (1 << 7)) != 0;
                    const bool f = (payload & (1 << 8)) != 0;
                    if(j != rem_jmp_tog) { pend_jmp = true; rem_jmp_tog = j; }
                    if(a != rem_atk_tog) { pend_atk = true; rem_atk_tog = a; }
                    if(s != rem_spc_tog) { pend_spc = true; rem_spc_tog = s; }
                    if(f != rem_fs_tog)  { pend_fs  = true; rem_fs_tog  = f; }
                    break;
                }
                case PKT_BTL_READY:
                    peer_ready = true;
                    break;
                case PKT_BTL_SYNC_POS:
                {
                    const int sub = (payload >> 10) & 3;
                    const int val = payload & 0x3FF;
                    if(sub == 0)      { sync_x[0] = val;       has_sync_x[0] = true; }
                    else if(sub == 1) { sync_y[0] = val - 256; has_sync_y[0] = true; }
                    else if(sub == 2) { sync_x[1] = val;       has_sync_x[1] = true; }
                    else              { sync_y[1] = val - 256; has_sync_y[1] = true; }
                    break;
                }
                case PKT_BTL_SYNC_STAT:
                {
                    const int sub = (payload >> 10) & 3;
                    const int val = payload & 0x3FF;
                    if(sub == 0)      { sync_hp[0]    = val; has_sync_hp[0]    = true; }
                    else if(sub == 1) { sync_hp[1]    = val; has_sync_hp[1]    = true; }
                    else if(sub == 2) { sync_smash[0] = val; has_sync_smash[0] = true; }
                    else              { sync_smash[1] = val; has_sync_smash[1] = true; }
                    break;
                }
                case PKT_BTL_EVENT:
                {
                    const int kind = (payload >> 8) & 15;
                    if(kind == 1)
                    {
                        const int seq = (payload >> 6) & 3;
                        if(seq != last_rx_fs_seq)
                        {
                            last_rx_fs_seq = seq;
                            ev_fs_atk = payload & 3;
                            ev_fs_vic = (payload >> 2) & 3;
                            ev_fs_id  = (payload >> 4) & 3;
                            got_fs_event = true;
                        }
                    }
                    else if(kind == 2)
                    {
                        ev_winner = payload & 3;
                        got_win_event = true;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        void poll()
        {
            just_connected = false;
            bn::optional<bn::link_state> state = bn::link::receive();
            if(state)
            {
                my_id = state->current_player_id();
                if(state->player_count() >= 2)
                {
                    disconnect_timer = 0;
                    if(!connected)
                    {
                        connected = true;
                        just_connected = true;
                    }
                }
                for(const bn::link_player& msg : state->other_players())
                {
                    if(msg.id() != my_id)
                    {
                        disconnect_timer = 0;
                        if(!connected)
                        {
                            connected = true;
                            just_connected = true;
                        }
                        handle_packet(msg.data());
                    }
                }
            }
            else if(connected)
            {
                if(++disconnect_timer >= 180)
                {
                    connected = false;
                    has_remote_pos = false;
                    remote_invite = INV_NONE;
                }
            }
        }

        void update_overworld(int x, int y, int map_id, int dir, bool running, bool moving)
        {
            in_battle = false;
            ++send_tick;
            if(!connected)
            {
                if(send_tick % 10 == 0)
                {
                    const int meta = (map_id & 7) | ((dir & 3) << 3) | ((local_invite & 7) << 7);
                    bn::link::send((PKT_OW_META << 12) | meta);
                }
            }
            else
            {
                switch(send_tick % 3)
                {
                    case 0:
                        bn::link::send((PKT_OW_X << 12) | clamp12(x));
                        break;
                    case 1:
                        bn::link::send((PKT_OW_Y << 12) | clamp12(y));
                        break;
                    default:
                    {
                        const int meta = (map_id & 7) | ((dir & 3) << 3) | (running ? (1 << 5) : 0) |
                                         (moving ? (1 << 6) : 0) | ((local_invite & 7) << 7);
                        bn::link::send((PKT_OW_META << 12) | meta);
                        break;
                    }
                }
            }
            poll();
        }

        void reset_battle_sync()
        {
            got_seed = false;
            peer_ready = false;
            loc_jmp_tog = loc_atk_tog = loc_spc_tog = loc_fs_tog = false;
            rem_jmp_tog = rem_atk_tog = rem_spc_tog = rem_fs_tog = false;
            pend_jmp = pend_atk = pend_spc = pend_fs = false;
            rem_held = battle::Input();
            for(int i = 0; i < 2; ++i)
            {
                has_sync_x[i] = has_sync_y[i] = has_sync_hp[i] = has_sync_smash[i] = false;
            }
            got_fs_event = false;
            last_rx_fs_seq = -1;
            got_win_event = false;
            ev_winner = -1;
        }

        void clear_cutscene_pending()
        {
            got_fs_event = false;
            pend_jmp = pend_atk = pend_spc = pend_fs = false;
            rem_held = battle::Input();
            for(int i = 0; i < 2; ++i)
            {
                has_sync_x[i] = has_sync_y[i] = has_sync_hp[i] = has_sync_smash[i] = false;
            }
        }
    };
}

#endif
