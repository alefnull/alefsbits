#include "plugin.hpp"
#include <thread>
#include <memory>
#include <osdialog.h>
#include <samplerate.h>
#include "inc/AudioFile.h"

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

	Polyplay() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(POLY_PARAM, 1, MAX_POLY, 1, "channels");
		getParamQuantity(POLY_PARAM)->snapEnabled = true;
		configParam(TRIGGER_PARAM, 0.0, 1.0, 0.0, "trigger");
		configInput(TRIGGER_INPUT, "trigger");
		configOutput(LEFT_OUTPUT, "left/mono");
		configOutput(RIGHT_OUTPUT, "right");
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
				if (current_wav_sample[i] >= num_samples) {
					playing[i] = false;
				}
			}
			else {
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
	}
};


struct PolyplayWidget : ModuleWidget {
	PolyplayWidget(Polyplay* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/polyplay.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		float x_start = RACK_GRID_WIDTH * 2;
		float y_start = RACK_GRID_WIDTH * 7 - RACK_GRID_WIDTH / 2;
		float dx = RACK_GRID_WIDTH * 2;
		float dy = RACK_GRID_WIDTH * 2;

		float x = x_start;
		float y = y_start;

		addParam(createParamCentered<RoundBlackKnob>(Vec(x, y), module, Polyplay::POLY_PARAM));
		y += dy * 2;
		addParam(createParamCentered<TL1105>(Vec(x, y), module, Polyplay::TRIGGER_PARAM));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Polyplay::TRIGGER_INPUT));
		y += dy * 3 + RACK_GRID_WIDTH / 2;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Polyplay::LEFT_OUTPUT));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Polyplay::RIGHT_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Polyplay* module = dynamic_cast<Polyplay*>(this->module);
		assert(module);

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
		LoadWavItem* loadWavItem = createMenuItem<LoadWavItem>("Load Sample");
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