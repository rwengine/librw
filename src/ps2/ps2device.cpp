#ifdef RW_PS2

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

#include "rwps2impl.h"

#define PLUGIN_ID 2

namespace rw {
namespace ps2 {

Device renderdevice = {16777215.0f,
                       0.0f,
                       null::beginUpdate,
                       null::endUpdate,
                       null::clearCamera,
                       null::showRaster,
                       null::setRenderState,
                       null::getRenderState,
                       null::im2DRenderIndexedPrimitive,
                       null::im3DTransform,
                       null::im3DRenderIndexed,
                       null::im3DEnd,
                       null::deviceSystem};

}
}  // namespace rw

#endif
