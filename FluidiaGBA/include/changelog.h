#ifndef CHANGELOG_H
#define CHANGELOG_H

// ================================================================================================
// THE CHANGELOG APP: edit this list any time. One string = one entry, newest first.
// Long entries are wrapped onto several lines for you. Just remember to put a comma after each one.
// (These are placeholders: replace them with your own notes.)
// ================================================================================================

namespace phone
{
    constexpr const char* changelog_entries[] =
    {
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
