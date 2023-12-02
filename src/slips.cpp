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
	// mod sequence
	json_t* mod_sequenceJ = json_array();
	for (int i = 0; i < MAX_STEPS; i++) {
		json_t* valueJ = json_real(mod_sequence[i]);
		json_array_append_new(mod_sequenceJ, valueJ);
	}
	json_object_set_new(rootJ, "mod_sequence", mod_sequenceJ);
	// the slips
	json_t* slipsJ = json_array();
	for (int i = 0; i < MAX_STEPS; i++) {
		json_t* valueJ = json_real(the_slips[i]);
		json_array_append_new(slipsJ, valueJ);
	}
	json_object_set_new(rootJ, "slips", slipsJ);
	// the cv range
	json_object_set_new(rootJ, "cv_range", cv_range.dataToJson());
	// the mod range
	json_object_set_new(rootJ, "mod_range", mod_range.dataToJson());
	// the slip range
	json_object_set_new(rootJ, "slip_range", slip_range.dataToJson());
	// root input voct
	json_object_set_new(rootJ, "root_input_voct", json_boolean(root_input_voct));
	// mod quantize bool
	json_object_set_new(rootJ, "mod_quantize", json_boolean(mod_quantize));
	// mod add slips bool
	json_object_set_new(rootJ, "mod_add_slips", json_boolean(mod_add_slips));
	// mod add prob bool
	json_object_set_new(rootJ, "mod_add_prob", json_boolean(mod_add_prob));
	// poly channels
	json_object_set_new(rootJ, "poly_channels", json_integer(channels));
	// poly mod bool
	json_object_set_new(rootJ, "poly_mod", json_boolean(poly_mod));
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
	// mod sequence
	json_t* mod_sequenceJ = json_object_get(rootJ, "mod_sequence");
	if (mod_sequenceJ) {
		for (int i = 0; i < (int) json_array_size(mod_sequenceJ); i++) {
			json_t* valueJ = json_array_get(mod_sequenceJ, i);
			if (valueJ) {
				mod_sequence[i] = json_number_value(valueJ);
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
	// the mod range
	json_t* mod_rangeJ = json_object_get(rootJ, "mod_range");
	if (mod_rangeJ) {
		mod_range.dataFromJson(mod_rangeJ);
	}
	// the slip range
	json_t* slip_rangeJ = json_object_get(rootJ, "slip_range");
	if (slip_rangeJ) {
		slip_range.dataFromJson(slip_rangeJ);
	}
	// root input voct
	json_t* root_input_voctJ = json_object_get(rootJ, "root_input_voct");
	if (root_input_voctJ) {
		root_input_voct = json_boolean_value(root_input_voctJ);
	}
	// mod quantize bool
	json_t* mod_quantizeJ = json_object_get(rootJ, "mod_quantize");
	if (mod_quantizeJ) {
		mod_quantize = json_boolean_value(mod_quantizeJ);
	}
	// mod add slips bool
	json_t* mod_add_slipsJ = json_object_get(rootJ, "mod_add_slips");
	if (mod_add_slipsJ) {
		mod_add_slips = json_boolean_value(mod_add_slipsJ);
	}
	// mod add prob bool
	json_t* mod_add_probJ = json_object_get(rootJ, "mod_add_prob");
	if (mod_add_probJ) {
		mod_add_prob = json_boolean_value(mod_add_probJ);
	}
	// poly channels
	json_t* poly_channelsJ = json_object_get(rootJ, "poly_channels");
	if (poly_channelsJ) {
		channels = json_integer_value(poly_channelsJ);
	}
	// poly mod bool
	json_t* poly_modJ = json_object_get(rootJ, "poly_mod");
	if (poly_modJ) {
		poly_mod = json_boolean_value(poly_modJ);
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
		float random_value = cv_range.map(random::uniform());
		// add the value to the sequence
		the_sequence.push_back(random_value);
	}
}

// function to generate a new mod sequence
void Slips::generate_mod_sequence() {
	// clear the mod sequence
	mod_sequence.clear();
	// generate the mod sequence
	for (int i = 0; i < MAX_STEPS; i++) {
		// generate a random value between 0 and 1
		float random_value = mod_range.map(random::uniform());
		// add the value to the mod sequence
		mod_sequence.push_back(random_value);
	}
}

// function to generate "slips" to apply to the sequence,
// given the slip amount (0 to 1) and slip range (0 to 1)
void Slips::generate_slips(float slip_amount) {
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
		// float slip_value = random::uniform() * slip_range * 2.0 - slip_range;
		float slip_value = slip_range.map(random::uniform());
		// add the slip to the slip vector at the slip step
		the_slips[slip_step] = slip_value;
	}
}

void Slips::onReset(const ResetEvent & e) {
	// reset the step
	current_step = starting_step;
	// reset the steps gone through counter
	steps_gone_through = 0;
	// call the base class reset
	Module::onReset(e);
}

void Slips::process(const ProcessArgs& args) {
	// set output channels
	outputs[SEQUENCE_OUTPUT].setChannels(channels + 1);
	outputs[GATE_OUTPUT].setChannels(channels + 1);
	if (poly_mod) {
		outputs[MOD_SEQUENCE_OUTPUT].setChannels(channels + 1);
	}
	else {
		outputs[MOD_SEQUENCE_OUTPUT].setChannels(1);
	}
	// get the starting step
	starting_step = params[START_PARAM].getValue() - 1;
	// get the number of steps
	int num_steps = params[STEPS_PARAM].getValue();
	// get the root note
	int root_note = params[ROOT_PARAM].getValue();
	// get the scale
	int current_scale = params[SCALE_PARAM].getValue();
	// get the slip amount
	float slip_amount = params[SLIPS_PARAM].getValue();
	// get the step probability
	float step_prob = params[PROB_PARAM].getValue();
	
	// check if the starting step input is connected
	if (inputs[START_CV_INPUT].isConnected()) {
		// get the starting step input voltage
		float starting_step_input = inputs[START_CV_INPUT].getVoltage();
		// set the starting step
		starting_step = (int) ((starting_step_input / 10) * 64);
		starting_step %= 64;

		if (starting_step != last_starting_step) {
			current_step = starting_step + steps_gone_through;
		}
		last_starting_step = starting_step;
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
		// increment the step
		current_step++;
		// increment curr_channel % channels
		curr_channel++;
		if (curr_channel > channels) curr_channel = 0;

		// check if the current step is higher than the number of steps
		if (current_step >= MAX_STEPS) {
			// loop back around to the beginning
			current_step = 0;
		}

		// increment the steps gone through counter
		steps_gone_through++;

		// if there are no steps left in the sequence
		if (steps_gone_through >= num_steps) {
			// reset the step
			current_step = starting_step;
			// reset the steps gone through counter
			steps_gone_through = 0;
			// generate an eoc pulse
			eoc_pulse.trigger(0.1);
		}

		// reset the skip flag
		skip_step = false;
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
	
	// check if the reset input is high
	if (reset_trigger.process(inputs[RESET_INPUT].getVoltage())) {
		// reset the step
		current_step = starting_step;
		// reset the steps gone through counter
		steps_gone_through = 0;
	}

	// check if the generate input is high
	if (generate_trigger.process(inputs[GENERATE_TRIGGER_INPUT].getVoltage())
		|| generate_button_trigger.process(params[GENERATE_PARAM].getValue())) {
		// generate a new sequence
		generate_sequence();
	}

	// check if the mod generate input is high
	if (modgen_trigger.process(inputs[MODGEN_TRIGGER_INPUT].getVoltage())
		|| modgen_button_trigger.process(params[MODGEN_PARAM].getValue())) {
		generate_mod_sequence();
	}

	// if we're on the first step
	if (current_step == starting_step) {
		// if we haven't generated new slips for this cycle
		if (!slips_generated) {
			// generate new slips
			generate_slips(slip_amount);
			// set the slips generated flag
			slips_generated = true;
		}
	} else {
		// reset the slips generated flag
		slips_generated = false;
	}

	// check if this step is a slip (the_slips[current_step] != 0)
	bool is_slip = the_slips[current_step] != 0;

	// get the voltage for the current step
	float out = clamp(the_sequence[current_step], -10.f, 10.f);
	float mod_out = clamp(mod_sequence[current_step], -10.f, 10.f);

	// if the step is a slip
	if (is_slip) {
		// add the slip amount to the output
		out = clamp(the_sequence[current_step] + the_slips[current_step], -10.f, 10.f);
		if (mod_add_slips) {
			mod_out = clamp(mod_sequence[current_step] + the_slips[current_step], -10.f, 10.f);
		}
	}

	// if the step should be skipped
	if (skip_step) {
		// check if the sequence output is connected
		if (outputs[SEQUENCE_OUTPUT].isConnected()) {
			// set the output voltage to the last value
			// outputs[SEQUENCE_OUTPUT].setVoltage(quantize(last_value, root_note, current_scale));
			for (int c = 0; c < channels + 1; c++) {
				if (expanded && custom_scale != NULL && custom_scale_len > 0) {
					if (c == curr_channel)
						outputs[SEQUENCE_OUTPUT].setVoltage(quantize(last_value, root_note, custom_scale, custom_scale_len), c);
				} else {
					if (c == curr_channel)
						outputs[SEQUENCE_OUTPUT].setVoltage(quantize(last_value, root_note, current_scale), c);
				}
			}
		}
		// check if the gate output is connected
		if (outputs[GATE_OUTPUT].isConnected()) {
			// set the gate output to 0
			for (int c = 0; c < channels + 1; c++) {
				outputs[GATE_OUTPUT].setVoltage(0.f, c);
			}
		}
		if (outputs[MOD_SEQUENCE_OUTPUT].isConnected()) {
			if (mod_add_prob) {
				mod_out = last_mod_value;
			}
			if (mod_quantize) {
				if (poly_mod) {
					for (int c = 0; c < channels + 1; c++) {
						if (expanded && custom_scale != NULL && custom_scale_len > 0) {
							if (c == curr_channel)
								outputs[MOD_SEQUENCE_OUTPUT].setVoltage(quantize(mod_out, root_note, custom_scale, custom_scale_len), c);
						} else {
							if (c == curr_channel)
								outputs[MOD_SEQUENCE_OUTPUT].setVoltage(quantize(mod_out, root_note, current_scale));
						}
					}
				}
				else {
					if (expanded && custom_scale != NULL && custom_scale_len > 0) {
						outputs[MOD_SEQUENCE_OUTPUT].setVoltage(quantize(mod_out, root_note, custom_scale, custom_scale_len));
					} else {
						outputs[MOD_SEQUENCE_OUTPUT].setVoltage(quantize(mod_out, root_note, current_scale));
					}
				}
			} else {
				if (poly_mod) {
					for (int c = 0; c < channels + 1; c++) {
						if (c == curr_channel)
							outputs[MOD_SEQUENCE_OUTPUT].setVoltage(mod_out, c);
					}
				}
				else {
					outputs[MOD_SEQUENCE_OUTPUT].setVoltage(mod_out);
				}
			}
		}
		// set the gate light
		lights[GATE_LIGHT].setBrightness(0);
	}
	else {
		// set the last value
		last_value = out;
		last_mod_value = mod_out;
		// check if the sequence output is connected
		if (outputs[SEQUENCE_OUTPUT].isConnected()) {
			// set the output voltage
			for (int c = 0; c < channels + 1; c++) {
				if (expanded && custom_scale != NULL && custom_scale_len > 0) {
					if (c == curr_channel)
						outputs[SEQUENCE_OUTPUT].setVoltage(quantize(out, root_note, custom_scale, custom_scale_len), c);
				} else {
					if (c == curr_channel)
						outputs[SEQUENCE_OUTPUT].setVoltage(quantize(out, root_note, current_scale), c);
				}
			}
		}
		// check if the gate output is connected
		if (outputs[GATE_OUTPUT].isConnected()) {
			// set the gate output to the incoming clock signal
			for (int c = 0; c < channels + 1; c++) {
				c == curr_channel ?
					outputs[GATE_OUTPUT].setVoltage(inputs[CLOCK_INPUT].getVoltage(), c)
					: outputs[GATE_OUTPUT].setVoltage(0.f, c);
			}
		}
		// check if the slip gate output is connected
		if (outputs[SLIP_GATE_OUTPUT].isConnected()) {
			// set the slip gate output to the incoming clock signal
			outputs[SLIP_GATE_OUTPUT].setVoltage(is_slip ? inputs[CLOCK_INPUT].getVoltage() : 0);
		}
		// check if the mod sequence output is connected
		if (outputs[MOD_SEQUENCE_OUTPUT].isConnected()) {
			if (mod_quantize) {
				if (poly_mod) {
					for (int c = 0; c < channels + 1; c++) {
						if (expanded && custom_scale != NULL && custom_scale_len > 0) {
							if (c == curr_channel)
								outputs[MOD_SEQUENCE_OUTPUT].setVoltage(quantize(mod_out, root_note, custom_scale, custom_scale_len), c);
						} else {
							if (c == curr_channel)
								outputs[MOD_SEQUENCE_OUTPUT].setVoltage(quantize(mod_out, root_note, current_scale), c);
						}
					}
				}
				else {
					if (expanded && custom_scale != NULL && custom_scale_len > 0) {
						outputs[MOD_SEQUENCE_OUTPUT].setVoltage(quantize(mod_out, root_note, custom_scale, custom_scale_len));
					} else {
						outputs[MOD_SEQUENCE_OUTPUT].setVoltage(quantize(mod_out, root_note, current_scale));
					}
				}
			}
			else {
				if (poly_mod) {
					for (int c = 0; c < channels + 1; c++) {
						if (c == curr_channel)
							outputs[MOD_SEQUENCE_OUTPUT].setVoltage(mod_out, c);
					}
				}
				else {
					outputs[MOD_SEQUENCE_OUTPUT].setVoltage(mod_out);
				}
			}
		}

		// set the gate light
		lights[GATE_LIGHT].setBrightness(inputs[CLOCK_INPUT].getVoltage() / 10);

		// set the slip gate light
		lights[SLIP_GATE_LIGHT].setBrightness(is_slip ? inputs[CLOCK_INPUT].getVoltage() / 10 : 0);
	}

	// set the eoc output
	outputs[EOC_OUTPUT].setVoltage(eoc_pulse.process(args.sampleTime) ? 10.f : 0.f);
	// set the eoc light
	lights[EOC_LIGHT].setBrightness(eoc_pulse.process(args.sampleTime) ? 1.f : 0.f);

	// set the segment lights
	for (int i = 0; i < MAX_STEPS; i++) {
		lights[SEGMENT_LIGHTS + i].setBrightness(i == current_step ? 1.f : 0.f);
	}

	// set the expanded light
	lights[EXPANDED_LIGHT].setBrightness(expanded ? 1.f : 0.f);

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

void SlipsWidget::addExpander() {
	Model* model = pluginInstance->getModel("slipspander");
	Module* module = model->createModule();
	APP->engine->addModule(module);
	ModuleWidget* modWidget = modelSlipspander->createModuleWidget(module);
	APP->scene->rack->setModulePosForce(modWidget, Vec(box.pos.x + box.size.x, box.pos.y));
	APP->scene->rack->addModule(modWidget);
	history::ModuleAdd* h = new history::ModuleAdd;
	h->name = "create slipspander";
	h->setModule(modWidget);
	APP->history->push(h);
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
			[]() {
				global_contrast = module_contrast[SLIPS];
				use_global_contrast[SLIPS] = true;
			}));
		menu->addChild(contrastMenu);
	}));
	menu->addChild(new MenuSeparator());
	menu->addChild(createBoolPtrMenuItem("root input v/oct", "", &module->root_input_voct));
	menu->addChild(createIndexPtrSubmenuItem("channels", {"1","2","3","4","5","6","7","8",
														  "9","10","11","12","13","14","15","16"},
														  &module->channels));
	menu->addChild(createBoolPtrMenuItem("poly mod output", "", &module->poly_mod));
	menu->addChild(new MenuSeparator());
	module->cv_range.addMenu(module, menu, "sequence range");
	module->slip_range.addMenu(module, menu, "slip range");
	module->mod_range.addMenu(module, menu, "mod sequence range");
	menu->addChild(new MenuSeparator());
	menu->addChild(createBoolPtrMenuItem("quantize mod sequence", "", &module->mod_quantize));
	menu->addChild(createBoolPtrMenuItem("apply slips to mod sequence", "", &module->mod_add_slips));
	menu->addChild(createBoolPtrMenuItem("apply step probability to mod sequence", "", &module->mod_add_prob));
	menu->addChild(new MenuSeparator());
	if (module->rightExpander.module && module->rightExpander.module->model == modelSlipspander) {
		menu->addChild(createMenuLabel("slipspander connected"));
	}
	else {
		menu->addChild(createMenuItem("add slipspander", "",
			[this]() {
				addExpander();
			}));
	}
}

Model* modelSlips = createModel<Slips, SlipsWidget>("slips");