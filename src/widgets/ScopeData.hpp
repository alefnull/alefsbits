#pragma once

#include "../utils/ResizableRingBuffer.hpp"

struct ScopeData {
    int scopeMode = 0;
    float zeroThreshold = 0.0f;
    float timeScale = 1.0f;
    ResizableRingBuffer<std::pair<float, bool>> buffer;

    NVGcolor backgroundColor;
    NVGcolor wavePrimaryColor;
    NVGcolor waveSecondaryColor;
    NVGcolor gridColor;
    NVGcolor triggerColor;
};
