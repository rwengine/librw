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
#include "rwps2.h"
#include "rwps2plg.h"

#define PLUGIN_ID ID_MATFX

namespace rw {
namespace ps2 {

static void *matfxOpen(void *o, int32, int32) {
    matFXGlobals.pipelines[PLATFORM_PS2] = makeMatFXPipeline();
    return o;
}

static void *matfxClose(void *o, int32, int32) {
    return o;
}

void initMatFX(void) {
    Driver::registerPlugin(PLATFORM_PS2, 0, ID_MATFX, matfxOpen, matfxClose);
}

ObjPipeline *makeMatFXPipeline(void) {
    MatPipeline *pipe = new MatPipeline(PLATFORM_PS2);
    pipe->pluginID = ID_MATFX;
    pipe->pluginData = 0;
    pipe->attribs[AT_XYZ] = &attribXYZ;
    pipe->attribs[AT_UV] = &attribUV;
    pipe->attribs[AT_RGBA] = &attribRGBA;
    pipe->attribs[AT_NORMAL] = &attribNormal;
    uint32 vertCount = MatPipeline::getVertCount(0x3C5, 4, 3, 3);
    pipe->setTriBufferSizes(4, vertCount);
    pipe->vifOffset = pipe->inputStride * vertCount;
    pipe->uninstanceCB = genericUninstanceCB;

    ObjPipeline *opipe = new ObjPipeline(PLATFORM_PS2);
    opipe->pluginID = ID_MATFX;
    opipe->pluginData = 0;
    opipe->groupPipeline = pipe;
    return opipe;
}

}  // namespace ps2
}  // namespace rw
