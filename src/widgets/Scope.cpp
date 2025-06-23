#include "Scope.hpp"

Scope::Scope(ScopeData *data) : data(data) {}

void Scope::onButton(const event::Button &e)
{
  if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS)
  {
    data->scopeMode[data->activeChannel] = (data->scopeMode[data->activeChannel] + 1) % 2;
    e.consume(this);
    return;
  }
}

void Scope::onHover(const event::Hover &e) { e.consume(this); }

void Scope::onHoverScroll(const event::HoverScroll &e)
{
  if (e.scrollDelta.y > 0)
  {
    data->timeScale = std::clamp(data->timeScale * 0.9f, 0.1f, 10.f);
  }
  else if (e.scrollDelta.y < 0)
  {
    data->timeScale = std::clamp(data->timeScale * 1.1f, 0.1f, 10.f);
  }

  e.consume(this);
}

std::pair<float, float> Scope::rangeForMode()
{
  if (data->scopeMode[data->activeChannel] == 1)
  {
    return {0.f, 10.f};
  }

  return {-10.f, 10.f};
}

float Scope::calculateX(int i)
{
  return (float)i / (data->buffer[data->activeChannel].size - 1) * box.size.x;
}

float Scope::calculateY(float min, float max, float value)
{
  auto range = max - min;
  return (value - min) / range * box.size.y;
}

void Scope::drawCurve(const DrawArgs &args,
                      std::function<float(float, float)> clamp,
                      Vec gradientPoint)
{
  auto range = rangeForMode();
  auto valueMin = range.first;
  auto valueMax = range.second;

  withPath(args, [=]()
           { withStroke(args, 1.f, data->wavePrimaryColor, [=]()
                        {
            float zeroY = calculateY(valueMin, valueMax, data->zeroThreshold[data->activeChannel]);
            zeroY = box.size.y - zeroY;
            nvgMoveTo(args.vg, 0, zeroY);
            nvgLineTo(args.vg, box.size.x, zeroY);

            for (int i = data->buffer[data->activeChannel].size - 1; i >= 0; i--) {
                auto value = data->buffer[data->activeChannel].get(i).first;
                float x = calculateX(i);
                float y = calculateY(valueMin, valueMax, value);
                y = box.size.y - y;
                y = clamp(y, zeroY);
                nvgLineTo(args.vg, x, y);
            }

            nvgLineTo(args.vg, 0, zeroY);
            NVGpaint paint = nvgLinearGradient(args.vg,
                                               box.size.x / 2, zeroY,
                                               gradientPoint.x, gradientPoint.y,
                                               NVGcolor{0, 0, 0, 0},
                                               data->wavePrimaryColor);
            nvgFillPaint(args.vg, paint);
            nvgFill(args.vg); }); });
}

void Scope::drawMinCurve(const DrawArgs &args)
{
  if (!data)
  {
    return;
  }

  auto min = [](float a, float b)
  { return std::min(a, b); };
  drawCurve(args, min, Vec(box.size.x / 2, 0));
}

void Scope::drawMaxCurve(const DrawArgs &args)
{
  if (!data)
  {
    return;
  }

  auto max = [](float a, float b)
  { return std::max(a, b); };
  drawCurve(args, max, Vec(box.size.x / 2, box.size.y));
}

void Scope::drawWave(const DrawArgs &args)
{
  if (!data)
  {
    return;
  }

  nvgScissor(args.vg, 0, 0, box.size.x, box.size.y);
  drawMinCurve(args);
  drawMaxCurve(args);
}

void Scope::drawTriggers(const DrawArgs &args)
{
  if (!data)
  {
    return;
  }

  for (int i = 0; i < data->buffer[data->activeChannel].size; i++)
  {
    auto triggered = data->buffer[data->activeChannel].get(i).second;
    if (triggered)
    {
      float x = calculateX(i);
      withStroke(args, 1.f, data->triggerColor, [=]()
                 { withPath(args, [=]()
                            {
                    nvgMoveTo(args.vg, x, 0);
                    nvgLineTo(args.vg, x, box.size.y); }); });
    }
  }
}

void Scope::drawGridline(const DrawArgs &args, float percent)
{
  if (!data)
  {
    return;
  }

  auto quarterY = box.size.y - box.size.y * percent;
  withPath(args, [=]()
           { withStroke(args, 1.f, data->gridColor, [=]
                        {
            nvgMoveTo(args.vg, 0, quarterY);
            nvgLineTo(args.vg, box.size.x, quarterY); }); });
}

void Scope::drawGridlines(const DrawArgs &args)
{
  if (!data)
  {
    return;
  }

  if (data->scopeMode[data->activeChannel] == 0)
  {
    drawGridline(args, 0.25f);
    drawGridline(args, 0.75f);
  }
  else
  {
    drawGridline(args, 0.5f);
  }
}

void Scope::drawBackground(const DrawArgs &args)
{
  if (!data)
  {
    return;
  }

  withFill(args, data->backgroundColor, [=]
           { withPath(args,
                      [=]()
                      { nvgRect(args.vg, 0, 0, box.size.x, box.size.y); }); });
}

void Scope::draw(const DrawArgs &args)
{
  OpaqueWidget::draw(args);
  drawBackground(args);
  drawGridlines(args);
  drawTriggers(args);
  drawWave(args);
}
