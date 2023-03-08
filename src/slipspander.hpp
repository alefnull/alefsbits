#pragma once

#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"


struct Slipspander : Module {
	enum ParamId {
		C_PARAM,
		C_SHARP_PARAM,
		D_PARAM,
		D_SHARP_PARAM,
		E_PARAM,
		F_PARAM,
		F_SHARP_PARAM,
		G_PARAM,
		G_SHARP_PARAM,
		A_PARAM,
		A_SHARP_PARAM,
		B_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		C_LIGHT,
		C_SHARP_LIGHT,
		D_LIGHT,
		D_SHARP_LIGHT,
		E_LIGHT,
		F_LIGHT,
		F_SHARP_LIGHT,
		G_LIGHT,
		G_SHARP_LIGHT,
		A_LIGHT,
		A_SHARP_LIGHT,
		B_LIGHT,
		LIGHTS_LEN
	};

	bool notes_on[12] = {false};
	std::vector<int> selected_notes;
	std::vector<int> selected_notes_prev;

	Slipspander() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(C_PARAM, 0.f, 1.f, 0.f, "C");
		configParam(C_SHARP_PARAM, 0.f, 1.f, 0.f, "C sharp");
		configParam(D_PARAM, 0.f, 1.f, 0.f, "D");
		configParam(D_SHARP_PARAM, 0.f, 1.f, 0.f, "D sharp");
		configParam(E_PARAM, 0.f, 1.f, 0.f, "E");
		configParam(F_PARAM, 0.f, 1.f, 0.f, "F");
		configParam(F_SHARP_PARAM, 0.f, 1.f, 0.f, "F sharp");
		configParam(G_PARAM, 0.f, 1.f, 0.f, "G");
		configParam(G_SHARP_PARAM, 0.f, 1.f, 0.f, "G sharp");
		configParam(A_PARAM, 0.f, 1.f, 0.f, "A");
		configParam(A_SHARP_PARAM, 0.f, 1.f, 0.f, "A sharp");
		configParam(B_PARAM, 0.f, 1.f, 0.f, "B");
        if (use_global_contrast[SLIPS]) {
            module_contrast[SLIPS] = global_contrast;
        }
	}
    void process(const ProcessArgs &args) override;
    json_t *dataToJson() override;
    void dataFromJson(json_t *rootJ) override;
};

struct SlipspanderWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	SlipspanderWidget(Slipspander* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/slipspander.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(20.827, 26.513)), module, Slipspander::B_PARAM, Slipspander::B_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(9.653, 32.627)), module, Slipspander::A_SHARP_PARAM, Slipspander::A_SHARP_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(20.827, 39.092)), module, Slipspander::A_PARAM, Slipspander::A_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(9.653, 45.171)), module, Slipspander::G_SHARP_PARAM, Slipspander::G_SHARP_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(20.827, 51.671)), module, Slipspander::G_PARAM, Slipspander::G_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(9.653, 57.715)), module, Slipspander::F_SHARP_PARAM, Slipspander::F_SHARP_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(20.827, 64.25)), module, Slipspander::F_PARAM, Slipspander::F_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(20.827, 76.829)), module, Slipspander::E_PARAM, Slipspander::E_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(9.653, 82.38)), module, Slipspander::D_SHARP_PARAM, Slipspander::D_SHARP_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(20.827, 89.408)), module, Slipspander::D_PARAM, Slipspander::D_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(9.653, 94.608)), module, Slipspander::C_SHARP_PARAM, Slipspander::C_SHARP_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<LargeSimpleLight<RedLight>>>(mm2px(Vec(20.827, 101.987)), module, Slipspander::C_PARAM, Slipspander::C_LIGHT));
	}
    void step() override;
    void appendContextMenu(Menu *menu) override;
};