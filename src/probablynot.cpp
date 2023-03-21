#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"


#define MAX_POLY 16

struct Probablynot : Module {
	enum ParamId {
		PROBABILITY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT,
		TRIGGER_INPUT,
		PROBABILITY_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	dsp::SchmittTrigger trigger;
	bool muted = false;
	float amplitude = 0.0f;
	bool fade = false;
	float fade_dur = 0.005f;

	Probablynot() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PROBABILITY_PARAM, 0.f, 1.f, 0.5f, "probability");
		configInput(SIGNAL_INPUT, "signal");
		configInput(TRIGGER_INPUT, "trigger");
		configInput(PROBABILITY_CV_INPUT, "probability cv");
		configOutput(SIGNAL_OUTPUT, "signal");
		if (use_global_contrast[PROBABLYNOT]) {
			module_contrast[PROBABLYNOT] = global_contrast;
		}
	}

	void process(const ProcessArgs& args) override {
		int channels = inputs[SIGNAL_INPUT].getChannels();
		outputs[SIGNAL_OUTPUT].setChannels(channels);

		float probability = params[PROBABILITY_PARAM].getValue();
		float cv = inputs[PROBABILITY_CV_INPUT].getVoltage();

		if (inputs[PROBABILITY_CV_INPUT].isConnected()) {
			probability = clamp(probability + (cv / 10.f), 0.f, 1.f);
		}

		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			if (random::uniform() < probability) {
				muted = true;
			}
			else {
				muted = false;
			}
		}

		if (muted) {
			if (fade) {
				amplitude = clamp(amplitude - args.sampleTime * (1.f / fade_dur), 0.f, 1.f);
			}
			else {
				amplitude = 0.f;
			}
		}
		else {
			if (fade) {
				amplitude = clamp(amplitude + args.sampleTime * (1.f / fade_dur), 0.f, 1.f);
			}
			else {
				amplitude = 1.f;
			}
		}
		for (int c = 0; c < channels; c++) {
			float signal = inputs[SIGNAL_INPUT].getVoltage(c);
			outputs[SIGNAL_OUTPUT].setVoltage(amplitude * signal, c);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "fade", json_boolean(fade));
		json_object_set_new(rootJ, "fade_dur", json_real(fade_dur));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* fadeJ = json_object_get(rootJ, "fade");
		if (fadeJ) {
			fade = json_boolean_value(fadeJ);
		}
		json_t* fade_durJ = json_object_get(rootJ, "fade_dur");
		if (fade_durJ) {
			fade_dur = json_real_value(fade_durJ);
		}
	}
};


struct ProbablynotWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	ProbablynotWidget(Probablynot* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/probablynot.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addParam(createParamCentered<BitKnob>(mm2px(Vec(7.62, 57.765)), module, Probablynot::PROBABILITY_PARAM));

		addInput(createInputCentered<BitPort>(mm2px(Vec(7.62, 27.196)), module, Probablynot::SIGNAL_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(7.62, 42.164)), module, Probablynot::TRIGGER_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(7.62, 73.576)), module, Probablynot::PROBABILITY_CV_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(7.62, 94.026)), module, Probablynot::SIGNAL_OUTPUT));
	}

	struct FadeDurationQuantity : Quantity {
		float* fade_duration;

		FadeDurationQuantity(float* fs) {
			fade_duration = fs;
		}

		void setValue(float value) override {
			*fade_duration = clamp(value, 0.005f, 10.f);
		}

		float getValue() override {
			return *fade_duration;
		}
		
		float getMinValue() override {return 0.005f;}
		float getMaxValue() override {return 10.f;}
		float getDefaultValue() override {return 0.005f;}
		float getDisplayValue() override {return *fade_duration;}

		std::string getUnit() override {
			return "s";
		}
	};

	struct FadeDurationSlider : ui::Slider {
		FadeDurationSlider(float* fade_duration) {
			quantity = new FadeDurationQuantity(fade_duration);
		}
		~FadeDurationSlider() {
			delete quantity;
		}
	};

	void step() override {
		Probablynot* notModule = dynamic_cast<Probablynot*>(this->module);
		if (!notModule) return;
		if (use_global_contrast[PROBABLYNOT]) {
			module_contrast[PROBABLYNOT] = global_contrast;
		}
		if (module_contrast[PROBABLYNOT] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[PROBABLYNOT];
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
		Probablynot* module = dynamic_cast<Probablynot*>(this->module);
		assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[PROBABLYNOT]));
            contrastSlider->box.size.x = 200.f;
			GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[PROBABLYNOT]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                []() {
					global_contrast = module_contrast[PROBABLYNOT];
					use_global_contrast[PROBABLYNOT] = true;
                }));
            menu->addChild(contrastMenu);
        }));

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Fade in/out", CHECKMARK(module->fade), [module]() { module->fade = !module->fade; }));
		FadeDurationSlider *fade_slider = new FadeDurationSlider(&(module->fade_dur));
		fade_slider->box.size.x = 200.f;
		menu->addChild(fade_slider);
	}
};


Model* modelProbablynot = createModel<Probablynot, ProbablynotWidget>("probablynot");