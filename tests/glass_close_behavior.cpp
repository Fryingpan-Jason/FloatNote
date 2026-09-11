#include "../experiments/glass_close_behavior.h"
#include <array>
#include <iostream>

int main() {
    using namespace GlassClose;
    for (int raw : {-1, 3, 99, 2147483647})
        if (FromStored(raw) != Behavior::Ask) return 1;
    if (FromStored(0) != Behavior::Ask || FromStored(1) != Behavior::Tray ||
        FromStored(2) != Behavior::Exit) return 2;
    struct Case { Behavior preference, choice; bool remember, accepted; Behavior action, saved; };
    constexpr std::array cases{
        Case{Behavior::Ask,Behavior::Tray,false,true,Behavior::Tray,Behavior::Ask},
        Case{Behavior::Ask,Behavior::Exit,false,true,Behavior::Exit,Behavior::Ask},
        Case{Behavior::Ask,Behavior::Tray,true,true,Behavior::Tray,Behavior::Tray},
        Case{Behavior::Ask,Behavior::Exit,true,true,Behavior::Exit,Behavior::Exit},
        // Checking remember, then Escape/cancel, must not save or close anything.
        Case{Behavior::Ask,Behavior::Tray,true,false,Behavior::Ask,Behavior::Ask},
        Case{Behavior::Ask,Behavior::Exit,true,false,Behavior::Ask,Behavior::Ask},
        Case{Behavior::Ask,Behavior::Ask,true,true,Behavior::Ask,Behavior::Ask},
        // Remembered preferences skip the dialog. Old dialog outputs are ignored.
        Case{Behavior::Tray,Behavior::Exit,false,false,Behavior::Tray,Behavior::Tray},
        Case{Behavior::Exit,Behavior::Tray,false,false,Behavior::Exit,Behavior::Exit}
    };
    for (const auto& test : cases) {
        const auto result = Resolve(test.preference,test.choice,test.remember,test.accepted);
        if (result.action != test.action || result.preference != test.saved) return 3;
    }
    const auto remembered = Resolve(Behavior::Ask,Behavior::Tray,true,true);
    const auto restarted = FromStored(static_cast<int>(remembered.preference));
    if (Resolve(restarted,Behavior::Ask,false,false).action != Behavior::Tray) return 4;
    if (Resolve(FromStored(0),Behavior::Ask,false,false).action != Behavior::Ask) return 5;
    std::cout << "PASS close choices, cancellation, remember, stored values and reset-to-ask\n";
}
