//
// Created by otrush on 2/28/2024.
//

#include "ShiftEngine.hpp"

int main() {
    Shift::ShiftEngine shiftEngine;

    shiftEngine.Init({.width = 1080, .height = 720});
    shiftEngine.LoadScene("Placeholder");
    shiftEngine.Run();

    shiftEngine.Cleanup();

    return EXIT_SUCCESS;
}