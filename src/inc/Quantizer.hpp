#include "rack.hpp"

struct Quantizer
{
  int SCALE_CHROMATIC[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  int SCALE_MAJOR[7] = {0, 2, 4, 5, 7, 9, 11};
  int SCALE_MINOR[7] = {0, 2, 3, 5, 7, 8, 10};
  int SCALE_MAJOR_PENTATONIC[5] = {0, 2, 4, 7, 9};
  int SCALE_MINOR_PENTATONIC[5] = {0, 3, 5, 7, 10};
  int SCALE_DORIAN[7] = {0, 2, 3, 5, 7, 9, 10};
  int SCALE_LYDIAN[7] = {0, 2, 4, 6, 7, 9, 11};
  int SCALE_MIXOLYDIAN[7] = {0, 2, 4, 5, 7, 9, 10};
  int SCALE_PHRYGIAN[7] = {0, 1, 3, 5, 7, 8, 10};
  int SCALE_LOCRIAN[7] = {0, 1, 3, 5, 6, 8, 10};
  int SCALE_BLUES[6] = {0, 3, 5, 6, 7, 10};

  enum NoteName
  {
    C,
    C_SHARP,
    D,
    D_SHARP,
    E,
    F,
    F_SHARP,
    G,
    G_SHARP,
    A,
    A_SHARP,
    B
  };

  enum ScaleName
  {
    CHROMATIC,
    MAJOR,
    MINOR,
    MAJOR_PENTATONIC,
    MINOR_PENTATONIC,
    DORIAN,
    LYDIAN,
    MIXOLYDIAN,
    PHRYGIAN,
    LOCRIAN,
    BLUES
  };

  float quantize(float input, int root, int scale)
  {
    int *curr_scale = SCALE_CHROMATIC;
    int curr_scale_size = 12;
    switch (scale)
    {
    case CHROMATIC:
      curr_scale = SCALE_CHROMATIC;
      curr_scale_size = 12;
      break;
    case MAJOR:
      curr_scale = SCALE_MAJOR;
      curr_scale_size = 7;
      break;
    case MINOR:
      curr_scale = SCALE_MINOR;
      curr_scale_size = 7;
      break;
    case MAJOR_PENTATONIC:
      curr_scale = SCALE_MAJOR_PENTATONIC;
      curr_scale_size = 5;
      break;
    case MINOR_PENTATONIC:
      curr_scale = SCALE_MINOR_PENTATONIC;
      curr_scale_size = 5;
      break;
    case DORIAN:
      curr_scale = SCALE_DORIAN;
      curr_scale_size = 7;
      break;
    case LYDIAN:
      curr_scale = SCALE_LYDIAN;
      curr_scale_size = 7;
      break;
    case MIXOLYDIAN:
      curr_scale = SCALE_MIXOLYDIAN;
      curr_scale_size = 7;
      break;
    case PHRYGIAN:
      curr_scale = SCALE_PHRYGIAN;
      curr_scale_size = 7;
      break;
    case LOCRIAN:
      curr_scale = SCALE_LOCRIAN;
      curr_scale_size = 7;
      break;
    case BLUES:
      curr_scale = SCALE_BLUES;
      curr_scale_size = 6;
      break;
    }

    float closest_value = 0.0f;
    float closest_distance = 100.0f;
    float note_in_volts = 0.0f;
    float distance = 0.0f;
    int octave = int(floorf(input));
    float volts_minus_octave = input - octave;

    for (int i = 0; i < curr_scale_size; i++)
    {
      note_in_volts = (curr_scale[i] + root) % 12 / 12.0f;
      distance = fabsf(volts_minus_octave - note_in_volts);
      if (distance < closest_distance)
      {
        closest_distance = distance;
        closest_value = note_in_volts;
      }
    }

    return closest_value + octave;
  }

  float quantize(float input, int root, int scale[], int length)
  {
    int curr_scale[length];
    for (int i = 0; i < length; i++)
    {
      curr_scale[i] = scale[i];
    }
    float closest_value = 0.0f;
    float closest_distance = 100.0f;
    float note_in_volts = 0.0f;
    float distance = 0.0f;
    int octave = int(floorf(input));
    float volts_minus_octave = input - octave;

    for (int i = 0; i < length; i++)
    {
      note_in_volts = (curr_scale[i] + root) % 12 / 12.0f;
      distance = fabsf(volts_minus_octave - note_in_volts);
      if (distance < closest_distance)
      {
        closest_distance = distance;
        closest_value = note_in_volts;
      }
    }

    return closest_value + octave;
  }
};