#include "plugin.hpp"

#define BUFFER_SIZE 10

struct Trigs : Module {
	enum ParamId {
		MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SOURCE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TRIG_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Trigs() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "mode", {"direction", "through zero", "both"});
		configInput(SOURCE_INPUT, "source");
		configOutput(TRIG_OUTPUT, "trigger");
	}

	float samples[BUFFER_SIZE] = { 0.f };
	float deltas[BUFFER_SIZE - 1] = { 0.f };
	dsp::PulseGenerator pulse;

	void process(const ProcessArgs& args) override {
		for (int i = 0; i < BUFFER_SIZE - 1; i++) {
			samples[i] = samples[i + 1];
		}
		samples[BUFFER_SIZE - 1] = inputs[SOURCE_INPUT].getVoltage();

		for (int i = 0; i < BUFFER_SIZE - 1; i++) {
			deltas[i] = samples[i + 1] - samples[i];
		}

		bool trig = true;
		// if all deltas are 0, positive, or negative, no trigger
		for (int i = 0; i < BUFFER_SIZE - 1; i++) {
			if (deltas[i] != 0.f) {
				trig = false;
				break;
			}
		}

		if (trig) {
			pulse.trigger(1e-3f);
		}

		outputs[TRIG_OUTPUT].setVoltage(pulse.process(args.sampleTime) ? 10.f : 0.f);
	}
};


struct TrigsWidget : ModuleWidget {
	TrigsWidget(Trigs* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/trigs.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CKSSThree>(mm2px(Vec(10.16, 64.25)), module, Trigs::MODE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 49.598)), module, Trigs::SOURCE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 78.902)), module, Trigs::TRIG_OUTPUT));
	}
};


Model* modelTrigs = createModel<Trigs, TrigsWidget>("trigs");