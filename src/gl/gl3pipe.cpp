#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../Base.hpp"
#include "../Engine.hpp"
#include "../Objects.hpp"
#include "../Error.hpp"
#include "../Pipeline.hpp"
#include "../Plg.hpp"
#ifdef RW_OPENGL
#include <GL/glew.h>
#endif
#include "rwgl3.h"
#include "rwgl3shader.h"

namespace rw {
namespace gl3 {

// TODO: make some of these things platform-independent

#ifdef RW_OPENGL

static void instance(rw::ObjPipeline *rwpipe, Atomic *atomic) {
    ObjPipeline *pipe = (ObjPipeline *)rwpipe;
    Geometry *geo = atomic->geometry;
    // TODO: allow for REINSTANCE
    if (geo->instData) return;
    InstanceDataHeader *header =
        rwNewT(InstanceDataHeader, 1, MEMDUR_EVENT | ID_GEOMETRY);
    MeshHeader *meshh = geo->meshHeader;
    geo->instData = header;
    header->platform = PLATFORM_GL3;

    header->serialNumber = 0;
    header->numMeshes = meshh->numMeshes;
    header->primType = meshh->flags == 1 ? GL_TRIANGLE_STRIP : GL_TRIANGLES;
    header->totalNumVertex = geo->numVertices;
    header->totalNumIndex = meshh->totalIndices;
    header->inst =
        rwNewT(InstanceData, header->numMeshes, MEMDUR_EVENT | ID_GEOMETRY);

    header->indexBuffer =
        rwNewT(uint16, header->totalNumIndex, MEMDUR_EVENT | ID_GEOMETRY);
    InstanceData *inst = header->inst;
    Mesh *mesh = meshh->getMeshes();
    uint32 offset = 0;
    for (uint32 i = 0; i < header->numMeshes; i++) {
        findMinVertAndNumVertices(mesh->indices, mesh->numIndices,
                                  &inst->minVert, &inst->numVertices);
        assert(inst->minVert != 0xFFFFFFFF);
        inst->numIndex = mesh->numIndices;
        inst->material = mesh->material;
        inst->vertexAlpha = 0;
        inst->program = 0;
        inst->offset = offset;
        memcpy((uint8 *)header->indexBuffer + inst->offset, mesh->indices,
               inst->numIndex * 2);
        offset += inst->numIndex * 2;
        mesh++;
        inst++;
    }

    header->vertexBuffer = nil;
    header->numAttribs = 0;
    header->attribDesc = nil;
    header->ibo = 0;
    header->vbo = 0;

    glGenBuffers(1, &header->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, header->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, header->totalNumIndex * 2,
                 header->indexBuffer, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    pipe->instanceCB(geo, header);
}

static void uninstance(rw::ObjPipeline *rwpipe, Atomic *atomic) {
    assert(0 && "can't uninstance");
}

static void render(rw::ObjPipeline *rwpipe, Atomic *atomic) {
    ObjPipeline *pipe = (ObjPipeline *)rwpipe;
    Geometry *geo = atomic->geometry;
    // TODO: allow for REINSTANCE
    if (geo->instData == nil) pipe->instance(atomic);
    assert(geo->instData != nil);
    assert(geo->instData->platform == PLATFORM_GL3);
    if (pipe->renderCB)
        pipe->renderCB(atomic, (InstanceDataHeader *)geo->instData);
}

ObjPipeline::ObjPipeline(uint32 platform) : rw::ObjPipeline(platform) {
    this->impl.instance = gl3::instance;
    this->impl.uninstance = gl3::uninstance;
    this->impl.render = gl3::render;
    this->instanceCB = nil;
    this->uninstanceCB = nil;
    this->renderCB = nil;
}

void defaultInstanceCB(Geometry *geo, InstanceDataHeader *header) {
    AttribDesc attribs[12], *a;
    uint32 stride;

    //
    // Create attribute descriptions
    //
    a = attribs;
    stride = 0;

    // Positions
    a->index = ATTRIB_POS;
    a->size = 3;
    a->type = GL_FLOAT;
    a->normalized = GL_FALSE;
    a->offset = stride;
    stride += 12;
    a++;

    // Normals
    // TODO: compress
    bool hasNormals = !!(geo->flags & Geometry::NORMALS);
    if (hasNormals) {
        a->index = ATTRIB_NORMAL;
        a->size = 3;
        a->type = GL_FLOAT;
        a->normalized = GL_FALSE;
        a->offset = stride;
        stride += 12;
        a++;
    }

    // Prelighting
    bool isPrelit = !!(geo->flags & Geometry::PRELIT);
    if (isPrelit) {
        a->index = ATTRIB_COLOR;
        a->size = 4;
        a->type = GL_UNSIGNED_BYTE;
        a->normalized = GL_TRUE;
        a->offset = stride;
        stride += 4;
        a++;
    }

    // Texture coordinates
    for (int32 n = 0; n < geo->numTexCoordSets; n++) {
        a->index = ATTRIB_TEXCOORDS0 + n;
        a->size = 2;
        a->type = GL_FLOAT;
        a->normalized = GL_FALSE;
        a->offset = stride;
        stride += 8;
        a++;
    }

    header->numAttribs = a - attribs;
    for (a = attribs; a != &attribs[header->numAttribs]; a++)
        a->stride = stride;
    header->attribDesc =
        rwNewT(AttribDesc, header->numAttribs, MEMDUR_EVENT | ID_GEOMETRY);
    memcpy(header->attribDesc, attribs,
           header->numAttribs * sizeof(AttribDesc));

    //
    // Allocate and fill vertex buffer
    //
    uint8 *verts = rwNewT(uint8, header->totalNumVertex * stride,
                          MEMDUR_EVENT | ID_GEOMETRY);
    header->vertexBuffer = verts;

    // Positions
    for (a = attribs; a->index != ATTRIB_POS; a++)
        ;
    instV3d(VERT_FLOAT3, verts + a->offset, geo->morphTargets[0].vertices,
            header->totalNumVertex, a->stride);

    // Normals
    if (hasNormals) {
        for (a = attribs; a->index != ATTRIB_NORMAL; a++)
            ;
        instV3d(VERT_FLOAT3, verts + a->offset, geo->morphTargets[0].normals,
                header->totalNumVertex, a->stride);
    }

    // Prelighting
    if (isPrelit) {
        for (a = attribs; a->index != ATTRIB_COLOR; a++)
            ;
        int n = header->numMeshes;
        InstanceData *inst = header->inst;
        while (n--) {
            assert(inst->minVert != 0xFFFFFFFF);
            inst->vertexAlpha = instColor(
                VERT_RGBA, verts + a->offset + a->stride * inst->minVert,
                geo->colors + inst->minVert, inst->numVertices, a->stride);
            inst++;
        }
    }

    // Texture coordinates
    for (int32 n = 0; n < geo->numTexCoordSets; n++) {
        for (a = attribs; a->index != ATTRIB_TEXCOORDS0 + n; a++)
            ;
        instTexCoords(VERT_FLOAT2, verts + a->offset, geo->texCoords[n],
                      header->totalNumVertex, a->stride);
    }

    glGenBuffers(1, &header->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, header->vbo);
    glBufferData(GL_ARRAY_BUFFER, header->totalNumVertex * stride,
                 header->vertexBuffer, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void defaultUninstanceCB(Geometry *geo, InstanceDataHeader *header) {
    assert(0 && "can't uninstance");
}

ObjPipeline *makeDefaultPipeline(void) {
    ObjPipeline *pipe = new ObjPipeline(PLATFORM_GL3);
    pipe->instanceCB = defaultInstanceCB;
    pipe->uninstanceCB = defaultUninstanceCB;
    pipe->renderCB = defaultRenderCB;
    return pipe;
}

#endif

}  // namespace gl3
}  // namespace rw
