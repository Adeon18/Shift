//
// Created by otrush on 2/28/2024.
//

#include "ShiftEngine.hpp"

int main() {
    shift::ShiftEngine shiftEngine;

    shiftEngine.Init(800, 600);
    shiftEngine.LoadScene("Placeholder");
    shiftEngine.Run();

    shiftEngine.Cleanup();

    return EXIT_SUCCESS;
}