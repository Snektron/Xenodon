#ifndef _XENODON_RENDER_RENDERMANAGER_H
#define _XENODON_RENDER_RENDERMANAGER_H

#include "render/RenderWorker.h"
#include "utility/Span.h"

class RenderManager {
    Span<RenderWorker> workers;

public:
    RenderManager(Span<RenderWorker> workers);
};

#endif
