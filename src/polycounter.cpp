#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

struct Polycounter : Module
{
    enum ParamId
    {
        START_PARAM,
        END_PARAM,
        INC_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        TRIGGER_INPUT,
        RESET_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        COUNT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId
    {
        LIGHTS_LEN
    };

    dsp::SchmittTrigger count_trigger[MAX_POLY];
    dsp::SchmittTrigger reset_trigger[MAX_POLY];
    float count[MAX_POLY] = {0.f};
    float start = 0.f;
    float end = 10.f;
    float inc = 1.f;
    float range = 10.f;

    Polycounter()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(TRIGGER_INPUT, "increment trigger");
        configInput(RESET_INPUT, "reset trigger");
        configParam(START_PARAM, -10.f, 10.f, 0.f, "start");
        configParam(END_PARAM, -10.f, 10.f, 10.f, "end");
        configParam(INC_PARAM, -10.f, 10.f, 1.f, "increment");
        configOutput(COUNT_OUTPUT, "count");
        if (use_global_contrast[POLYRAND])
        {
            module_contrast[POLYRAND] = global_contrast;
        }
    }

    json_t *dataToJson() override
    {
        json_t *rootJ = json_object();
        json_t *countJ = json_array();
        for (int c = 0; c < MAX_POLY; c++)
        {
            json_t *valueJ = json_real(count[c]);
            json_array_append_new(countJ, valueJ);
        }
        json_object_set_new(rootJ, "count", countJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override
    {
        json_t *countJ = json_object_get(rootJ, "count");
        if (countJ)
        {
            for (int c = 0; c < MAX_POLY; c++)
            {
                json_t *valueJ = json_array_get(countJ, c);
                if (valueJ)
                {
                    count[c] = json_number_value(valueJ);
                }
            }
        }
    }

    void onReset(const ResetEvent &e) override
    {
        Module::onReset(e);
        for (int c = 0; c < MAX_POLY; c++)
        {
            count[c] = 0.f;
        }
    }

    void process(const ProcessArgs &args) override
    {
        int channels = inputs[TRIGGER_INPUT].getChannels();
        int reset_chans = inputs[RESET_INPUT].getChannels();
        if (channels > MAX_POLY)
            channels = MAX_POLY;
        outputs[COUNT_OUTPUT].setChannels(channels);

        start = params[START_PARAM].getValue();
        end = params[END_PARAM].getValue();
        if (start > end)
        {
            float tmp = start;
            start = end;
            end = tmp;
        }
        inc = params[INC_PARAM].getValue();
        range = end - start;

        if (reset_chans != 1)
        {
            for (int c = 0; c < channels; c++)
            {
                if (reset_trigger[c].process(inputs[RESET_INPUT].getVoltage(c)))
                {
                    count[c] = start;
                }
            }
        }
        else
        {
            if (reset_trigger[0].process(inputs[RESET_INPUT].getVoltage(0)))
            {
                for (int c = 0; c < channels; c++)
                {
                    count[c] = start;
                }
            }
        }

        for (int c = 0; c < channels; c++)
        {
            if (count_trigger[c].process(inputs[TRIGGER_INPUT].getVoltage(c)))
            {
                count[c] += inc;
                if (count[c] > end)
                {
                    float rem = count[c] - end;
                    count[c] = start + rem;
                }
                if (count[c] < start)
                {
                    float rem = start - count[c];
                    count[c] = end - rem;
                }
            }
            outputs[COUNT_OUTPUT].setVoltage(clamp(count[c], -10.f, 10.f), c);
        }
    }
};

struct PolycounterWidget : ModuleWidget
{
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
    PolycounterWidget(Polycounter *module)
    {
        setModule(module);
        svgPanel = createPanel(asset::plugin(pluginInstance, "res/polycounter.svg"));
        setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

        float startx = box.size.x / 2.f;
        float starty = RACK_GRID_WIDTH * 6.f;
        float dy = RACK_GRID_WIDTH;
        float x = startx;
        float y = starty;
        addInput(createInputCentered<BitPort>(Vec(x, y), module, Polycounter::TRIGGER_INPUT));
        y += dy * 3.f;
        addInput(createInputCentered<BitPort>(Vec(x, y), module, Polycounter::RESET_INPUT));
        y += dy * 3.f;
        addParam(createParamCentered<SmallBitKnob>(Vec(x, y), module, Polycounter::START_PARAM));
        y += dy * 3.f;
        addParam(createParamCentered<SmallBitKnob>(Vec(x, y), module, Polycounter::END_PARAM));
        y += dy * 3.f;
        addParam(createParamCentered<SmallBitKnob>(Vec(x, y), module, Polycounter::INC_PARAM));
        y += dy * 3.25f;
        addOutput(createOutputCentered<BitPort>(Vec(x, y), module, Polycounter::COUNT_OUTPUT));
    }

    void step() override
    {
        Polycounter *counterModule = dynamic_cast<Polycounter *>(this->module);
        if (!counterModule)
            return;
        if (use_global_contrast[POLYCOUNTER])
        {
            module_contrast[POLYCOUNTER] = global_contrast;
        }
        if (module_contrast[POLYCOUNTER] != panelBackground->contrast)
        {
            panelBackground->contrast = module_contrast[POLYCOUNTER];
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
        Polycounter *module = dynamic_cast<Polycounter *>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu *menu)
                                         {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[POLYCOUNTER]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[POLYCOUNTER]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                []() {
					global_contrast = module_contrast[POLYCOUNTER];
					use_global_contrast[POLYCOUNTER] = true;
                }));
            menu->addChild(contrastMenu); }));
    }
};

Model *modelPolycounter = createModel<Polycounter, PolycounterWidget>("polycounter");