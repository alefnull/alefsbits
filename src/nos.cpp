#include "plugin.hpp"
#include "inc/SimplexNoise.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

#define MIN_TABLE_SIZE 64
#define MAX_TABLE_SIZE 1024
#define DEFAULT_TABLE_SIZE 64


struct NoiseOSC {
	// construct a noise oscillator using a lookup table
	float phase = 0.f;
	float freq = dsp::FREQ_C4 * 0.5f;
	float sampleRate = 44100.f;
	int tableSize = DEFAULT_TABLE_SIZE;
	// float *table = new float[tableSize];
	std::vector<float> table;
	SimplexNoise simplexNoise;
	float xInc = 0.01;

	// constructor
	NoiseOSC() {
		rand_regen();
		simplexNoise.init();
	}

	// get min value
	float get_min() {
		float min = 1000.f;
		for (int i = 0; i < tableSize; i++) {
			if (table[i] < min) {
				min = table[i];
			}
		}
		return min;
	}

	// get max value
	float get_max() {
		float max = -1000.f;
		for (int i = 0; i < tableSize; i++) {
			if (table[i] > max) {
				max = table[i];
			}
		}
		return max;
	}

	// rescale the lookup table to -1 to 1
	void rescale() {
		float min = get_min();
		float max = get_max();
		float range = max - min;
		for (int i = 0; i < tableSize; i++) {
			table[i] = (table[i] - min) / range * 2.f - 1.f;
		}
	}

	// regenerate the lookup table
	void rand_regen() {
		table.clear();
		for (int i = 0; i < tableSize; i++) {
			table.push_back(random::uniform() * 2.f - 1.f);
		}
	}

	// regenerate the lookup table
	// using simplex noise
	void simplex_regen() {
		table.clear();
		float x = random::u32() % 10000;
		for (int i = 0; i < tableSize; i++) {
			x += xInc;
			table.push_back(simplexNoise.noise(x, 0));
		}
		rescale();
	}

	// set the frequency
	void setFreq(float freq) {
		this->freq = freq;
	}

	// set the sample rate
	void setSampleRate(float sampleRate) {
		this->sampleRate = sampleRate;
	}

	// get the next sample
	float next() {
		phase += freq / sampleRate;
		if (phase >= 1.f) {
			phase -= 1.f;
		}
		return table[(int) (phase * tableSize)];
	}
};


struct Nos : Module {
	enum ParamId {
		FREQ_PARAM,
		INJECT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		PITCH_INPUT,
		INJECT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		INJECT_LIGHT,
		LIGHTS_LEN
	};

	NoiseOSC osc;
	dsp::SchmittTrigger injectTrigger;
	dsp::BooleanTrigger injectButton;
	bool simplex = false;

	Nos() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		// configParam(FREQ_PARAM, 32.7f, 523.3f, dsp::FREQ_C4, "frequency", " hz");
		// configure the frequency param to go from C1 to C6
		configParam(FREQ_PARAM, 32.7f, 1046.5f, dsp::FREQ_C4, "frequency", " hz");
		configButton(INJECT_PARAM, "inject");
		configInput(PITCH_INPUT, "pitch");
		configInput(INJECT_INPUT, "inject");
		configOutput(SIGNAL_OUTPUT, "signal");
        if (use_global_contrast[NOS]) {
            module_contrast[NOS] = global_contrast;
        }
		osc.setSampleRate(APP->engine->getSampleRate());
	}

	void onSampleRateChange() override {
		osc.setSampleRate(APP->engine->getSampleRate());
	}

	void onReset() override {
		simplex = false;
		osc.rand_regen();
	}

	void onRandomize() override {
		if (simplex) {
			osc.simplex_regen();
		}
		else {
			osc.rand_regen();
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "tableSize", json_integer(osc.tableSize));
		json_object_set_new(rootJ, "table", json_pack("[F*]", osc.tableSize, &osc.table[0]));
		json_object_set_new(rootJ, "simplexMode", json_boolean(simplex));
		json_object_set_new(rootJ, "simplexSpeed", json_real(osc.xInc));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* tableSizeJ = json_object_get(rootJ, "tableSize");
		if (tableSizeJ) {
			osc.tableSize = json_integer_value(tableSizeJ);
			osc.tableSize = clamp(osc.tableSize, MIN_TABLE_SIZE, MAX_TABLE_SIZE);
		}
		json_t* tableJ = json_object_get(rootJ, "table");
		if (tableJ) {
			json_t* tableArrayJ = json_array_get(tableJ, 0);
			if (tableArrayJ) {
				osc.table.clear();
				for (int i = 0; i < osc.tableSize; i++) {
					json_t* tableValueJ = json_array_get(tableArrayJ, i);
					if (tableValueJ) {
						osc.table.push_back(json_real_value(tableValueJ));
					}
				}
			}
		}
		json_t* simplexJ = json_object_get(rootJ, "simplexMode");
		if (simplexJ) {
			simplex = json_boolean_value(simplexJ);
		}
		json_t* simplexSpeedJ = json_object_get(rootJ, "simplexSpeed");
		if (simplexSpeedJ) {
			osc.xInc = clamp(json_real_value(simplexSpeedJ), 0.01f, 0.1f);
		}
		if (simplex) {
			osc.simplex_regen();
		}
		else {
			osc.rand_regen();
		}
	}

	void process(const ProcessArgs& args) override {
		float freq = params[FREQ_PARAM].getValue();
		float voct = inputs[PITCH_INPUT].getVoltage();
		osc.setFreq(freq * std::pow(2.f, voct));
		if (injectTrigger.process(inputs[INJECT_INPUT].getVoltage())) {
			if (simplex) {
				osc.simplex_regen();
			}
			else {
				osc.rand_regen();
			}
		}
		if (injectButton.process(params[INJECT_PARAM].getValue())) {
			if (simplex) {
				osc.simplex_regen();
			}
			else {
				osc.rand_regen();
			}
		}
		lights[INJECT_LIGHT].setBrightness((injectTrigger.isHigh() || injectButton.state) ? 1.f : 0.f);
		outputs[SIGNAL_OUTPUT].setVoltage(clamp(osc.next() * 5, -5.f, 5.f));
	}
};


struct NosWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	NosWidget(Nos* module) {
		setModule(module);
        svgPanel = createPanel(asset::plugin(pluginInstance, "res/nos.svg"));
		setPanel(svgPanel);
		panelBackground->box.size = svgPanel->box.size;
		svgPanel->fb->addChildBottom(panelBackground);
		inverter->box.pos = box.pos;
		inverter->box.size = box.size;
		addChild(inverter);

		float x_start = box.size.x / 2.f;
		float y_start = RACK_GRID_WIDTH * 7.5f;
		float dy = RACK_GRID_WIDTH * 2.f;
		float x = x_start;
		float y = y_start;

		addParam(createParamCentered<BitKnob>(Vec(x, y), module, Nos::FREQ_PARAM));
		y += dy;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Nos::PITCH_INPUT));
		y += dy * 2.5f;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Nos::INJECT_INPUT));
		y += dy - dy * 0.1f;
		addParam(createLightParamCentered<VCVLightButton<LargeSimpleLight<RedLight>>>(Vec(x, y), module, Nos::INJECT_PARAM, Nos::INJECT_LIGHT));
		y += dy * 2.f;
		addOutput(createOutputCentered<BitPort>(Vec(x, y), module, Nos::SIGNAL_OUTPUT));
	}

	void step() override {
		Nos* nosModule = dynamic_cast<Nos*>(this->module);
		if (!nosModule) return;
        if (use_global_contrast[NOS]) {
            module_contrast[NOS] = global_contrast;
        }
		if (module_contrast[NOS] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[NOS];
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
		Nos* module = dynamic_cast<Nos*>(this->module);
		assert(module);

		struct TableSizeItem : MenuItem {
			Nos* module;
			int tableSize;
			void onAction(const event::Action& e) override {
				module->osc.tableSize = tableSize;
				if (module->simplex) {
					module->osc.simplex_regen();
				}
				else {
					module->osc.rand_regen();
				}
			}
			void step() override {
				rightText = (module->osc.tableSize == tableSize) ? "✔" : "";
				MenuItem::step();
			}
		};

		struct SpeedQuantity : Quantity {
			Nos* module;
			float* speed;

			SpeedQuantity(float* speed) {
				this->speed = speed;
			}

			void setValue(float value) override {
				*speed = clamp(value, 0.01f, 0.1f);
			}

			float getValue() override {
				return *speed;
			}

			float getDefaultValue() override {
				return 0.01f;
			}

			float getDisplayValue() override {
				return *speed;
			}

			void setDisplayValue(float displayValue) override {
				*speed = displayValue;
			}

			std::string getLabel() override {
				return "simplex speed";
			}

			int getDisplayPrecision() override {
				return 2;
			}

			float getMinValue() override {
				return 0.01f;
			}

			float getMaxValue() override {
				return 0.1f;
			}
		};

		struct SpeedSlider : ui::Slider {
			SpeedSlider(float* speed) {
				quantity = new SpeedQuantity(speed);
			}
			~SpeedSlider() {
				delete quantity;
			}
		};

		menu->addChild(new MenuSeparator());
        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[NOS]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[NOS]));
            contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
                    global_contrast = module_contrast[NOS];
                }));
            menu->addChild(contrastMenu);
        }));

		menu->addChild(new MenuSeparator());

		menu->addChild(createMenuItem("enable simplex mode", CHECKMARK(module->simplex), [module]() {
			module->simplex = !module->simplex;
			if (module->simplex) {
				module->osc.simplex_regen();
			}
			else {
				module->osc.rand_regen();
			}
		}));

		menu->addChild(createMenuItem("inject simplex", "",
			[module]() {
				module->simplex = true;
				module->osc.simplex_regen();
			}));

		SpeedSlider* speedSlider = new SpeedSlider(&(module->osc.xInc));
		speedSlider->box.size.x = 200.f;
		menu->addChild(speedSlider);

		menu->addChild(new MenuSeparator());

		MenuLabel* tableSizeLabel = new MenuLabel();
		tableSizeLabel->text = "table size";
		menu->addChild(tableSizeLabel);
		for (int tableSize = MIN_TABLE_SIZE; tableSize <= MAX_TABLE_SIZE; tableSize *= 2) {
			TableSizeItem* tableSizeItem = createMenuItem<TableSizeItem>(std::to_string(tableSize), CHECKMARK(tableSize == module->osc.tableSize));
			tableSizeItem->module = module;
			tableSizeItem->tableSize = tableSize;
			menu->addChild(tableSizeItem);
		}
	}
};


Model* modelNos = createModel<Nos, NosWidget>("nos");