#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Blank6hp : Module
{
  enum ParamId
  {
    PARAMS_LEN
  };
  enum InputId
  {
    HIDDEN_INPUT,
    INPUTS_LEN
  };
  enum OutputId
  {
    OUTPUTS_LEN
  };
  enum LightId
  {
    LIGHTS_LEN
  };

  Blank6hp()
  {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configInput(HIDDEN_INPUT, "secret");
    if (use_global_contrast[BLANK6HP])
    {
      module_contrast[BLANK6HP] = global_contrast;
    }
  }

  float angle = 0.f;

  void process(const ProcessArgs &args) override
  {
    if (inputs[HIDDEN_INPUT].isConnected())
    {
      float in = inputs[HIDDEN_INPUT].getVoltage();
      angle = in * 36.f;
    }
    else
    {
      angle = 0.f;
    }
  }
};

struct Blank6hpWidget : ModuleWidget
{

  struct LogoWidget : SvgWidget
  {
    Module *module = NULL;
    float angle = 0.f;
    LogoWidget()
    {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/logo.svg")));
      if (module)
      {
        Blank6hp *blankModule = dynamic_cast<Blank6hp *>(module);
        if (blankModule)
        {
          angle = blankModule->angle;
        }
      }
    }
    void step() override
    {
      if (module)
      {
        Blank6hp *blankModule = dynamic_cast<Blank6hp *>(module);
        if (blankModule)
        {
          angle = blankModule->angle;
        }
      }
      SvgWidget::step();
    }
    void draw(const DrawArgs &args) override
    {
      nvgSave(args.vg);
      nvgTranslate(args.vg, box.size.x / 2.f, box.size.y / 2.f);
      nvgRotate(args.vg, angle * M_PI / 180.f);
      nvgTranslate(args.vg, -box.size.x / 2.f, -box.size.y / 2.f);
      SvgWidget::draw(args);
      nvgRestore(args.vg);
    }
  };

  PanelBackground *panelBackground = new PanelBackground();
  SvgPanel *svgPanel;
  Inverter *inverter = new Inverter();
  LogoWidget *svgLogoWidget = new LogoWidget();
  Blank6hpWidget(Blank6hp *module)
  {
    setModule(module);
    svgPanel = createPanel(asset::plugin(pluginInstance, "res/blank6hp.svg"));
    setPanel(svgPanel);

    Blank6hp *blankModule = dynamic_cast<Blank6hp *>(module);
    if (blankModule)
    {
      svgLogoWidget->module = module;
    }

    std::shared_ptr<Svg> svgLogo = APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/logo.svg"));
    svgLogoWidget->setSvg(svgLogo);
    svgLogoWidget->box.pos = Vec(box.size.x / 2.f - svgLogoWidget->box.size.x / 2.f,
                                 box.size.y / 2.f - svgLogoWidget->box.size.y / 2.f);

    panelBackground->box.size = svgPanel->box.size;
    svgPanel->fb->addChildBottom(panelBackground);
    inverter->box.pos = Vec(0.f, 0.f);
    inverter->box.size = Vec(box.size.x, box.size.y);
    addChild(inverter);
    addChild(svgLogoWidget);
    addChild(createInputCentered<EmptyPort>(Vec(box.size.x / 2.f, box.size.y - 25.f), module, Blank6hp::HIDDEN_INPUT));
  }

  void step() override
  {
    Blank6hp *blankModule = dynamic_cast<Blank6hp *>(this->module);
    if (!blankModule)
      return;
    if (use_global_contrast[BLANK6HP])
    {
      module_contrast[BLANK6HP] = global_contrast;
    }
    if (module_contrast[BLANK6HP] != panelBackground->contrast)
    {
      panelBackground->contrast = module_contrast[BLANK6HP];
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
    Blank6hp *module = dynamic_cast<Blank6hp *>(this->module);
    assert(module);

    menu->addChild(new MenuSeparator());

    menu->addChild(createSubmenuItem("contrast", "", [=](Menu *menu)
                                     {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[BLANK6HP]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[BLANK6HP]));
            contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                []() {
                    global_contrast = module_contrast[BLANK6HP];
					use_global_contrast[BLANK6HP] = true;
                }));
            menu->addChild(contrastMenu); }));
  }
};

Model *modelBlank6hp = createModel<Blank6hp, Blank6hpWidget>("blank6hp");