/*
* Description: 
* cvRange Configurable Voltage Range for any VCV Rack Module.
* 
* Usage:
* 1. Copy this file into your project.
* 2. Add a `CVRange range` to your Module.
* 3. In your Module constructor Use `configCVParam` instead of `configParam` to configure any voltage knobs.
* 4. In your process call `range.map` on the knob values to convert them to Voltages.
* 5. Add a call to `module->range.addMenu` in your ModuleWidget.
*
* Example:
* An example of cvRange in use can be found in OneShot in the PathSet-Free collection
* Source code is available here https://github.com/patheros/PathSetModules/blob/main/src/OneShot.cpp
*
* License:
* Copyright (C) 2022  Andrew Hanson (PathSet)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "plugin.hpp"

#define CV_MIN -10.f
#define CV_MAX 10.f

/**
 * Stores a configurable Control Voltage (CV) range.
 *
 * Example use case would be as a field on a Sequencer Module.
 *
 * `dataToJson` and `dataFromJson` functions to persist the configured range.
 *
 * `addMenu` adds a configuration option to the drop-down menu.
 */
struct CVRange {

	/**
	 * Stores the first voltage value for the range.
	 * 
	 * In the UI this is labeled "Min" for minimum, but if this is the larger of the two values it will function as the maximum.
	 */
	float cv_a;

	/**
	 * Stores the second voltage value for the range.
	 *
	 * In the UI this is labeled "Max" for maximum, but if this is the lower of the two values it will function as the minimum.
	 */
	float cv_b;

	///Internal snapshot of the range full voltage range.
	float range;

	///Internal snapshot of the minimum voltage value for this range.
	float min;

	///Constructs a default CVRange with a range of +/- 1V.
	CVRange(){
		cv_a = -1;
		cv_b = 1;
		updateInternal();
	}

	/**
	 * Constructs a CVRange with a given minimum and maximum.
	 *
	 * Note cv_a is always set to min and cv_b is set to max, but if cv_a is greater than cv_b, cv_a will function as a the maximum and cv_b will function as the minimum.
	 */
	CVRange(float min, float max){
		cv_a = min;
		cv_b = max;
		updateInternal();
	}

	/**
	 * Returns a json_t object that stores the information for this CVRange.
	 *
	 * Expected use is calling it from inside dataToJson on the module.
	 *
	 * Example:
	 *
	 *     json_t *dataToJson() override{
	 *         json_t *jobj = json_object();
	 *         json_object_set_new(jobj, "range", range.dataToJson());
	 *         return jobj;
	 *     }
	 *
	 */
	json_t *dataToJson() {
		json_t *jobj = json_object();
		json_object_set_new(jobj, "a", json_real(cv_a));
		json_object_set_new(jobj, "b", json_real(cv_b));
		return jobj;
	}

	/**
	 * Sets this CVRange to the data present in a json_t object.
	 *
	 * If json_t is not a JSON_OBJECT, then the data on this CVRange is not modified.
	 *
	 * Expected use is calling it from inside dataFromJson on the module.
	 *
	 * Example:
	 *
	 *     void dataFromJson(json_t *jobj) override {
	 *         range.dataFromJson(json_object_get(jobj, "range"));
	 *     }
	 *
	 */
	void dataFromJson(json_t *jobj) {		
		if(json_typeof(jobj) == JSON_OBJECT){
			cv_a = json_real_value(json_object_get(jobj, "a"));
			cv_b = json_real_value(json_object_get(jobj, "b"));
			updateInternal();
		}		
	}

	///Must be called after cv_a or cv_b is updated
	void updateInternal(){
		range = std::abs(cv_a - cv_b);
		min = std::min(cv_a, cv_b);
	}

	/**
	 * Converts value from a parameter to voltage.
	 * 
	 * Example:
	 * 
	 *     float paramValue = params[CV_1_PARAM].getValue();
	 *     float voltage = range.map(paramValue);
	 *     outputs[SEQUENCE_OUTPUT].setVoltage(voltage);
	 *
	 */
	float map(float zero_to_one){
		return range * zero_to_one + min;
	}

	///Performs inverse of the `map` function
	float invMap(float cv_value){
		return (cv_value - min) / range;
	}

	/**
	 * Adds a "Range" menu to the passed in menu
	 *
	 * Expected use is calling this from appendContextMenu in the ModuleWidget
	 *
	 * Example:
	 *
	 *     void appendContextMenu(Menu* menu) override {
	 *         auto module = dynamic_cast<MyModule*>(this->module);
	 *         module->range.addMenu(module,menu);
	 *     }
	 *
	 */
	void addMenu(Module* module, Menu* menu, std::string menuName = "range"){

		//Wrapper for cv_a and cv_b to interface with CVTextFiled and CVSlider
		struct CVQuantity : Quantity {
			float* value_pointer;
			CVRange* range;

			CVQuantity(CVRange* range, float* value_pointer) {
				this->value_pointer = value_pointer;
				this->range = range;
			}

			void setValue(float value) override {
				*value_pointer = clamp(value, CV_MIN, CV_MAX);
				range->updateInternal();
			}

			float getValue() override {
				return *value_pointer;
			}
			
			float getMinValue() override {return CV_MIN;}
			float getMaxValue() override {return CV_MAX;}
			float getDisplayValue() override {return *value_pointer;}

			std::string getUnit() override {
				return "V";
			}
		};

		//Text Field for editing cv_a or cv_b in the menu
		struct CVTextFiled : ui::TextField {
			Quantity* quantity;

			CVTextFiled(Quantity* quantity) {
				this->quantity = quantity;
				box.size.x = 100;
				updateText();
			}

			void updateText(){				
				text = quantity->getDisplayValueString();
			}

			void onSelectKey(const SelectKeyEvent& e) override {
				if (e.action == GLFW_PRESS && (e.key == GLFW_KEY_ENTER || e.key == GLFW_KEY_KP_ENTER)) {
					quantity->setDisplayValueString(text);
				}

				if (!e.getTarget())
					TextField::onSelectKey(e);
			}
		};

		//Slider for editing cv_a or cv_b in the menu
		struct CVSlider : ui::Slider {
			CVTextFiled* textField;
			CVSlider(CVRange* range, float* value_pointer) {
				quantity = new CVQuantity(range, value_pointer);
				box.size.x = 200.f;
			}
			~CVSlider() {
				delete quantity;
			}
			void onDragEnd(const DragEndEvent& e) override {
				Slider::onDragEnd(e);
				textField->updateText();
			}
		};

		std::string s_min = std::to_string((int)std::floor(min));
		std::string s_max = std::to_string((int)std::ceil(min+range));
		std::string curLabel = s_min + "V to " + s_max + "V";

		menu->addChild(createSubmenuItem(menuName, curLabel,
			[=](Menu* menu) {
				menu->addChild(createSubmenuItem("custom", "",
					[=](Menu* menu) {
						{
							menu->addChild(createMenuLabel("min value"));
							auto slider = new CVSlider(this,&cv_a);
							auto textField = new CVTextFiled(slider->quantity);
							slider->textField = textField;
							menu->addChild(textField);
							menu->addChild(slider);
						}
						{
							menu->addChild(createMenuLabel("max value"));
							auto slider = new CVSlider(this,&cv_b);
							auto textField = new CVTextFiled(slider->quantity);
							slider->textField = textField;
							menu->addChild(textField);
							menu->addChild(slider);
						}
					}
				));

				struct Preset{
					std::string label;
					float min;
					float max;
					Preset(std::string label, float min, float max){
						this->label = label;
						this->min = min;
						this->max = max;
					}
				};

				//Preset constants for the CVRange menu
				const int PRESET_COUNT = 12;
				const Preset preset [PRESET_COUNT] = {
					Preset("+/-10V",-10,10),
					Preset("+/-5V",-5,5),
					Preset("+/-4V",-4,4),
					Preset("+/-3V",-3,3),
					Preset("+/-2V",-2,2),
					Preset("+/-1V",-1,1),
					Preset("0V-10V",0,10),
					Preset("0V-5V",0,5),
					Preset("0V-4V",0,4),
					Preset("0V-3V",0,3),
					Preset("0V-2V",0,2),
					Preset("0V-1V",0,1),
				};

				for(int pi = 0; pi < PRESET_COUNT; pi ++){
					bool checkmark = (min == preset[pi].min) && ((min + range) == preset[pi].max);
					menu->addChild(createMenuItem(preset[pi].label, CHECKMARK(checkmark), [=]() { 
						cv_a = preset[pi].min;
						cv_b = preset[pi].max;
						updateInternal();
					}));
				}
			}
		));
	}

};

/**
 * ParamQuantity for any CV Knob. Not required, but makes the knob show the selected Voltage in the tooltip and edit menu.
 *
 * WARNING: the range field on this must explicitly be set or this will crash.
 *
 * A helper function `configCVParam` is provided to make this easier
 *
 * Helper, One-Line Example:
 *
 *     void appendContextMenu(Menu* menu) override {
 *         ...
 *         configCVParam(CV_1_PARAM, this, &range, "CV 1");
 *         ...
 *     }
 *
 * Non-Helper, One-Line Example:
 *
 *     void appendContextMenu(Menu* menu) override {
 *         ...
 *         configParam<CVRangeParamQuantity>(CV_PARAM, 0.f, 1.f, 0.5f, "CV", "V")->range = &range;
 *         ...
 *     }
 *
 * Non-Helper, Multi-Line Example:
 *
 *     void appendContextMenu(Menu* menu) override {
 *         ...
 *         {
 *             auto param = configParam<CVRangeParamQuantity>(CV_PARAM, 0.f, 1.f, 0.5f, "CV", "V");
 *             param->range = &range;
 *         }
 *         ...
 *     }
 *
 */
struct CVRangeParamQuantity : ParamQuantity  {
	CVRange* range;
	float getDisplayValue() override {
		float v = getValue();
		return range->map(v);
	}
	void setDisplayValue(float v) override {
		setValue(range->invMap(v));
	}
};

///See CVRangeParamQuantity for details
template <class TParamQuantity = CVRangeParamQuantity>
TParamQuantity* configCVParam(int paramId, Module* module, CVRange* range, std::string name = "") {
	TParamQuantity* q = module->configParam<TParamQuantity>(paramId,0.0f,1.1f,0.5f,name,"V");
	q->range = range;
	return q;
}