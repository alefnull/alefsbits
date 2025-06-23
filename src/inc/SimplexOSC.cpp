#include <algorithm>
#include "SimplexNoise.hpp"

#define FREQ_C3 130.8128f

struct SimplexOSC
{
  static const int BUFFER_LEN = 2048;
  double phase = 0.0;
  double freq = 0.0;
  unsigned int tick = 0;
  double buffer[BUFFER_LEN] = {0};
  int buffer_pos = 0;
  double min = -1.0;
  double max = 1.0;
  SimplexNoise noise;

  SimplexOSC()
  {
    noise.init();
  }

  void step(float delta_time)
  {
    double delta = freq * delta_time;
    phase += delta;
    if (phase >= 1.0)
    {
      phase -= 1.0;
    }
  }

  void reset()
  {
    phase = 0.f;
  }

  void set_pitch(double pitch)
  {
    freq = FREQ_C3 * powf(2.0, pitch);
  }

  float get_value(double detail, double x, double y, double z, double scale)
  {
    return noise.SumOctaveSmooth(detail, x + phase, y, z, scale);
  }

  float get_osc(double detail, double x, double y, double z, double scale)
  {
    float value = get_value(detail, x, y, z, scale);
    return value * 5.0;
  }

  float get_norm_osc(double detail, double x, double y, double z, double scale)
  {
    float value = get_value(detail, x, y, z, scale);
    if (buffer_pos >= BUFFER_LEN)
    {
      buffer_pos = 0;
    }
    buffer[buffer_pos] = value;
    buffer_pos++;
    if (tick % 256 == 0)
    {
      // auto minmax = std::minmax_element(begin(buffer), end(buffer));
      auto minmax = std::minmax_element(buffer, buffer + BUFFER_LEN);
      min = *minmax.first;
      max = *minmax.second;
    }
    tick++;

    value = clamp(rescale(value, min, max, -1.0, 1.0), -1.0, 1.0);
    return value;
  }

  double rescale(double value, double old_min, double old_max, double new_min, double new_max)
  {
    return (value - old_min) / (old_max - old_min) * (new_max - new_min) + new_min;
  }
};