#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"


struct Blank6hp : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Blank6hp() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        if (use_global_contrast[BLANK6HP]) {
            module_contrast[BLANK6HP] = global_contrast;
        }
	}

	void process(const ProcessArgs& args) override {}
};


struct Blank6hpWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	Blank6hpWidget(Blank6hp* module) {
		setModule(module);
        svgPanel = createPanel(asset::plugin(pluginInstance, "res/blank6hp.svg"));
		setPanel(svgPanel);

        std::shared_ptr<Svg> svgLogo = APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/logo.svg"));
        SvgWidget* svgLogoWidget = new SvgWidget();
        svgLogoWidget->setSvg(svgLogo);
        svgLogoWidget->box.pos = Vec(box.size.x / 2.f - svgLogoWidget->box.size.x / 2.f,
                                    box.size.y / 2.f - svgLogoWidget->box.size.y / 2.f);
		
        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);
        addChild(svgLogoWidget);
	}
	
	void step() override {
		Blank6hp* blankModule = dynamic_cast<Blank6hp*>(this->module);
		if (!blankModule) return;
        if (use_global_contrast[BLANK6HP]) {
            module_contrast[BLANK6HP] = global_contrast;
        }
		if (module_contrast[BLANK6HP] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[BLANK6HP];
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
        Blank6hp* module = dynamic_cast<Blank6hp*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[BLANK6HP]));
            contrastSlider->box.size.x = 200.f;
            contrastMenu->addChild(createMenuItem("use global contrast",
                CHECKMARK(use_global_contrast[BLANK6HP]),
                [module]() { 
                    use_global_contrast[BLANK6HP] = !use_global_contrast[BLANK6HP];
                    if (use_global_contrast[BLANK6HP]) {
                        module_contrast[BLANK6HP] = global_contrast;
                    }
                }));
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
                    global_contrast = module_contrast[BLANK6HP];
                }));
            menu->addChild(contrastMenu);
        }));
	}
};


Model* modelBlank6hp = createModel<Blank6hp, Blank6hpWidget>("blank6hp");