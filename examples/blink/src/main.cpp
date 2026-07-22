// Portable blink — identical bytes on every supported board.
//
// No #ifdef, no arch checks, no hand-rolled delay: the board role layer and
// alloy::sleep_for are the contract (NORTH_STAR guards #3 and #6).
#include <alloy/board.hpp>
using namespace alloy::literals;

int main() {
    board::init();
    while (true) {
        board::led.toggle();
        alloy::sleep_for(500ms);
    }
}
