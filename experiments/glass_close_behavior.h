#pragma once

namespace GlassClose {
// These values are persisted in experience.ini. Unknown values must ask rather
// than silently acquiring permission to exit after a future version change.
enum class Behavior : int { Ask = 0, Tray = 1, Exit = 2 };

constexpr Behavior FromStored(int value) {
    return value == 1 ? Behavior::Tray : value == 2 ? Behavior::Exit : Behavior::Ask;
}

struct Decision {
    Behavior action; // Ask means cancelled / no action.
    Behavior preference;
};

constexpr Decision Resolve(Behavior preference, Behavior choice, bool remember, bool accepted) {
    if (preference != Behavior::Ask) return {preference, preference};
    if (!accepted || choice == Behavior::Ask) return {Behavior::Ask, preference};
    return {choice, remember ? choice : preference};
}
}
