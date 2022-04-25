#include "plugin.hpp"
#include "inc/SimplexNoise.cpp"

#define MAX_POLY 16

struct Simplexandhold : Module {
	enum ParamId {
		RANGE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SAMPLE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SimplexNoise noise;
	dsp::SchmittTrigger trigger[MAX_POLY];
	double last_sample[MAX_POLY] = {0.0};
	double x[MAX_POLY] = {0.0};
	double range = 1.0;

	Simplexandhold() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(RANGE_PARAM, 0, 2, 0, "noise range", {"1", "3", "5"});
		configInput(TRIGGER_INPUT, "trigger");
		configOutput(SAMPLE_OUTPUT, "sample");
		noise.init();
	}

	void process(const ProcessArgs& args) override {
		int chans = std::max(1, inputs[TRIGGER_INPUT].getChannels());
		outputs[SAMPLE_OUTPUT].setChannels(chans);
		// get the desired output range (-1.0 to 1.0, -3.0 to 3.0, -5.0 to 5.0)
		double range_param = params[RANGE_PARAM].getValue();
		if (range_param == 0.0) {
			range = 1.0;
		}
		else if (range_param == 1.0) {
			range = 3.0;
		}
		else if (range_param == 2.0) {
			range = 5.0;
		}
		for (int c = 0; c < chans; c++) {
			if (trigger[c].process(inputs[TRIGGER_INPUT].getVoltage(c))) {
				last_sample[c] = noise.noise(x[c], 0.0) * range;
			}
			x[c] += args.sampleTime;
			outputs[SAMPLE_OUTPUT].setVoltage(last_sample[c], c);
		}
	}
};


struct SimplexandholdWidget : ModuleWidget {
	SimplexandholdWidget(Simplexandhold* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/simplexandhold.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CKSSThreeHorizontal>(mm2px(Vec(10.16, 78.539)), module, Simplexandhold::RANGE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 35.937)), module, Simplexandhold::TRIGGER_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 100.446)), module, Simplexandhold::SAMPLE_OUTPUT));
	}
};


Model* modelSimplexandhold = createModel<Simplexandhold, SimplexandholdWidget>("simplexandhold");