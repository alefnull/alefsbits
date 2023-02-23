#include "plugin.hpp"


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

	float prev_input = 0.f;
	float prev_delta = 0.f;
	dsp::PulseGenerator pulse;

	void process(const ProcessArgs& args) override {
		// get the input value
		float input = inputs[SOURCE_INPUT].getVoltage();
		// calculate the delta between the current input value and the previous input value
		float delta = input - prev_input;
		
		// if the current delta is negative and the previous delta was positive
		// or if the current delta is positive and the previous delta was negative
		if ((delta < 0.f && prev_delta >= 0.f) || (delta > 0.f && prev_delta <= 0.f)) {
			// generate a trigger
			pulse.trigger(1e-3f);
		}

		// set the output
		// if the pulse generator is generating a trigger
		if (pulse.process(args.sampleTime)) {
			// set the output to high
			outputs[TRIG_OUTPUT].setVoltage(10.f);
		}
		// if the pulse generator is not generating a trigger
		else {
			// set the output to low
			outputs[TRIG_OUTPUT].setVoltage(0.f);
		}

		// set the last input value to the current input value
		prev_input = input;
		// set the last delta to the current delta
		prev_delta = delta;
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