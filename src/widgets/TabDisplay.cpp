#include "FancyWidget.hpp"

// create a tabbed display, which should be an opaque rectangle,
// with a single horizontal "grid" of tabs, where each tab is a
// rectangle with a label and an onButton callback.
// the individual tabs should be FancyWidgets, so that they can
// be styled with a background color, border, etc.
// the tabs should be added in order, and the first tab should be
// selected by default. the tab display should be able to
// dynamically add new tabs, remove existing tabs, change the
// selected tab, and resize the grid of tabs so that they fit
// within the tab display no matter how many tabs there are.

struct TabDisplay : FancyWidget
{
    std::vector<std::string> tabLabels;
    std::vector<std::function<void()>> tabCallbacks;
    bool tabAvailable[8] = {true, true, true, true, true, true, true, true};
    int selectedTab = 0;
    float tabWidth = 100;
    NVGcolor activeTabColor = nvgRGB(0x8f, 0x3b, 0x3b);
    NVGcolor unavailableTabColor = nvgRGB(0x3b, 0x3b, 0x3b);
    NVGcolor inactiveTabColor = nvgRGB(0x1e, 0x1e, 0x1e);
    NVGcolor activeLabelColor = nvgRGB(0xff, 0xff, 0xff);
    NVGcolor unavailableLabelColor = nvgRGB(0x7f, 0x7f, 0x7f);
    NVGcolor inactiveLabelColor = nvgRGB(0xb0, 0xb0, 0xb0);
    NVGcolor tabBorderColor = nvgRGB(0x3b, 0x3b, 0x3b);

    TabDisplay()
    {
        if (tabLabels.size() > 0)
        {
            tabWidth = this->box.size.x / (float)tabLabels.size();
        }
    }

    void setActive(int index)
    {
        selectedTab = index;
    }

    void setInactive(int index)
    {
        if (selectedTab == index)
        {
            selectedTab = -1;
        }
    }

    void setAvailable(int index, bool available)
    {
        tabAvailable[index] = available;
        if (!available && selectedTab == index)
        {
            selectedTab = -1;
        }
    }

    void addTab(std::string label, std::function<void()> callback)
    {
        tabLabels.push_back(label);
        tabCallbacks.push_back(callback);
        tabWidth = this->box.size.x / (float)tabLabels.size();
    }

    void removeTab(int index)
    {
        selectedTab = index == selectedTab ? selectedTab - 1 : selectedTab;
        tabLabels.erase(tabLabels.begin() + index);
        tabCallbacks.erase(tabCallbacks.begin() + index);
        tabWidth = this->box.size.x / (float)tabLabels.size();
    }

    void draw(const DrawArgs &args) override
    {
        // draw the tab display
        for (int i = 0; i < (int)tabLabels.size(); i++)
        {
            if (tabAvailable[i])
            {
                // draw the tab background
                withPath(args, [&]
                         { withFill(args,
                                    i == selectedTab ? activeTabColor : inactiveTabColor,
                                    [&]
                                    {
                                        nvgRect(args.vg, i * tabWidth, 0,
                                                this->box.size.x / (float)tabLabels.size(),
                                                this->box.size.y);
                                    }); });

                // draw the tab label
                withPath(args, [&]
                         { withFill(
                               args,
                               i == selectedTab ? activeLabelColor : inactiveLabelColor,
                               [&]
                               {
                                   nvgFontFaceId(args.vg, APP->window->uiFont->handle);
                                   nvgFontSize(args.vg, 8);
                                   nvgTextAlign(args.vg,
                                                NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                                   nvgText(args.vg, i * tabWidth + tabWidth / 2,
                                           this->box.size.y / 2, tabLabels[i].c_str(),
                                           NULL);
                               }); });
            }
            else
            {
                // draw the tab background
                withPath(args, [&]
                         { withFill(args, unavailableTabColor, [&]
                                    { nvgRect(args.vg, i * tabWidth, 0,
                                              this->box.size.x / (float)tabLabels.size(),
                                              this->box.size.y); }); });

                // draw the tab label
                withPath(args, [&]
                         { withFill(args, unavailableLabelColor, [&]
                                    {
                        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
                        nvgFontSize(args.vg, 8);
                        nvgTextAlign(args.vg,
                                    NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                        nvgText(args.vg, i * tabWidth + tabWidth / 2,
                                this->box.size.y / 2, tabLabels[i].c_str(), NULL); }); });
            }
        }
    }

    void onButton(const event::Button &e) override
    {
        if (e.action == GLFW_PRESS)
        {
            int tab = (int)(e.pos.x / tabWidth);
            if (tab < (int)tabCallbacks.size())
            {
                tabCallbacks[tab]();
            }
        }
    }

    void step() override
    {
        // update the tab display size
        tabWidth = this->box.size.x / (float)tabLabels.size();
    }

    void onHoverKey(const event::HoverKey &e) override
    {
        if (e.action == GLFW_PRESS)
        {
            if (e.key == GLFW_KEY_LEFT)
            {
                // select the previous tab
                int tab = (int)(e.pos.x / tabWidth);
                if (tab > 0)
                {
                    tabCallbacks[tab - 1]();
                }
            }
            else if (e.key == GLFW_KEY_RIGHT)
            {
                // select the next tab
                int tab = (int)(e.pos.x / tabWidth);
                if (tab < (int)tabCallbacks.size() - 1)
                {
                    tabCallbacks[tab + 1]();
                }
            }
        }
    }

    void onHoverScroll(const event::HoverScroll &e) override
    {
        if (e.scrollDelta.x < 0)
        {
            // select the next tab
            int tab = (int)(e.pos.x / tabWidth);
            if (tab < (int)tabCallbacks.size() - 1)
            {
                tabCallbacks[tab + 1]();
            }
        }
        else if (e.scrollDelta.x > 0)
        {
            // select the previous tab
            int tab = (int)(e.pos.x / tabWidth);
            if (tab > 0)
            {
                tabCallbacks[tab - 1]();
            }
        }
    }
};
