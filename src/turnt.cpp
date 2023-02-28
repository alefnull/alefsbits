#include "plugin.hpp"
#include "utils/ResizableRingBuffer.hpp"
#include "widgets/Scope.hpp"
#include "widgets/ScopeData.hpp"
#include "widgets/TabDisplay.cpp"

#define MAX_POLY 16

struct Turnt : Module {
    enum ParamId { MODE_PARAM, ZERO_PARAM, PROB_PARAM, PARAMS_LEN };
    enum InputId { SOURCE_INPUT, ZERO_INPUT, PROB_INPUT, INPUTS_LEN };
    enum OutputId { TRIG_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    struct Point {
        float min = 0.f;
        float max = 0.f;
    };

    Turnt() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "mode",
                     {"direction", "through zero", "both"});
        configParam(ZERO_PARAM, -10.f, 10.f, 0.f, "zero threshold", "V");
        configParam(PROB_PARAM, 0.f, 1.f, 1.f, "probability", "%", 0.f, 100.f);
        configInput(SOURCE_INPUT, "source");
        configInput(ZERO_INPUT, "zero threshold");
        configInput(PROB_INPUT, "probability");
        configOutput(TRIG_OUTPUT, "trigger");

        // dark vscode background color
        scope_data.backgroundColor = nvgRGB(0x1e, 0x1e, 0x1e);
        // very bright white
        scope_data.wavePrimaryColor = nvgRGB(0xff, 0xff, 0xff);
        // dark gray
        scope_data.gridColor = nvgRGB(0x50, 0x57, 0x5c);
        // desaturated red
        scope_data.triggerColor = nvgRGB(0x8f, 0x3b, 0x3b);

        for (int ch = 0; ch < MAX_POLY; ch++) {
            scope_data.buffer[ch].resize(256);
            for (int i = 0; i < scope_data.buffer[ch].size; i++) {
                scope_data.buffer[ch].add(std::make_pair(0.f, false));
            }
        }
    }

    ScopeData scope_data;

    float samples[3] = {0.f, 0.f, 0.f};
    bool freeze_when_idle = false;
    bool triggered[MAX_POLY] = {false};
    int trigger_mode = {0};
    bool gate_high[MAX_POLY] = {false};
    dsp::PulseGenerator pulse[MAX_POLY];
    int buffer_index[MAX_POLY] = {0};
    int frame_index[MAX_POLY] = {0};

    void process(const ProcessArgs& args) override {
        int mode = params[MODE_PARAM].getValue();
        float zero = params[ZERO_PARAM].getValue();
        scope_data.zeroThreshold[scope_data.activeChannel] = zero;
        float prob = params[PROB_PARAM].getValue();
        if (inputs[ZERO_INPUT].isConnected()) {
            // use param as attenuverter
            zero *= inputs[ZERO_INPUT].getVoltage() / 10.f;
        }
        if (inputs[PROB_INPUT].isConnected()) {
            // use param as attenuator
            prob *= inputs[PROB_INPUT].getVoltage() / 10.f;
        }

        int channels = std::max(1, inputs[SOURCE_INPUT].getChannels());
        outputs[TRIG_OUTPUT].setChannels(channels);

        if (!inputs[SOURCE_INPUT].isConnected()) {
            // keep buffers if freeze_when_idle
            if (freeze_when_idle) {
                return;
            }
            // clear buffers otherwise
            for (int ch = 0; ch < MAX_POLY; ch++) {
                for (int i = 0; i < scope_data.buffer[ch].size; i++) {
                    scope_data.buffer[ch].get(i).operator=(std::make_pair(
                        0.f, false));
                }
            }
        }

        for (int ch = 0; ch < channels; ch++) {
            auto in = inputs[SOURCE_INPUT].getVoltage(ch);

            // if the input isn't the same as the last value in the sample buffer
            if (in != samples[0]) {
                // add the input to the sample buffer
                samples[2] = samples[1];
                samples[1] = samples[0];
                samples[0] = in;

                // auto v1 = samples[2];
                // auto v2 = samples[1];
                // auto d1 = v2 - v1;
                // auto v3 = samples[0];
                // auto d2 = v3 - v2;
                auto v1 = samples[0];
                auto v2 = samples[1];
                auto d1 = v2 - v1;
                auto v3 = samples[2];
                auto d2 = v3 - v2;

                bool trig = false;
                float r = random::uniform();

                switch (mode) {
                    case 0:  // direction
                        trig =
                            ((d1 > 0.f && d2 < 0.f) || (d1 < 0.f && d2 > 0.f))
                                ? (r < prob)
                                : false;
                        break;
                    case 1:  // through zero
                        trig = ((v1 > zero && v2 <= zero) || (v1 < zero && v2 >= zero))
                                ? (r < prob)
                                : false;
                        break;
                    case 2:  // both
                        trig =
                            ((d1 > 0.f && d2 < 0.f) || (d1 < 0.f && d2 > 0.f)
                                                    || (v1 > zero && v2 <= zero)
                                                    || (v1 < zero && v2 >= zero))
                                ? (r < prob)
                                : false;
                        break;
                }

                if (trig) {
                    triggered[ch] = true;
                    // pulse.trigger(1e-3f);
                    switch (trigger_mode) {
                        case 0:  // trigger
                            pulse[ch].trigger(1e-3f);
                            break;
                        case 1:  // latch
                            gate_high[ch] = !gate_high[ch];
                            break;
                    }
                }
            }

            if (trigger_mode == 1) {
                outputs[TRIG_OUTPUT].setVoltage(gate_high[ch] ? 10.f : 0.f, ch);
                triggered[ch] = gate_high[ch] || triggered[ch];
            } else {
                auto pulseValue = pulse[ch].process(args.sampleTime);
                outputs[TRIG_OUTPUT].setVoltage(pulseValue ? 10.f : 0.f, ch);
                triggered[ch] = bool(pulseValue) || triggered[ch];
            }

            // add point to buffer
            if (buffer_index[ch] < scope_data.buffer[ch].size) {
                float dt = dsp::approxExp2_taylor5(
                                -(-std::log2(scope_data.timeScale))) / scope_data.buffer[ch].size;
                int frame_count = (int)std::ceil(dt * args.sampleRate);

                if (++frame_index[ch] >= frame_count) {
                    frame_index[ch] = 0;
                    scope_data.buffer[ch].add(std::make_pair(in, triggered[ch]));
                    triggered[ch] = false;
                    buffer_index[ch]++;
                }
            } else {
                // buffer full - reset
                buffer_index[ch] = 0;
                frame_index[ch] = 0;
            }
        }
    }

    void onReset() override {
        for (int i = 0; i < MAX_POLY; i++) {
            pulse[i].reset();
            gate_high[i] = false;
        }
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* modeJ = json_object_get(rootJ, "trigger mode");
        if (modeJ) {
            trigger_mode = json_integer_value(modeJ);
        }
        json_t* freezeJ = json_object_get(rootJ, "freeze when idle");
        if (freezeJ) {
            freeze_when_idle = json_boolean_value(freezeJ);
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "trigger mode", 
                            json_integer(trigger_mode));
        json_object_set_new(rootJ, "freeze on disconnect",
                            json_boolean(freeze_when_idle));
        return rootJ;
    }
};

struct TurntWidget : ModuleWidget {
    TurntWidget(Turnt* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/turnt.svg")));

        // addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        // addChild(createWidget<ScrewSilver>(
        //     Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        // addChild(createWidget<ScrewSilver>(
        //     Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        // addChild(
        //     createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH,
        //                                   RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        float y_start = RACK_GRID_WIDTH * 4;
        float dy = RACK_GRID_WIDTH;
        float x = box.size.x / 2;
        float y = y_start;

        addInput(createInputCentered<PJ301MPort>(Vec(x, y), module,
                                                 Turnt::SOURCE_INPUT));
        y += dy * 3;
        x -= RACK_GRID_WIDTH * 1.5;
        addInput(createInputCentered<PJ301MPort>(Vec(x, y), module,
                                                 Turnt::ZERO_INPUT));
        y += dy * 2;
        addParam(createParamCentered<RoundBlackKnob>(Vec(x, y), module,
                                                     Turnt::ZERO_PARAM));
        y -= dy * 2;
        x += RACK_GRID_WIDTH * 3;
        addInput(createInputCentered<PJ301MPort>(Vec(x, y), module,
                                                 Turnt::PROB_INPUT));
        y += dy * 2;
        addParam(createParamCentered<RoundBlackKnob>(Vec(x, y), module,
                                                     Turnt::PROB_PARAM));
        y += dy * 2;
        x -= RACK_GRID_WIDTH * 1.5;
        addParam(createParamCentered<CKSSThreeHorizontal>(Vec(x, y), module,
                                                          Turnt::MODE_PARAM));
        y += dy * 2;
        addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module,
                                                   Turnt::TRIG_OUTPUT));

        y += dy * 2.f;

        auto scopeData = module ? &module->scope_data : nullptr;
        auto scope = new Scope(scopeData);
        scope->box.pos = Vec(0.f, y);
        scope->box.size = Vec(box.size.x, 100.f);
        auto topTabDisplay = new TabDisplay();
        auto bottomTabDisplay = new TabDisplay();
        topTabDisplay->box.pos = Vec(scope->box.pos.x, scope->box.pos.y - 9.f);
        topTabDisplay->box.size = Vec(scope->box.size.x, 10.f);
        bottomTabDisplay->box.pos = Vec(scope->box.pos.x, scope->box.pos.y + scope->box.size.y - 1.f);
        bottomTabDisplay->box.size = Vec(scope->box.size.x, 10.f);
        for (int i = 0; i < 8; i++) {
            topTabDisplay->addTab(std::to_string(i + 1), [topTabDisplay, bottomTabDisplay, scopeData, i]() {
                scopeData->activeChannel = i;
                bottomTabDisplay->selectedTab = -1;
                topTabDisplay->selectedTab = i;
            });
        }
        for (int i = 8; i < 16; i++) {
            bottomTabDisplay->addTab(std::to_string(i + 1), [bottomTabDisplay, topTabDisplay, scopeData, i]() {
                scopeData->activeChannel = i;
                topTabDisplay->selectedTab = -1;
                bottomTabDisplay->selectedTab = i - 8;
            });
        }
        topTabDisplay->selectedTab = 0;
        bottomTabDisplay->selectedTab = -1;
        addChild(topTabDisplay);
        addChild(bottomTabDisplay);
        addChild(scope);
    }

    void appendContextMenu(Menu* menu) override {
        Turnt* module = dynamic_cast<Turnt*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createMenuItem("freeze when idle",
            CHECKMARK(module->freeze_when_idle), 
            [module]() { module->freeze_when_idle = !module->freeze_when_idle; }));

        menu->addChild(createSubmenuItem("trigger mode", "", [=](Menu* menu) {
            Menu* trigMenu = new Menu();
            trigMenu->addChild(
                createMenuItem("trigger", CHECKMARK(module->trigger_mode == 0),
                               [module]() { module->trigger_mode = 0; }));
            trigMenu->addChild(
                createMenuItem("latch", CHECKMARK(module->trigger_mode == 1),
                               [module]() { module->trigger_mode = 1; }));
            menu->addChild(trigMenu);
        }));

        // create menu for unipolar / bipolar scope
        menu->addChild(createSubmenuItem("scope mode", "", [=](Menu* menu) {
            Menu* scopeMenu = new Menu();
            scopeMenu->addChild(createMenuItem(
                "bipolar", CHECKMARK(module->scope_data.scopeMode[module->scope_data.activeChannel] == 0),
                [module]() { module->scope_data.scopeMode[module->scope_data.activeChannel] = 0; }));
            scopeMenu->addChild(createMenuItem(
                "unipolar", CHECKMARK(module->scope_data.scopeMode[module->scope_data.activeChannel] == 1),
                [module]() { module->scope_data.scopeMode[module->scope_data.activeChannel] = 1; }));
            menu->addChild(scopeMenu);
        }));

        // create ui slider for timer division
        menu->addChild(createSubmenuItem("time scale", "", [=](Menu* menu) {
            Menu* divMenu = new Menu();
            divMenu->addChild(createMenuItem(
                "Low", CHECKMARK(module->scope_data.buffer[module->scope_data.activeChannel].size == 64),
                [module]() { module->scope_data.buffer[module->scope_data.activeChannel].resize(64); }));
            divMenu->addChild(createMenuItem(
                "Medium", CHECKMARK(module->scope_data.buffer[module->scope_data.activeChannel].size == 256),
                [module]() { module->scope_data.buffer[module->scope_data.activeChannel].resize(256); }));
            divMenu->addChild(createMenuItem(
                "High", CHECKMARK(module->scope_data.buffer[module->scope_data.activeChannel].size == 2048),
                [module]() { module->scope_data.buffer[module->scope_data.activeChannel].resize(2048); }));
            menu->addChild(divMenu);
        }));
    }
};

Model* modelTurnt = createModel<Turnt, TurntWidget>("turnt");