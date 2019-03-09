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
#include "../Render.hpp"
#ifdef RW_OPENGL
#include <GL/glew.h>
#endif
#include "rwgl3.h"
#include "rwgl3plg.h"
#include "rwgl3shader.h"

#include "rwgl3impl.h"

namespace rw {
namespace gl3 {

#ifdef RW_OPENGL

#define U(i) currentShader->uniformLocations[i]

static Shader *skinShader;
static int32 u_boneMatrices;

static void *skinOpen(void *o, int32, int32) {
    u_boneMatrices = registerUniform("u_boneMatrices");
#include "shaders/simple_fs_gl3.inc"
#include "shaders/skin_gl3.inc"
    skinGlobals.pipelines[PLATFORM_GL3] = makeSkinPipeline();
    skinShader = Shader::fromStrings(skin_vert_src, simple_frag_src);
    return o;
}

static void *skinClose(void *o, int32, int32) {
    return o;
}

void initSkin(void) {
    Driver::registerPlugin(PLATFORM_GL3, 0, ID_SKIN, skinOpen, skinClose);
}

enum { ATTRIB_WEIGHTS = ATTRIB_TEXCOORDS7 + 1, ATTRIB_INDICES };

void skinInstanceCB(Geometry *geo, InstanceDataHeader *header) {
    AttribDesc attribs[14], *a;
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

    // Weights
    a->index = ATTRIB_WEIGHTS;
    a->size = 4;
    a->type = GL_FLOAT;
    a->normalized = GL_FALSE;
    a->offset = stride;
    stride += 16;
    a++;

    // Indices
    a->index = ATTRIB_INDICES;
    a->size = 4;
    a->type = GL_UNSIGNED_BYTE;
    a->normalized = GL_FALSE;
    a->offset = stride;
    stride += 4;
    a++;

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
    Skin *skin = Skin::get(geo);
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
        instColor(VERT_RGBA, verts + a->offset, geo->colors,
                  header->totalNumVertex, a->stride);
    }

    // Texture coordinates
    for (int32 n = 0; n < geo->numTexCoordSets; n++) {
        for (a = attribs; a->index != ATTRIB_TEXCOORDS0 + n; a++)
            ;
        instTexCoords(VERT_FLOAT2, verts + a->offset, geo->texCoords[n],
                      header->totalNumVertex, a->stride);
    }

    // Weights
    for (a = attribs; a->index != ATTRIB_WEIGHTS; a++)
        ;
    float *w = skin->weights;
    instV4d(VERT_FLOAT4, verts + a->offset, (V4d *)w, header->totalNumVertex,
            a->stride);

    // Indices
    for (a = attribs; a->index != ATTRIB_INDICES; a++)
        ;
    // not really colors of course but what the heck
    instColor(VERT_RGBA, verts + a->offset, (RGBA *)skin->indices,
              header->totalNumVertex, a->stride);

    glGenBuffers(1, &header->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, header->vbo);
    glBufferData(GL_ARRAY_BUFFER, header->totalNumVertex * stride,
                 header->vertexBuffer, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void skinUninstanceCB(Geometry *geo, InstanceDataHeader *header) {
    assert(0 && "can't uninstance");
}

static float skinMatrices[64 * 16];

void updateSkinMatrices(Atomic *a) {
    Skin *skin = Skin::get(a->geometry);
    HAnimHierarchy *hier = Skin::getHierarchy(a);
    Matrix *invMats = (Matrix *)skin->inverseMatrices;

    float *m;
    m = (float *)skinMatrices;
    for (int i = 0; i < hier->numNodes; i++) {
        invMats[i].flags = 0;
        Matrix::mult((Matrix *)m, &invMats[i], &hier->matrices[i]);
        m[3] = 0.0f;
        m[7] = 0.0f;
        m[11] = 0.0f;
        m[15] = 1.0f;
        m += 16;
    }
}

void skinRenderCB(Atomic *atomic, InstanceDataHeader *header) {
    Material *m;
    RGBAf col;
    GLfloat surfProps[4];

    setWorldMatrix(atomic->getFrame()->getLTM());
    lightingCB(!!(atomic->geometry->flags & Geometry::NORMALS));

    glBindBuffer(GL_ARRAY_BUFFER, header->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, header->ibo);
    setAttribPointers(header->attribDesc, header->numAttribs);

    InstanceData *inst = header->inst;
    int32 n = header->numMeshes;

    //	rw::setRenderState(ALPHATESTFUNC, 1);
    //	rw::setRenderState(ALPHATESTREF, 50);

    skinShader->use();

    updateSkinMatrices(atomic);
    glUniformMatrix4fv(U(u_boneMatrices), 64, GL_FALSE,
                       (GLfloat *)skinMatrices);

    while (n--) {
        m = inst->material;

        convColor(&col, &m->color);
        glUniform4fv(U(u_matColor), 1, (GLfloat *)&col);

        surfProps[0] = m->surfaceProps.ambient;
        surfProps[1] = m->surfaceProps.specular;
        surfProps[2] = m->surfaceProps.diffuse;
        surfProps[3] = 0.0f;
        glUniform4fv(U(u_surfaceProps), 1, surfProps);

        setTexture(0, m->texture);

        rw::SetRenderState(VERTEXALPHA,
                           inst->vertexAlpha || m->color.alpha != 0xFF);

        flushCache();
        glDrawElements(header->primType, inst->numIndex, GL_UNSIGNED_SHORT,
                       (void *)(uintptr)inst->offset);
        inst++;
    }
    disableAttribPointers(header->attribDesc, header->numAttribs);
}

ObjPipeline *makeSkinPipeline(void) {
    ObjPipeline *pipe = new ObjPipeline(PLATFORM_GL3);
    pipe->instanceCB = skinInstanceCB;
    pipe->uninstanceCB = skinUninstanceCB;
    pipe->renderCB = skinRenderCB;
    pipe->pluginID = ID_SKIN;
    pipe->pluginData = 1;
    return pipe;
}

#else

void initSkin(void) {
}

#endif

}  // namespace gl3
}  // namespace rw
