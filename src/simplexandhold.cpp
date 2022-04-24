#include "plugin.hpp"
#include "inc/SimplexNoise.cpp"


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
	dsp::SchmittTrigger trigger;
	double last_sample = 0.0;
	double x = 0.0;
	double range = 1.0;

	Simplexandhold() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RANGE_PARAM, 0.0, 2.0, 0.0, "noise range", "" );
		getParamQuantity(RANGE_PARAM)->snapEnabled = true;
		configInput(TRIGGER_INPUT, "trigger");
		configOutput(SAMPLE_OUTPUT, "sample");
		noise.init();
	}

	void process(const ProcessArgs& args) override {
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
		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			last_sample = noise.noise(x, 0.0) * range;
		}
		x += args.sampleTime;
		outputs[SAMPLE_OUTPUT].setVoltage(last_sample);
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 79.539)), module, Simplexandhold::RANGE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 35.937)), module, Simplexandhold::TRIGGER_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 100.446)), module, Simplexandhold::SAMPLE_OUTPUT));
	}
};


Model* modelSimplexandhold = createModel<Simplexandhold, SimplexandholdWidget>("simplexandhold");