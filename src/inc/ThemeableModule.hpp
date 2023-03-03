#pragma once
#include <rack.hpp>

#define CONTRAST_MIN 0.1f
#define CONTRAST_MAX 0.9f

struct ThemeableModule : rack::Module {
    float contrast = CONTRAST_MAX;
    float global_contrast = CONTRAST_MAX;
    bool use_global_contrast = true;

    ThemeableModule() {
        load_global_contrast();
    }

    void load_global_contrast() {
        FILE *f = fopen(rack::asset::user("alefsbits.json").c_str(), "r");
        if (f) {
            json_error_t error;
            json_t *rootJ = json_loadf(f, 0, &error);
            if (rootJ) {
                json_t *global_contrastJ = json_object_get(rootJ, "global_contrast");
                if (global_contrastJ) {
                    this->global_contrast = json_real_value(global_contrastJ);
                    if (this->global_contrast < CONTRAST_MIN) {
                        this->global_contrast = CONTRAST_MIN;
                    }
                    if (this->global_contrast > CONTRAST_MAX) {
                        this->global_contrast = CONTRAST_MAX;
                    }
                }
                json_decref(rootJ);
            }
            fclose(f);
        }
        else {
            this->global_contrast = CONTRAST_MIN;
        }
    }

    void save_global_contrast(float gc) {
        if (gc < CONTRAST_MIN) {
            gc = CONTRAST_MIN;
        }
        if (gc > CONTRAST_MAX) {
            gc = CONTRAST_MAX;
        }
        FILE *f = fopen(rack::asset::user("alefsbits.json").c_str(), "w");
        if (f) {
            json_t *rootJ = json_object();
            json_object_set_new(rootJ, "global_contrast", json_real(gc));
            json_dumpf(rootJ, f, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
            json_decref(rootJ);
            fclose(f);
        }
    }
};