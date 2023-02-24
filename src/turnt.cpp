#include "plugin.hpp"

#define MAX_POLY 16
#define BUFFER_SIZE 3

struct Turnt : Module {
	enum ParamId {
		MODE_PARAM,
		ZERO_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SOURCE_INPUT,
		ZERO_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TRIG_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Turnt() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "mode", {"direction", "through zero", "both"});
		configParam(ZERO_PARAM, -10.f, 10.f, 0.f, "zero threshold");
		configInput(SOURCE_INPUT, "source");
		configInput(ZERO_INPUT, "zero threshold");
		configOutput(TRIG_OUTPUT, "trigger");
	}

	float samples[BUFFER_SIZE] = { 0.f };
	dsp::PulseGenerator pulse;

	void process(const ProcessArgs& args) override {
		int mode = params[MODE_PARAM].getValue();
		float zero = params[ZERO_PARAM].getValue();
		if (inputs[ZERO_INPUT].isConnected()) {
			zero = inputs[ZERO_INPUT].getVoltage();
		}

		auto in = inputs[SOURCE_INPUT].getVoltage();
		if (in != samples[BUFFER_SIZE-1]) {
			for (int i = 0; i < BUFFER_SIZE - 1; i++) {
				samples[i] = samples[i + 1];
			}
			samples[BUFFER_SIZE - 1] = inputs[SOURCE_INPUT].getVoltage();

			auto d1 = samples[1]-samples[0];
			auto d2 = samples[2]-samples[1];

			bool trig = false;

			switch (mode) {
				case 0: // direction
					trig = ((d1 > 0.f && d2 < 0.f) || (d1 < 0.f && d2 > 0.f));
					break;
				case 1: // through zero
					trig = (samples[1] > zero && samples[2] <= zero) || 
							(samples[1] < zero && samples[2] >= zero);
					break;
				case 2: // both
					trig = (((d1 > 0.f && d2 < 0.f) || (d1 < 0.f && d2 > 0.f)) || 
							(samples[1] > zero && samples[2] <= zero) || 
							(samples[1] < zero && samples[2] >= zero));
					break;
			}

			if (trig) {
				pulse.trigger(1e-3f);
			}
		}

		outputs[TRIG_OUTPUT].setVoltage(pulse.process(args.sampleTime) ? 10.f : 0.f);
	}
};


struct TurntWidget : ModuleWidget {
	TurntWidget(Turnt* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/turnt.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		float x_start = box.size.x / 2;
		float y_start = RACK_GRID_WIDTH * 6;
		float dy = RACK_GRID_WIDTH * 3;

		addInput(createInputCentered<PJ301MPort>(Vec(x_start, y_start), module, Turnt::SOURCE_INPUT));
		addParam(createParamCentered<CKSSThree>(Vec(x_start, y_start + dy), module, Turnt::MODE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(Vec(x_start, y_start + dy * 2), module, Turnt::ZERO_PARAM));
		addInput(createInputCentered<PJ301MPort>(Vec(x_start, y_start + dy * 3), module, Turnt::ZERO_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(x_start, y_start + dy * 4), module, Turnt::TRIG_OUTPUT));
	}
};


Model* modelTurnt = createModel<Turnt, TurntWidget>("turnt");