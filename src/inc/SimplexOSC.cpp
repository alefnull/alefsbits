#include <algorithm>
#include "SimplexNoise.hpp"

struct SimplexOSC {
    static const int BUFFER_LEN = 2048;
    float phase = 0.f;
    float freq = 0.f;
    unsigned int tick = 0;
    float buffer[BUFFER_LEN] = {0};
    int buffer_pos = 0;
    float min = -1.f;
    float max = 1.f;
    SimplexNoise noise;

    SimplexOSC() {
        noise.init();
    }

    void step(float delta_time) {
        float delta = freq * delta_time;
        phase += delta;
        if (phase >= 1.f) {
            phase -= 1.f;
        }
    }

    void reset() {
        phase = 0.f;
    }

    void set_pitch(float pitch) {
        freq = dsp::FREQ_C4 * powf(2.f, pitch);
    }

    float get_value(float detail, float x, float y, float z, float scale) {
        return noise.SumOctaveSmooth(detail, x + phase, y, z, scale);
    }

    float get_osc(float detail, float x, float y, float z, float scale) {
        float value = get_value(detail, x, y, z, scale);
        return value * 5.f;
    }

    float get_norm_osc(float detail, float x, float y, float z, float scale) {
        float value = get_value(detail, x, y, z, scale);
        if (buffer_pos >= BUFFER_LEN) {
            buffer_pos = 0;
        }
        buffer[buffer_pos] = value;
        buffer_pos++;
        if (tick % 256 == 0) {
            auto minmax = std::minmax_element(begin(buffer), end(buffer));
            min = *minmax.first;
            max = *minmax.second;
        }
        tick++;

        value = clamp(rescale(value, min, max, -1.f, 1.f), -1.f, 1.f);
        return value;
    }
};