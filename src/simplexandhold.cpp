#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "inc/SimplexNoise.cpp"
#include "inc/cvRange.hpp"

#define MAX_POLY 16

struct Simplexandhold : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SAMPLE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SimplexNoise noise;
	dsp::SchmittTrigger trigger[MAX_POLY];
	bool unipolar = false;
	float last_sample[MAX_POLY] = {0.0};
	double x[MAX_POLY] = {0.0};
	CVRange cv_range;

	Simplexandhold() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(TRIGGER_INPUT, "trigger");
		configOutput(SAMPLE_OUTPUT, "sample");
		noise.init();
		if (use_global_contrast[SIMPLEXANDHOLD]) {
			module_contrast[SIMPLEXANDHOLD] = global_contrast;
		}
	}

	void process(const ProcessArgs& args) override {
		int chans = std::max(1, inputs[TRIGGER_INPUT].getChannels());
		outputs[SAMPLE_OUTPUT].setChannels(chans);
		for (int c = 0; c < chans; c++) {
			if (trigger[c].process(inputs[TRIGGER_INPUT].getVoltage(c))) {
				last_sample[c] = ((noise.noise(x[c], 0.0) + 1.0) / 2.0);
				last_sample[c] = cv_range.map(last_sample[c]);
			}
			x[c] += 0.1;
			outputs[SAMPLE_OUTPUT].setVoltage(last_sample[c], c);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "unipolar", json_boolean(unipolar));
		json_object_set_new(rootJ, "cv_range", cv_range.dataToJson());
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* unipolarJ = json_object_get(rootJ, "unipolar");
		if (unipolarJ)
			unipolar = json_boolean_value(unipolarJ);
		json_t* cv_rangeJ = json_object_get(rootJ, "cv_range");
		if (cv_rangeJ)
			cv_range.dataFromJson(cv_rangeJ);
	}
};


struct SimplexandholdWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	SimplexandholdWidget(Simplexandhold* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/simplexandhold.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addInput(createInputCentered<BitPort>(mm2px(Vec(10.16, 35.937)), module, Simplexandhold::TRIGGER_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(10.16, 100.446)), module, Simplexandhold::SAMPLE_OUTPUT));
	}
	
	void step() override {
		Simplexandhold* simplexModule = dynamic_cast<Simplexandhold*>(this->module);
		if (!simplexModule) return;
		if (use_global_contrast[SIMPLEXANDHOLD]) {
			module_contrast[SIMPLEXANDHOLD] = global_contrast;
		}
		if (module_contrast[SIMPLEXANDHOLD] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[SIMPLEXANDHOLD];
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
		Simplexandhold* module = dynamic_cast<Simplexandhold*>(this->module);
		assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[SIMPLEXANDHOLD]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[SIMPLEXANDHOLD]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
					global_contrast = module_contrast[SIMPLEXANDHOLD];
                }));
            menu->addChild(contrastMenu);
        }));

		menu->addChild(new MenuSeparator());
		module->cv_range.addMenu(module, menu);
	}
};

Model* modelSimplexandhold = createModel<Simplexandhold, SimplexandholdWidget>("simplexandhold");