#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

struct Polyshuffle : Module
{
    enum ParamId
    {
        PARAMS_LEN
    };
    enum InputId
    {
        SIGNAL_INPUT,
        SHUFFLE_TRIGGER_INPUT,
        RESET_TRIGGER_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        SIGNAL_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId
    {
        STATUS_LIGHT,
        LIGHTS_LEN
    };

    enum Mode
    {
        SHUFFLE,
        ROTATE_UP,
        ROTATE_DOWN,
        NUM_MODES
    };
    Mode mode = SHUFFLE;

    dsp::SchmittTrigger shuffle_trigger;
    dsp::SchmittTrigger reset_trigger;

    // channel map
    int channel_map[MAX_POLY];

    // light brightness
    float light_brightness = 0.0f;

    Polyshuffle()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(SIGNAL_INPUT, "polyphonic signal");
        configInput(SHUFFLE_TRIGGER_INPUT, "shuffle trigger");
        configInput(RESET_TRIGGER_INPUT, "reset trigger");
        configOutput(SIGNAL_OUTPUT, "polyphonic signal");
        reset_channel_map();
    }

    void onReset() override
    {
        reset_channel_map();
    }

    void reset_channel_map(int channels = MAX_POLY)
    {
        for (int i = 0; i < channels; i++)
        {
            channel_map[i] = i;
        }
    }

    void shuffle_channels(int channels = MAX_POLY)
    {
        for (int i = 0; i < channels; i++)
        {
            int j = random::u32() % channels;
            int tmp = channel_map[i];
            channel_map[i] = channel_map[j];
            channel_map[j] = tmp;
        }
    }

    void rotate_channels_up(int channels = MAX_POLY)
    {
        int tmp = channel_map[channels - 1];
        for (int i = channels - 1; i > 0; i--)
        {
            channel_map[i] = channel_map[i - 1];
        }
        channel_map[0] = tmp;
    }

    void rotate_channels_down(int channels = MAX_POLY)
    {
        int tmp = channel_map[0];
        for (int i = 0; i < channels - 1; i++)
        {
            channel_map[i] = channel_map[i + 1];
        }
        channel_map[channels - 1] = tmp;
    }

    void process(const ProcessArgs &args) override
    {
        int channels = inputs[SIGNAL_INPUT].getChannels();
        if (channels == 0)
            return;
        if (channels > MAX_POLY)
        {
            channels = MAX_POLY;
        }
        outputs[SIGNAL_OUTPUT].setChannels(channels);

        if (shuffle_trigger.process(inputs[SHUFFLE_TRIGGER_INPUT].getVoltage()))
        {
            switch (mode)
            {
            case SHUFFLE:
                shuffle_channels(channels);
                break;
            case ROTATE_UP:
                rotate_channels_up(channels);
                break;
            case ROTATE_DOWN:
                rotate_channels_down(channels);
                break;
            default:
                break;
            }
        }

        if (reset_trigger.process(inputs[RESET_TRIGGER_INPUT].getVoltage()))
        {
            reset_channel_map(channels);
        }

        // process each channel
        for (int i = 0; i < channels; i++)
        {
            // get input voltage
            float voltage = inputs[SIGNAL_INPUT].getVoltage(channel_map[i]);
            // set output voltage
            outputs[SIGNAL_OUTPUT].setVoltage(voltage, i);
        }

        // set light brightness
        if (shuffle_trigger.isHigh() || reset_trigger.isHigh())
        {
            light_brightness = 1.0f;
        }
        else
        {
            light_brightness -= args.sampleTime * 10.0f;
            if (light_brightness < 0.0f)
            {
                light_brightness = 0.0f;
            }
        }

        // set light
        lights[STATUS_LIGHT].setBrightness(light_brightness);
    }
};

struct PolyshuffleWidget : ModuleWidget
{
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
    PolyshuffleWidget(Polyshuffle *module)
    {
        setModule(module);
        svgPanel = createPanel(asset::plugin(pluginInstance, "res/polyshuffle.svg"));
        setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

        float y_start = RACK_GRID_WIDTH * 7;
        float dy = RACK_GRID_WIDTH;
        float x = box.size.x / 2;
        float y = y_start;

        addInput(createInputCentered<BitPort>(Vec(x, y), module, Polyshuffle::SIGNAL_INPUT));
        y += dy * 2.5f;
        x += dy / 2;
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(x, y), module, Polyshuffle::STATUS_LIGHT));
        y += dy;
        x -= dy / 2;
        addInput(createInputCentered<BitPort>(Vec(x, y), module, Polyshuffle::SHUFFLE_TRIGGER_INPUT));
        y += dy * 2.75f;
        addInput(createInputCentered<BitPort>(Vec(x, y), module, Polyshuffle::RESET_TRIGGER_INPUT));
        y += dy * 3.25f;
        addOutput(createOutputCentered<BitPort>(Vec(x, y), module, Polyshuffle::SIGNAL_OUTPUT));
    }

    void step() override
    {
        if (module)
        {
            Polyshuffle *polyshuffleModule = dynamic_cast<Polyshuffle *>(this->module);
            if (!polyshuffleModule)
                return;
            if (use_global_contrast[POLYSHUFFLE])
            {
                module_contrast[POLYSHUFFLE] = global_contrast;
            }
            if (module_contrast[POLYSHUFFLE] != panelBackground->contrast)
            {
                panelBackground->contrast = module_contrast[POLYSHUFFLE];
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
        }
        ModuleWidget::step();
    }

    void drawLayer(const DrawArgs &args, int layer) override
    {
        ModuleWidget::drawLayer(args, layer);
    }

    void draw(const DrawArgs &args) override
    {
        ModuleWidget::draw(args);
    }

    void appendContextMenu(Menu *menu) override
    {
        Polyshuffle *module = dynamic_cast<Polyshuffle *>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu *menu)
                                         {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[POLYSHUFFLE]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[POLYSHUFFLE]));
            contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                []() {
                    global_contrast = module_contrast[POLYSHUFFLE];
                    use_global_contrast[POLYSHUFFLE] = true;
                }));
            menu->addChild(contrastMenu); }));

        menu->addChild(new MenuSeparator());
        // add mode items in submenu item
        menu->addChild(createMenuLabel("mode:"));
        menu->addChild(createCheckMenuItem("shuffle", "", [=]()
                                           { return module->mode == Polyshuffle::SHUFFLE; }, [=]()
                                           { module->mode = Polyshuffle::SHUFFLE; }));
        menu->addChild(createCheckMenuItem("rotate up", "", [=]()
                                           { return module->mode == Polyshuffle::ROTATE_UP; }, [=]()
                                           { module->mode = Polyshuffle::ROTATE_UP; }));
        menu->addChild(createCheckMenuItem("rotate down", "", [=]()
                                           { return module->mode == Polyshuffle::ROTATE_DOWN; }, [=]()
                                           { module->mode = Polyshuffle::ROTATE_DOWN; }));
    }
};

Model *modelPolyshuffle = createModel<Polyshuffle, PolyshuffleWidget>("polyshuffle");