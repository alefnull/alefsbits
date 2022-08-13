#include "plugin.hpp"


struct Sswitch : Module {
	enum ParamId {
		MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIGGER_INPUT,
		SIGNAL1_INPUT,
		SIGNAL2_INPUT,
		SIGNAL3_INPUT,
		SIGNAL4_INPUT,
		SIGNAL5_INPUT,
		SIGNAL6_INPUT,
		SIGNAL7_INPUT,
		SIGNAL8_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		SIGNAL1_LIGHT,
		SIGNAL2_LIGHT,
		SIGNAL3_LIGHT,
		SIGNAL4_LIGHT,
		SIGNAL5_LIGHT,
		SIGNAL6_LIGHT,
		SIGNAL7_LIGHT,
		SIGNAL8_LIGHT,
		LIGHTS_LEN
	};

	Sswitch() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MODE_PARAM, 0.f, 3.f, 0.f, "mode");
		getParamQuantity(MODE_PARAM)->snapEnabled = true;
		configInput(TRIGGER_INPUT, "trigger");
		configInput(SIGNAL1_INPUT, "signal 1");
		configInput(SIGNAL2_INPUT, "signal 2");
		configInput(SIGNAL3_INPUT, "signal 3");
		configInput(SIGNAL4_INPUT, "signal 4");
		configInput(SIGNAL5_INPUT, "signal 5");
		configInput(SIGNAL6_INPUT, "signal 6");
		configInput(SIGNAL7_INPUT, "signal 7");
		configInput(SIGNAL8_INPUT, "signal 8");
		configLight(SIGNAL1_LIGHT, "signal 1");
		configLight(SIGNAL2_LIGHT, "signal 2");
		configLight(SIGNAL3_LIGHT, "signal 3");
		configLight(SIGNAL4_LIGHT, "signal 4");
		configLight(SIGNAL5_LIGHT, "signal 5");
		configLight(SIGNAL6_LIGHT, "signal 6");
		configLight(SIGNAL7_LIGHT, "signal 7");
		configLight(SIGNAL8_LIGHT, "signal 8");
		configOutput(SIGNAL_OUTPUT, "signal");
	}

	dsp::SchmittTrigger trigger;
	int mode = 0;
	int index = 0;
	int index_old = 0;
	bool going_up = true;

	enum Mode {
		UP,
		DOWN,
		PINGPONG,
		RANDOM
	};

	void process(const ProcessArgs& args) override {
		bool trig = trigger.process(inputs[TRIGGER_INPUT].getVoltage());
		mode = (int)roundf(params[MODE_PARAM].getValue());
		if (trig) {
			switch(mode) {
				case UP:
					index++;
					if (index >= 8) {
						index = 0;
					}
					break;
				case DOWN:
					index--;
					if (index < 0) {
						index = 7;
					}
					break;
				case PINGPONG:
					if (going_up) {
						index++;
						if (index >= 8) {
							index = 6;
							going_up = false;
						}
					} else {
						index--;
						if (index < 0) {
							index = 1;
							going_up = true;
						}
					}
					break;
				case RANDOM:
					index = random::u32() % 8;
					break;
			}
		}
		for (int i = 0; i < 8; i++) {
			lights[i].setBrightness(i == index ? 1.0f : 0.0f);
		}
		outputs[SIGNAL_OUTPUT].setVoltage(inputs[index + 1].getVoltage());
	}
};


struct SswitchWidget : ModuleWidget {
	SswitchWidget(Sswitch* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/switch.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(15.127, 19.131)), module, Sswitch::MODE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.377, 19.131)), module, Sswitch::TRIGGER_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.16, 34.03)), module, Sswitch::SIGNAL1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.16, 43.044)), module, Sswitch::SIGNAL2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.16, 51.873)), module, Sswitch::SIGNAL3_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.16, 60.887)), module, Sswitch::SIGNAL4_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.16, 69.716)), module, Sswitch::SIGNAL5_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.16, 78.729)), module, Sswitch::SIGNAL6_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.16, 87.559)), module, Sswitch::SIGNAL7_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.16, 96.572)), module, Sswitch::SIGNAL8_INPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 34.03)), module, Sswitch::SIGNAL1_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 43.044)), module, Sswitch::SIGNAL2_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 51.873)), module, Sswitch::SIGNAL3_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 60.887)), module, Sswitch::SIGNAL4_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 69.716)), module, Sswitch::SIGNAL5_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 78.729)), module, Sswitch::SIGNAL6_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 87.559)), module, Sswitch::SIGNAL7_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 96.572)), module, Sswitch::SIGNAL8_LIGHT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 109.265)), module, Sswitch::SIGNAL_OUTPUT));
	}
};


Model* modelSswitch = createModel<Sswitch, SswitchWidget>("sswitch");