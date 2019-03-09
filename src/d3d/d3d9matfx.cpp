#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../Animation.hpp"

#include "../Base.hpp"
#include "../Engine.hpp"
#include "../Objects.hpp"
#include "../Error.hpp"
#include "../Pipeline.hpp"
#include "../Plg.hpp"
#include "../rwplugins.h"
#include "rwd3d.h"
#include "rwd3d9.h"

namespace rw {
namespace d3d9 {
using namespace d3d;

static void* matfxOpen(void* o, int32, int32) {
    matFXGlobals.pipelines[PLATFORM_D3D9] = makeMatFXPipeline();
    return o;
}

static void* matfxClose(void* o, int32, int32) {
    return o;
}

void initMatFX(void) {
    Driver::registerPlugin(PLATFORM_D3D9, 0, ID_MATFX, matfxOpen, matfxClose);
}

ObjPipeline* makeMatFXPipeline(void) {
    ObjPipeline* pipe = new ObjPipeline(PLATFORM_D3D9);
    pipe->instanceCB = defaultInstanceCB;
    pipe->uninstanceCB = defaultUninstanceCB;
    pipe->renderCB = defaultRenderCB;
    pipe->pluginID = ID_MATFX;
    pipe->pluginData = 0;
    return pipe;
}

}  // namespace d3d9
}  // namespace rw
