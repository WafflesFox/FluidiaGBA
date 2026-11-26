// g++ -std=c++17 -O2 -Iinclude lockstep_test.cpp -o lockstep_test && ./lockstep_test
#include "net_lockstep.h"
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <random>
#include <vector>

using net::Lockstep;

struct Peer
{
    Lockstep ls;
    int idx;
    std::deque<int> inbox;                    // hardware receive queue
    std::vector<unsigned> truth;              // what this peer scheduled, by frame
    std::vector<unsigned> stepped_local, stepped_remote;
    int pause_left = 0;                       // simulates a cutscene: pumping but not stepping
};

struct Config { double loss; double lag; double pause; int frames; unsigned seed; };

static bool run(const Config& c)
{
    std::mt19937 rng(c.seed);
    auto chance = [&](double p) { return std::uniform_real_distribution<double>(0, 1)(rng) < p; };

    Peer p[2];
    for(int i = 0; i < 2; ++i) { p[i].idx = i; p[i].ls.start(); p[i].truth.assign(100000, 0); }
    // frames 0..DELAY-1 are neutral for both
    std::vector<int> outbox[2];

    long video = 0;
    while((p[0].ls.sim < unsigned(c.frames) || p[1].ls.sim < unsigned(c.frames)) && video < c.frames * 40L)
    {
        ++video;
        for(int i = 0; i < 2; ++i)
        {
            Peer& me = p[i];
            Peer& other = p[1 - i];
            if(chance(c.lag)) { continue; }           // this console dropped a whole video frame (lag)

            // random button mashing, with plenty of contradictory directions to exercise normalize()
            const unsigned held_now = unsigned(rng() & 0x1FF);

            const bool in_cutscene = me.pause_left > 0;
            if(in_cutscene)
            {
                --me.pause_left;
            }
            else
            {
                const unsigned f_before = me.ls.sched;
                me.ls.schedule(held_now);
                if(me.ls.sched != f_before) { me.truth[f_before] = me.ls.loc[f_before % Lockstep::RING]; }
            }

            // send (maybe lost). The hardware queue holds at most 8 messages, oldest dropped.
            const int pkt = me.ls.next_packet(in_cutscene);
            if(pkt < 0 || pkt > 65533) { std::printf("BAD PACKET VALUE %d\n", pkt); return false; }
            if(!chance(c.loss))
            {
                other.inbox.push_back(pkt);
                if(other.inbox.size() > 8) { other.inbox.pop_front(); }
            }

            // receive: up to 4 messages per video frame, like the Battle loop
            for(int k = 0; k < 4 && !me.inbox.empty(); ++k)
            {
                me.ls.on_packet(me.inbox.front());
                me.inbox.pop_front();
            }

            if(!in_cutscene)
            {
                unsigned held[2], before[2];
                if(me.ls.try_step(me.idx, held, before))
                {
                    me.stepped_local.push_back(held[me.idx]);
                    me.stepped_remote.push_back(held[1 - me.idx]);
                    if(chance(c.pause)) { me.pause_left = 30 + int(rng() % 120); }
                }
                if(me.ls.timed_out()) { std::printf("  TIMEOUT (peer %d) at sim %u\n", i, me.ls.sim); return false; }
            }
        }
    }

    // verify: what each side simulated for frame f must be exactly what each player scheduled for frame f
    const size_t n = std::min(p[0].stepped_local.size(), p[1].stepped_local.size());
    if(n < size_t(c.frames)) { std::printf("  only %zu/%d frames done (deadlock?)\n", n, c.frames); return false; }
    for(size_t f = 0; f < n; ++f)
    {
        const unsigned t0 = f < Lockstep::DELAY ? 0 : p[0].truth[f];
        const unsigned t1 = f < Lockstep::DELAY ? 0 : p[1].truth[f];
        const unsigned a0 = p[0].stepped_local[f], a1 = p[0].stepped_remote[f];
        const unsigned b1 = p[1].stepped_local[f], b0 = p[1].stepped_remote[f];
        if(a0 != t0 || a1 != t1 || b0 != t0 || b1 != t1)
        {
            std::printf("  MISMATCH at frame %zu: truth(%x,%x) A saw(%x,%x) B saw(%x,%x)\n", f, t0, t1, a0, a1, b0, b1);
            return false;
        }
    }
    std::printf("  ok: %zu frames identical on both sides, %ld video frames (%.2f video/sim)\n", n, video, double(video) / double(n));
    return true;
}

int main()
{
    const Config cases[] = {
        { 0.00, 0.00, 0.00, 5000, 1 },   // perfect cable
        { 0.00, 0.10, 0.00, 5000, 2 },   // lagging consoles
        { 0.05, 0.05, 0.00, 5000, 3 },   // 5% loss
        { 0.20, 0.10, 0.00, 5000, 4 },   // 20% loss
        { 0.40, 0.10, 0.00, 5000, 5 },   // brutal: 40% loss
        { 0.10, 0.05, 0.01, 5000, 6 },   // cutscene pauses (pumping, not stepping)
        { 0.30, 0.20, 0.02, 5000, 7 },   // everything at once
    };
    int bad = 0;
    for(const Config& c : cases)
    {
        std::printf("loss=%.0f%% lag=%.0f%% pause=%.0f%%\n", c.loss * 100, c.lag * 100, c.pause * 100);
        if(!run(c)) { ++bad; }
    }
    for(unsigned s = 100; s < 300; ++s)           // random sweep
    {
        const Config c = { (s % 7) * 0.06, (s % 5) * 0.05, (s % 3) * 0.01, 3000, s };
        if(!run(c)) { std::printf("  (sweep seed %u failed)\n", s); ++bad; }
    }
    std::printf(bad ? "FAILED %d case(s)\n" : "ALL PASSED\n", bad);
    return bad ? 1 : 0;
}
