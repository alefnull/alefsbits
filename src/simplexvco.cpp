#include "plugin.hpp"
#include "inc/SimplexOSC.cpp"

#define MAX_POLY 16

const float SCALE_MAX = 3.f;
const float SCALE_MIN = 0.1f;
const float DETAIL_MIN = 1.f;
const float DETAIL_MAX = 8.f;

struct Simplexvco : Module {
	enum ParamId {
		FREQUENCY_PARAM,
		X_PARAM,
		Y_PARAM,
		SCALE_PARAM,
		DETAIL_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FREQUENCY_INPUT,
		X_INPUT,
		Y_INPUT,
		SCALE_INPUT,
		DETAIL_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SimplexOSC osc[MAX_POLY];
	float prev_pitch[MAX_POLY];

	Simplexvco() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQUENCY_PARAM, -3.f, 3.f, 0.f, "pitch");
		configParam(X_PARAM, 0.f, 2.f, 1.f, "x");
		configParam(Y_PARAM, 0.f, 2.f, 1.f, "y");
		configParam(SCALE_PARAM, SCALE_MIN, SCALE_MAX, 1.5f, "scale");
		configParam(DETAIL_PARAM, DETAIL_MIN, DETAIL_MAX, DETAIL_MIN, "detail");
		configInput(FREQUENCY_INPUT, "frequency");
		configInput(X_INPUT, "x");
		configInput(Y_INPUT, "y");
		configInput(SCALE_INPUT, "scale");
		configInput(DETAIL_INPUT, "detail");
		configOutput(OUT_OUTPUT, "signal");
		for (auto i = 0; i < MAX_POLY; ++i) {
			prev_pitch[i] = 999999.f;
		}
	}

	void process(const ProcessArgs& args) override {
		int chans = std::max(1, inputs[FREQUENCY_INPUT].getChannels());
		outputs[OUT_OUTPUT].setChannels(chans);

		for (auto c = 0; c < chans; c++) {
			float pitch = params[FREQUENCY_PARAM].getValue();
			if (inputs[FREQUENCY_INPUT].isConnected()) {
				pitch += inputs[FREQUENCY_INPUT].getPolyVoltage(c);
				pitch = clamp(pitch, -3.f, 3.f);
			}
			pitch = clamp(pitch, -3.f, 3.f);
			if (pitch != prev_pitch[c]) {
				osc[c].set_pitch(pitch);
				prev_pitch[c] = pitch;
			}
			float scale = params[SCALE_PARAM].getValue();
			if (inputs[SCALE_INPUT].isConnected()) {
				scale += inputs[SCALE_INPUT].getPolyVoltage(c) / 5.f;
				scale = clamp(scale, SCALE_MIN, SCALE_MAX);
			}
			float detail = params[DETAIL_PARAM].getValue();
			if (inputs[DETAIL_INPUT].isConnected()) {
				detail += inputs[DETAIL_INPUT].getPolyVoltage(c) * 0.8f;
				detail = clamp(detail, DETAIL_MIN, DETAIL_MAX);
			}
			float x = params[X_PARAM].getValue();
			if (inputs[X_INPUT].isConnected()) {
				x += inputs[X_INPUT].getPolyVoltage(c) / 5.f;
				x = clamp(x, 0.f, 3.f);
			}
			float y = params[Y_PARAM].getValue();
			if (inputs[Y_INPUT].isConnected()) {
				y += inputs[Y_INPUT].getPolyVoltage(c) / 5.f;
				y = clamp(y, 0.f, 3.f);
			}
			osc[c].step(args.sampleTime);
			float out = osc[c].get_norm_osc(detail, x, y, 0.5f, scale);
			outputs[OUT_OUTPUT].setVoltage(out, c);
		}
	}

};


struct SimplexvcoWidget : ModuleWidget {
	SimplexvcoWidget(Simplexvco* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/simplexvco.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.498, 26.85)), module, Simplexvco::FREQUENCY_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.498, 43.993)), module, Simplexvco::X_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.498, 57.211)), module, Simplexvco::Y_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.498, 71.114)), module, Simplexvco::SCALE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.498, 85.985)), module, Simplexvco::DETAIL_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 26.85)), module, Simplexvco::FREQUENCY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 43.993)), module, Simplexvco::X_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 57.211)), module, Simplexvco::Y_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 71.114)), module, Simplexvco::SCALE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 85.985)), module, Simplexvco::DETAIL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 104.573)), module, Simplexvco::OUT_OUTPUT));
	}
};


Model* modelSimplexvco = createModel<Simplexvco, SimplexvcoWidget>("simplexvco");