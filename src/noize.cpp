#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"


struct Noize : Module {
	enum ParamId {
		DURATION_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DURATION_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		NOISE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	enum RandomMode {
		UNIFORM,
		GAUSSIAN
	};

	Noize() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DURATION_PARAM, 0.0f, 0.001f, 0.0f, "duration");
		configInput(DURATION_INPUT, "duration cv");
		configOutput(NOISE_OUTPUT, "noize");
		if (use_global_contrast[NOIZE]) {
			module_contrast[NOIZE] = global_contrast;
		}
	}

	int randomMode = UNIFORM;
	float deviation = 0.5f;
	float last_value = 0.0f;
	float time = 0.0f;

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "randomMode", json_integer(randomMode));
		json_object_set_new(rootJ, "deviation", json_real(deviation));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* randomModeJ = json_object_get(rootJ, "randomMode");
		if (randomModeJ) {
			randomMode = json_integer_value(randomModeJ);
		}
		json_t* deviationJ = json_object_get(rootJ, "deviation");
		if (deviationJ) {
			deviation = json_real_value(deviationJ);
		}
	}

	void onReset() override {
		randomMode = UNIFORM;
		deviation = 0.5f;
		last_value = 0.0f;
		time = 0.0f;
	}

	void process(const ProcessArgs& args) override {
		float duration = params[DURATION_PARAM].getValue();
		if (inputs[DURATION_INPUT].isConnected()) {
			float cv = rescale(inputs[DURATION_INPUT].getVoltage(), 0.0f, 10.0f, 0.0f, 0.001f);
			duration = clamp(duration + cv, 0.0f, 0.001f);
		}
		if (time > duration) {
			if (randomMode == GAUSSIAN) {
				last_value = random::normal() * deviation;
			}
			else {
				last_value = random::uniform() * 2.0f - 1.0f;
			}
			time = 0;
		}
		time += args.sampleTime;
		outputs[NOISE_OUTPUT].setVoltage(clamp(last_value * 5.0f, -5.0f, 5.0f));
	}
};


struct NoizeWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	NoizeWidget(Noize* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/noize.svg"));
		setPanel(svgPanel);
        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addParam(createParamCentered<BitKnob>(mm2px(Vec(10.16, 26.056)), module, Noize::DURATION_PARAM));

		addInput(createInputCentered<BitPort>(mm2px(Vec(10.16, 42.056)), module, Noize::DURATION_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(10.16, 109.258)), module, Noize::NOISE_OUTPUT));
	}

	void step() override {
		Noize* noizeModule = dynamic_cast<Noize*>(this->module);
		if (!noizeModule) return;
		if (use_global_contrast[NOIZE]) {
			module_contrast[NOIZE] = global_contrast;
		}
		if (module_contrast[NOIZE] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[NOIZE];
			if (panelBackground->contrast < 0.4f) {
				panelBackground->invert(true);
				inverter->invert = true;
			}
			else {
				panelBackground->invert(false);
				inverter->invert = false;
			}
			svgPanel->fb->dirty = true;
		}
		ModuleWidget::step();
	}

	void appendContextMenu(Menu* menu) override {
        Noize* module = dynamic_cast<Noize*>(this->module);
        assert(module);
		
		menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[NOIZE]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[NOIZE]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                []() {
					global_contrast = module_contrast[NOIZE];
					use_global_contrast[NOIZE] = true;
                }));
            menu->addChild(contrastMenu);
        }));

        menu->addChild(new MenuSeparator());

		menu->addChild(createMenuLabel("random mode:"));
		menu->addChild(createCheckMenuItem("uniform", "",
			[=]() { return module->randomMode == Noize::UNIFORM; },
			[=]() { module->randomMode = Noize::UNIFORM; }));
		menu->addChild(createCheckMenuItem("gaussian", "",
			[=]() { return module->randomMode == Noize::GAUSSIAN; },
			[=]() { module->randomMode = Noize::GAUSSIAN; }));
		
		struct DeviationQuantity : Quantity {
			float* deviation;
			DeviationQuantity(float* deviation) {
				this->deviation = deviation;
			}
			void setValue(float value) override {
				*deviation = clamp(value, 0.1f, 0.9f);
			}
			float getValue() override {
				return *deviation;
			}
			float getDefaultValue() override {
				return 0.5f;
			}
			float getDisplayValue() override {
				return getValue();
			}
			void setDisplayValue(float displayValue) override {
				setValue(displayValue);
			}
			std::string getLabel() override {
				return "gaussian deviation";
			}
			std::string getUnit() override {
				return "";
			}
			int getDisplayPrecision() override {
				return 2;
			}
			float getMinValue() override {
				return 0.1f;
			}
			float getMaxValue() override {
				return 0.9f;
			}
		};

		struct DeviationSlider : ui::Slider {
			DeviationSlider(float* value) {
				quantity = new DeviationQuantity(value);
			}
			~DeviationSlider() {
				delete quantity;
			}
		};

		menu->addChild(new MenuSeparator());

		DeviationSlider *deviationSlider = new DeviationSlider(&(module->deviation));
		deviationSlider->box.size.x = 200.f;
		menu->addChild(deviationSlider);
	}
};


Model* modelNoize = createModel<Noize, NoizeWidget>("noize");