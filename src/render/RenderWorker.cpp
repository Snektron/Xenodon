#include "render/RenderWorker.h"

RenderWorker::RenderWorker(DeviceContext& device, vk::Rect2D area):
    device(device), area(area) {
}
