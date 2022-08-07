#include "plugin.hpp"


struct Shift : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT,
		TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		REGISTER_1_OUTPUT,
		REGISTER_2_OUTPUT,
		REGISTER_3_OUTPUT,
		REGISTER_4_OUTPUT,
		REGISTER_5_OUTPUT,
		REGISTER_6_OUTPUT,
		REGISTER_7_OUTPUT,
		REGISTER_8_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Shift() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(SIGNAL_INPUT, "signal");
		configInput(TRIGGER_INPUT, "trigger");
		configOutput(REGISTER_1_OUTPUT, "register 1");
		configOutput(REGISTER_2_OUTPUT, "register 2");
		configOutput(REGISTER_3_OUTPUT, "register 3");
		configOutput(REGISTER_4_OUTPUT, "register 4");
		configOutput(REGISTER_5_OUTPUT, "register 5");
		configOutput(REGISTER_6_OUTPUT, "register 6");
		configOutput(REGISTER_7_OUTPUT, "register 7");
		configOutput(REGISTER_8_OUTPUT, "register 8");
	}

	float last_sample[OUTPUTS_LEN] = {0.0};
	dsp::SchmittTrigger trigger;

	void process(const ProcessArgs& args) override {
		if (!inputs[TRIGGER_INPUT].isConnected() || !inputs[SIGNAL_INPUT].isConnected()) {
			for (int i = 0; i < OUTPUTS_LEN; i++) {
				last_sample[i] = 0.0;
				outputs[i].setVoltage(last_sample[i]);
			}
			return;
		}

		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			for (int i = OUTPUTS_LEN - 1; i > 0; i--) {
				last_sample[i] = last_sample[i - 1];
			}
			last_sample[0] = inputs[SIGNAL_INPUT].getVoltage();

			for (int i = 0; i < OUTPUTS_LEN; i++) {
				outputs[i].setVoltage(last_sample[i]);
			}
		}
	}
};


struct ShiftWidget : ModuleWidget {
	ShiftWidget(Shift* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/shift.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.347, 21.057)), module, Shift::SIGNAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.165, 21.057)), module, Shift::TRIGGER_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 41.655)), module, Shift::REGISTER_1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 50.764)), module, Shift::REGISTER_2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 59.874)), module, Shift::REGISTER_3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 68.983)), module, Shift::REGISTER_4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 78.092)), module, Shift::REGISTER_5_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 87.202)), module, Shift::REGISTER_6_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 96.311)), module, Shift::REGISTER_7_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 105.42)), module, Shift::REGISTER_8_OUTPUT));
	}
};


Model* modelShift = createModel<Shift, ShiftWidget>("shift");