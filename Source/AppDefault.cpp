//
// Created by otrush on 2/28/2024.
//

#include "ShiftEngine.hpp"

int main() {
    shift::ShiftEngine shiftEngine;

    shiftEngine.Init(1080, 720);
    shiftEngine.LoadScene("Placeholder");
    shiftEngine.Run();

    shiftEngine.Cleanup();

    return EXIT_SUCCESS;
}