#include "plugin.hpp"
#include "inc/PerlinNoise.hpp"
#include "inc/SimplexNoise.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

#define MAX_POLY 16
#define MIN_TABLE_SIZE 64
#define MAX_TABLE_SIZE 1024
#define DEFAULT_TABLE_SIZE 64

using simd::float_4;


struct NoiseOSC {
	enum Mode {
		RAND,
		PERLIN,
		SIMPLEX,
		WORLEY,
		MODES_LEN
	};
	std::vector<std::string> modeNames = {"rand", "perlin", "simplex", "worley"};
	siv::PerlinNoise perlinNoise;
	SimplexNoise simplexNoise;
	float xInc = 0.01f;
	float_4 phase = float_4(0.f);
	float_4 freq = float_4(dsp::FREQ_C4);

	float sampleRate = 44100.f;
	int tableSize = DEFAULT_TABLE_SIZE;
	std::vector<float> table;

	struct WPoint {
		float x;
		float y;
	};

	// constructor
	NoiseOSC() {
		rand_regen();
		simplexNoise.init();
		perlinNoise.reseed(random::u32());
	}

	// get min value
	float get_min() {
		float min = 10.f;
		for (int i = 0; i < tableSize; i++) {
			if (table[i] < min) {
				min = table[i];
			}
		}
		return min;
	}

	// get max value
	float get_max() {
		float max = -10.f;
		for (int i = 0; i < tableSize; i++) {
			if (table[i] > max) {
				max = table[i];
			}
		}
		return max;
	}

	// get average value
	float get_avg() {
		float avg = 0.f;
		for (int i = 0; i < tableSize; i++) {
			avg += table[i];
		}
		return avg / tableSize;
	}

	// apply offset
	void apply_offset() {
		float avg = get_avg();
		for (int i = 0; i < tableSize; i++) {
			table[i] -= avg;
		}
	}

	// rescale the lookup table to -1 to 1
	void rescale() {
		float min = get_min();
		float max = get_max();
		float range = max - min;
		for (int i = 0; i < tableSize; i++) {
			table[i] = (table[i] - min) / range * 2.f - 1.f;
		}
		apply_offset();
	}

	// regenerate the lookup table
	void rand_regen() {
		table.clear();
		for (int i = 0; i < tableSize; i++) {
			table.push_back(random::uniform() * 2.f - 1.f);
		}
		rescale();
	}

	// regenerate the lookup table
	// using perlin noise
	void perlin_regen() {
		table.clear();
		float x = random::u32() % 10000;
		for (int i = 0; i < tableSize; i++) {
			x += xInc;
			table.push_back(perlinNoise.noise1D(x));
		}
		rescale();
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

	// regenerate the lookup table
	// using worley noise
	void worley_regen() {
		table.clear();
		std::vector<WPoint> points;
		for (int i = 0; i < (int) (tableSize * (xInc * 5.f)); i++) {
			WPoint p;
			p.x = random::uniform();
			p.y = random::uniform();
			points.push_back(p);
		}
		for (int i = 0; i < tableSize; i++) {
			float minDist = 10.f;
			for (int j = 0; j < (int) points.size(); j++) {
				float dist = std::sqrt(std::pow(points[j].x - (float) i / tableSize, 2) + std::pow(points[j].y - 0.5f, 2));
				if (dist < minDist) {
					minDist = dist;
				}
			}
			table.push_back(minDist);
		}
		rescale();
	}

	// inject a new sequence
	void inject(int mode, int tableSize) {
		this->tableSize = tableSize;
		switch (mode) {
			case RAND: {
				rand_regen();
				break;
			}
			case PERLIN: {
				perlin_regen();
				break;
			}
			case SIMPLEX: {
				simplex_regen();
				break;
			}
			case WORLEY: {
				worley_regen();
				break;
			}
		}
	}

	// set the frequency simd
	void setFreqSimd(float_4 freq) {
		this->freq = freq;
	}

	// set the sample rate
	void setSampleRate(float sampleRate) {
		this->sampleRate = sampleRate;
	}

	// get the next sample simd
	float_4 next4() {
		phase += freq / sampleRate;
		for (int i = 0; i < 4; i++) {
			if (phase[i] >= 1.f) {
				phase[i] -= 1.f;
			}
		}
		float_4 out;
		for (int i = 0; i < 4; i++) {
			out[i] = table[(int) (phase[i] * tableSize)];
		}
		return out;
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
	int tableSize = DEFAULT_TABLE_SIZE;
	int mode = NoiseOSC::RAND;

	Nos() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
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
		mode = NoiseOSC::RAND;
		osc.inject((int)mode, tableSize);
	}

	void onRandomize() override {
		osc.inject((int)mode, tableSize);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "tableSize", json_integer(osc.tableSize));
		json_t* tableJ = json_array();
		for (int i = 0; i < osc.tableSize; i++) {
			json_array_append_new(tableJ, json_real(osc.table[i]));
		}
		json_object_set_new(rootJ, "table", tableJ);
		json_object_set_new(rootJ, "mode", json_integer(mode));
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
			osc.table.clear();
			for (int i = 0; i < osc.tableSize; i++) {
				json_t* tableValueJ = json_array_get(tableJ, i);
				if (tableValueJ) {
					osc.table.push_back(json_real_value(tableValueJ));
				}
			}
		}
		json_t* modeJ = json_object_get(rootJ, "mode");
		if (modeJ) {
			mode = (NoiseOSC::Mode) json_integer_value(modeJ);
		}
		json_t* simplexSpeedJ = json_object_get(rootJ, "simplexSpeed");
		if (simplexSpeedJ) {
			osc.xInc = clamp(json_real_value(simplexSpeedJ), 0.01f, 0.1f);
		}
	}

	void process(const ProcessArgs& args) override {
		int channels = inputs[PITCH_INPUT].getChannels();
		if (channels == 0) channels = 1;
		if (channels > MAX_POLY) channels = MAX_POLY;
		outputs[SIGNAL_OUTPUT].setChannels(channels);
		
		float freq = params[FREQ_PARAM].getValue();
		
		for (int c = 0; c < channels; c += 4) {
			float_4 pitch = inputs[PITCH_INPUT].getVoltageSimd<float_4>(c);
			osc.setFreqSimd(freq * simd::pow(2.f, pitch));
			float_4 out = osc.next4() * 5.f;
			outputs[SIGNAL_OUTPUT].setVoltageSimd(clamp(out, -5.f, 5.f), c);
		}

		if (injectTrigger.process(inputs[INJECT_INPUT].getVoltage())) {
			osc.inject((int)mode, tableSize);
		}
		if (injectButton.process(params[INJECT_PARAM].getValue())) {
			osc.inject((int)mode, tableSize);
		}
		lights[INJECT_LIGHT].setBrightness((injectTrigger.isHigh() || injectButton.state) ? 1.f : 0.f);
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
				return "noise increment";
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

		struct SizeQuantity : Quantity {
			Nos* module;
			int* size;

			SizeQuantity(int* size) {
				this->size = size;
			}

			void setValue(float value) override {
				*size = clamp((int)value, MIN_TABLE_SIZE, MAX_TABLE_SIZE);
			}

			float getValue() override {
				return (float)*size;
			}

			float getDefaultValue() override {
				return 64.f;
			}

			float getDisplayValue() override {
				return *size;
			}

			void setDisplayValue(float displayValue) override {
				*size = displayValue;
			}

			std::string getLabel() override {
				return "table size";
			}

			int getDisplayPrecision() override {
				return 4;
			}

			float getMinValue() override {
				return 64.f;
			}

			float getMaxValue() override {
				return 1024.f;
			}

			std::string getUnit() override {
				return " samples";
			}
		};

		struct SizeSlider : ui::Slider {
			Nos* module;
			SizeSlider(Nos* module, int* size) {
				this->module = dynamic_cast<Nos*>(module);
				assert(this->module);
				quantity = new SizeQuantity(size);
			}
			~SizeSlider() {
				delete quantity;
			}
			void onDragStart(const event::DragStart& e) override {
				Slider::onDragStart(e);
				module->tableSize = (int)quantity->getValue();
			}
		};

		struct ModeMenuItem : ui::MenuItem {
			Nos* module;
			int mode;
			ModeMenuItem(Nos* module, int mode) {
				this->module = module;
				this->mode = mode;
				this->text = this->module->osc.modeNames[mode];
				this->rightText = CHECKMARK(module->mode == mode);
			}
			void onAction(const ActionEvent& e) override {
				module->mode = (NoiseOSC::Mode) mode;
				e.unconsume();
			}
			void step() override {
				rightText = CHECKMARK(module->mode == mode);
				MenuItem::step();
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
                []() {
                    global_contrast = module_contrast[NOS];
					use_global_contrast[NOS] = true;
                }));
            menu->addChild(contrastMenu);
        }));

		menu->addChild(new MenuSeparator());

		SizeSlider* sizeSlider = new SizeSlider(module, &(module->tableSize));
		sizeSlider->box.size.x = 200.f;
		menu->addChild(sizeSlider);

		MenuLabel* modeLabel = new MenuLabel();
		modeLabel->text = "mode";
		menu->addChild(modeLabel);
		for (int i = 0; i < NoiseOSC::MODES_LEN; i++) {
			menu->addChild(new ModeMenuItem(module, i));
		}
		
		SpeedSlider* speedSlider = new SpeedSlider(&(module->osc.xInc));
		speedSlider->box.size.x = 200.f;
		menu->addChild(speedSlider);
	}
};


Model* modelNos = createModel<Nos, NosWidget>("nos");