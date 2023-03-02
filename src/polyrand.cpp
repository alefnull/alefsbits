#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "widgets/BitPort.hpp"

#define MAX_POLY 16

struct Polyrand : ThemeableModule {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		TRIGGER_INPUT,
		POLY_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		RANDOM_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	dsp::SchmittTrigger trigger[MAX_POLY];
	float last_value[MAX_POLY] = { 0.f };
	int current_channel[MAX_POLY] = { 0 };

	Polyrand() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(TRIGGER_INPUT, "");
		configInput(POLY_INPUT, "");
		configOutput(RANDOM_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		int trig_channels = inputs[TRIGGER_INPUT].isConnected() ? inputs[TRIGGER_INPUT].getChannels() : 1;
		if (trig_channels > MAX_POLY) {
			trig_channels = MAX_POLY;
		}
		outputs[RANDOM_OUTPUT].setChannels(trig_channels);

		if (inputs[TRIGGER_INPUT].isConnected() && 
				inputs[POLY_INPUT].isConnected() && 
				outputs[RANDOM_OUTPUT].isConnected()) {

			int poly_channels = inputs[POLY_INPUT].getChannels();
			if (poly_channels > MAX_POLY) {
				poly_channels = MAX_POLY;
			}
			for (int i = 0; i < trig_channels; i++) {
				if (trigger[i].process(inputs[TRIGGER_INPUT].getVoltage(i))) {
					int chan = random::u32() % poly_channels;
					current_channel[i] = chan;
				}
				last_value[i] = inputs[POLY_INPUT].getVoltage(current_channel[i]);
			}

			for (int i = 0; i < trig_channels; i++) {
				outputs[RANDOM_OUTPUT].setVoltage(last_value[i], i);
			}
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "contrast", json_real(contrast));
		json_object_set_new(rootJ, "use_global_contrast", json_boolean(use_global_contrast));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* contrastJ = json_object_get(rootJ, "contrast");
		if (contrastJ) {
			contrast = json_number_value(contrastJ);
		}
		json_t* use_global_contrastJ = json_object_get(rootJ, "use_global_contrast");
		if (use_global_contrastJ) {
			use_global_contrast = json_boolean_value(use_global_contrastJ);
		}
	}
};


struct PolyrandWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	PolyrandWidget(Polyrand* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/polyrand.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addInput(createInputCentered<BitPort>(mm2px(Vec(5.08, 30.154)), module, Polyrand::TRIGGER_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.08, 46.058)), module, Polyrand::POLY_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 105.127)), module, Polyrand::RANDOM_OUTPUT));
	}

	void step() override {
		Polyrand* randModule = dynamic_cast<Polyrand*>(this->module);
		if (!randModule) return;
		if (randModule->contrast != randModule->global_contrast) {
			randModule->use_global_contrast = false;
		}
		if (randModule->contrast != panelBackground->contrast) {
			panelBackground->contrast = randModule->contrast;
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
        Polyrand* module = dynamic_cast<Polyrand*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module->contrast));
            contrastSlider->box.size.x = 200.f;
            contrastMenu->addChild(createMenuItem("use global contrast",
                CHECKMARK(module->use_global_contrast),
                [module]() { 
                    module->use_global_contrast = !module->use_global_contrast;
                    if (module->use_global_contrast) {
                        module->load_global_contrast();
                        module->contrast = module->global_contrast;
                    }
                }));
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
                    module->save_global_contrast(module->contrast);
                }));
            menu->addChild(contrastMenu);
        }));
	}
};


Model* modelPolyrand = createModel<Polyrand, PolyrandWidget>("polyrand");