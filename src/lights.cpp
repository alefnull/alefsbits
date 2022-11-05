#include "plugin.hpp"


struct Lights : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(GATE_INPUT, 8),
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(GATE_LIGHT, 8),
		LIGHTS_LEN
	};

	Lights() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	}

	bool latch = false;
	bool gate[8] = { false };
	dsp::SchmittTrigger gate_trigger[8];

	void process(const ProcessArgs& args) override {
		if (latch) {
			for (int i = 0; i < 8; i++) {
				if (gate_trigger[i].process(inputs[GATE_INPUT + i].getVoltage())) {
					gate[i] = !gate[i];
				}
				lights[GATE_LIGHT + i].setBrightness(gate[i] ? 1.f : 0.f);
			}
		}
		else {
			for (int i = 0; i < 8; i++) {
				lights[GATE_LIGHT + i].setBrightness(inputs[GATE_INPUT + i].getVoltage() > 5.0f ? 1.0f : 0.0f);
			}
		}
	}
};


struct LightsWidget : ModuleWidget {
	LightsWidget(Lights* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/lights.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		float x = RACK_GRID_WIDTH;
		float y = RACK_GRID_WIDTH;
		float x_start = x;
		float y_start = y * 6;
		float dx = RACK_GRID_WIDTH * 2;
		float dy = RACK_GRID_WIDTH * 2;

		addInput(createInputCentered<PJ301MPort>(Vec(x, y_start), module, Lights::GATE_INPUT + 0));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y_start), module, Lights::GATE_LIGHT + 0));

		y = y_start + dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Lights::GATE_INPUT + 1));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 1));

		y = y + dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Lights::GATE_INPUT + 2));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 2));

		y = y + dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Lights::GATE_INPUT + 3));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 3));

		y = y + dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Lights::GATE_INPUT + 4));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 4));

		y = y + dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Lights::GATE_INPUT + 5));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 5));

		y = y + dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Lights::GATE_INPUT + 6));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 6));

		y = y + dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Lights::GATE_INPUT + 7));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 7));
	}

	void appendContextMenu(Menu* menu) override {
		Lights* module = dynamic_cast<Lights*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Latch", CHECKMARK(module->latch), [module]() {
			module->latch = !module->latch;
		}));
	}
};


Model* modelLights = createModel<Lights, LightsWidget>("lights");