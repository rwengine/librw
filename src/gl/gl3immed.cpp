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
#include "rwgl3.h"
#include "rwgl3impl.h"
#include "rwgl3shader.h"

namespace rw {
namespace gl3 {

uint32 im2DVbo, im2DIbo;
static int32 u_xform;

#define STARTINDICES 10000
#define STARTVERTICES 10000

static Shader *im2dShader;
static AttribDesc im2dattribDesc[3] = {
    {ATTRIB_POS, GL_FLOAT, GL_FALSE, 4, sizeof(Im2DVertex), 0},
    {ATTRIB_COLOR, GL_UNSIGNED_BYTE, GL_TRUE, 4, sizeof(Im2DVertex),
     offsetof(Im2DVertex, r)},
    {ATTRIB_TEXCOORDS0, GL_FLOAT, GL_FALSE, 2, sizeof(Im2DVertex),
     offsetof(Im2DVertex, u)},
};

static int primTypeMap[] = {GL_POINTS,  // invalid
                            GL_LINES,          GL_LINE_STRIP,   GL_TRIANGLES,
                            GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_POINTS};

void openIm2D(void) {
    u_xform = registerUniform("u_xform");

#include "shaders/im2d_gl3.inc"
#include "shaders/simple_fs_gl3.inc"
    im2dShader = Shader::fromStrings(im2d_vert_src, simple_frag_src);

    glGenBuffers(1, &im2DIbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, im2DIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, STARTINDICES * 2, nil,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenBuffers(1, &im2DVbo);
    glBindBuffer(GL_ARRAY_BUFFER, im2DVbo);
    glBufferData(GL_ARRAY_BUFFER, STARTVERTICES * sizeof(Im2DVertex), nil,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void closeIm2D(void) {
    glDeleteBuffers(1, &im2DIbo);
    glDeleteBuffers(1, &im2DVbo);
    im2dShader->destroy();
    im2dShader = nil;
}

static Im2DVertex tmpprimbuf[3];

void im2DRenderLine(void *vertices, int32 numVertices, int32 vert1,
                    int32 vert2) {
    Im2DVertex *verts = (Im2DVertex *)vertices;
    tmpprimbuf[0] = verts[vert1];
    tmpprimbuf[1] = verts[vert2];
    im2DRenderPrimitive(PRIMTYPELINELIST, tmpprimbuf, 2);
}

void im2DRenderTriangle(void *vertices, int32 numVertices, int32 vert1,
                        int32 vert2, int32 vert3) {
    Im2DVertex *verts = (Im2DVertex *)vertices;
    tmpprimbuf[0] = verts[vert1];
    tmpprimbuf[1] = verts[vert2];
    tmpprimbuf[2] = verts[vert3];
    im2DRenderPrimitive(PRIMTYPETRILIST, tmpprimbuf, 3);
}

void im2DRenderPrimitive(PrimitiveType primType, void *vertices,
                         int32 numVertices) {
    GLfloat xform[4];
    Camera *cam;
    cam = (Camera *)engine->currentCamera;

    glBindBuffer(GL_ARRAY_BUFFER, im2DVbo);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Im2DVertex), vertices,
                 GL_DYNAMIC_DRAW);

    xform[0] = 2.0f / cam->frameBuffer->width;
    xform[1] = -2.0f / cam->frameBuffer->height;
    xform[2] = -1.0f;
    xform[3] = 1.0f;

    im2dShader->use();
    setAttribPointers(im2dattribDesc, 3);

    glUniform4fv(currentShader->uniformLocations[u_xform], 1, xform);

    flushCache();
    glDrawArrays(primTypeMap[primType], 0, numVertices);
    disableAttribPointers(im2dattribDesc, 3);
}

void im2DRenderIndexedPrimitive(PrimitiveType primType, void *vertices,
                                int32 numVertices, void *indices,
                                int32 numIndices) {
    GLfloat xform[4];
    Camera *cam;
    cam = (Camera *)engine->currentCamera;

    // TODO: fixed size
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, im2DIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * 2, indices,
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, im2DVbo);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Im2DVertex), vertices,
                 GL_DYNAMIC_DRAW);

    xform[0] = 2.0f / cam->frameBuffer->width;
    xform[1] = -2.0f / cam->frameBuffer->height;
    xform[2] = -1.0f;
    xform[3] = 1.0f;

    im2dShader->use();
    setAttribPointers(im2dattribDesc, 3);

    glUniform4fv(currentShader->uniformLocations[u_xform], 1, xform);

    flushCache();
    glDrawElements(primTypeMap[primType], numIndices, GL_UNSIGNED_SHORT, nil);
    disableAttribPointers(im2dattribDesc, 3);
}

// Im3D

static Shader *im3dShader;
static AttribDesc im3dattribDesc[3] = {
    {ATTRIB_POS, GL_FLOAT, GL_FALSE, 3, sizeof(Im3DVertex), 0},
    {ATTRIB_COLOR, GL_UNSIGNED_BYTE, GL_TRUE, 4, sizeof(Im3DVertex),
     offsetof(Im3DVertex, r)},
    {ATTRIB_TEXCOORDS0, GL_FLOAT, GL_FALSE, 2, sizeof(Im3DVertex),
     offsetof(Im3DVertex, u)},
};
static uint32 im3DVbo, im3DIbo;
static int32 num3DVertices;  // not actually needed here

void openIm3D(void) {
#include "shaders/im3d_gl3.inc"
#include "shaders/simple_fs_gl3.inc"
    im3dShader = Shader::fromStrings(im3d_vert_src, simple_frag_src);

    glGenBuffers(1, &im3DIbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, im3DIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, STARTINDICES * 2, nil,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenBuffers(1, &im3DVbo);
    glBindBuffer(GL_ARRAY_BUFFER, im3DVbo);
    glBufferData(GL_ARRAY_BUFFER, STARTVERTICES * sizeof(Im3DVertex), nil,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void closeIm3D(void) {
    glDeleteBuffers(1, &im3DIbo);
    glDeleteBuffers(1, &im3DVbo);
    im3dShader->destroy();
    im3dShader = nil;
}

void im3DTransform(void *vertices, int32 numVertices, Matrix *world) {
    if (world == nil) {
        Matrix ident;
        ident.setIdentity();
        world = &ident;
    }
    setWorldMatrix(world);
    im3dShader->use();

    // TODO: fixed size
    glBindBuffer(GL_ARRAY_BUFFER, im3DVbo);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Im3DVertex), vertices,
                 GL_DYNAMIC_DRAW);
    setAttribPointers(im3dattribDesc, 3);
    num3DVertices = numVertices;
}

void im3DRenderIndexed(PrimitiveType primType, void *indices,
                       int32 numIndices) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, im3DIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * 2, indices,
                 GL_DYNAMIC_DRAW);

    flushCache();
    glDrawElements(primTypeMap[primType], numIndices, GL_UNSIGNED_SHORT, nil);
    disableAttribPointers(im3dattribDesc, 3);
}

void im3DEnd(void) {
}

}  // namespace gl3
}  // namespace rw

#endif
