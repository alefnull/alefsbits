#pragma once

#include "plugin.hpp"
// #include "stepspander.hpp"

struct Steps : Module {
	enum ParamId {
		STEPS_PARAM,
		STEP1_PARAM,
		STEP2_PARAM,
		STEP3_PARAM,
		STEP4_PARAM,
		STEP5_PARAM,
		STEP6_PARAM,
		STEP7_PARAM,
		STEP8_PARAM,
		RAND_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		RESET_INPUT,
		RAND_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		EOC_OUTPUT,
		CV_OUTPUT,
		STEP1_OUTPUT,
		STEP2_OUTPUT,
		STEP3_OUTPUT,
		STEP4_OUTPUT,
		STEP5_OUTPUT,
		STEP6_OUTPUT,
		STEP7_OUTPUT,
		STEP8_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		STEP1_LIGHT,
		STEP2_LIGHT,
		STEP3_LIGHT,
		STEP4_LIGHT,
		STEP5_LIGHT,
		STEP6_LIGHT,
		STEP7_LIGHT,
		STEP8_LIGHT,
		LIGHTS_LEN
	};

	dsp::PulseGenerator eoc_pulse;
	dsp::PulseGenerator rand_pulse;
	dsp::SchmittTrigger clock_trigger;
	dsp::SchmittTrigger rand_trigger;
	dsp::SchmittTrigger prand_trigger;
	dsp::SchmittTrigger reset_trigger;
	dsp::PulseGenerator step_pulse[8];
	bool reset_queued = false;
	bool latch = true;
	bool unipolar = false;
	int step = 0;
	int steps = 8;
	int range = 1;

	Steps() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(STEPS_PARAM, 1.f, 8.f, 8.f, "steps");
		getParamQuantity(STEPS_PARAM)->snapEnabled = true;
		configParam(STEP1_PARAM, -1.f, 1.f, 0.f, "step 1 cv");
		configParam(STEP2_PARAM, -1.f, 1.f, 0.f, "step 2 cv");
		configParam(STEP3_PARAM, -1.f, 1.f, 0.f, "step 3 cv");
		configParam(STEP4_PARAM, -1.f, 1.f, 0.f, "step 4 cv");
		configParam(STEP5_PARAM, -1.f, 1.f, 0.f, "step 5 cv");
		configParam(STEP6_PARAM, -1.f, 1.f, 0.f, "step 6 cv");
		configParam(STEP7_PARAM, -1.f, 1.f, 0.f, "step 7 cv");
		configParam(STEP8_PARAM, -1.f, 1.f, 0.f, "step 8 cv");
		configParam(RAND_PARAM, 0.f, 10.f, 0.f, "randomize steps");
		configInput(CLOCK_INPUT, "clock");
		configInput(RESET_INPUT, "reset");
		configInput(RAND_INPUT, "random trigger");
		configOutput(EOC_OUTPUT, "end of cycle");
		configOutput(CV_OUTPUT, "cv");
		configOutput(STEP1_OUTPUT, "step 1");
		configOutput(STEP2_OUTPUT, "step 2");
		configOutput(STEP3_OUTPUT, "step 3");
		configOutput(STEP4_OUTPUT, "step 4");
		configOutput(STEP5_OUTPUT, "step 5");
		configOutput(STEP6_OUTPUT, "step 6");
		configOutput(STEP7_OUTPUT, "step 7");
		configOutput(STEP8_OUTPUT, "step 8");
		configLight(STEP1_LIGHT, "step 1");
		configLight(STEP2_LIGHT, "step 2");
		configLight(STEP3_LIGHT, "step 3");
		configLight(STEP4_LIGHT, "step 4");
		configLight(STEP5_LIGHT, "step 5");
		configLight(STEP6_LIGHT, "step 6");
		configLight(STEP7_LIGHT, "step 7");
		configLight(STEP8_LIGHT, "step 8");
	}

    void process(const ProcessArgs& args) override;
    void advance_lights(int step);
    void advance_gate_outputs(int step);
    void randomize_steps();
};