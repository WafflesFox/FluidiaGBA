#ifndef CHANGELOG_H
#define CHANGELOG_H

namespace phone
{
    constexpr const char* changelog_entries[] =
    {
        "0.4.4p: Added 3D environment for battle scenes.",
        "0.4.2p: Almost Finshed adding Filip as a fighter.",
        "0.4.1p: Working on Filip as a fighter.",
        "0.4.0p: Finished Waffles as fighter.",
        "0.3.9p: Added more sprites and moveset to Waffles.",
        "0.3.8p: Fixed the Background music getting fucked up when three audio plays at once.",
        "0.3.7p: Added a third map to test the Map specific Battle field",
        "0.3.6p: Added a simple Battle System",
        "0.3.4p: Added a stop watch app",
        "0.3.1p: Added the phone (press L): calculator, flappy bird and this changelog.",
        "0.3.0p: Added traits to the NPC's that changes how they behave.",
        "0.2.9: NPCs can follow you, ask you questions and remember where they stand.",
        "0.2.0: House interior, a second world and the sliding screen transition.",
        "0.1.0: Title screen with parallax mountains and music.",
    };

    constexpr int changelog_count = sizeof(changelog_entries) / sizeof(changelog_entries[0]);
}

#endif
