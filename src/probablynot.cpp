#include "plugin.hpp"


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
	}

	void process(const ProcessArgs& args) override {
		float signal = inputs[SIGNAL_INPUT].getVoltage();
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
		outputs[SIGNAL_OUTPUT].setVoltage(amplitude * signal);
	}
};


struct ProbablynotWidget : ModuleWidget {
	ProbablynotWidget(Probablynot* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/probablynot.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 57.765)), module, Probablynot::PROBABILITY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 27.196)), module, Probablynot::SIGNAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 42.164)), module, Probablynot::TRIGGER_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 73.576)), module, Probablynot::PROBABILITY_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 94.026)), module, Probablynot::SIGNAL_OUTPUT));
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

	void appendContextMenu(Menu* menu) override {
		Probablynot* module = dynamic_cast<Probablynot*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Fade in/out", CHECKMARK(module->fade), [module]() { module->fade = !module->fade; }));
		FadeDurationSlider *fade_slider = new FadeDurationSlider(&(module->fade_dur));
		fade_slider->box.size.x = 200.f;
		menu->addChild(fade_slider);
	}
};


Model* modelProbablynot = createModel<Probablynot, ProbablynotWidget>("probablynot");