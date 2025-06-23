#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

struct Math : Module
{
  enum ParamId
  {
    PARAMS_LEN
  };
  enum InputId
  {
    A_INPUT,
    B_INPUT,
    INPUTS_LEN
  };
  enum OutputId
  {
    ADD_OUTPUT,
    SUB_OUTPUT,
    MULT_OUTPUT,
    DIV_OUTPUT,
    MOD_OUTPUT,
    AVG_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId
  {
    LIGHTS_LEN
  };

  Math()
  {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configInput(A_INPUT, "");
    configInput(B_INPUT, "");
    configOutput(ADD_OUTPUT, "");
    configOutput(SUB_OUTPUT, "");
    configOutput(MULT_OUTPUT, "");
    configOutput(DIV_OUTPUT, "");
    configOutput(MOD_OUTPUT, "");
    configOutput(AVG_OUTPUT, "");
    if (use_global_contrast[MATH])
    {
      module_contrast[MATH] = global_contrast;
    }
  }

  void process(const ProcessArgs &args) override
  {
    int a_channels = inputs[A_INPUT].getChannels();
    int b_channels = inputs[B_INPUT].getChannels();
    int channels = std::max(a_channels, b_channels);
    for (int o = 0; o < OUTPUTS_LEN; o++)
    {
      outputs[o].setChannels(channels);
    }
    for (int c = 0; c < channels; c++)
    {
      float a = inputs[A_INPUT].getVoltage(c);
      float b = inputs[B_INPUT].getVoltage(c);
      outputs[ADD_OUTPUT].setVoltage(clamp(a + b, -10.0f, 10.0f), c);
      outputs[SUB_OUTPUT].setVoltage(clamp(a - b, -10.0f, 10.0f), c);
      outputs[MULT_OUTPUT].setVoltage(clamp(a * b, -10.0f, 10.0f), c);
      outputs[DIV_OUTPUT].setVoltage(clamp(a / b, -10.0f, 10.0f), c);
      outputs[MOD_OUTPUT].setVoltage(clamp(fmod(a, b), -10.0f, 10.0f), c);
      outputs[AVG_OUTPUT].setVoltage(clamp((a + b) / 2.0f, -10.0f, 10.0f), c);
    }
  }
};

struct MathWidget : ModuleWidget
{
  PanelBackground *panelBackground = new PanelBackground();
  SvgPanel *svgPanel;
  Inverter *inverter = new Inverter();
  MathWidget(Math *module)
  {
    setModule(module);
    svgPanel = createPanel(asset::plugin(pluginInstance, "res/math.svg"));
    setPanel(svgPanel);
    panelBackground->box.size = svgPanel->box.size;
    svgPanel->fb->addChildBottom(panelBackground);
    inverter->box.pos = Vec(0.f, 0.f);
    inverter->box.size = Vec(box.size.x, box.size.y);
    addChild(inverter);

    addInput(createInputCentered<BitPort>(mm2px(Vec(10.599, 24.981)), module, Math::A_INPUT));
    addInput(createInputCentered<BitPort>(mm2px(Vec(10.599, 36.724)), module, Math::B_INPUT));

    addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 51.547)), module, Math::ADD_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 62.079)), module, Math::SUB_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 73.563)), module, Math::MULT_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 84.639)), module, Math::DIV_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 96.023)), module, Math::MOD_OUTPUT));
    addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 106.963)), module, Math::AVG_OUTPUT));
  }

  void step() override
  {
    Math *mathModule = dynamic_cast<Math *>(this->module);
    if (!mathModule)
      return;
    if (use_global_contrast[MATH])
    {
      module_contrast[MATH] = global_contrast;
    }
    if (module_contrast[MATH] != panelBackground->contrast)
    {
      panelBackground->contrast = module_contrast[MATH];
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
    Math *module = dynamic_cast<Math *>(this->module);
    assert(module);

    menu->addChild(new MenuSeparator());

    menu->addChild(createSubmenuItem("contrast", "", [=](Menu *menu)
                                     {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[MATH]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[MATH]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                []() {
					global_contrast = module_contrast[MATH];
					use_global_contrast[MATH] = true;
                }));
            menu->addChild(contrastMenu); }));
  }
};

Model *modelMath = createModel<Math, MathWidget>("math");