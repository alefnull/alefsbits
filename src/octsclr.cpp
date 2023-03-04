#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "widgets/BitPort.hpp"

#define MAX_POLY 16


struct Octsclr : Module {
	enum ParamId {
		SCALER_PARAM,
		OFFSET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SOURCE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SCALED_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Octsclr() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SCALER_PARAM, 0, 20, 10, "scaler");
		getParamQuantity(SCALER_PARAM)->snapEnabled = true;
		configParam(OFFSET_PARAM, -3, 3, 0, "offset");
		getParamQuantity(OFFSET_PARAM)->snapEnabled = true;
		configInput(SOURCE_INPUT, "source");
		configOutput(SCALED_OUTPUT, "scaled");
		if (use_global_contrast[OCTSCLR]) {
			module_contrast[OCTSCLR] = global_contrast;
		}
	}

	void process(const ProcessArgs& args) override {
		int channels = inputs[SOURCE_INPUT].getChannels();
		if (channels > MAX_POLY) {
			channels = MAX_POLY;
		}
		outputs[SCALED_OUTPUT].setChannels(channels);
		float scaler = (float)params[SCALER_PARAM].getValue() / 10.0f;
		float offset = (float)params[OFFSET_PARAM].getValue();
		for (int i = 0; i < channels; i++) {
			float source = inputs[SOURCE_INPUT].getPolyVoltage(i);
			outputs[SCALED_OUTPUT].setVoltage(clamp(source * scaler + offset, -10.0f, 10.0f), i);
		}
	}
};


struct OctsclrWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	OctsclrWidget(Octsclr* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/octsclr.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 28.443)), module, Octsclr::SCALER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 48.843)), module, Octsclr::OFFSET_PARAM));

		addInput(createInputCentered<BitPort>(mm2px(Vec(7.62, 91.678)), module, Octsclr::SOURCE_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(7.62, 106.319)), module, Octsclr::SCALED_OUTPUT));
	}

	void step() override {
		Octsclr* octModule = dynamic_cast<Octsclr*>(this->module);
		if (!octModule) return;
		if (module_contrast[OCTSCLR] != global_contrast) {
			use_global_contrast[OCTSCLR] = false;
		}
		if (module_contrast[OCTSCLR] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[OCTSCLR];
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
        Octsclr* module = dynamic_cast<Octsclr*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[OCTSCLR]));
            contrastSlider->box.size.x = 200.f;
            contrastMenu->addChild(createMenuItem("use global contrast",
                CHECKMARK(use_global_contrast[OCTSCLR]),
                [module]() { 
                    use_global_contrast[OCTSCLR] = !use_global_contrast[OCTSCLR];
                    if (use_global_contrast[OCTSCLR]) {
						module_contrast[OCTSCLR] = global_contrast;
                    }
                }));
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
					global_contrast = module_contrast[OCTSCLR];
                }));
            menu->addChild(contrastMenu);
        }));
	}
};


Model* modelOctsclr = createModel<Octsclr, OctsclrWidget>("octsclr");