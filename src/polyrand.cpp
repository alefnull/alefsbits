#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

#define MAX_POLY 16

struct Polyrand : Module {
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
		if (use_global_contrast[POLYRAND]) {
			module_contrast[POLYRAND] = global_contrast;
		}
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
		if (use_global_contrast[POLYRAND]) {
			module_contrast[POLYRAND] = global_contrast;
		}
		if (module_contrast[POLYRAND] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[POLYRAND];
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
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[POLYRAND]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[POLYRAND]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
					global_contrast = module_contrast[POLYRAND];
                }));
            menu->addChild(contrastMenu);
        }));
	}
};


Model* modelPolyrand = createModel<Polyrand, PolyrandWidget>("polyrand");