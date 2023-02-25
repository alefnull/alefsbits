#include "plugin.hpp"

#define MAX_POLY 16
#define BUFFER_SIZE 3
#define PT_BUFFER_SIZE 256

struct Turnt : Module {
	enum ParamId {
		MODE_PARAM,
		ZERO_PARAM,
		PROB_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SOURCE_INPUT,
		ZERO_INPUT,
		PROB_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TRIG_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	struct Point {
		float min = 0.f;
		float max = 0.f;
	};

	Turnt() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "mode", {"direction", "through zero", "both"});
		configParam(ZERO_PARAM, -10.f, 10.f, 0.f, "zero threshold", "V");
		configParam(PROB_PARAM, 0.f, 1.f, 1.f, "probability", "%", 0.f, 100.f);
		configInput(SOURCE_INPUT, "source");
		configInput(ZERO_INPUT, "zero threshold");
		configInput(PROB_INPUT, "probability");
		configOutput(TRIG_OUTPUT, "trigger");
	}

	int trigger_mode = 0;
	bool gate_high[MAX_POLY] = { false };
	float samples[MAX_POLY][BUFFER_SIZE] = { 0.f };
	dsp::PulseGenerator pulse[MAX_POLY];
	Point point_buffer[PT_BUFFER_SIZE];
	int buffer_index = 0;
	int frame_index = 0;
	Point current_point;

	void process(const ProcessArgs& args) override {
		int mode = params[MODE_PARAM].getValue();
		float zero = params[ZERO_PARAM].getValue();
		float prob = params[PROB_PARAM].getValue();
		if (inputs[ZERO_INPUT].isConnected()) {
			// use param as attenuverter
			zero *= inputs[ZERO_INPUT].getVoltage() / 10.f;
		}
		if (inputs[PROB_INPUT].isConnected()) {
			// use param as attenuator
			prob *= inputs[PROB_INPUT].getVoltage() / 10.f;
		}

		int channels = inputs[SOURCE_INPUT].getChannels();
		outputs[TRIG_OUTPUT].setChannels(channels);

		for (int ch = 0; ch < channels; ch++) {
			auto in = inputs[SOURCE_INPUT].getVoltage(ch);
			if (in != samples[ch][BUFFER_SIZE-1]) {
				for (int i = 0; i < BUFFER_SIZE - 1; i++) {
					samples[ch][i] = samples[ch][i + 1];
				}
				samples[ch][BUFFER_SIZE - 1] = inputs[SOURCE_INPUT].getVoltage(ch);

				auto d1 = samples[ch][1]-samples[ch][0];
				auto d2 = samples[ch][2]-samples[ch][1];

				bool trig = false;
				float r = random::uniform();

				switch (mode) {
					case 0: // direction
						trig = ((d1 > 0.f && d2 < 0.f) || (d1 < 0.f && d2 > 0.f)) ? (r < prob) : false;
						break;
					case 1: // through zero
						trig = (samples[ch][1] > zero && samples[ch][2] <= zero) || 
								(samples[ch][1] < zero && samples[ch][2] >= zero) ? (r < prob) : false;
						break;
					case 2: // both
						trig = (((d1 > 0.f && d2 < 0.f) || (d1 < 0.f && d2 > 0.f)) || 
								(samples[ch][1] > zero && samples[ch][2] <= zero) || 
								(samples[ch][1] < zero && samples[ch][2] >= zero)) ? (r < prob) : false;
						break;
				}

				if (trig) {
					// pulse.trigger(1e-3f);
					switch (trigger_mode) {
						case 0: // trigger
							pulse[ch].trigger(1e-3f);
							break;
						case 1: // gate
							// output a gate for 0.5 seconds
							pulse[ch].trigger(0.5f);
							break;
						case 2: // latch
							gate_high[ch] = !gate_high[ch];
							break;
					}
				}
			}

			if (trigger_mode == 2) {
				outputs[TRIG_OUTPUT].setVoltage(gate_high[ch] ? 10.f : 0.f, ch);
			} else {
				outputs[TRIG_OUTPUT].setVoltage(pulse[ch].process(args.sampleTime) ? 10.f : 0.f, ch);
			}

			if (ch == 0) {
				// add point to buffer
				if (buffer_index < PT_BUFFER_SIZE) {
					float dt = dsp::approxExp2_taylor5(-(-std::log2(1.f))) / PT_BUFFER_SIZE;
					int frame_count = (int) std::ceil(dt * args.sampleRate);

					current_point.min = std::min(current_point.min, in);
					current_point.max = std::max(current_point.max, in);

					if (++frame_index >= frame_count) {
						frame_index = 0;
						point_buffer[buffer_index] = current_point;
						current_point = Point();
						buffer_index++;
					}
				}
				else {
					// buffer full - reset
					buffer_index = 0;
					frame_index = 0;
					current_point = Point();
				}
			}
		}
	}

	void onReset() override {
		for (int i = 0; i < MAX_POLY; i++) {
			pulse[i].reset();
			gate_high[i] = false;
		}
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modeJ = json_object_get(rootJ, "trigger mode");
		if (modeJ) {
			trigger_mode = json_integer_value(modeJ);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "trigger mode", json_integer(trigger_mode));
		return rootJ;
	}
};


struct Scope : LedDisplay {
    Turnt* module;
    ModuleWidget* moduleWidget;

    Scope() {
        // set the box position to the bottom center of the module
		box.pos = Vec(moduleWidget->box.size.x / 2 - 100, moduleWidget->box.size.y - 100);
		box.size = Vec(200, 200);
    }
    
	void draw_wave(const DrawArgs& args) {
		if (!module) {
			return;
		}

		// copy point buffer
		Turnt::Point point_buffer[PT_BUFFER_SIZE];
		std::copy(std::begin(module->point_buffer), std::end(module->point_buffer), std::begin(point_buffer));

		// draw the waveform
		nvgSave(args.vg);
		Rect b = box.zeroPos().shrink(Vec(0, 15));
		nvgScissor(args.vg, RECT_ARGS(b));
		nvgBeginPath(args.vg);
		for (int i = 0; i < PT_BUFFER_SIZE; i++) {
			const Turnt::Point& p = point_buffer[i];
			float max = p.max;
			if (!std::isfinite(max))
				max = 0.f;
			Vec pt;
			pt.x = (float) i / (PT_BUFFER_SIZE - 1);
			pt.y = max * 0.1f + 0.5f;
			pt = b.interpolate(pt);
			pt.y -= 0.5f;
			if (i == 0)
				nvgMoveTo(args.vg, pt.x, pt.y);
			else
				nvgLineTo(args.vg, pt.x, pt.y);
		}
		for (int i = PT_BUFFER_SIZE - 1; i >= 0; i--) {
			const Turnt::Point& p = point_buffer[i];
			float min = p.min;
			if (!std::isfinite(min))
				min = 0.f;
			Vec pt;
			pt.x = (float) i / (PT_BUFFER_SIZE - 1);
			pt.y = min * 0.1f + 0.5f;
			pt = b.interpolate(pt);
			pt.y += 0.5f;
			nvgLineTo(args.vg, pt.x, pt.y);
		}
		nvgClosePath(args.vg);
		nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
		nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x75));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
		nvgStrokeWidth(args.vg, 1.0);
		nvgStroke(args.vg);
		nvgRestore(args.vg);
	}

	void draw_background(const DrawArgs& args) {
		Rect b = box.zeroPos().shrink(Vec(0, 15));

		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x20));
		for (int i = 0; i < 5; i++) {
			nvgBeginPath(args.vg);
			Vec pt;
			pt.x = 0.f;
			pt.y = (float) i / 4.f;
			nvgMoveTo(args.vg, VEC_ARGS(b.interpolate(pt)));

			pt.x = 1.f;
			nvgLineTo(args.vg, VEC_ARGS(b.interpolate(pt)));
			nvgStroke(args.vg);	
		}
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1)
			return;
		
		draw_background(args);

		if (!module) {
			return;
		}

		draw_wave(args);
	}
};


struct TurntWidget : ModuleWidget {
	TurntWidget(Turnt* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/turnt.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		float y_start = RACK_GRID_WIDTH * 4;
		float dy = RACK_GRID_WIDTH;
		float x = box.size.x / 2;
		float y = y_start;

		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Turnt::SOURCE_INPUT));
		y += dy * 3;
		x -= RACK_GRID_WIDTH * 1.5;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Turnt::ZERO_INPUT));
		y += dy * 2;
		addParam(createParamCentered<RoundBlackKnob>(Vec(x, y), module, Turnt::ZERO_PARAM));
		y -= dy * 2;
		x += RACK_GRID_WIDTH * 3;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Turnt::PROB_INPUT));
		y += dy * 2;
		addParam(createParamCentered<RoundBlackKnob>(Vec(x, y), module, Turnt::PROB_PARAM));
		y += dy * 3;
		x -= RACK_GRID_WIDTH * 1.5;
		addParam(createParamCentered<CKSSThreeHorizontal>(Vec(x, y), module, Turnt::MODE_PARAM));
		y += dy * 2;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Turnt::TRIG_OUTPUT));

		y += dy * 2;
		Scope *scope = createWidget<Scope>(Vec(x - 50.f, y));
		scope->box.size = Vec(100.f, 100.f);
		scope->module = module;
		scope->moduleWidget = this;
		addChild(scope);
	}

	void appendContextMenu(Menu* menu) override {
		Turnt* module = dynamic_cast<Turnt*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		
		menu->addChild(createSubmenuItem("trigger mode", "", [=](Menu* menu) {
			Menu* trigMenu = new Menu();
			trigMenu->addChild(createMenuItem("trigger", CHECKMARK(module->trigger_mode == 0), [module]() { module->trigger_mode = 0; }));
			trigMenu->addChild(createMenuItem("gate", CHECKMARK(module->trigger_mode == 1), [module]() { module->trigger_mode = 1; }));
			trigMenu->addChild(createMenuItem("latch", CHECKMARK(module->trigger_mode == 2), [module]() { module->trigger_mode = 2; }));
			menu->addChild(trigMenu);
		}));
	}
};


Model* modelTurnt = createModel<Turnt, TurntWidget>("turnt");