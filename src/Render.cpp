#include <cstdio>

#include "Base.hpp"
#include "Engine.hpp"
#include "Plg.hpp"

namespace rw {

void SetRenderState(int32 state, uint32 value) {
    engine->device.setRenderState(state, (void *)(uintptr)value);
}

void SetRenderStatePtr(int32 state, void *value) {
    engine->device.setRenderState(state, value);
}

uint32 GetRenderState(int32 state) {
    return (uint32)(uintptr)engine->device.getRenderState(state);
}

void *GetRenderStatePtr(int32 state) {
    return engine->device.getRenderState(state);
}

// Im2D

namespace im2d {

float32 GetNearZ(void) {
    return engine->device.zNear;
}
float32 GetFarZ(void) {
    return engine->device.zFar;
}
void RenderLine(void *verts, int32 numVerts, int32 vert1, int32 vert2) {
    engine->device.im2DRenderLine(verts, numVerts, vert1, vert2);
}
void RenderTriangle(void *verts, int32 numVerts, int32 vert1, int32 vert2,
                    int32 vert3) {
    engine->device.im2DRenderTriangle(verts, numVerts, vert1, vert2, vert3);
}
void RenderIndexedPrimitive(PrimitiveType type, void *verts, int32 numVerts,
                            void *indices, int32 numIndices) {
    engine->device.im2DRenderIndexedPrimitive(type, verts, numVerts, indices,
                                              numIndices);
}
void RenderPrimitive(PrimitiveType type, void *verts, int32 numVerts) {
    engine->device.im2DRenderPrimitive(type, verts, numVerts);
}

}  // namespace im2d

// Im3D

namespace im3d {

void Transform(void *vertices, int32 numVertices, Matrix *world) {
    engine->device.im3DTransform(vertices, numVertices, world);
}
void RenderIndexed(PrimitiveType primType, void *indices, int32 numIndices) {
    engine->device.im3DRenderIndexed(primType, indices, numIndices);
}
void End(void) {
    engine->device.im3DEnd();
}

}  // namespace im3d

}  // namespace rw
