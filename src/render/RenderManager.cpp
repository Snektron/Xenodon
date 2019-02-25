#include "render/RenderManager.h"

RenderManager::RenderManager(Span<RenderWorker> workers):
    workers(workers) {
}
