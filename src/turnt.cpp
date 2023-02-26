#include "plugin.hpp"
#include "utils/ResizableRingBuffer.hpp"
#include "widgets/Scope.hpp"
#include "widgets/ScopeData.hpp"
#include "widgets/TabDisplay.cpp"

#define MAX_POLY 16
#define BUFFER_SIZE 3
#define PT_BUFFER_SIZE 256

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
            scope_data.buffer[ch].resize(PT_BUFFER_SIZE);
            for (int i = 0; i < PT_BUFFER_SIZE; i++) {
                point_buffer[i] = 0.f;
                scope_data.buffer[ch].add(std::make_pair(0.f, false));
            }
        }
    }

    ScopeData scope_data;

    bool freeze_when_idle = false;
    bool triggered = false;
    int trigger_mode = 0;
    bool gate_high[MAX_POLY] = {false};
    float samples[MAX_POLY][BUFFER_SIZE] = {0.f};
    dsp::PulseGenerator pulse[MAX_POLY];
    float point_buffer[PT_BUFFER_SIZE];
    int buffer_index[MAX_POLY] = {0};
    int frame_index[MAX_POLY] = {0};
    int current_channel = 0;
    float current_point;

    void process(const ProcessArgs& args) override {
        int mode = params[MODE_PARAM].getValue();
        float zero = params[ZERO_PARAM].getValue();
        scope_data.zeroThreshold[current_channel] = zero;
        float prob = params[PROB_PARAM].getValue();
        if (inputs[ZERO_INPUT].isConnected()) {
            // use param as attenuverter
            zero *= inputs[ZERO_INPUT].getVoltage() / 10.f;
        }
        if (inputs[PROB_INPUT].isConnected()) {
            // use param as attenuator
            prob *= inputs[PROB_INPUT].getVoltage() / 10.f;
        }

        int channels = inputs[SOURCE_INPUT].isConnected()
                           ? inputs[SOURCE_INPUT].getChannels()
                           : 1;

        if (!inputs[SOURCE_INPUT].isConnected()) {
            if (freeze_when_idle) {
                return;
            }
            // clear buffer
            for (int ch = 0; ch < MAX_POLY; ch++) {
                for (int i = 0; i < BUFFER_SIZE; i++) {
                    samples[ch][i] = 0.f;
                }
            }
        }

        for (int ch = 0; ch < channels; ch++) {
            auto in = inputs[SOURCE_INPUT].getVoltage(ch);
            if (in != samples[ch][BUFFER_SIZE - 1]) {
                for (int i = 0; i < BUFFER_SIZE - 1; i++) {
                    samples[ch][i] = samples[ch][i + 1];
                }
                samples[ch][BUFFER_SIZE - 1] =
                    inputs[SOURCE_INPUT].getVoltage(ch);

                auto d1 = samples[ch][1] - samples[ch][0];
                auto d2 = samples[ch][2] - samples[ch][1];

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
                        trig =
                            (samples[ch][1] > zero && samples[ch][2] <= zero) ||
                                    (samples[ch][1] < zero &&
                                     samples[ch][2] >= zero)
                                ? (r < prob)
                                : false;
                        break;
                    case 2:  // both
                        trig =
                            (((d1 > 0.f && d2 < 0.f) ||
                              (d1 < 0.f && d2 > 0.f)) ||
                             (samples[ch][1] > zero &&
                              samples[ch][2] <= zero) ||
                             (samples[ch][1] < zero && samples[ch][2] >= zero))
                                ? (r < prob)
                                : false;
                        break;
                }

                if (trig) {
                    triggered = true;
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
                triggered = gate_high[ch] || triggered;
            } else {
                auto pulseValue = pulse[ch].process(args.sampleTime);
                outputs[TRIG_OUTPUT].setVoltage(pulseValue ? 10.f : 0.f, ch);
                triggered = bool(pulseValue) || triggered;
            }

            // add point to buffer
            if (buffer_index[ch] < PT_BUFFER_SIZE) {
                float dt = dsp::approxExp2_taylor5(
                                -(-std::log2(scope_data.timeScale))) /
                            PT_BUFFER_SIZE;
                int frame_count = (int)std::ceil(dt * args.sampleRate);

                current_point = -in;

                if (++frame_index[ch] >= frame_count) {
                    frame_index[ch] = 0;
                    point_buffer[buffer_index[ch]] = current_point;
                    scope_data.buffer[current_channel].add(std::make_pair(in, triggered));
                    triggered = false;
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

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(
            Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(
            Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(
            createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH,
                                          RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

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
        y += dy * 3;
        x -= RACK_GRID_WIDTH * 1.5;
        addParam(createParamCentered<CKSSThreeHorizontal>(Vec(x, y), module,
                                                          Turnt::MODE_PARAM));
        y += dy * 2;
        addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module,
                                                   Turnt::TRIG_OUTPUT));

        y += dy * 2;

        auto scopeData = module ? &module->scope_data : nullptr;
        auto scope = new Scope(scopeData);
        scope->box.pos = Vec(x - 50.f, y);
        scope->box.size = Vec(100.f, 100.f);
        auto tabDisplay = new TabDisplay();
        tabDisplay->box.pos = Vec(x - 50.f, scope->box.pos.y - 10.f);
        tabDisplay->box.size = Vec(100.f, 10.f);
        tabDisplay->addTab("ch 1", [tabDisplay, scopeData]() {
            tabDisplay->selectedTab = 0;
            tabDisplay->activeLabelColor = scopeData->backgroundColor;
            scopeData->activeChannel = 0;
        });
        tabDisplay->addTab("ch 2", [tabDisplay, scopeData]() {
            tabDisplay->selectedTab = 1;
            tabDisplay->activeLabelColor = scopeData->backgroundColor;
            scopeData->activeChannel = 1;
        });
        tabDisplay->addTab("ch 3", [tabDisplay, scopeData]() {
            tabDisplay->selectedTab = 2;
            tabDisplay->activeLabelColor = scopeData->backgroundColor;
            scopeData->activeChannel = 2;
        });
        addChild(tabDisplay);
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
                "bipolar", CHECKMARK(module->scope_data.scopeMode[module->current_channel] == 0),
                [module]() { module->scope_data.scopeMode[module->current_channel] = 0; }));
            scopeMenu->addChild(createMenuItem(
                "unipolar", CHECKMARK(module->scope_data.scopeMode[module->current_channel] == 1),
                [module]() { module->scope_data.scopeMode[module->current_channel] = 1; }));
            menu->addChild(scopeMenu);
        }));

        // create ui slider for timer division
        menu->addChild(createSubmenuItem("time scale", "", [=](Menu* menu) {
            Menu* divMenu = new Menu();
            divMenu->addChild(createMenuItem(
                "Low", CHECKMARK(module->scope_data.buffer[module->current_channel].size == 64),
                [module]() { module->scope_data.buffer[module->current_channel].resize(64); }));
            divMenu->addChild(createMenuItem(
                "Medium", CHECKMARK(module->scope_data.buffer[module->current_channel].size == 256),
                [module]() { module->scope_data.buffer[module->current_channel].resize(256); }));
            divMenu->addChild(createMenuItem(
                "High", CHECKMARK(module->scope_data.buffer[module->current_channel].size == 2048),
                [module]() { module->scope_data.buffer[module->current_channel].resize(2048); }));
            menu->addChild(divMenu);
        }));
    }
};

Model* modelTurnt = createModel<Turnt, TurntWidget>("turnt");