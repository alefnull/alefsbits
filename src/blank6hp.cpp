#include "plugin.hpp"
#include "inc/ThemeableModule.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"


struct Blank6hp : ThemeableModule {
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
	}

	void process(const ProcessArgs& args) override {
	}

    void dataFromJson(json_t* rootJ) override {
        json_t* contrastJ = json_object_get(rootJ, "contrast");
        if (contrastJ) {
            contrast = json_real_value(contrastJ);
        }
        json_t* use_global_contrastJ = json_object_get(rootJ, "use_global_contrast");
        if (use_global_contrastJ) {
            use_global_contrast = json_boolean_value(use_global_contrastJ);
        }
	}

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "contrast", json_real(contrast));
        json_object_set_new(rootJ, "use_global_contrast", json_boolean(use_global_contrast));
		return rootJ;
	}
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
		if (blankModule->contrast != blankModule->global_contrast) {
			blankModule->use_global_contrast = false;
		}
		if (blankModule->contrast != panelBackground->contrast) {
			panelBackground->contrast = blankModule->contrast;
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


Model* modelBlank6hp = createModel<Blank6hp, Blank6hpWidget>("blank6hp");