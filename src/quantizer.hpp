#include "rack.hpp"

struct Quantizer {
    // a series of scales represented as arrays of intervals
    // the first element of each array is the root note (0)
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

    // an enum of note names
    enum NoteName {
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

    // an enum of scales
    enum ScaleName {
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

    // a function that takes a volt/octave value, a root note, and a scale
    // and returns the closest note in the scale
    float quantize(float input, int root, int scale) {
        // the current scale, initialized to the chromatic scale
        int *curr_scale = SCALE_CHROMATIC;
        int curr_scale_size = 12;
        switch (scale) {
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

        // closest value
        float closest_note = 0;
        // closest distance
        float closest_dist = 10;
        // note in volts
        float note_volts = 0;
        // distance between input and note in volts
        float dist = 0;
        // octave number
        int oct = int(floorf(input));
        // volts modulo 1
        float mod = input - oct;

        // iterate through all notes in the scale
        for (int i = 0; i < curr_scale_size; i++) {
            // calculate the note in volts
            note_volts = (oct + (curr_scale[i] + root) / 12.0);
            // calculate the distance between the input and the note
            dist = fabsf(mod - (curr_scale[i] + root) / 12.0);
            // if the distance is less than the closest distance
            if (dist < closest_dist) {
                // set the closest distance to the distance
                closest_dist = dist;
                // set the closest value to the note in volts
                closest_note = note_volts;
            }
        }
        // return the closest value
        return closest_note;
    }
};