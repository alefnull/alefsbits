#pragma once

#include "plugin.hpp"
#include "inc/Quantizer.hpp"
#include "inc/cvRange.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "slipspander.hpp"

#define MAX_POLY 16
#define MAX_STEPS 64

struct Slips : Module, Quantizer
{
    enum ParamId
    {
        ROOT_PARAM,
        STEPS_PARAM,
        SCALE_PARAM,
        START_PARAM,
        GENERATE_PARAM,
        PROB_PARAM,
        SLIPS_PARAM,
        MODGEN_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        CLOCK_INPUT,
        ROOT_CV_INPUT,
        STEPS_CV_INPUT,
        RESET_INPUT,
        SCALE_CV_INPUT,
        START_CV_INPUT,
        GENERATE_TRIGGER_INPUT,
        PROB_CV_INPUT,
        QUANTIZE_INPUT,
        SLIPS_CV_INPUT,
        MODGEN_TRIGGER_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        QUANTIZE_OUTPUT,
        SEQUENCE_OUTPUT,
        GATE_OUTPUT,
        SLIP_GATE_OUTPUT,
        EOC_OUTPUT,
        MOD_SEQUENCE_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId
    {
        GATE_LIGHT,
        SLIP_GATE_LIGHT,
        ENUMS(SEGMENT_LIGHTS, 64),
        EOC_LIGHT,
        EXPANDED_LIGHT,
        LIGHTS_LEN
    };

    Slips()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(STEPS_PARAM, 1, MAX_STEPS, 16, "steps");
        getParamQuantity(STEPS_PARAM)->snapEnabled = true;
        configSwitch(ROOT_PARAM, 0, 11, 0, "root note", {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"});
        getParamQuantity(ROOT_PARAM)->snapEnabled = true;
        configSwitch(SCALE_PARAM, 0, 10, 0, "scale", {"chromatic", "major", "minor", "major pentatonic", "minor pentatonic", "dorian", "lydian", "mixolydian", "phrygian", "locrian", "blues"});
        getParamQuantity(SCALE_PARAM)->snapEnabled = true;
        configParam(GENERATE_PARAM, 0, 1, 0, "generate sequence");
        configParam(SLIPS_PARAM, 0, 1, 0, "slips", " %", 0, 100);
        configParam(MODGEN_PARAM, 0, 1, 0, "generate mod sequence");
        configInput(CLOCK_INPUT, "clock");
        configInput(RESET_INPUT, "reset");
        configInput(STEPS_CV_INPUT, "steps cv");
        getInputInfo(STEPS_CV_INPUT)->description = "0V to 10V";
        configInput(ROOT_CV_INPUT, "root cv");
        getInputInfo(ROOT_CV_INPUT)->description = "0V to 10V (v/oct if enabled)";
        configInput(SCALE_CV_INPUT, "scale cv");
        getInputInfo(SCALE_CV_INPUT)->description = "0V to 10V";
        configInput(GENERATE_TRIGGER_INPUT, "generate sequence");
        configInput(SLIPS_CV_INPUT, "slips cv");
        getInputInfo(SLIPS_CV_INPUT)->description = "0V to 10V";
        configInput(MODGEN_TRIGGER_INPUT, "generate mod sequence");
        configOutput(SEQUENCE_OUTPUT, "sequence");
        configOutput(GATE_OUTPUT, "gate");
        configOutput(SLIP_GATE_OUTPUT, "slip gate");
        configParam(START_PARAM, 1, 64, 1, "starting step");
        getParamQuantity(START_PARAM)->snapEnabled = true;
        configParam(PROB_PARAM, 0, 1, 1, "step probability", " %", 0, 100);
        configInput(START_CV_INPUT, "starting step cv");
        getInputInfo(START_CV_INPUT)->description = "0V to 10V";
        configInput(PROB_CV_INPUT, "step probability cv");
        getInputInfo(PROB_CV_INPUT)->description = "0V to 10V";
        configOutput(EOC_OUTPUT, "end of cycle");
        configOutput(MOD_SEQUENCE_OUTPUT, "mod sequence");
        generate_sequence();
        generate_mod_sequence();
        if (use_global_contrast[SLIPS])
        {
            module_contrast[SLIPS] = global_contrast;
        }
    }

    // the sequence
    // float the_sequence[64] = {0.0f};
    std::vector<float> the_sequence = std::vector<float>(MAX_STEPS, 0.0f);
    // mod sequence
    std::vector<float> mod_sequence = std::vector<float>(MAX_STEPS, 0.0f);
    // the slips
    // float the_slips[64] = {0.0f};
    std::vector<float> the_slips = std::vector<float>(MAX_STEPS, 0.0f);
    // the number of steps gone through in this cycle
    int steps_gone_through = 0;
    // the current step
    int current_step = 0;
    // the last value
    float last_value = 0.0f;
    // schmitt trigger for clock input
    dsp::SchmittTrigger clock_trigger;
    // schmitt trigger for reset input
    dsp::SchmittTrigger reset_trigger;
    // schmitt trigger for generatee input
    dsp::SchmittTrigger generate_trigger;
    // schmitt trigger for generatee manual button
    dsp::SchmittTrigger generate_button_trigger;
    // schmitt trigger for modgen input
    dsp::SchmittTrigger modgen_trigger;
    // schmitt trigger for modgen manual button
    dsp::SchmittTrigger modgen_button_trigger;
    // pulse generator for end of cycle output
    dsp::PulseGenerator eoc_pulse;
    // a bool to check if slips have already been generated for this cycle
    bool slips_generated = false;
    // a bool to check if root note input expects 0-10V or a v/oct value
    bool root_input_voct = false;
    // track whether the current step should be skipped
    bool skip_step = false;
    // a bool to track if slipspander is connected
    bool expanded = false;
    // a bool to track if slipspander was connected last cycle
    bool was_expanded = false;
    // a custom scale array in case slipspander is connected
    int *custom_scale = NULL;
    // the length of the custom scale
    int custom_scale_len = 0;
    // the starting step
    int starting_step = 0;
    // the last frame's starting step
    int last_starting_step = 0;
    // output channels
    int channels = 0;
    // current channel
    int curr_channel = 0;
    // toggle poly mod output
    bool poly_mod = true;

    // a cv range object to convert voltages with a range of 0V to 1V into a given range
    CVRange cv_range;
    // a cv range object to do the same for the mod sequence
    CVRange mod_range;
    // a cv range object to do the same for the slip range setting
    CVRange slip_range;

    // a bool to track whether to quantize the mod sequence
    bool mod_quantize = false;
    // a bool to track if slips should be added to the mod sequence
    bool mod_add_slips = false;
    // a bool to track if step probability should be added to the mod sequence
    bool mod_add_prob = false;
    // a float to track the last mod sequence value
    float last_mod_value = 0.0f;

    json_t *dataToJson() override;
    void dataFromJson(json_t *rootJ) override;
    void get_custom_scale();
    void generate_sequence();
    void generate_mod_sequence();
    void generate_slips(float slip_amount);
    void process(const ProcessArgs &args) override;
    void onReset(const ResetEvent &e) override;
};

struct SlipsWidget : ModuleWidget
{
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
    SlipsWidget(Slips *module)
    {
        setModule(module);
        svgPanel = createPanel(asset::plugin(pluginInstance, "res/slips.svg"));
        setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

        addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(30.279, 24.08)), module, Slips::ROOT_PARAM));
        addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(51.782, 24.08)), module, Slips::STEPS_PARAM));
        addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(30.279, 41.974)), module, Slips::SCALE_PARAM));
        addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(51.782, 41.974)), module, Slips::START_PARAM));
        addParam(createParamCentered<LEDButton>(mm2px(Vec(18.254, 59.869)), module, Slips::GENERATE_PARAM));
        addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(51.782, 59.869)), module, Slips::PROB_PARAM));
        addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(51.782, 77.763)), module, Slips::SLIPS_PARAM));
        addParam(createParamCentered<LEDButton>(mm2px(Vec(18.245, 77.763)), module, Slips::MODGEN_PARAM));

        addOutput(createOutputCentered<BitPort>(mm2px(Vec(30.279, 59.869)), module, Slips::EOC_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(34.279, 56.869)), module, Slips::EOC_LIGHT));

        addInput(createInputCentered<BitPort>(mm2px(Vec(8.854, 24.08)), module, Slips::CLOCK_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(21.082, 24.08)), module, Slips::ROOT_CV_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(42.586, 24.08)), module, Slips::STEPS_CV_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(8.854, 41.974)), module, Slips::RESET_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(21.082, 41.974)), module, Slips::SCALE_CV_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(42.586, 41.974)), module, Slips::START_CV_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(8.556, 59.869)), module, Slips::GENERATE_TRIGGER_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(42.586, 59.869)), module, Slips::PROB_CV_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(8.258, 77.763)), module, Slips::MODGEN_TRIGGER_INPUT));
        addInput(createInputCentered<BitPort>(mm2px(Vec(42.586, 77.763)), module, Slips::SLIPS_CV_INPUT));

        addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.854, 95.657)), module, Slips::SEQUENCE_OUTPUT));
        addOutput(createOutputCentered<BitPort>(mm2px(Vec(18.763, 95.657)), module, Slips::GATE_OUTPUT));
        addOutput(createOutputCentered<BitPort>(mm2px(Vec(28.882, 95.657)), module, Slips::SLIP_GATE_OUTPUT));
        addOutput(createOutputCentered<BitPort>(mm2px(Vec(38.991, 95.657)), module, Slips::MOD_SEQUENCE_OUTPUT));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(23.255, 92.872)), module, Slips::GATE_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(33.541, 92.872)), module, Slips::SLIP_GATE_LIGHT));

        float start_x = box.size.x / 2 - RACK_GRID_WIDTH * 4.5;
        float start_y = box.size.y - RACK_GRID_WIDTH * 5;
        float x = start_x;
        float y = start_y;

        SegmentDisplay *first_segment_display = createWidget<SegmentDisplay>(Vec(x, y));
        first_segment_display->box.size = Vec(RACK_GRID_WIDTH * 9, RACK_GRID_WIDTH / 2);
        first_segment_display->setLights<RedLight>(module, Slips::SEGMENT_LIGHTS, 16);
        addChild(first_segment_display);
        y += RACK_GRID_WIDTH / 2;
        SegmentDisplay *second_segment_display = createWidget<SegmentDisplay>(Vec(x, y));
        second_segment_display->box.size = Vec(RACK_GRID_WIDTH * 9, RACK_GRID_WIDTH / 2);
        second_segment_display->setLights<RedLight>(module, Slips::SEGMENT_LIGHTS + 16, 16);
        addChild(second_segment_display);
        y += RACK_GRID_WIDTH / 2;
        SegmentDisplay *third_segment_display = createWidget<SegmentDisplay>(Vec(x, y));
        third_segment_display->box.size = Vec(RACK_GRID_WIDTH * 9, RACK_GRID_WIDTH / 2);
        third_segment_display->setLights<RedLight>(module, Slips::SEGMENT_LIGHTS + 32, 16);
        addChild(third_segment_display);
        y += RACK_GRID_WIDTH / 2;
        SegmentDisplay *fourth_segment_display = createWidget<SegmentDisplay>(Vec(x, y));
        fourth_segment_display->box.size = Vec(RACK_GRID_WIDTH * 9, RACK_GRID_WIDTH / 2);
        fourth_segment_display->setLights<RedLight>(module, Slips::SEGMENT_LIGHTS + 48, 16);
        addChild(fourth_segment_display);

        // place the 'expanded' light at the bottom right corner of the module
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(box.size.x - RACK_GRID_WIDTH / 2, box.size.y - RACK_GRID_WIDTH / 2), module, Slips::EXPANDED_LIGHT));
    }
    void step() override;
    void appendContextMenu(Menu *menu) override;
    void addExpander();
};