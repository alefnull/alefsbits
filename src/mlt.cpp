#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "widgets/BitPort.hpp"

#define MAX_POLY 16

struct Mlt : ThemeableModule {
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

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "contrast", json_real(contrast));
		json_object_set_new(rootJ, "use_global_contrast", json_boolean(use_global_contrast));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* contrastJ = json_object_get(rootJ, "contrast");
		if (contrastJ)
			contrast = json_number_value(contrastJ);
		json_t* use_global_contrastJ = json_object_get(rootJ, "use_global_contrast");
		if (use_global_contrastJ)
			use_global_contrast = json_boolean_value(use_global_contrastJ);
	}
};


struct MltWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	MltWidget(Mlt* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/mlt.svg"));
		setPanel(svgPanel);
        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addInput(createInputCentered<BitPort>(mm2px(Vec(5.08, 14.679)), module, Mlt::A_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.08, 67.158)), module, Mlt::B_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 24.849)), module, Mlt::A1_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 32.963)), module, Mlt::A2_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 41.078)), module, Mlt::A3_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 49.192)), module, Mlt::A4_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 57.307)), module, Mlt::A5_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 77.407)), module, Mlt::B1_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 85.726)), module, Mlt::B2_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 94.044)), module, Mlt::B3_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 102.363)), module, Mlt::B4_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(5.08, 110.682)), module, Mlt::B5_OUTPUT));
	}
	
	void step() override {
		Mlt* mltModule = dynamic_cast<Mlt*>(this->module);
		if (!mltModule) return;
		if (mltModule->contrast != mltModule->global_contrast) {
			mltModule->use_global_contrast = false;
		}
		if (mltModule->contrast != panelBackground->contrast) {
			panelBackground->contrast = mltModule->contrast;
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
        Mlt* module = dynamic_cast<Mlt*>(this->module);
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


Model* modelMlt = createModel<Mlt, MltWidget>("mlt");