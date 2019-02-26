#include "interactive/DisplayArray.h"
#include <algorithm>

DisplayArray::DisplayArray(std::vector<std::unique_ptr<Display>>&& displays):
    displays(std::move(displays)) {
}

void DisplayArray::reconfigure(xcb_window_t xid, int16_t, int16_t, uint16_t width, uint16_t height) {
    const auto it = std::find_if(this->displays.begin(), this->displays.end(), [xid](const auto& display) {
        return display->xid() == xid;
    });

    if (it == this->displays.end())
        return;

    const auto area = vk::Rect2D(
        {0, 0}, // TODO: add monitor offset
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        }
    );

    it->get()->reconfigure(area);
}

void DisplayArray::present() {
    for (auto& display : this->displays) {
        display->present();
    }
}
