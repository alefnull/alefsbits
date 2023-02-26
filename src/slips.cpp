#include "plugin.hpp"
#include "quantizer.hpp"
#include "inc/cvRange.hpp"

#define MAX_POLY 16
#define MAX_STEPS 64


struct Slips : Module, Quantizer {
	enum ParamId {
		ROOT_PARAM,
		STEPS_PARAM,
		SCALE_PARAM,
		START_PARAM,
		GENERATE_PARAM,
		PROB_PARAM,
		SLIPS_PARAM,
		SLIP_RANGE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		ROOT_CV_INPUT,
		STEPS_CV_INPUT,
		RESET_INPUT,
		SCALE_CV_INPUT,
		START_CV_INPUT,
		GENERATE_TRIGGER_INPUT,
		PROB_CV_INPUT,
		QUANTIZE_INPUT,
		SLIPS_CV_INPUT,
		SLIP_RANGE_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		QUANTIZE_OUTPUT,
		SEQUENCE_OUTPUT,
		GATE_OUTPUT,
		SLIP_GATE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		GATE_LIGHT,
		SLIP_GATE_LIGHT,
		ENUMS(SEGMENT_LIGHTS, 64),
		LIGHTS_LEN
	};

	Slips() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(STEPS_PARAM, 1, MAX_STEPS, 16, "steps");
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
		configInput(STEPS_CV_INPUT, "steps cv");
		getInputInfo(STEPS_CV_INPUT)->description = "0V to 10V";
		configInput(ROOT_CV_INPUT, "root cv");
		getInputInfo(ROOT_CV_INPUT)->description = "0V to 10V (v/oct if enabled)";
		configInput(SCALE_CV_INPUT, "scale cv");
		getInputInfo(SCALE_CV_INPUT)->description = "0V to 10V";
		configInput(GENERATE_TRIGGER_INPUT, "generate");
		configInput(QUANTIZE_INPUT, "unquantized");
		configInput(SLIPS_CV_INPUT, "slips cv");
		getInputInfo(SLIPS_CV_INPUT)->description = "0V to 10V";
		configInput(SLIP_RANGE_CV_INPUT, "slip range cv");
		getInputInfo(SLIP_RANGE_CV_INPUT)->description = "0V to 10V";
		configOutput(SEQUENCE_OUTPUT, "sequence");
		configOutput(GATE_OUTPUT, "gate");
		configOutput(QUANTIZE_OUTPUT, "quantized");
		configOutput(SLIP_GATE_OUTPUT, "slip gate");
		configParam(START_PARAM, 1, 64, 1, "starting step");
		getParamQuantity(START_PARAM)->snapEnabled = true;
		configParam(PROB_PARAM, 0, 1, 1, "step probability", " %", 0, 100);
		configInput(START_CV_INPUT, "starting step cv");
		getInputInfo(START_CV_INPUT)->description = "0V to 10V";
		configInput(PROB_CV_INPUT, "step probability cv");
		getInputInfo(PROB_CV_INPUT)->description = "0V to 10V";
	}

	// the sequence
	float the_sequence[64] = {0.0f};
	// the slips
	float the_slips[64] = {0.0f};
	// the number of steps gone through in this cycle
	int steps_gone_through = 0;
	// the current step
	int current_step = 0;
	// the last value
	float last_value = 0.0f;
	// schmitt trigger for clock input
	dsp::SchmittTrigger clock_trigger;
	// schmitt trigger for reset input
	dsp::SchmittTrigger reset_trigger;
	// schmitt trigger for generatee input
	dsp::SchmittTrigger generate_trigger;
	// schmitt trigger for generatee manual button
	dsp::SchmittTrigger generate_button_trigger;
	// a bool to check if slips have already been generated for this cycle
	bool slips_generated = false;
	// a bool to check if root note input expects 0-10V or a v/oct value
	bool root_input_voct = false;
	// track whether the current step should be skipped
	bool skip_step = false;

	// a cv range object to convert voltages with a range of 0V to 1V into a given range
	CVRange cv_range;

	// dataToJson override
	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		// the sequence
		json_t* sequenceJ = json_array();
		for (int i = 0; i < MAX_STEPS; i++) {
			json_t* valueJ = json_real(the_sequence[i]);
			json_array_append_new(sequenceJ, valueJ);
		}
		json_object_set_new(rootJ, "sequence", sequenceJ);
		// the slips
		json_t* slipsJ = json_array();
		for (int i = 0; i < MAX_STEPS; i++) {
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
			for (int i = 0; i < (int) json_array_size(sequenceJ); i++) {
				json_t* valueJ = json_array_get(sequenceJ, i);
				if (valueJ) {
					the_sequence[i] = json_number_value(valueJ);
				}
			}
		}
		// the slips
		json_t* slipsJ = json_object_get(rootJ, "slips");
		if (slipsJ) {
			for (int i = 0; i < (int) json_array_size(slipsJ); i++) {
				json_t* valueJ = json_array_get(slipsJ, i);
				if (valueJ) {
					the_slips[i] = json_number_value(valueJ);
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
	void generate_sequence() {
		// generate the sequence
		for (int i = 0; i < MAX_STEPS; i++) {
			// generate a random value between 0 and 1
			float random_value = random::uniform();
			// add the value to the sequence
			the_sequence[i] = random_value;
		}
	}

	// function to generate "slips" to apply to the sequence,
	// given the slip amount (0 to 1) and slip range (0 to 1)
	void generate_slips(float slip_amount, float slip_range) {
		// clear the slips
		for (int i = 0; i < MAX_STEPS; i++) {
			the_slips[i] = 0.0;
		}
		// convert the slip amount to a number of steps to slip
		int num_slips =	(int) (MAX_STEPS * slip_amount);
		// generate the slips
		for (int i = 0; i < num_slips; i++) {
			// pick a random step to slip (as long as it hasn't already been slipped this cycle)
			int slip_step = -1;
			while (slip_step < 0 || the_slips[slip_step] != 0.0) {
				slip_step = random::u32() % MAX_STEPS;
			}
			// pick a random amount to slip by (-1 to 1)
			float slip_value = random::uniform() * slip_range * 2.0 - slip_range;
			// add the slip to the slip vector at the slip step
			the_slips[slip_step] = slip_value;
		}
	}

	void process(const ProcessArgs& args) override {
		// get the starting step
		int starting_step = params[START_PARAM].getValue() - 1;
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
		// get the step probability
		float step_prob = params[PROB_PARAM].getValue();
		
		// check if the starting step input is connected
		if (inputs[START_CV_INPUT].isConnected()) {
			// get the starting step input voltage
			float starting_step_input = inputs[START_CV_INPUT].getVoltage();
			// check if the starting step input voltage is out of bounds
			if (starting_step_input < 0 || starting_step_input > 10) {
				// clamp the starting step input voltage
				starting_step_input = clamp(starting_step_input, 0.0f, 10.0f);
			}
			// set the starting step
			starting_step = (int) ((starting_step_input / 10) * 64);
		}

		// check if the steps input is connected
		if (inputs[STEPS_CV_INPUT].isConnected()) {
			// get the steps input voltage
			float num_steps_input = inputs[STEPS_CV_INPUT].getVoltage();
			// check if the steps input voltage is out of bounds
			if (num_steps_input < 0 || num_steps_input > 10) {
				// clamp the steps input voltage
				num_steps_input = clamp(num_steps_input, 0.0f, 10.0f);
			}
			// set the number of steps
			num_steps = (int) ((num_steps_input / 10) * 64);
		}

		// check if the root input is connected
		if (inputs[ROOT_CV_INPUT].isConnected()) {
			// if the root input is not in v/oct mode
			if (!root_input_voct) {
				// get the root input voltage
				float root_note_input = inputs[ROOT_CV_INPUT].getVoltage();
				// check if the root input voltage is out of bounds
				if (root_note_input < 0 || root_note_input > 10) {
					// clamp the root input voltage
					root_note_input = clamp(root_note_input, 0.0f, 10.0f);
				}
				// set the root note
				root_note = (int) ((root_note_input / 10) * 12);
			}
			// if the root input is in v/oct mode
			else {
				// get the root input voltage
				float root_note_input = inputs[ROOT_CV_INPUT].getVoltage();
				// strip out the octave and convert to a note number between 0 and 11
				root_note = (int) (root_note_input * 12) % 12;
			}
		}

		// check if the scale input is connected
		if (inputs[SCALE_CV_INPUT].isConnected()) {
			// get the scale input voltage
			float current_scale_input = inputs[SCALE_CV_INPUT].getVoltage();
			// check if the scale input voltage is out of bounds
			if (current_scale_input < 0 || current_scale_input > 10) {
				// clamp the scale input voltage
				current_scale_input = clamp(current_scale_input, 0.0f, 10.0f);
			}
			// set the scale
			current_scale = (int) ((current_scale_input / 10) * 11);
		}

		// check if the slip amount input is connected
		if (inputs[SLIPS_CV_INPUT].isConnected()) {
			// get the slip amount input voltage
			float slip_amount_input = inputs[SLIPS_CV_INPUT].getVoltage();
			// check if the slip amount input voltage is out of bounds
			if (slip_amount_input < 0 || slip_amount_input > 10) {
				// clamp the slip amount input voltage
				slip_amount_input = clamp(slip_amount_input, 0.0f, 10.0f);
			}
			// set the slip amount
			slip_amount = slip_amount_input / 10;
		}

		// check if the slip range input is connected
		if (inputs[SLIP_RANGE_CV_INPUT].isConnected()) {
			// get the slip range input voltage
			float slip_range_input = inputs[SLIP_RANGE_CV_INPUT].getVoltage();
			// check if the slip range input voltage is out of bounds
			if (slip_range_input < 0 || slip_range_input > 10) {
				// clamp the slip range input voltage
				slip_range_input = clamp(slip_range_input, 0.0f, 10.0f);
			}
			// set the slip range
			slip_range = slip_range_input / 10;
		}

		// check if the step probability input is connected
		if (inputs[PROB_CV_INPUT].isConnected()) {
			// get the step probability input voltage
			float step_prob_input = inputs[PROB_CV_INPUT].getVoltage();
			// check if the step probability input voltage is out of bounds
			if (step_prob_input < 0 || step_prob_input > 10) {
				// clamp the step probability input voltage
				step_prob_input = clamp(step_prob_input, 0.0f, 10.0f);
			}
			// set the step probability
			step_prob = step_prob_input / 10;
		}

		// check if the clock input is high
		if (clock_trigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			// reset the skip flag
			skip_step = false;
			
			// increment the step
			current_step++;

			// increment the steps gone through counter
			steps_gone_through++;
			
			// get the number of steps left in the sequence
			int steps_left = num_steps - steps_gone_through;
			
			// if there are no steps left in the sequence
			if (steps_left <= 0) {
				// reset the step
				current_step = starting_step;
				// reset the steps gone through counter
				steps_gone_through = 0;
			}

			// check if the current step is higher than the number of steps
			if (current_step > MAX_STEPS) {
				// loop back around to the beginning
				current_step = 0;
			}

			// determine if the step should be skipped
			if (step_prob < 1) {
				// get a random number between 0 and 1
				float rand = random::uniform();
				// check if the random number is greater than the step probability
				if (rand > step_prob) {
					// skip the step
					skip_step = true;
				}
			}
		}
		
		// if we're on the first step
		if (current_step == starting_step) {
			// if we haven't generated new slips for this cycle
			if (!slips_generated) {
				// generate new slips
				generate_slips(slip_amount, slip_range);
				// set the slips generated flag
				slips_generated = true;
			}
		} else {
			// reset the slips generated flag
			slips_generated = false;
		}

		// check if the reset input is high
		if (reset_trigger.process(inputs[RESET_INPUT].getVoltage())) {
			// reset the step
			current_step = starting_step;
			// reset the steps gone through counter
			steps_gone_through = 0;
		}

		// check if the generatee input is high
		if (generate_trigger.process(inputs[GENERATE_TRIGGER_INPUT].getVoltage())) {
			// reset the step
			current_step = starting_step;
			// reset the steps gone through counter
			steps_gone_through = 0;
			// generate a new sequence
			generate_sequence();
		}

		// check if the generatee button is pressed
		if (generate_button_trigger.process(params[GENERATE_PARAM].getValue())) {
			// reset the step
			current_step = starting_step;
			// reset the steps gone through counter
			steps_gone_through = 0;
			// generate a new sequence
			generate_sequence();
		}

		// check if this step is a slip (the_slips[current_step] != 0)
		bool is_slip = the_slips[current_step] != 0;

		// get the voltage for the current step
		float out = clamp(the_sequence[current_step], -10.f, 10.f);

		// scale the voltage to the desired range
		out = cv_range.map(out);

		// if the step is a slip
		if (is_slip) {
			// add the slip amount to the output
			out = clamp(the_sequence[current_step] + the_slips[current_step], -10.f, 10.f);
		}

		// if the step should be skipped
		if (skip_step) {
			// check if the sequence output is connected
			if (outputs[SEQUENCE_OUTPUT].isConnected()) {
				// set the output voltage to the last value
				outputs[SEQUENCE_OUTPUT].setVoltage(quantize(last_value, root_note, current_scale));
			}
			// check if the gate output is connected
			if (outputs[GATE_OUTPUT].isConnected()) {
				// set the gate output to 0
				outputs[GATE_OUTPUT].setVoltage(0);
			}
			// set the gate light
			lights[GATE_LIGHT].setBrightness(0);
		}
		else {
			// set the last value
			last_value = out;
			// check if the sequence output is connected
			if (outputs[SEQUENCE_OUTPUT].isConnected()) {
				// set the output voltage
				outputs[SEQUENCE_OUTPUT].setVoltage(quantize(out, root_note, current_scale));
			}
			// check if the gate output is connected
			if (outputs[GATE_OUTPUT].isConnected()) {
				// set the gate output to the incoming clock signal
				outputs[GATE_OUTPUT].setVoltage(inputs[CLOCK_INPUT].getVoltage());
			}
			// check if the slip gate output is connected
			if (outputs[SLIP_GATE_OUTPUT].isConnected()) {
				// set the slip gate output to the incoming clock signal
				outputs[SLIP_GATE_OUTPUT].setVoltage(is_slip ? inputs[CLOCK_INPUT].getVoltage() : 0);
			}
			// set the gate light
			lights[GATE_LIGHT].setBrightness(inputs[CLOCK_INPUT].getVoltage() / 10);

			// set the slip gate light
			lights[SLIP_GATE_LIGHT].setBrightness(is_slip ? inputs[CLOCK_INPUT].getVoltage() / 10 : 0);
		}

		// get the input voltage to be quantized
		if (inputs[QUANTIZE_INPUT].isConnected() && outputs[QUANTIZE_OUTPUT].isConnected()) {
			int q_chans = inputs[QUANTIZE_INPUT].getChannels();
			outputs[QUANTIZE_OUTPUT].setChannels(q_chans);
			for (int ch = 0; ch < q_chans; ch++) {
				// get the input voltage
				float in = inputs[QUANTIZE_INPUT].getVoltage(ch);
				// quantize the input voltage
				float out = quantize(in, root_note, current_scale);
				// set the output voltage
				outputs[QUANTIZE_OUTPUT].setVoltage(out, ch);
			}
		}

		// set the segment lights
		for (int i = 0; i < MAX_STEPS; i++) {
			lights[SEGMENT_LIGHTS + i].setBrightness(i == current_step ? 1.f : 0.f);
		}
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.279, 24.08)), module, Slips::ROOT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(51.782, 24.08)), module, Slips::STEPS_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.279, 41.974)), module, Slips::SCALE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(51.782, 41.974)), module, Slips::START_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(18.254, 59.869)), module, Slips::GENERATE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(51.782, 59.869)), module, Slips::PROB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(51.782, 77.763)), module, Slips::SLIPS_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(51.782, 95.657)), module, Slips::SLIP_RANGE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.854, 24.08)), module, Slips::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.082, 24.08)), module, Slips::ROOT_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.586, 24.08)), module, Slips::STEPS_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.854, 41.974)), module, Slips::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.082, 41.974)), module, Slips::SCALE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.586, 41.974)), module, Slips::START_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.556, 59.869)), module, Slips::GENERATE_TRIGGER_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.586, 59.869)), module, Slips::PROB_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.258, 77.763)), module, Slips::QUANTIZE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.586, 77.763)), module, Slips::SLIPS_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.586, 95.657)), module, Slips::SLIP_RANGE_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19.745, 77.763)), module, Slips::QUANTIZE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.854, 95.657)), module, Slips::SEQUENCE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.763, 95.657)), module, Slips::GATE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(28.882, 95.657)), module, Slips::SLIP_GATE_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(23.255, 92.872)), module, Slips::GATE_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(33.541, 92.872)), module, Slips::SLIP_GATE_LIGHT));

		float start_x = box.size.x / 2 - RACK_GRID_WIDTH * 4.5;
		float start_y = box.size.y - RACK_GRID_WIDTH * 5;
		float x = start_x;
		float y = start_y;

		SegmentDisplay* first_segment_display = createWidget<SegmentDisplay>(Vec(x, y));
		first_segment_display->box.size = Vec(RACK_GRID_WIDTH * 9, RACK_GRID_WIDTH / 2);
		first_segment_display->setLights<RedLight>(module, Slips::SEGMENT_LIGHTS, 16);
		addChild(first_segment_display);
		y += RACK_GRID_WIDTH / 2;
		SegmentDisplay* second_segment_display = createWidget<SegmentDisplay>(Vec(x, y));
		second_segment_display->box.size = Vec(RACK_GRID_WIDTH * 9, RACK_GRID_WIDTH / 2);
		second_segment_display->setLights<RedLight>(module, Slips::SEGMENT_LIGHTS + 16, 16);
		addChild(second_segment_display);
		y += RACK_GRID_WIDTH / 2;
		SegmentDisplay* third_segment_display = createWidget<SegmentDisplay>(Vec(x, y));
		third_segment_display->box.size = Vec(RACK_GRID_WIDTH * 9, RACK_GRID_WIDTH / 2);
		third_segment_display->setLights<RedLight>(module, Slips::SEGMENT_LIGHTS + 32, 16);
		addChild(third_segment_display);
		y += RACK_GRID_WIDTH / 2;
		SegmentDisplay* fourth_segment_display = createWidget<SegmentDisplay>(Vec(x, y));
		fourth_segment_display->box.size = Vec(RACK_GRID_WIDTH * 9, RACK_GRID_WIDTH / 2);
		fourth_segment_display->setLights<RedLight>(module, Slips::SEGMENT_LIGHTS + 48, 16);
		addChild(fourth_segment_display);
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