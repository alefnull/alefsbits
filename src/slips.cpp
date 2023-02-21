#include "plugin.hpp"
#include "quantizer.hpp"
#include "cvRange.hpp"


struct Slips : Module, Quantizer {
	enum ParamId {
		STEPS_PARAM,
		ROOT_PARAM,
		SCALE_PARAM,
		GENERATE_PARAM,
		SLIPS_PARAM,
		SLIP_RANGE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		ROOT_INPUT,
		SCALE_INPUT,
		GENERATE_INPUT,
		QUANTIZE_INPUT,
		SLIPS_INPUT,
		SLIP_RANGE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SEQUENCE_OUTPUT,
		GATE_OUTPUT,
		QUANTIZE_OUTPUT,
		SLIP_GATE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		GATE_LIGHT,
		SLIP_GATE_LIGHT,
		ENUMS(STEP_1_LIGHT, 4),
		ENUMS(STEP_2_LIGHT, 4),
		ENUMS(STEP_3_LIGHT, 4),
		ENUMS(STEP_4_LIGHT, 4),
		ENUMS(STEP_5_LIGHT, 4),
		ENUMS(STEP_6_LIGHT, 4),
		ENUMS(STEP_7_LIGHT, 4),
		ENUMS(STEP_8_LIGHT, 4),
		ENUMS(STEP_9_LIGHT, 4),
		ENUMS(STEP_10_LIGHT, 4),
		ENUMS(STEP_11_LIGHT, 4),
		ENUMS(STEP_12_LIGHT, 4),
		ENUMS(STEP_13_LIGHT, 4),
		ENUMS(STEP_14_LIGHT, 4),
		ENUMS(STEP_15_LIGHT, 4),
		ENUMS(STEP_16_LIGHT, 4),
		LIGHTS_LEN
	};

	Slips() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(STEPS_PARAM, 1, 64, 16, "steps");
		getParamQuantity(STEPS_PARAM)->snapEnabled = true;
		configSwitch(ROOT_PARAM, 0, 11, 0, "root note", {
			"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
		});
		getParamQuantity(ROOT_PARAM)->snapEnabled = true;
		configSwitch(SCALE_PARAM, 0, 10, 0, "scale", {
			"chromatic", "major", "minor", "major pentatonic", "minor pentatonic",
			"dorian", "lydian", "mixolydian", "phrygian", "locrian", "blues"
		});
		getParamQuantity(SCALE_PARAM)->snapEnabled = true;
		configParam(GENERATE_PARAM, 0, 1, 0, "generate");
		configParam(SLIPS_PARAM, 0, 1, 0, "slips", " %", 0, 100);
		configParam(SLIP_RANGE_PARAM, 0, 1, 0, "slip range", " +/- volts");
		configInput(CLOCK_INPUT, "clock");
		configInput(RESET_INPUT, "reset");
		configInput(STEPS_INPUT, "steps cv");
		getInputInfo(STEPS_INPUT)->description = "0V to 10V";
		configInput(ROOT_INPUT, "root cv");
		getInputInfo(ROOT_INPUT)->description = "0V to 10V";
		configInput(SCALE_INPUT, "scale cv");
		getInputInfo(SCALE_INPUT)->description = "0V to 10V";
		configInput(GENERATE_INPUT, "generate");
		configInput(QUANTIZE_INPUT, "unquantized");
		configInput(SLIPS_INPUT, "slips cv");
		getInputInfo(SLIPS_INPUT)->description = "0V to 10V";
		configInput(SLIP_RANGE_INPUT, "slip range cv");
		getInputInfo(SLIP_RANGE_INPUT)->description = "0V to 10V";
		configOutput(SEQUENCE_OUTPUT, "sequence");
		configOutput(GATE_OUTPUT, "gate");
		configOutput(QUANTIZE_OUTPUT, "quantized");
		configOutput(SLIP_GATE_OUTPUT, "slip gate");
	}

	// the sequence
	std::vector<float> the_sequence;
	// the slips
	std::vector<float> the_slips;
	// the current step
	int current_step = 0;
	// schmitt trigger for clock input
	dsp::SchmittTrigger clock_trigger;
	// schmitt trigger for reset input
	dsp::SchmittTrigger reset_trigger;
	// schmitt trigger for generatee input
	dsp::SchmittTrigger generate_trigger;
	// schmitt trigger for generatee manual button
	dsp::SchmittTrigger generate_button_trigger;
	// a bool to check if a reset has been requested
	bool reset_requested = false;
	// a bool to check if slips have already been generated for this cycle
	bool slips_generated = false;
	// a bool to check if root note input expects 0-10V or a v/oct value
	bool root_input_voct = false;

	// a cv range object to convert voltages with a range of 0V to 1V into a given range
	CVRange cv_range;

	// dataToJson override
	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		// the sequence
		json_t* sequenceJ = json_array();
		for (int i = 0; i < (int) the_sequence.size(); i++) {
			json_t* valueJ = json_real(the_sequence[i]);
			json_array_append_new(sequenceJ, valueJ);
		}
		json_object_set_new(rootJ, "sequence", sequenceJ);
		// the slips
		json_t* slipsJ = json_array();
		for (int i = 0; i < (int) the_slips.size(); i++) {
			json_t* valueJ = json_real(the_slips[i]);
			json_array_append_new(slipsJ, valueJ);
		}
		json_object_set_new(rootJ, "slips", slipsJ);
		// the cv range
		json_object_set_new(rootJ, "cv_range", cv_range.dataToJson());
		// root input voct
		json_object_set_new(rootJ, "root_input_voct", json_boolean(root_input_voct));
		return rootJ;
	}

	// dataFromJson override
	void dataFromJson(json_t* rootJ) override {
		// the sequence
		json_t* sequenceJ = json_object_get(rootJ, "sequence");
		if (sequenceJ) {
			the_sequence.clear();
			for (int i = 0; i < (int) json_array_size(sequenceJ); i++) {
				json_t* valueJ = json_array_get(sequenceJ, i);
				if (valueJ) {
					the_sequence.push_back(json_number_value(valueJ));
				}
			}
		}
		// the slips
		json_t* slipsJ = json_object_get(rootJ, "slips");
		if (slipsJ) {
			the_slips.clear();
			for (int i = 0; i < (int) json_array_size(slipsJ); i++) {
				json_t* valueJ = json_array_get(slipsJ, i);
				if (valueJ) {
					the_slips.push_back(json_number_value(valueJ));
				}
			}
		}
		// the cv range
		json_t* cv_rangeJ = json_object_get(rootJ, "cv_range");
		if (cv_rangeJ) {
			cv_range.dataFromJson(cv_rangeJ);
		}
		// root input voct
		json_t* root_input_voctJ = json_object_get(rootJ, "root_input_voct");
		if (root_input_voctJ) {
			root_input_voct = json_boolean_value(root_input_voctJ);
		}
	}

	// function to generate a new sequence,
	// given the number of steps, root note, and scale
	void generate_sequence(int num_steps, int root_note, int current_scale) {
		// clear the sequence
		the_sequence.clear();
		// generate the sequence
		for (int i = 0; i < num_steps; i++) {
			// generate a random value between 0 and 1
			float random_value = random::uniform();
			// scale the random value to the desired cv range
			random_value = cv_range.map(random_value);
			// add the quantized value to the sequence
			the_sequence.push_back(random_value);
		}
	}

	// function to set the octave of the sequence,
	// given the desired octave (0 == -4, 1 == -3, 2 == -2, 3 == -1, 4 == 0, etc.)
	void set_octave(int octave) {
		for (int i = 0; i < (int)the_sequence.size(); i++) {
			the_sequence[i] += (octave - 4) * 12;
		}
	}

	// function to generate "slips" to apply to the sequence,
	// given the slip amount (0 to 1) and slip range (0 to 1)
	void generate_slips(float slip_amount, float slip_range) {
		// clear the slips
		the_slips.clear();
		// make sure the slip vector is the same size as the sequence
		for (int i = 0; i < (int)the_sequence.size(); i++) {
			the_slips.push_back(0.0);
		}
		// convert the slip amount to a number of steps to slip
		int num_slips = (int)(the_sequence.size() * slip_amount);
		// generate the slips
		for (int i = 0; i < num_slips; i++) {
			// pick a random step to slip (as long as it hasn't already been slipped this cycle)
			int slip_step = -1;
			while (slip_step < 0 || the_slips[slip_step] != 0.0) {
				slip_step = random::u32() % (int)the_sequence.size();
			}
			// pick a random amount to slip by (-1 to 1)
			float slip_offset = random::uniform() * slip_range * 2.0 - slip_range;
			// add the slip to the slip vector at the slip step
			the_slips[slip_step] = slip_offset;
		}
	}

	void process(const ProcessArgs& args) override {
		// get the number of steps
		int num_steps = params[STEPS_PARAM].getValue();
		// get the root note
		int root_note = params[ROOT_PARAM].getValue();
		// get the scale
		int current_scale = params[SCALE_PARAM].getValue();
		// get the slip amount
		float slip_amount = params[SLIPS_PARAM].getValue();
		// get the slip range
		float slip_range = params[SLIP_RANGE_PARAM].getValue();
		
		// check if the clock input is high
		if (clock_trigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			// check if a reset has been requested
			if (reset_requested) {
				// reset the step
				current_step = 0;
				// reset the reset request
				reset_requested = false;
				// break out of the conditional
				return;
			}
			// increment the step
			current_step++;
			// check if the step is out of bounds
			if (current_step >= (int)the_sequence.size()) {
				// reset the step
				current_step = 0;
			}
		}

		// check if the reset input is high
		if (reset_trigger.process(inputs[RESET_INPUT].getVoltage())) {
			// request a reset
			reset_requested = true;
		}

		// check if the steps input is connected
		if (inputs[STEPS_INPUT].isConnected()) {
			// get the steps input voltage
			float num_steps_input = inputs[STEPS_INPUT].getVoltage();
			// check if the steps input voltage is out of bounds
			if (num_steps_input < 0 || num_steps_input > 10) {
				// clamp the steps input voltage
				num_steps_input = clamp(num_steps_input, 0.0f, 10.0f);
			}
			// set the number of steps
			num_steps = (int) (num_steps_input / 10 * 32);
		}

		// check if the root input is connected
		if (inputs[ROOT_INPUT].isConnected()) {
			if (!root_input_voct) {
				// get the root input voltage
				float root_note_input = inputs[ROOT_INPUT].getVoltage();
				// check if the root input voltage is out of bounds
				if (root_note_input < 0 || root_note_input > 10) {
					// clamp the root input voltage
					root_note_input = clamp(root_note_input, 0.0f, 10.0f);
				}
				// set the root note
				root_note = (int) (root_note_input / 10 * 12);
			}
			else {
				// get the root input voltage
				float root_note_input = inputs[ROOT_INPUT].getVoltage();
				// the root note input expects a v/oct value,
				// so strip out the octave and convert to a note
				// number between 0 and 11
				root_note = (int) (root_note_input * 12) % 12;
			}
		}

		// check if the scale input is connected
		if (inputs[SCALE_INPUT].isConnected()) {
			// get the scale input voltage
			float current_scale_input = inputs[SCALE_INPUT].getVoltage();
			// check if the scale input voltage is out of bounds
			if (current_scale_input < 0 || current_scale_input > 10) {
				// clamp the scale input voltage
				current_scale_input = clamp(current_scale_input, 0.0f, 10.0f);
			}
			// set the scale
			current_scale = (int) (current_scale_input / 10 * 11);
		}

		// check if the slip amount input is connected
		if (inputs[SLIPS_INPUT].isConnected()) {
			// get the slip amount input voltage
			float slip_amount_input = inputs[SLIPS_INPUT].getVoltage();
			// check if the slip amount input voltage is out of bounds
			if (slip_amount_input < 0 || slip_amount_input > 10) {
				// clamp the slip amount input voltage
				slip_amount_input = clamp(slip_amount_input, 0.0f, 10.0f);
			}
			// set the slip amount
			slip_amount = slip_amount_input / 10;
		}

		// check if the slip range input is connected
		if (inputs[SLIP_RANGE_INPUT].isConnected()) {
			// get the slip range input voltage
			float slip_range_input = inputs[SLIP_RANGE_INPUT].getVoltage();
			// check if the slip range input voltage is out of bounds
			if (slip_range_input < 0 || slip_range_input > 10) {
				// clamp the slip range input voltage
				slip_range_input = clamp(slip_range_input, 0.0f, 10.0f);
			}
			// set the slip range
			slip_range = slip_range_input / 10;
		}

		// if the sequence is empty, generate a new sequence
		if (the_sequence.empty()) {
			generate_sequence(num_steps, root_note, current_scale);
		}

		// if the slip vector is empty, generate a new slip vector
		if (the_slips.empty()) {
			for (int i = 0; i < num_steps; i++) {
				the_slips.push_back(0);
			}
		}

		// if we're on the first step
		if (current_step == 0) {
			// if we haven't generated a new slip vector for this cycle
			if (!slips_generated) {
				// generate a new slip vector
				generate_slips(slip_amount, slip_range);
				// set the slips generated flag
				slips_generated = true;
			}
		} else {
			// reset the slips generated flag
			slips_generated = false;
		}

		// check if the generatee input is high
		if (generate_trigger.process(inputs[GENERATE_INPUT].getVoltage())) {
			// request a reset
			reset_requested = true;
			// generate a new sequence
			generate_sequence(num_steps, root_note, current_scale);
		}

		// check if the generatee button is pressed
		if (generate_button_trigger.process(params[GENERATE_PARAM].getValue())) {
			// request a reset
			reset_requested = true;
			// generate a new sequence
			generate_sequence(num_steps, root_note, current_scale);
		}

		// check if this step is a slip (the_slips[current_step] != 0)
		float out = the_slips[current_step] != 0 ? clamp(the_sequence[current_step] + the_slips[current_step], -10.f, 10.f) : clamp(the_sequence[current_step], -10.f, 10.f);
		// set the output voltage
		outputs[SEQUENCE_OUTPUT].setVoltage(quantize(out, root_note, current_scale));
		// set the gate output
		if (outputs[GATE_OUTPUT].isConnected() && inputs[CLOCK_INPUT].isConnected()) {
			// set the gate output voltage to the same as the clock input
			outputs[GATE_OUTPUT].setVoltage(inputs[CLOCK_INPUT].getVoltage());
		}

		// set the slip gate output
		if (outputs[SLIP_GATE_OUTPUT].isConnected() && inputs[CLOCK_INPUT].isConnected()) {
			// if this step is a slip (the_slips[current_step] != 0)
			if (the_slips[current_step] != 0) {
				// set the slip gate output voltage to the same as the clock input
				outputs[SLIP_GATE_OUTPUT].setVoltage(inputs[CLOCK_INPUT].getVoltage());
			} else {
				// set the slip gate output voltage to 0
				outputs[SLIP_GATE_OUTPUT].setVoltage(0);
			}
		}

		// get the input voltage to be quantized
		float quantize_input = inputs[QUANTIZE_INPUT].getVoltage();
		// set the quantize output voltage
		outputs[QUANTIZE_OUTPUT].setVoltage(quantize(quantize_input, root_note, current_scale));

		// set the gate light
		lights[GATE_LIGHT].setBrightness(inputs[CLOCK_INPUT].getVoltage() / 10);

		// set the slip gate light
		lights[SLIP_GATE_LIGHT].setBrightness(the_slips[current_step] != 0 ? inputs[CLOCK_INPUT].getVoltage() / 10 : 0);

		// set the step lights
		for (int i = 0; i < 16; i++) {
			lights[STEP_1_LIGHT + (i * 4)].setBrightness(i == current_step ? 1.f : 0.f);
			lights[STEP_1_LIGHT + (i * 4) + 1].setBrightness(i + 16 == current_step ? 1.f : 0.f);
			lights[STEP_1_LIGHT + (i * 4) + 2].setBrightness(i + 32 == current_step ? 1.f : 0.f);
			lights[STEP_1_LIGHT + (i * 4) + 3].setBrightness(i + 48 == current_step ? 1.f : 0.f);
		}
	}
};

static const NVGcolor COLOR_RED = nvgRGB(0xff, 0x00, 0x00);
static const NVGcolor COLOR_GREEN = nvgRGB(0x00, 0xff, 0x00);
static const NVGcolor COLOR_BLUE = nvgRGB(0x00, 0x00, 0xff);
static const NVGcolor COLOR_YELLOW = nvgRGB(0xff, 0xff, 0x00);
static const NVGcolor COLOR_CYAN = nvgRGB(0x00, 0xff, 0xff);
static const NVGcolor COLOR_MAGENTA = nvgRGB(0xff, 0x00, 0xff);
static const NVGcolor COLOR_IBM_PURPLE = nvgRGB(0x78, 0x5e, 0xf0);
static const NVGcolor COLOR_IBM_MAGENTA = nvgRGB(0xdc, 0x26, 0x7f);
static const NVGcolor COLOR_IBM_ORANGE = nvgRGB(0xfe, 0x61, 0x00);
static const NVGcolor COLOR_IBM_YELLOW = nvgRGB(0xff, 0xb0, 0x00);
static const NVGcolor COLOR_CUD_ORANGE = nvgRGB(0xe6, 0x9f, 0x00);
static const NVGcolor COLOR_CUD_BLUE = nvgRGB(0x00, 0x72, 0xb2);
static const NVGcolor COLOR_CUD_SKYBLUE = nvgRGB(0x56, 0xb4, 0xe9);
static const NVGcolor COLOR_CUD_GREEN = nvgRGB(0x00, 0x9e, 0x73);
static const NVGcolor COLOR_CUD_PINK = nvgRGB(0xcc, 0x79, 0xa7);

struct LightColors : GrayModuleLightWidget {
	LightColors() {
		addBaseColor(COLOR_RED);
		addBaseColor(COLOR_GREEN);
	}
};

struct IBMLightColors : GrayModuleLightWidget {
	IBMLightColors() {
		addBaseColor(COLOR_IBM_PURPLE);
		addBaseColor(COLOR_IBM_MAGENTA);
		addBaseColor(COLOR_IBM_ORANGE);
		addBaseColor(COLOR_IBM_YELLOW);
	}
};

struct CUDLightColors : GrayModuleLightWidget {
	CUDLightColors() {
		addBaseColor(COLOR_CUD_BLUE);
		addBaseColor(COLOR_CUD_GREEN);
		addBaseColor(COLOR_CUD_ORANGE);
		addBaseColor(COLOR_CUD_PINK);
	}
};


struct SlipsWidget : ModuleWidget {
	SlipsWidget(Slips* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/slips.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(51.651, 25.72)), module, Slips::STEPS_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.164, 25.72)), module, Slips::STEPS_INPUT));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(51.651, 40.056)), module, Slips::ROOT_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.164, 39.845)), module, Slips::ROOT_INPUT));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(51.651, 54.391)), module, Slips::SCALE_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.164, 54.181)), module, Slips::SCALE_INPUT));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(51.651, 68.938)), module, Slips::SLIPS_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.164, 68.727)), module, Slips::SLIPS_INPUT));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(51.651, 82.852)), module, Slips::SLIP_RANGE_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.164, 82.641)), module, Slips::SLIP_RANGE_INPUT));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(18.552, 54.391)), module, Slips::GENERATE_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.854, 54.181)), module, Slips::GENERATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.854, 25.72)), module, Slips::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.854, 40.056)), module, Slips::RESET_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.854, 87.912)), module, Slips::SEQUENCE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.028, 87.912)), module, Slips::GATE_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.028, 96.134)), module, Slips::GATE_LIGHT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.412, 87.912)), module, Slips::SLIP_GATE_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(31.412, 96.134)), module, Slips::SLIP_GATE_LIGHT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.854, 67.462)), module, Slips::QUANTIZE_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.552, 67.673)), module, Slips::QUANTIZE_OUTPUT));

		float start_x = box.size.x / 2 - RACK_GRID_WIDTH * 4.5;
		float start_y = box.size.y - RACK_GRID_WIDTH * 4.5;
		float dx = RACK_GRID_WIDTH * 1.25;
		float dy = RACK_GRID_WIDTH * 1.25;
		float x = start_x;
		float y = start_y;

		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_1_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_2_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_3_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_4_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_5_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_6_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_7_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_8_LIGHT));
		x -= 7 * dx;
		y += dy;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_9_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_10_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_11_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_12_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_13_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_14_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_15_LIGHT));
		x += dx;
		addChild(createLightCentered<LargeLight<CUDLightColors>>(Vec(x, y), module, Slips::STEP_16_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		Slips* module = dynamic_cast<Slips*>(this->module);
		assert(module);
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("root input v/oct", CHECKMARK(module->root_input_voct), [module]() { module->root_input_voct = !module->root_input_voct; }));
		module->cv_range.addMenu(module, menu);
	}
};


Model* modelSlips = createModel<Slips, SlipsWidget>("slips");