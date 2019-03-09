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
#include "rwxbox.h"

namespace rw {
namespace xbox {

static void* matfxOpen(void* o, int32, int32) {
    matFXGlobals.pipelines[PLATFORM_XBOX] = makeMatFXPipeline();
    return o;
}

static void* matfxClose(void* o, int32, int32) {
    return o;
}

void initMatFX(void) {
    Driver::registerPlugin(PLATFORM_XBOX, 0, ID_MATFX, matfxOpen, matfxClose);
}

ObjPipeline* makeMatFXPipeline(void) {
    ObjPipeline* pipe = new ObjPipeline(PLATFORM_XBOX);
    pipe->instanceCB = defaultInstanceCB;
    pipe->uninstanceCB = defaultUninstanceCB;
    pipe->pluginID = ID_MATFX;
    pipe->pluginData = 0;
    return pipe;
}

}  // namespace xbox
}  // namespace rw
