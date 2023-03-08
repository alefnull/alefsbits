#include "plugin.hpp"
#include "slipspander.hpp"
#include "slips.hpp"


void Slipspander::process(const ProcessArgs& args) {
	// clear the selected notes
	selected_notes.clear();
	// check if the notes are on
	for (int i = 0; i < 12; i++) {
		if (params[C_PARAM + i].getValue() > 0.5f) {
			notes_on[i] = true;
			selected_notes.push_back(i);
		} else {
			notes_on[i] = false;
		}
	}

	if (leftExpander.module && leftExpander.module->model == modelSlips) {
		if (selected_notes != selected_notes_prev) {
			Slips* slipsModule = dynamic_cast<Slips*>(leftExpander.module);
			if (slipsModule) {
				slipsModule->get_custom_scale();
			}
		}
	}

	// set the lights
	for (int i = 0; i < 12; i++) {
		lights[i].setBrightness(notes_on[i] ? 1.f : 0.f);
	}

	selected_notes_prev = selected_notes;
}

json_t* Slipspander::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "notes_on", json_pack("[b, b, b, b, b, b, b, b, b, b, b, b]",
		notes_on[0], notes_on[1], notes_on[2], notes_on[3], notes_on[4], notes_on[5], notes_on[6], notes_on[7], notes_on[8], notes_on[9], notes_on[10], notes_on[11]));
	return rootJ;
}

void Slipspander::dataFromJson(json_t* rootJ) {
	json_t* notes_onJ = json_object_get(rootJ, "notes_on");
	if (notes_onJ) {
		for (int i = 0; i < 12; i++) {
			json_t* note_onJ = json_array_get(notes_onJ, i);
			if (note_onJ) {
				notes_on[i] = json_boolean_value(note_onJ);
			}
		}
	}
}


void SlipspanderWidget::step() {
	Slipspander* spanderModule = dynamic_cast<Slipspander*>(this->module);
	if (!spanderModule) return;
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

void SlipspanderWidget::appendContextMenu(Menu* menu) {
	Slipspander* module = dynamic_cast<Slipspander*>(this->module);
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
}


Model* modelSlipspander = createModel<Slipspander, SlipspanderWidget>("slipspander");