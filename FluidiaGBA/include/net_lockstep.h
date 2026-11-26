#ifndef NET_LOCKSTEP_H
#define NET_LOCKSTEP_H

// Deterministic lockstep for 2-player Link battles.
//
// Both consoles simulate EVERYTHING themselves and only exchange button states. A sim frame is run only when the
// button states of both players for that frame are known, so both consoles always feed the exact same inputs into
// the exact same simulation. No position/HP/clone snapshots are needed.
//
//  * Inputs are delayed by DELAY frames (input sampled now is applied DELAY sim frames later).
//  * One 16-bit packet per video frame:   1 aa tttt hhhhhhhhh
//        bit 15      : always 1 (tells it apart from the old 0x1000..0x7FFF typed packets)
//        bits 13..14 : ack  = (my rx_next) & 3     -> "I have all your frames below this one"
//        bits  9..12 : tag  = (frame being sent) & 15
//        bits  0..8  : held buttons for that frame
//  * The link can lose messages (Butano docs say so), so while stalled the sender cycles through every frame the
//    peer has not acknowledged yet. Duplicates are harmless.
//  * Every 64 sim frames a hash of the simulation is exchanged to DETECT desyncs (it never "fixes" anything).
//
// This header has no Butano dependency on purpose (see lockstep_test.cpp).

namespace net
{
    struct Lockstep
    {
        enum { DELAY = 2, RING = 8, STALL_LIMIT = 600, CHECK_EVERY = 64 };

        enum : unsigned
        {
            H_LEFT = 1, H_RIGHT = 2, H_UP = 4, H_DOWN = 8, H_SHIELD = 16,
            H_JUMP = 32, H_ATTACK = 64, H_SPECIAL = 128, H_FS = 256,
            H_BUTTONS = H_JUMP | H_ATTACK | H_SPECIAL | H_FS
        };

        unsigned sim = 0;          // next sim frame to run
        unsigned sched = 0;        // next local frame to schedule
        unsigned rx_next = 0;      // lowest remote frame still missing
        unsigned peer_rx = 0;      // (estimate of) the peer's rx_next: it has every local frame below this
        unsigned loc[RING] = {};   // local held buttons by frame % RING
        unsigned rem[RING] = {};   // remote held buttons by frame % RING
        bool     have[RING] = {};  // rem[] slot holds a frame that has not been simulated yet
        unsigned prev[2] = {};     // held buttons of the last simulated frame, per player
        unsigned latch = 0;        // button presses seen since the last scheduled frame (so quick taps survive a stall)
        unsigned resend = 0;
        bool     stalled = false;
        int      stall = 0;

        // desync detection
        unsigned our_hash[4] = {};
        bool     our_valid[4] = {};
        unsigned their_hash[4] = {};
        bool     their_valid[4] = {};
        bool     desync = false;
        unsigned desync_frame = 0;

        void start()
        {
            sim = 0;
            sched = DELAY;
            rx_next = DELAY;
            peer_rx = DELAY;
            for(int i = 0; i < RING; ++i) { loc[i] = rem[i] = 0; have[i] = false; }
            for(int i = 0; i < DELAY; ++i) { have[i] = true; }   // frames 0..DELAY-1: nobody pressed anything yet
            prev[0] = prev[1] = 0;
            latch = 0;
            resend = 0;
            stalled = false;
            stall = 0;
            for(int i = 0; i < 4; ++i) { our_valid[i] = their_valid[i] = false; }
            desync = false;
            desync_frame = 0;
        }

        // never produce contradicting directions (also keeps every packet below the reserved 0xFFFE/0xFFFF values)
        static unsigned normalize(unsigned h)
        {
            if((h & (H_LEFT | H_RIGHT)) == (H_LEFT | H_RIGHT)) { h &= ~(H_LEFT | H_RIGHT);}
            if((h & (H_UP | H_DOWN)) == (H_UP | H_DOWN))       { h &= ~unsigned(H_DOWN); }
            return h & 0x1FF;
        }

        // call once per video frame with the buttons held right now
        void schedule(unsigned held_now)
        {
            latch |= held_now & H_BUTTONS;
            if(sched == sim + DELAY)
            {
                loc[sched % RING] = normalize((held_now & ~unsigned(H_BUTTONS)) | latch);
                ++sched;
                latch = 0;
            }
        }

        // The packet to send this video frame. `idle` = we are not simulating right now (cutscene, result screen):
        // keep re-sending what the peer may still be missing.
        int next_packet(bool idle = false)
        {
            const unsigned newest = sched - 1;
            unsigned f = newest;
            if(stalled || idle)
            {
                // The peer can never need anything older than newest-(2*DELAY+1): we can run at most DELAY+1 frames
                // ahead of its sim, and our newest frame is DELAY ahead of our own sim. The (possibly stale) ack may
                // only narrow that range, never widen it, so a ring slot can never hold the wrong frame.
                const unsigned back = 2u * DELAY + 1u;
                unsigned lo = (newest > back) ? newest - back : 0u;
                if(peer_rx > lo) { lo = peer_rx; }
                if(lo <= newest)
                {
                    // The peer's rx_next IS the frame it is blocked on, so `lo` is almost always the one it needs:
                    // send that 3 times out of 4 and sweep the rest of the range on the 4th, in case the ack is stale.
                    const unsigned n = resend++;
                    f = ((n & 3u) != 3u) ? lo : lo + ((n >> 2) % (newest - lo + 1));
                }
            }
            return int(0x8000u | ((rx_next & 3u) << 13) | ((f & 15u) << 9) | (loc[f % RING] & 0x1FFu));
        }

        void on_packet(int pkt)
        {
            const unsigned p = unsigned(pkt);
            const unsigned ack = (p >> 13) & 3u;
            const unsigned tag = (p >> 9) & 15u;

            // the peer's frame
            const unsigned d = (tag - rx_next) & 15u;
            if(d <= 5u && rx_next + d < sim + RING)
            {
                const unsigned f = rx_next + d;
                if(!have[f % RING])
                {
                    rem[f % RING] = p & 0x1FFu;
                    have[f % RING] = true;
                }
                for(int guard = 0; guard < RING && have[rx_next % RING]; ++guard) { ++rx_next; }
            }

            // what the peer has of ours (only ever moves forward, and never past what we actually sent)
            const unsigned pd = (ack - peer_rx) & 3u;
            if(peer_rx + pd <= sched) { peer_rx += pd; }
        }

        // Runs one sim frame if both inputs are known. held[] / before[] are indexed by PLAYER (0 = host).
        bool try_step(int local_idx, unsigned held[2], unsigned before[2])
        {
            if(rx_next <= sim)
            {
                stalled = true;
                ++stall;
                return false;
            }
            stalled = false;
            stall = 0;
            held[local_idx] = loc[sim % RING];
            held[1 - local_idx] = rem[sim % RING];
            before[0] = prev[0];
            before[1] = prev[1];
            prev[0] = held[0];
            prev[1] = held[1];
            have[sim % RING] = false;
            ++sim;
            return true;
        }

        bool timed_out() const { return stall > STALL_LIMIT; }

        // ---- desync detection --------------------------------------------------------------------------------
        bool is_checkpoint() const { return sim > 0 && (sim % CHECK_EVERY) == 0; }

        // payload for a PKT_BTL_HASH packet (type nibble is added by the caller)
        int record_hash(unsigned h)
        {
            const unsigned v = (h ^ (h >> 10) ^ (h >> 20)) & 0x3FFu;
            const unsigned c = (sim / CHECK_EVERY) & 3u;
            for(unsigned i = 0; i < 4; ++i)
            {
                our_valid[i] = false;
                if(i != c) { their_valid[i] = false; }
            }
            our_hash[c] = v;
            our_valid[c] = true;
            if(their_valid[c] && their_hash[c] != v) { desync = true; desync_frame = sim; }
            their_valid[c] = false;
            return int((c << 10) | v);
        }

        void on_hash(int payload)
        {
            const unsigned c = (unsigned(payload) >> 10) & 3u;
            const unsigned v = unsigned(payload) & 0x3FFu;
            if(our_valid[c])
            {
                if(our_hash[c] != v) { desync = true; desync_frame = sim; }
            }
            else
            {
                their_hash[c] = v;
                their_valid[c] = true;
            }
        }
    };
}

#endif
