#include "plugin.hpp"
#include <thread>
#include <memory>
#include <osdialog.h>
#include <samplerate.h>
#include "inc/AudioFile.h"
#include "inc/cvRange.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

#define MAX_POLY 16


struct Polyplay : Module {
	enum ParamId {
		POLY_PARAM,
		TRIGGER_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		PHASE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	dsp::SchmittTrigger button_trigger;
	dsp::SchmittTrigger input_trigger;
	AudioFile<float> my_file;
	int file_sample_rate;
	int rack_sample_rate = APP->engine->getSampleRate();
	int num_samples;
	int num_channels;
	int current_wav_sample[MAX_POLY];
	int current_poly_channel = 0;
	bool playing[MAX_POLY] = { false };
	bool load_success = false;
	bool file_loaded = false;
	std::string loaded_file_name;
	std::string file_path;
	SRC_STATE *src;
	std::unique_ptr<std::thread> load_thread;
	std::mutex lock_thread_mutex;
	std::atomic<bool> process_audio{true};
	float phase[MAX_POLY] = { 0.0f };
	// float phase_range = 10.0f;
	CVRange phase_range;
	// bool phase_unipolar = true;

	Polyplay() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(POLY_PARAM, 1, MAX_POLY, 1, "channels");
		getParamQuantity(POLY_PARAM)->snapEnabled = true;
		configParam(TRIGGER_PARAM, 0.0, 1.0, 0.0, "trigger");
		configInput(TRIGGER_INPUT, "trigger");
		configOutput(LEFT_OUTPUT, "left/mono");
		configOutput(RIGHT_OUTPUT, "right");
		configOutput(PHASE_OUTPUT, "phase");
		if (use_global_contrast[POLYPLAY]) {
			module_contrast[POLYPLAY] = global_contrast;
		}
	}

	~Polyplay() {
		std::lock_guard<std::mutex> mg(lock_thread_mutex);
		if (load_thread) {
			load_thread->join();
		}
	}

	void resample_file(AudioFile<float> &file, int new_sample_rate) {
		int file_sample_rate = file.getSampleRate();
		int num_samples = file.getNumSamplesPerChannel();
		int num_channels = file.getNumChannels();
		int new_num_samples = (int)((float)num_samples * (float)new_sample_rate / (float)file_sample_rate);
		int processed_samples = 0;
		AudioFile<float> new_file;
		new_file.setBitDepth(file.getBitDepth());
		new_file.setSampleRate(new_sample_rate);
		new_file.setNumChannels(num_channels);
		new_file.setNumSamplesPerChannel(new_num_samples);
		float *data = new float[new_num_samples];
		for (int i = 0; i < num_channels; i++) {
			src = src_new(SRC_SINC_FASTEST, 1, NULL);
			SRC_DATA src_data;
			src_data.end_of_input = 1;
			src_data.data_in = file.samples[i].data();
			src_data.data_out = data;
			src_data.input_frames = num_samples;
			src_data.output_frames = new_num_samples;
			src_data.src_ratio = (double)new_sample_rate / (double)file_sample_rate;
			src_process(src, &src_data);
			processed_samples = src_data.output_frames_gen;
			for (int j = 0; j < processed_samples; j++) {
				new_file.samples[i][j] = data[j];
			}
			src_delete(src);
		}
		new_file.setAudioBufferSize(num_channels, processed_samples);
		delete[] data;
		file = new_file;
	}

	void load_from_file() {
		load_success = my_file.load(file_path);
		if (load_success) {
			file_loaded = true;
			loaded_file_name = file_path;
			file_sample_rate = my_file.getSampleRate();
			num_samples = my_file.getNumSamplesPerChannel();
			num_channels = my_file.getNumChannels();
			if (file_sample_rate != rack_sample_rate) {
				resample_file(my_file, rack_sample_rate);
			}
		}
		else {
			file_loaded = false;
		}
		file_path = "";
		process_audio = true;
	}

	void process(const ProcessArgs& args) override {
		rack_sample_rate = args.sampleRate;
		if (!process_audio) {
			return;
		}
		int poly = params[POLY_PARAM].getValue();
		outputs[PHASE_OUTPUT].setChannels(poly);
		outputs[LEFT_OUTPUT].setChannels(poly);
		outputs[RIGHT_OUTPUT].setChannels(poly);

		if (button_trigger.process(params[TRIGGER_PARAM].getValue() || input_trigger.process(inputs[TRIGGER_INPUT].getVoltage()))) {
			if (load_success) {
				playing[current_poly_channel] = true;
				current_wav_sample[current_poly_channel] = 0;
				current_poly_channel = (current_poly_channel + 1) % poly;
			}
		}

		for (int i = 0; i < poly; i++) {
			if (file_loaded && playing[i]) {
				phase[i] = (float)current_wav_sample[i] / (float)num_samples;
				if (outputs[PHASE_OUTPUT].isConnected()) {
					// if (phase_unipolar) {
					// 	float phase_out = phase[i] * phase_range;
					// 	outputs[PHASE_OUTPUT].setVoltage(phase_out, i);
					// }
					// else {
					// 	float phase_out = (phase[i] * 2 - 1) * phase_range;
					// 	outputs[PHASE_OUTPUT].setVoltage(phase_out, i);
					// }
					float phase_out = phase_range.map(phase[i]);
					outputs[PHASE_OUTPUT].setVoltage(phase_out, i);
				}
				if (current_wav_sample[i] >= num_samples) {
					playing[i] = false;
					phase[i] = 0.0f;
				}
				if (outputs[LEFT_OUTPUT].isConnected() && outputs[RIGHT_OUTPUT].isConnected()) {
					if (num_channels > 1) {
						outputs[LEFT_OUTPUT].setVoltage(my_file.samples[0][current_wav_sample[i]], i);
						outputs[RIGHT_OUTPUT].setVoltage(my_file.samples[1][current_wav_sample[i]], i);
					}
					else {
						outputs[LEFT_OUTPUT].setVoltage(my_file.samples[0][current_wav_sample[i]], i);
						outputs[RIGHT_OUTPUT].setVoltage(my_file.samples[0][current_wav_sample[i]], i);
					}
				}
				else if (outputs[LEFT_OUTPUT].isConnected() && !outputs[RIGHT_OUTPUT].isConnected()) {
					float output_sample = 0;
					for (int j = 0; j < num_channels; j++) {
						output_sample += my_file.samples[j][current_wav_sample[i]];
					}
					output_sample /= num_channels;
					outputs[LEFT_OUTPUT].setVoltage(output_sample, i);
				}
				current_wav_sample[i]++;
			}
			else {
				outputs[PHASE_OUTPUT].setVoltage(0.0f, i);
				outputs[LEFT_OUTPUT].setVoltage(0, i);
				outputs[RIGHT_OUTPUT].setVoltage(0, i);
			}
		}
	}

	void onReset() override {
		file_loaded = false;
		load_success = false;
		loaded_file_name = "";
		file_path = "";
		file_sample_rate = 0;
		num_samples = 0;
		num_channels = 0;
		current_poly_channel = 0;
		for (int i = 0; i < 16; i++) {
			playing[i] = false;
			current_wav_sample[i] = 0;
		}
	}

	void onSampleRateChange() override {
		rack_sample_rate = APP->engine->getSampleRate();
		if (file_loaded) {
			resample_file(my_file, rack_sample_rate);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "loaded_file_name", json_string(loaded_file_name.c_str()));
		json_object_set_new(rootJ, "file_loaded", json_boolean(file_loaded));
		// json_object_set_new(rootJ, "phase_range", json_real(phase_range));
		json_object_set_new(rootJ, "phase_range", phase_range.dataToJson());
		// json_object_set_new(rootJ, "phase_unipolar", json_boolean(phase_unipolar));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* loaded_file_nameJ = json_object_get(rootJ, "loaded_file_name");
		if (loaded_file_nameJ) {
			loaded_file_name = json_string_value(loaded_file_nameJ);
		}
		json_t* file_loadedJ = json_object_get(rootJ, "file_loaded");
		if (file_loadedJ) {
			file_loaded = json_boolean_value(file_loadedJ);
		}
		if (file_loaded) {
			load_success = my_file.load(loaded_file_name);
			file_sample_rate = my_file.getSampleRate();
			num_samples = my_file.getNumSamplesPerChannel();
			num_channels = my_file.getNumChannels();
			for (int i = 0; i < MAX_POLY; i++) {
				current_wav_sample[i] = 0;
			}
			current_poly_channel = 0;
		}
		// json_t* phase_rangeJ = json_object_get(rootJ, "phase_range");
		// if (phase_rangeJ) {
		// 	phase_range = json_real_value(phase_rangeJ);
		// }
		json_t* phase_rangeJ = json_object_get(rootJ, "phase_range");
		if (phase_rangeJ) {
			phase_range.dataFromJson(phase_rangeJ);
		}
		// json_t* phase_unipolarJ = json_object_get(rootJ, "phase_unipolar");
		// if (phase_unipolarJ) {
		// 	phase_unipolar = json_boolean_value(phase_unipolarJ);
		// }
	}
};


struct PolyplayWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	PolyplayWidget(Polyplay* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/polyplay.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		float x_start = RACK_GRID_WIDTH * 2;
		float y_start = RACK_GRID_WIDTH * 7 - RACK_GRID_WIDTH / 2;
		// float dx = RACK_GRID_WIDTH * 2;
		float dy = RACK_GRID_WIDTH * 2;

		float x = x_start;
		float y = y_start;

		addParam(createParamCentered<BitKnob>(Vec(x, y), module, Polyplay::POLY_PARAM));
		y += dy * 2 - RACK_GRID_WIDTH;
		addParam(createParamCentered<TL1105>(Vec(x, y), module, Polyplay::TRIGGER_PARAM));
		y += dy - RACK_GRID_WIDTH / 2;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Polyplay::TRIGGER_INPUT));
		y += dy * 2 - RACK_GRID_WIDTH / 4;
		addOutput(createOutputCentered<BitPort>(Vec(x, y), module, Polyplay::PHASE_OUTPUT));
		y += dy * 2 + RACK_GRID_WIDTH / 4;
		addOutput(createOutputCentered<BitPort>(Vec(x, y), module, Polyplay::LEFT_OUTPUT));
		y += dy;
		addOutput(createOutputCentered<BitPort>(Vec(x, y), module, Polyplay::RIGHT_OUTPUT));
	}

	void step() override {
		Polyplay* playModule = dynamic_cast<Polyplay*>(this->module);
		if (!playModule) return;
		if (use_global_contrast[POLYPLAY]) {
			module_contrast[POLYPLAY] = global_contrast;
		}
		if (module_contrast[POLYPLAY] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[POLYPLAY];
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

	void appendContextMenu(Menu* menu) override {
		Polyplay* module = dynamic_cast<Polyplay*>(this->module);
		assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[POLYPLAY]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[POLYPLAY]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                []() {
					global_contrast = module_contrast[POLYPLAY];
					use_global_contrast[POLYPLAY] = true;
                }));
            menu->addChild(contrastMenu);
        }));

		menu->addChild(new MenuSeparator());
		module->phase_range.addMenu(module, menu, "phase range");

		struct LoadWavItem : MenuItem {
			Polyplay* module;
			void onAction(const event::Action& e) override {
				char* path = osdialog_file(OSDIALOG_OPEN, "", NULL, NULL);
				if (path) {
					module->file_path = path;
					for (int i = 0; i < MAX_POLY; i++) {
						module->playing[i] = false;
						module->current_wav_sample[i] = 0;
					}
					module->current_poly_channel = 0;
					std::lock_guard<std::mutex> mg(module->lock_thread_mutex);
					if (module->load_thread) {
						module->load_thread->join();
					}
					module->process_audio = false;
					module->load_thread = std::make_unique<std::thread>([this](){this->module->load_from_file();});
					free(path);
				}
			}
		};

		menu->addChild(new MenuSeparator());
		LoadWavItem* loadWavItem = createMenuItem<LoadWavItem>("load sample");
		loadWavItem->module = module;
		menu->addChild(loadWavItem);

		if (module->file_loaded) {
			struct UnloadWavItem : MenuItem {
				Polyplay* module;
				void onAction(const event::Action& e) override {
					module->file_loaded = false;
					module->load_success = false;
					module->loaded_file_name = "";
				}
			};
			UnloadWavItem* unloadWavItem = createMenuItem<UnloadWavItem>(module->loaded_file_name);
			unloadWavItem->module = module;
			menu->addChild(unloadWavItem);
		}
	}
};


Model* modelPolyplay = createModel<Polyplay, PolyplayWidget>("polyplay");