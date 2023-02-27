#include "plugin.hpp"

#define MAX_POLY 16

struct Mlt : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		A_INPUT,
		B_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		A1_OUTPUT,
		A2_OUTPUT,
		A3_OUTPUT,
		A4_OUTPUT,
		A5_OUTPUT,
		B1_OUTPUT,
		B2_OUTPUT,
		B3_OUTPUT,
		B4_OUTPUT,
		B5_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Mlt() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(A_INPUT, "");
		configInput(B_INPUT, "");
		configOutput(A1_OUTPUT, "");
		configOutput(A2_OUTPUT, "");
		configOutput(A3_OUTPUT, "");
		configOutput(A4_OUTPUT, "");
		configOutput(A5_OUTPUT, "");
		configOutput(B1_OUTPUT, "");
		configOutput(B2_OUTPUT, "");
		configOutput(B3_OUTPUT, "");
		configOutput(B4_OUTPUT, "");
		configOutput(B5_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		int a_channels = inputs[A_INPUT].getChannels();
		int b_channels = inputs[B_INPUT].getChannels();
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			if (i < a_channels) {
				outputs[i].setChannels(a_channels);
			} else {
				outputs[i].setChannels(b_channels);
			}
		}
		float a = inputs[A_INPUT].getVoltage();
		float b = inputs[B_INPUT].getVoltage();
		for (int i = 0; i < OUTPUTS_LEN / 2; i++) {
			for (int chan = 0; chan < a_channels; chan++) {
				outputs[i].setVoltage(a, chan);
			}
		}
		for (int i = OUTPUTS_LEN / 2; i < OUTPUTS_LEN; i++) {
			for (int chan = 0; chan < b_channels; chan++) {
				outputs[i].setVoltage(b, chan);
			}
		}
	}
};


struct MltWidget : ModuleWidget {
	MltWidget(Mlt* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/mlt.svg")));

		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 14.679)), module, Mlt::A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 67.158)), module, Mlt::B_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 24.849)), module, Mlt::A1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 32.963)), module, Mlt::A2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 41.078)), module, Mlt::A3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 49.192)), module, Mlt::A4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 57.307)), module, Mlt::A5_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 77.407)), module, Mlt::B1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 85.726)), module, Mlt::B2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 94.044)), module, Mlt::B3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 102.363)), module, Mlt::B4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 110.682)), module, Mlt::B5_OUTPUT));
	}
};


Model* modelMlt = createModel<Mlt, MltWidget>("mlt");