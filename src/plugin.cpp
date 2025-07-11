#include "plugin.hpp"

Plugin *pluginInstance;

void init(Plugin *p)
{
  pluginInstance = p;

  p->addModel(modelSimplexandhold);
  p->addModel(modelBlank6hp);
  p->addModel(modelPolyrand);
  p->addModel(modelNoize);
  p->addModel(modelSteps);
  p->addModel(modelFibb);
  p->addModel(modelOctsclr);
  p->addModel(modelShift);
  p->addModel(modelMlt);
  p->addModel(modelMath);
  p->addModel(modelLogic);
  p->addModel(modelProbablynot);
  p->addModel(modelPolyplay);
  p->addModel(modelLights);
  p->addModel(modelSlips);
  p->addModel(modelTurnt);
  p->addModel(modelSlipspander);
  p->addModel(modelNos);
  p->addModel(modelLucc);
  p->addModel(modelPolyshuffle);
  p->addModel(modelPolycounter);

  settings_load();
}

void destroy()
{
  settings_save();
}

bool use_global_contrast[MODULES_LEN] = {
    true, true, true, true, true, true, true, true,
    true, true, true, true, true, true, true, true,
    true, true, true, true};
float global_contrast = CONTRAST_MAX;
float module_contrast[MODULES_LEN] = {
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX,
    CONTRAST_MAX};

void settings_save()
{
  json_t *rootJ = settingsToJson();
  std::string path = asset::user("alefsbits.json");
  FILE *file = fopen(path.c_str(), "w");
  if (file)
  {
    json_dumpf(rootJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
    fclose(file);
  }
  json_decref(rootJ);
}

void settings_load()
{
  std::string path = asset::user("alefsbits.json");
  FILE *file = fopen(path.c_str(), "r");
  if (file)
  {
    json_error_t error;
    json_t *rootJ = json_loadf(file, 0, &error);
    if (rootJ)
    {
      settingsFromJson(rootJ);
      json_decref(rootJ);
    }
    else
    {
      WARN("alefsbits.json: %s", error.text);
    }
    fclose(file);
  }
}

json_t *settingsToJson()
{
  json_t *rootJ = json_object();
  json_object_set_new(rootJ, "global_contrast", json_real(global_contrast));
  json_t *use_global_contrastJ = json_array();
  for (int i = 0; i < MODULES_LEN; i++)
  {
    json_array_insert_new(use_global_contrastJ, i, json_boolean(use_global_contrast[i]));
  }
  json_object_set_new(rootJ, "use_global_contrast", use_global_contrastJ);
  json_t *module_contrastJ = json_array();
  for (int i = 0; i < MODULES_LEN; i++)
  {
    json_array_insert_new(module_contrastJ, i, json_real(module_contrast[i]));
  }
  json_object_set_new(rootJ, "module_contrast", module_contrastJ);
  return rootJ;
}

void settingsFromJson(json_t *rootJ)
{
  json_t *global_contrastJ = json_object_get(rootJ, "global_contrast");
  if (global_contrastJ)
  {
    global_contrast = json_number_value(global_contrastJ);
  }
  json_t *use_global_contrastJ = json_object_get(rootJ, "use_global_contrast");
  if (use_global_contrastJ)
  {
    for (int i = 0; i < MODULES_LEN; i++)
    {
      json_t *use_global_contrastJ_i = json_array_get(use_global_contrastJ, i);
      if (use_global_contrastJ_i)
      {
        use_global_contrast[i] = json_boolean_value(use_global_contrastJ_i);
      }
    }
  }
  json_t *module_contrastJ = json_object_get(rootJ, "module_contrast");
  if (module_contrastJ)
  {
    for (int i = 0; i < MODULES_LEN; i++)
    {
      json_t *module_contrastJ_i = json_array_get(module_contrastJ, i);
      if (module_contrastJ_i)
      {
        module_contrast[i] = json_number_value(module_contrastJ_i);
      }
    }
  }
}
