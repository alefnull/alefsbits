#include "slips.hpp"
#include "plugin.hpp"

json_t *Slips::dataToJson()
{
  json_t *rootJ = json_object();
  json_t *sequenceJ = json_array();
  for (int i = 0; i < MAX_STEPS; i++)
  {
    json_t *valueJ = json_real(the_sequence[i]);
    json_array_append_new(sequenceJ, valueJ);
  }
  json_object_set_new(rootJ, "sequence", sequenceJ);
  json_t *mod_sequenceJ = json_array();
  for (int i = 0; i < MAX_STEPS; i++)
  {
    json_t *valueJ = json_real(mod_sequence[i]);
    json_array_append_new(mod_sequenceJ, valueJ);
  }
  json_object_set_new(rootJ, "mod_sequence", mod_sequenceJ);
  json_t *slipsJ = json_array();
  for (int i = 0; i < MAX_STEPS; i++)
  {
    json_t *valueJ = json_real(the_slips[i]);
    json_array_append_new(slipsJ, valueJ);
  }
  json_object_set_new(rootJ, "slips", slipsJ);
  json_object_set_new(rootJ, "cv_range", cv_range.dataToJson());
  json_object_set_new(rootJ, "mod_range", mod_range.dataToJson());
  json_object_set_new(rootJ, "slip_range", slip_range.dataToJson());
  json_object_set_new(rootJ, "remap_on_generate", json_boolean(remap_on_generate));
  json_object_set_new(rootJ, "root_input_voct", json_boolean(root_input_voct));
  json_object_set_new(rootJ, "mod_quantize", json_boolean(mod_quantize));
  json_object_set_new(rootJ, "mod_add_slips", json_boolean(mod_add_slips));
  json_object_set_new(rootJ, "mod_add_prob", json_boolean(mod_add_prob));
  json_object_set_new(rootJ, "poly_channels", json_integer(channels));
  json_object_set_new(rootJ, "poly_mod", json_boolean(poly_mod));
  return rootJ;
}

void Slips::dataFromJson(json_t *rootJ)
{
  json_t *sequenceJ = json_object_get(rootJ, "sequence");
  if (sequenceJ)
  {
    for (int i = 0; i < (int)json_array_size(sequenceJ); i++)
    {
      json_t *valueJ = json_array_get(sequenceJ, i);
      if (valueJ)
      {
        the_sequence[i] = json_number_value(valueJ);
      }
    }
  }
  json_t *mod_sequenceJ = json_object_get(rootJ, "mod_sequence");
  if (mod_sequenceJ)
  {
    for (int i = 0; i < (int)json_array_size(mod_sequenceJ); i++)
    {
      json_t *valueJ = json_array_get(mod_sequenceJ, i);
      if (valueJ)
      {
        mod_sequence[i] = json_number_value(valueJ);
      }
    }
  }
  json_t *slipsJ = json_object_get(rootJ, "slips");
  if (slipsJ)
  {
    for (int i = 0; i < (int)json_array_size(slipsJ); i++)
    {
      json_t *valueJ = json_array_get(slipsJ, i);
      if (valueJ)
      {
        the_slips[i] = json_number_value(valueJ);
      }
    }
  }
  json_t *cv_rangeJ = json_object_get(rootJ, "cv_range");
  if (cv_rangeJ)
  {
    cv_range.dataFromJson(cv_rangeJ);
  }
  json_t *mod_rangeJ = json_object_get(rootJ, "mod_range");
  if (mod_rangeJ)
  {
    mod_range.dataFromJson(mod_rangeJ);
  }
  json_t *slip_rangeJ = json_object_get(rootJ, "slip_range");
  if (slip_rangeJ)
  {
    slip_range.dataFromJson(slip_rangeJ);
  }
  json_t *remap_on_generateJ = json_object_get(rootJ, "remap_on_generate");
  if (remap_on_generateJ)
  {
    remap_on_generate = json_boolean_value(remap_on_generateJ);
  }
  json_t *root_input_voctJ = json_object_get(rootJ, "root_input_voct");
  if (root_input_voctJ)
  {
    root_input_voct = json_boolean_value(root_input_voctJ);
  }
  json_t *mod_quantizeJ = json_object_get(rootJ, "mod_quantize");
  if (mod_quantizeJ)
  {
    mod_quantize = json_boolean_value(mod_quantizeJ);
  }
  json_t *mod_add_slipsJ = json_object_get(rootJ, "mod_add_slips");
  if (mod_add_slipsJ)
  {
    mod_add_slips = json_boolean_value(mod_add_slipsJ);
  }
  json_t *mod_add_probJ = json_object_get(rootJ, "mod_add_prob");
  if (mod_add_probJ)
  {
    mod_add_prob = json_boolean_value(mod_add_probJ);
  }
  json_t *poly_channelsJ = json_object_get(rootJ, "poly_channels");
  if (poly_channelsJ)
  {
    channels = json_integer_value(poly_channelsJ);
  }
  json_t *poly_modJ = json_object_get(rootJ, "poly_mod");
  if (poly_modJ)
  {
    poly_mod = json_boolean_value(poly_modJ);
  }
}

void Slips::get_custom_scale()
{
  Slipspander *slipspander = dynamic_cast<Slipspander *>(rightExpander.module);
  if (slipspander)
  {
    custom_scale_len = (int)slipspander->selected_notes.size();
    if (custom_scale_len > 0)
    {
      custom_scale = new int[custom_scale_len];
      for (int i = 0; i < custom_scale_len; i++)
      {
        custom_scale[i] = slipspander->selected_notes[i];
      }
    }
    else
    {
      custom_scale_len = 0;
      custom_scale = NULL;
    }
  }
  else
  {
    custom_scale_len = 0;
    custom_scale = NULL;
  }
}

void Slips::generate_sequence()
{
  the_sequence.clear();
  for (int i = 0; i < MAX_STEPS; i++)
  {
    float random_value = 0.0f;
    if (remap_on_generate)
    {
      random_value = cv_range.map(random::uniform());
    }
    else
    {
      random_value = random::uniform();
    }
    the_sequence.push_back(random_value);
  }
}

void Slips::generate_mod_sequence()
{
  mod_sequence.clear();
  for (int i = 0; i < MAX_STEPS; i++)
  {
    float random_value = 0.0f;
    if (remap_on_generate)
    {
      random_value = mod_range.map(random::uniform());
    }
    else
    {
      random_value = random::uniform();
    }
    mod_sequence.push_back(random_value);
  }
}

void Slips::generate_slips(float slip_amount)
{
  the_slips.clear();
  for (int i = 0; i < MAX_STEPS; i++)
  {
    the_slips.push_back(0.0);
  }
  int num_slips = (int)(MAX_STEPS * slip_amount);
  for (int i = 0; i < num_slips; i++)
  {
    int slip_step = -1;
    while (slip_step < 0 || the_slips[slip_step] != 0.0)
    {
      slip_step = random::u32() % MAX_STEPS;
    }
    float slip_value = 0.0f;
    if (remap_on_generate)
    {
      slip_value = slip_range.map(random::uniform());
    }
    else
    {
      slip_value = random::uniform();
    }
    the_slips[slip_step] = slip_value;
  }
}

void Slips::onReset(const ResetEvent &e)
{
  current_step = starting_step;
  steps_gone_through = 0;
  Module::onReset(e);
}

void Slips::process(const ProcessArgs &args)
{
  outputs[SEQUENCE_OUTPUT].setChannels(channels + 1);
  outputs[GATE_OUTPUT].setChannels(channels + 1);
  if (poly_mod)
  {
    outputs[MOD_SEQUENCE_OUTPUT].setChannels(channels + 1);
  }
  else
  {
    outputs[MOD_SEQUENCE_OUTPUT].setChannels(1);
  }
  starting_step = params[START_PARAM].getValue() - 1;
  int num_steps = params[STEPS_PARAM].getValue();
  int root_note = params[ROOT_PARAM].getValue();
  int current_scale = params[SCALE_PARAM].getValue();
  float slip_amount = params[SLIPS_PARAM].getValue();
  float step_prob = params[PROB_PARAM].getValue();

  if (inputs[START_CV_INPUT].isConnected())
  {
    float starting_step_input = inputs[START_CV_INPUT].getVoltage();
    starting_step = (int)((starting_step_input / 10) * 64);
    starting_step %= 64;
    if (starting_step < 0)
      starting_step += 64;
    if (starting_step != last_starting_step)
    {
      current_step = starting_step + steps_gone_through;
    }
    last_starting_step = starting_step;
  }

  if (inputs[STEPS_CV_INPUT].isConnected())
  {
    float num_steps_input = inputs[STEPS_CV_INPUT].getVoltage();
    if (num_steps_input < 0 || num_steps_input > 10)
    {
      num_steps_input = clamp(num_steps_input, 0.0f, 10.0f);
    }
    num_steps = (int)((num_steps_input / 10) * 64);
  }

  if (inputs[ROOT_CV_INPUT].isConnected())
  {
    if (!root_input_voct)
    {
      float root_note_input = inputs[ROOT_CV_INPUT].getVoltage();
      if (root_note_input < 0 || root_note_input > 10)
      {
        root_note_input = clamp(root_note_input, 0.0f, 10.0f);
      }
      root_note = (int)((root_note_input / 10) * 12);
    }
    else
    {
      float root_note_input = inputs[ROOT_CV_INPUT].getVoltage();
      root_note_input += 10;
      float octave = floor(root_note_input);
      float note = root_note_input - octave;
      root_note = (int)floor(note * 12) % 12;
    }
  }

  if (inputs[SCALE_CV_INPUT].isConnected())
  {
    float current_scale_input = inputs[SCALE_CV_INPUT].getVoltage();
    if (current_scale_input < 0 || current_scale_input > 10)
    {
      current_scale_input = clamp(current_scale_input, 0.0f, 10.0f);
    }
    current_scale = (int)((current_scale_input / 10) * 11);
  }

  if (rightExpander.module && rightExpander.module->model == modelSlipspander)
  {
    expanded = true;
    if (!was_expanded)
      get_custom_scale();
  }
  else
  {
    expanded = false;
  }

  if (inputs[SLIPS_CV_INPUT].isConnected())
  {
    float slip_amount_input = inputs[SLIPS_CV_INPUT].getVoltage();
    if (slip_amount_input < 0 || slip_amount_input > 10)
    {
      slip_amount_input = clamp(slip_amount_input, 0.0f, 10.0f);
    }
    slip_amount = slip_amount_input / 10;
  }

  if (inputs[PROB_CV_INPUT].isConnected())
  {
    float step_prob_input = inputs[PROB_CV_INPUT].getVoltage();
    if (step_prob_input < 0 || step_prob_input > 10)
    {
      step_prob_input = clamp(step_prob_input, 0.0f, 10.0f);
    }
    step_prob = step_prob_input / 10;
  }

  if (clock_trigger.process(inputs[CLOCK_INPUT].getVoltage()))
  {
    current_step++;
    curr_channel++;
    if (curr_channel > channels)
      curr_channel = 0;

    if (current_step >= MAX_STEPS)
    {
      current_step = 0;
    }

    steps_gone_through++;

    if (steps_gone_through >= num_steps)
    {
      current_step = starting_step;
      steps_gone_through = 0;
      eoc_pulse.trigger(0.1);
    }

    skip_step = false;
    if (step_prob < 1)
    {
      float rand = random::uniform();
      if (rand > step_prob)
      {
        skip_step = true;
      }
    }
  }

  if (reset_trigger.process(inputs[RESET_INPUT].getVoltage()))
  {
    current_step = starting_step;
    steps_gone_through = 0;
  }

  if (generate_trigger.process(inputs[GENERATE_TRIGGER_INPUT].getVoltage()) ||
      generate_button_trigger.process(params[GENERATE_PARAM].getValue()))
  {
    generate_sequence();
  }

  if (modgen_trigger.process(inputs[MODGEN_TRIGGER_INPUT].getVoltage()) ||
      modgen_button_trigger.process(params[MODGEN_PARAM].getValue()))
  {
    generate_mod_sequence();
  }

  if (current_step == starting_step)
  {
    if (!slips_generated)
    {
      generate_slips(slip_amount);
      slips_generated = true;
    }
  }
  else
  {
    slips_generated = false;
  }

  bool is_slip = the_slips[current_step] != 0;

  float out = 0.0f;
  float mod_out = 0.0f;
  if (remap_on_generate)
  {
    out = clamp(the_sequence[current_step], -10.f, 10.f);
    mod_out = clamp(mod_sequence[current_step], -10.f, 10.f);

    if (is_slip)
    {
      out = clamp(the_sequence[current_step] + the_slips[current_step], -10.f, 10.f);
      if (mod_add_slips)
      {
        mod_out = clamp(mod_sequence[current_step] + the_slips[current_step], -10.f, 10.f);
      }
    }
  }
  else
  {
    out = clamp(cv_range.map(the_sequence[current_step]), -10.f, 10.f);
    mod_out = clamp(mod_range.map(mod_sequence[current_step]), -10.f, 10.f);

    if (is_slip)
    {
      out = clamp(cv_range.map(the_sequence[current_step]) + slip_range.map(the_slips[current_step]), -10.f, 10.f);
      if (mod_add_slips)
      {
        mod_out = clamp(mod_range.map(mod_sequence[current_step]) + slip_range.map(the_slips[current_step]), -10.f, 10.f);
      }
    }
  }

  if (skip_step)
  {
    if (outputs[SEQUENCE_OUTPUT].isConnected())
    {
      for (int c = 0; c < channels + 1; c++)
      {
        if (expanded && custom_scale != NULL && custom_scale_len > 0)
        {
          if (c == curr_channel)
            outputs[SEQUENCE_OUTPUT].setVoltage(
                quantize(last_value, root_note, custom_scale, custom_scale_len),
                c);
        }
        else
        {
          if (c == curr_channel)
            outputs[SEQUENCE_OUTPUT].setVoltage(
                quantize(last_value, root_note, current_scale), c);
        }
      }
    }
    if (outputs[GATE_OUTPUT].isConnected())
    {
      for (int c = 0; c < channels + 1; c++)
      {
        outputs[GATE_OUTPUT].setVoltage(0.f, c);
      }
    }
    if (outputs[MOD_SEQUENCE_OUTPUT].isConnected())
    {
      if (mod_add_prob)
      {
        mod_out = last_mod_value;
      }
      if (mod_quantize)
      {
        if (poly_mod)
        {
          for (int c = 0; c < channels + 1; c++)
          {
            if (expanded && custom_scale != NULL && custom_scale_len > 0)
            {
              if (c == curr_channel)
                outputs[MOD_SEQUENCE_OUTPUT].setVoltage(
                    quantize(mod_out, root_note, custom_scale,
                             custom_scale_len),
                    c);
            }
            else
            {
              if (c == curr_channel)
                outputs[MOD_SEQUENCE_OUTPUT].setVoltage(
                    quantize(mod_out, root_note, current_scale));
            }
          }
        }
        else
        {
          if (expanded && custom_scale != NULL && custom_scale_len > 0)
          {
            outputs[MOD_SEQUENCE_OUTPUT].setVoltage(
                quantize(mod_out, root_note, custom_scale, custom_scale_len));
          }
          else
          {
            outputs[MOD_SEQUENCE_OUTPUT].setVoltage(
                quantize(mod_out, root_note, current_scale));
          }
        }
      }
      else
      {
        if (poly_mod)
        {
          for (int c = 0; c < channels + 1; c++)
          {
            if (c == curr_channel)
              outputs[MOD_SEQUENCE_OUTPUT].setVoltage(mod_out, c);
          }
        }
        else
        {
          outputs[MOD_SEQUENCE_OUTPUT].setVoltage(mod_out);
        }
      }
    }
    lights[GATE_LIGHT].setBrightness(0);
  }
  else
  {
    last_value = out;
    last_mod_value = mod_out;
    if (outputs[SEQUENCE_OUTPUT].isConnected())
    {
      for (int c = 0; c < channels + 1; c++)
      {
        if (expanded && custom_scale != NULL && custom_scale_len > 0)
        {
          if (c == curr_channel)
            outputs[SEQUENCE_OUTPUT].setVoltage(
                quantize(out, root_note, custom_scale, custom_scale_len), c);
        }
        else
        {
          if (c == curr_channel)
            outputs[SEQUENCE_OUTPUT].setVoltage(
                quantize(out, root_note, current_scale), c);
        }
      }
    }
    if (outputs[GATE_OUTPUT].isConnected())
    {
      for (int c = 0; c < channels + 1; c++)
      {
        c == curr_channel ? outputs[GATE_OUTPUT].setVoltage(
                                inputs[CLOCK_INPUT].getVoltage(), c)
                          : outputs[GATE_OUTPUT].setVoltage(0.f, c);
      }
    }
    if (outputs[SLIP_GATE_OUTPUT].isConnected())
    {
      outputs[SLIP_GATE_OUTPUT].setVoltage(
          is_slip ? inputs[CLOCK_INPUT].getVoltage() : 0);
    }
    if (outputs[MOD_SEQUENCE_OUTPUT].isConnected())
    {
      if (mod_quantize)
      {
        if (poly_mod)
        {
          for (int c = 0; c < channels + 1; c++)
          {
            if (expanded && custom_scale != NULL && custom_scale_len > 0)
            {
              if (c == curr_channel)
                outputs[MOD_SEQUENCE_OUTPUT].setVoltage(
                    quantize(mod_out, root_note, custom_scale,
                             custom_scale_len),
                    c);
            }
            else
            {
              if (c == curr_channel)
                outputs[MOD_SEQUENCE_OUTPUT].setVoltage(
                    quantize(mod_out, root_note, current_scale), c);
            }
          }
        }
        else
        {
          if (expanded && custom_scale != NULL && custom_scale_len > 0)
          {
            outputs[MOD_SEQUENCE_OUTPUT].setVoltage(
                quantize(mod_out, root_note, custom_scale, custom_scale_len));
          }
          else
          {
            outputs[MOD_SEQUENCE_OUTPUT].setVoltage(
                quantize(mod_out, root_note, current_scale));
          }
        }
      }
      else
      {
        if (poly_mod)
        {
          for (int c = 0; c < channels + 1; c++)
          {
            if (c == curr_channel)
              outputs[MOD_SEQUENCE_OUTPUT].setVoltage(mod_out, c);
          }
        }
        else
        {
          outputs[MOD_SEQUENCE_OUTPUT].setVoltage(mod_out);
        }
      }
    }

    lights[GATE_LIGHT].setBrightness(inputs[CLOCK_INPUT].getVoltage() / 10);

    lights[SLIP_GATE_LIGHT].setBrightness(
        is_slip ? inputs[CLOCK_INPUT].getVoltage() / 10 : 0);
  }

  outputs[EOC_OUTPUT].setVoltage(eoc_pulse.process(args.sampleTime) ? 10.f
                                                                    : 0.f);
  lights[EOC_LIGHT].setBrightness(eoc_pulse.process(args.sampleTime) ? 1.f
                                                                     : 0.f);

  for (int i = 0; i < MAX_STEPS; i++)
  {
    lights[SEGMENT_LIGHTS + i].setBrightness(i == current_step ? 1.f : 0.f);
  }

  lights[EXPANDED_LIGHT].setBrightness(expanded ? 1.f : 0.f);

  was_expanded = expanded;
}

void SlipsWidget::step()
{
  Slips *slipsModule = dynamic_cast<Slips *>(this->module);
  if (!slipsModule)
    return;
  if (use_global_contrast[SLIPS])
  {
    module_contrast[SLIPS] = global_contrast;
  }
  if (module_contrast[SLIPS] != panelBackground->contrast)
  {
    panelBackground->contrast = module_contrast[SLIPS];
    if (panelBackground->contrast < 0.4f)
    {
      panelBackground->invert(true);
      inverter->invert = true;
    }
    else
    {
      panelBackground->invert(false);
      inverter->invert = false;
    }
    svgPanel->fb->dirty = true;
  }
  ModuleWidget::step();
}

void SlipsWidget::addExpander()
{
  Model *model = pluginInstance->getModel("slipspander");
  Module *module = model->createModule();
  APP->engine->addModule(module);
  ModuleWidget *modWidget = modelSlipspander->createModuleWidget(module);
  APP->scene->rack->setModulePosForce(modWidget,
                                      Vec(box.pos.x + box.size.x, box.pos.y));
  APP->scene->rack->addModule(modWidget);
  history::ModuleAdd *h = new history::ModuleAdd;
  h->name = "create slipspander";
  h->setModule(modWidget);
  APP->history->push(h);
}

void SlipsWidget::appendContextMenu(Menu *menu)
{
  Slips *module = dynamic_cast<Slips *>(this->module);
  assert(module);
  menu->addChild(new MenuSeparator());

  menu->addChild(createSubmenuItem("contrast", "", [=](Menu *menu)
                                   {
    Menu *contrastMenu = new Menu();
    ContrastSlider *contrastSlider =
        new ContrastSlider(&(module_contrast[SLIPS]));
    contrastSlider->box.size.x = 200.f;
    GlobalOption *globalOption =
        new GlobalOption(&(use_global_contrast[SLIPS]));
    contrastMenu->addChild(globalOption);
    contrastMenu->addChild(new MenuSeparator());
    contrastMenu->addChild(contrastSlider);
    contrastMenu->addChild(createMenuItem("set global contrast", "", []() {
      global_contrast = module_contrast[SLIPS];
      use_global_contrast[SLIPS] = true;
    }));
    menu->addChild(contrastMenu); }));
  menu->addChild(new MenuSeparator());
  menu->addChild(
      createBoolPtrMenuItem("root input v/oct", "", &module->root_input_voct));
  menu->addChild(
      createIndexPtrSubmenuItem("poly channels",
                                {"1", "2", "3", "4", "5", "6", "7", "8", "9",
                                 "10", "11", "12", "13", "14", "15", "16"},
                                &module->channels));
  menu->addChild(createBoolPtrMenuItem("poly mod output", "",
                                       &module->poly_mod));
  menu->addChild(new MenuSeparator());
  menu->addChild(
      createBoolPtrMenuItem("remap on generate", "", &module->remap_on_generate));
  module->cv_range.addMenu(module, menu, "sequence range");
  module->slip_range.addMenu(module, menu, "slip sequence range");
  module->mod_range.addMenu(module, menu, "mod sequence range");
  menu->addChild(new MenuSeparator());
  menu->addChild(createSubmenuItem("mod sequence options", "",
                                   [=](Menu *menu)
                                   {
                                     menu->addChild(createBoolPtrMenuItem("quantize mod sequence", "",
                                                                          &module->mod_quantize));
                                     menu->addChild(createBoolPtrMenuItem("apply slips to mod sequence", "",
                                                                          &module->mod_add_slips));
                                     menu->addChild(createBoolPtrMenuItem("apply step probability to mod sequence",
                                                                          "", &module->mod_add_prob));
                                   }));
  menu->addChild(new MenuSeparator());
  if (module->rightExpander.module &&
      module->rightExpander.module->model == modelSlipspander)
  {
    menu->addChild(createMenuLabel("slipspander connected"));
  }
  else
  {
    menu->addChild(
        createMenuItem("add slipspander", "", [this]()
                       { addExpander(); }));
  }
}

Model *modelSlips = createModel<Slips, SlipsWidget>("slips");