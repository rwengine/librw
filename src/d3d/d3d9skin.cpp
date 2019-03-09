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

static void* skinOpen(void* o, int32, int32) {
    skinGlobals.pipelines[PLATFORM_D3D9] = makeSkinPipeline();
    return o;
}

static void* skinClose(void* o, int32, int32) {
    return o;
}

void initSkin(void) {
    Driver::registerPlugin(PLATFORM_D3D9, 0, ID_SKIN, skinOpen, skinClose);
}

ObjPipeline* makeSkinPipeline(void) {
    ObjPipeline* pipe = new ObjPipeline(PLATFORM_D3D9);
    pipe->instanceCB = defaultInstanceCB;
    pipe->uninstanceCB = defaultUninstanceCB;
    pipe->renderCB = defaultRenderCB;
    pipe->pluginID = ID_SKIN;
    pipe->pluginData = 1;
    return pipe;
}

}  // namespace d3d9
}  // namespace rw
