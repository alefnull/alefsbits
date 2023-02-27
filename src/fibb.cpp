#include "plugin.hpp"


struct Fibb : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		RESET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		FIBB2_OUTPUT,
		FIBB3_OUTPUT,
		FIBB5_OUTPUT,
		FIBB8_OUTPUT,
		FIBB13_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FIBB2_LIGHT,
		FIBB3_LIGHT,
		FIBB5_LIGHT,
		FIBB8_LIGHT,
		FIBB13_LIGHT,
		LIGHTS_LEN
	};

	dsp::SchmittTrigger clock_trigger;
	dsp::SchmittTrigger reset_trigger;
	dsp::ClockDivider div_2;
	dsp::ClockDivider div_3;
	dsp::ClockDivider div_5;
	dsp::ClockDivider div_8;
	dsp::ClockDivider div_13;
	bool clock_high = false;
	bool out_2 = false;
	bool out_3 = false;
	bool out_5 = false;
	bool out_8 = false;
	bool out_13 = false;

	Fibb() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(CLOCK_INPUT, "clock");
		configInput(RESET_INPUT, "reset");
		configOutput(FIBB2_OUTPUT, "clock / 2");
		configOutput(FIBB3_OUTPUT, "clock / 3");
		configOutput(FIBB5_OUTPUT, "clock / 5");
		configOutput(FIBB8_OUTPUT, "clock / 8");
		configOutput(FIBB13_OUTPUT, "clock / 13");
		configLight(FIBB2_LIGHT, "clock / 2");
		configLight(FIBB3_LIGHT, "clock / 3");
		configLight(FIBB5_LIGHT, "clock / 5");
		configLight(FIBB8_LIGHT, "clock / 8");
		configLight(FIBB13_LIGHT, "clock / 13");
		div_2.setDivision(2);
		div_3.setDivision(3);
		div_5.setDivision(5);
		div_8.setDivision(8);
		div_13.setDivision(13);
	}

	void process(const ProcessArgs& args) override {
		if (reset_trigger.process(inputs[RESET_INPUT].getVoltage())) {
			reset();
		}
		clock_high = inputs[CLOCK_INPUT].getVoltage() > 0.0;
		if (!clock_high) {
			out_2 = false;
			out_3 = false;
			out_5 = false;
			out_8 = false;
			out_13 = false;
		}
		if (clock_trigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			if (div_2.process()) {
				out_2 = true;
			}
			if (div_3.process()) {
				out_3 = true;
			}
			if (div_5.process()) {
				out_5 = true;
			}
			if (div_8.process()) {
				out_8 = true;
			}
			if (div_13.process()) {
				out_13 = true;
			}
		}
		lights[FIBB2_LIGHT].setBrightness(out_2 && clock_high ? 1.0 : 0.0);
		lights[FIBB3_LIGHT].setBrightness(out_3 && clock_high ? 1.0 : 0.0);
		lights[FIBB5_LIGHT].setBrightness(out_5 && clock_high ? 1.0 : 0.0);
		lights[FIBB8_LIGHT].setBrightness(out_8 && clock_high ? 1.0 : 0.0);
		lights[FIBB13_LIGHT].setBrightness(out_13 && clock_high ? 1.0 : 0.0);
		outputs[FIBB2_OUTPUT].setVoltage(out_2 && clock_high ? 10.0 : 0.0);
		outputs[FIBB3_OUTPUT].setVoltage(out_3 && clock_high ? 10.0 : 0.0);
		outputs[FIBB5_OUTPUT].setVoltage(out_5 && clock_high ? 10.0 : 0.0);
		outputs[FIBB8_OUTPUT].setVoltage(out_8 && clock_high ? 10.0 : 0.0);
		outputs[FIBB13_OUTPUT].setVoltage(out_13 && clock_high ? 10.0 : 0.0);
	}

	void reset() {
		clock_trigger.reset();
		div_2.reset();
		div_3.reset();
		div_5.reset();
		div_8.reset();
		div_13.reset();
		clock_high = false;
		out_2 = false;
		out_3 = false;
		out_5 = false;
		out_8 = false;
		out_13 = false;
	}
};


struct FibbWidget : ModuleWidget {
	FibbWidget(Fibb* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/fibb.svg")));

		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.16, 22.719)), module, Fibb::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.16, 22.719)), module, Fibb::RESET_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.16, 37.177)), module, Fibb::FIBB2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.16, 53.028)), module, Fibb::FIBB3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.16, 68.88)), module, Fibb::FIBB5_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.16, 84.732)), module, Fibb::FIBB8_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.16, 100.583)), module, Fibb::FIBB13_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 37.177)), module, Fibb::FIBB2_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 53.028)), module, Fibb::FIBB3_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 68.88)), module, Fibb::FIBB5_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 84.732)), module, Fibb::FIBB8_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 100.583)), module, Fibb::FIBB13_LIGHT));
	}
};


Model* modelFibb = createModel<Fibb, FibbWidget>("fibb");