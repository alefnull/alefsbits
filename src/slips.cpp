#include "plugin.hpp"
#include "slips.hpp"

// dataToJson override
json_t* Slips::dataToJson() {
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
void Slips::dataFromJson(json_t* rootJ) {
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

// function to get the custom scale from slipspander
// it is a vector of ints, so we need to convert it to an array
void Slips::get_custom_scale() {
	Slipspander *slipspander = dynamic_cast<Slipspander*>(rightExpander.module);
	if (slipspander) {
		custom_scale_len = (int)slipspander->selected_notes.size();
		if (custom_scale_len > 0) {
			custom_scale = new int[custom_scale_len];
			for (int i = 0; i < custom_scale_len; i++) {
				custom_scale[i] = slipspander->selected_notes[i];
			}
		}
		else {
			custom_scale_len = 0;
			custom_scale = NULL;
		}
	}
	else {
		custom_scale_len = 0;
		custom_scale = NULL;
	}
}

// function to generate a new sequence,
// given the number of steps, root note, and scale
void Slips::generate_sequence() {
	// clear the sequence
	the_sequence.clear();
	// generate the sequence
	for (int i = 0; i < MAX_STEPS; i++) {
		// generate a random value between 0 and 1
		float random_value = random::uniform();
		// add the value to the sequence
		the_sequence.push_back(random_value);
	}
}

// function to generate "slips" to apply to the sequence,
// given the slip amount (0 to 1) and slip range (0 to 1)
void Slips::generate_slips(float slip_amount, float slip_range) {
	// clear the slips
	the_slips.clear();
	// generate the slip vector
	for (int i = 0; i < MAX_STEPS; i++) {
		the_slips.push_back(0.0);
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

void Slips::process(const ProcessArgs& args) {
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
			// add 10 to the root input voltage to keep it positive
			root_note_input += 10;
			// get the octave
			float octave = floor(root_note_input);
			// strip out the octave
			float note = root_note_input - octave;
			// set the root note (0 to 11)
			root_note = (int)floor(note * 12) % 12;
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

	if (rightExpander.module && rightExpander.module->model == modelSlipspander) {
		expanded = true;
		if (!was_expanded) get_custom_scale();
	}
	else {
		expanded = false;
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
			// outputs[SEQUENCE_OUTPUT].setVoltage(quantize(last_value, root_note, current_scale));
			if (!expanded) {
				outputs[SEQUENCE_OUTPUT].setVoltage(quantize(last_value, root_note, current_scale));
			} else {
				outputs[SEQUENCE_OUTPUT].setVoltage(quantize(last_value, root_note, custom_scale, custom_scale_len));
			}
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
			if (expanded && custom_scale != NULL && custom_scale_len > 0) {
				outputs[SEQUENCE_OUTPUT].setVoltage(quantize(out, root_note, custom_scale, custom_scale_len));
			} else {
				outputs[SEQUENCE_OUTPUT].setVoltage(quantize(out, root_note, current_scale));
			}
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
			if (expanded && custom_scale != NULL && custom_scale_len > 0) {
				out = quantize(in, root_note, custom_scale, custom_scale_len);
			} else {
				out = quantize(in, root_note, current_scale);
			}
			// set the output voltage
			outputs[QUANTIZE_OUTPUT].setVoltage(out, ch);
		}
	}

	// set the segment lights
	for (int i = 0; i < MAX_STEPS; i++) {
		lights[SEGMENT_LIGHTS + i].setBrightness(i == current_step ? 1.f : 0.f);
	}

	// set the was_expanded flag
	was_expanded = expanded;
}

	
void SlipsWidget::step() {
	Slips* slipsModule = dynamic_cast<Slips*>(this->module);
	if (!slipsModule) return;
	if (use_global_contrast[SLIPS]) {
		module_contrast[SLIPS] = global_contrast;
	}
	if (module_contrast[SLIPS] != panelBackground->contrast) {
		panelBackground->contrast = module_contrast[SLIPS];
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

void SlipsWidget::appendContextMenu(Menu* menu) {
	Slips* module = dynamic_cast<Slips*>(this->module);
	assert(module);
	menu->addChild(new MenuSeparator());

	menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
		Menu* contrastMenu = new Menu();
		ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[SLIPS]));
		contrastSlider->box.size.x = 200.f;
		GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[SLIPS]));
		contrastMenu->addChild(globalOption);
		contrastMenu->addChild(new MenuSeparator());
		contrastMenu->addChild(contrastSlider);
		contrastMenu->addChild(createMenuItem("set global contrast", "",
			[module]() {
				global_contrast = module_contrast[SLIPS];
			}));
		menu->addChild(contrastMenu);
	}));
	menu->addChild(new MenuSeparator());
	menu->addChild(createMenuItem("root input v/oct", CHECKMARK(module->root_input_voct), [module]() { module->root_input_voct = !module->root_input_voct; }));
	module->cv_range.addMenu(module, menu);
}

Model* modelSlips = createModel<Slips, SlipsWidget>("slips");