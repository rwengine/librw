#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../Base.hpp"
#include "../Engine.hpp"
#include "../Objects.hpp"
#include "../Pipeline.hpp"
#include "../Plg.hpp"
#include "rwd3d.h"
#include "rwd3d8.h"

namespace rw {
namespace d3d8 {
using namespace d3d;

#ifndef RW_D3D9
void defaultRenderCB(Atomic*, InstanceDataHeader*) {
}
#else

// This is a bit abandoned, use d3d9 instead

void defaultRenderCB(Atomic *atomic, InstanceDataHeader *header) {
    RawMatrix world;

    d3d::lightingCB(!!(atomic->geometry->flags & Geometry::NORMALS));

    Geometry *geo = atomic->geometry;
    d3d::setRenderState(D3DRS_LIGHTING, !!(geo->flags & rw::Geometry::LIGHT));

    Frame *f = atomic->getFrame();
    convMatrix(&world, f->getLTM());
    d3ddevice->SetTransform(D3DTS_WORLD, (D3DMATRIX *)&world);

    InstanceData *inst = header->inst;
    for (uint32 i = 0; i < header->numMeshes; i++) {
        d3d::setTexture(0, inst->material->texture);
        d3d::setMaterial(inst->material->surfaceProps, inst->material->color);

        d3d::setRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
        d3d::setRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
        if (geo->flags & Geometry::PRELIT)
            d3d::setRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
        else
            d3d::setRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);

        d3ddevice->SetFVF(inst->vertexShader);
        d3ddevice->SetStreamSource(
            0, (IDirect3DVertexBuffer9 *)inst->vertexBuffer, 0, inst->stride);
        d3ddevice->SetIndices((IDirect3DIndexBuffer9 *)inst->indexBuffer);
        uint32 numPrim = inst->primType == D3DPT_TRIANGLESTRIP
                             ? inst->numIndices - 2
                             : inst->numIndices / 3;
        d3d::flushCache();
        d3ddevice->DrawIndexedPrimitive((D3DPRIMITIVETYPE)inst->primType,
                                        inst->baseIndex, 0, inst->numVertices,
                                        0, numPrim);
        inst++;
    }
}

#endif
}  // namespace d3d8
}  // namespace rw
