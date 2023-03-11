#pragma once

#include "../inc/ResizableRingBuffer.hpp"

#define MAX_POLY 16

struct ScopeData {
    int scopeMode[MAX_POLY];
    float zeroThreshold[MAX_POLY] = {0.f};
    float timeScale = 1.0f;
    int activeChannel = 0;
    ResizableRingBuffer<std::pair<float, bool>> buffer[MAX_POLY];

    NVGcolor backgroundColor;
    NVGcolor wavePrimaryColor;
    NVGcolor gridColor;
    NVGcolor triggerColor;
};
