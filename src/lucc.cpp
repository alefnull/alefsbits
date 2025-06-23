#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

struct Lucc : Module
{
  enum ParamId
  {
    PARAMS_LEN
  };
  enum InputId
  {
    CLOCK_INPUT,
    RESET_INPUT,
    INPUTS_LEN
  };
  enum OutputId
  {
    LUCC3_OUTPUT,
    LUCC4_OUTPUT,
    LUCC7_OUTPUT,
    LUCC11_OUTPUT,
    LUCC18_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId
  {
    LUCC3_LIGHT,
    LUCC4_LIGHT,
    LUCC7_LIGHT,
    LUCC11_LIGHT,
    LUCC18_LIGHT,
    LIGHTS_LEN
  };

  dsp::SchmittTrigger clock_trigger;
  dsp::SchmittTrigger reset_trigger;
  dsp::ClockDivider div_3;
  dsp::ClockDivider div_4;
  dsp::ClockDivider div_7;
  dsp::ClockDivider div_11;
  dsp::ClockDivider div_18;
  bool clock_high = false;
  bool out_3 = false;
  bool out_4 = false;
  bool out_7 = false;
  bool out_11 = false;
  bool out_18 = false;

  Lucc()
  {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configInput(CLOCK_INPUT, "clock");
    configInput(RESET_INPUT, "reset");
    configOutput(LUCC3_OUTPUT, "clock / 3");
    configOutput(LUCC4_OUTPUT, "clock / 4");
    configOutput(LUCC7_OUTPUT, "clock / 7");
    configOutput(LUCC11_OUTPUT, "clock / 11");
    configOutput(LUCC18_OUTPUT, "clock / 18");
    div_3.setDivision(3);
    div_4.setDivision(4);
    div_7.setDivision(7);
    div_11.setDivision(11);
    div_18.setDivision(18);
    if (use_global_contrast[LUCC])
    {
      module_contrast[LUCC] = global_contrast;
    }
  }

  void process(const ProcessArgs &args) override
  {
    if (reset_trigger.process(inputs[RESET_INPUT].getVoltage()))
    {
      reset();
    }
    clock_high = inputs[CLOCK_INPUT].getVoltage() > 0.0;
    if (!clock_high)
    {
      out_3 = false;
      out_4 = false;
      out_7 = false;
      out_11 = false;
      out_18 = false;
    }
    if (clock_trigger.process(inputs[CLOCK_INPUT].getVoltage()))
    {
      if (div_3.process())
      {
        out_3 = true;
      }
      if (div_4.process())
      {
        out_4 = true;
      }
      if (div_7.process())
      {
        out_7 = true;
      }
      if (div_11.process())
      {
        out_11 = true;
      }
      if (div_18.process())
      {
        out_18 = true;
      }
    }
    lights[LUCC3_LIGHT].setBrightness(out_3 && clock_high ? 1.0 : 0.0);
    lights[LUCC4_LIGHT].setBrightness(out_4 && clock_high ? 1.0 : 0.0);
    lights[LUCC7_LIGHT].setBrightness(out_7 && clock_high ? 1.0 : 0.0);
    lights[LUCC11_LIGHT].setBrightness(out_11 && clock_high ? 1.0 : 0.0);
    lights[LUCC18_LIGHT].setBrightness(out_18 && clock_high ? 1.0 : 0.0);
    outputs[LUCC3_OUTPUT].setVoltage(out_3 && clock_high ? 10.0 : 0.0);
    outputs[LUCC4_OUTPUT].setVoltage(out_4 && clock_high ? 10.0 : 0.0);
    outputs[LUCC7_OUTPUT].setVoltage(out_7 && clock_high ? 10.0 : 0.0);
    outputs[LUCC11_OUTPUT].setVoltage(out_11 && clock_high ? 10.0 : 0.0);
    outputs[LUCC18_OUTPUT].setVoltage(out_18 && clock_high ? 10.0 : 0.0);
  }

  void reset()
  {
    clock_trigger.reset();
    div_3.reset();
    div_4.reset();
    div_7.reset();
    div_11.reset();
    div_18.reset();
    clock_high = false;
    out_3 = false;
    out_4 = false;
    out_7 = false;
    out_11 = false;
    out_18 = false;
  }
};

struct LuccWidget : ModuleWidget
{
  PanelBackground *panelBackground = new PanelBackground();
  SvgPanel *svgPanel;
  Inverter *inverter = new Inverter();
  LuccWidget(Lucc *module)
  {
    setModule(module);
    svgPanel = createPanel(asset::plugin(pluginInstance, "res/lucc.svg"));
    setPanel(svgPanel);

    panelBackground->box.size = svgPanel->box.size;
    svgPanel->fb->addChildBottom(panelBackground);
    inverter->box.pos = Vec(0.f, 0.f);
    inverter->box.size = Vec(box.size.x, box.size.y);
    addChild(inverter);

    addInput(createInputCentered<BitPort>(mm2px(Vec(4.943, 22.719)), module, Lucc::CLOCK_INPUT));
    addInput(createInputCentered<BitPort>(mm2px(Vec(15.163, 22.719)), module, Lucc::RESET_INPUT));

    addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 37.177)), module, Lucc::LUCC3_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 53.028)), module, Lucc::LUCC4_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 68.88)), module, Lucc::LUCC7_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 84.732)), module, Lucc::LUCC11_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 100.583)), module, Lucc::LUCC18_OUTPUT));

    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 37.177)), module, Lucc::LUCC3_LIGHT));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 53.028)), module, Lucc::LUCC4_LIGHT));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 68.88)), module, Lucc::LUCC7_LIGHT));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 84.732)), module, Lucc::LUCC11_LIGHT));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 100.583)), module, Lucc::LUCC18_LIGHT));
  }

  void step() override
  {
    Lucc *luccModule = dynamic_cast<Lucc *>(this->module);
    if (!luccModule)
      return;
    if (use_global_contrast[LUCC])
    {
      module_contrast[LUCC] = global_contrast;
    }
    if (module_contrast[LUCC] != panelBackground->contrast)
    {
      panelBackground->contrast = module_contrast[LUCC];
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

  void appendContextMenu(Menu *menu) override
  {
    Lucc *module = dynamic_cast<Lucc *>(this->module);
    assert(module);

    menu->addChild(new MenuSeparator());

    menu->addChild(createSubmenuItem("contrast", "", [=](Menu *menu)
                                     {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[LUCC]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[LUCC]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                []() {
					global_contrast = module_contrast[LUCC];
					use_global_contrast[LUCC] = true;
                }));
            menu->addChild(contrastMenu); }));
  }
};

Model *modelLucc = createModel<Lucc, LuccWidget>("lucc");