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

static Shader *envShader;
static int32 u_texMatrix;
static int32 u_coefficient;

static void *matfxOpen(void *o, int32, int32) {
    u_texMatrix = registerUniform("u_texMatrix");
    u_coefficient = registerUniform("u_coefficient");
#include "shaders/matfx_gl3.inc"
    matFXGlobals.pipelines[PLATFORM_GL3] = makeMatFXPipeline();
    envShader = Shader::fromStrings(matfx_env_vert_src, matfx_env_frag_src);
    return o;
}

static void *matfxClose(void *o, int32, int32) {
    return o;
}

void initMatFX(void) {
    Driver::registerPlugin(PLATFORM_GL3, 0, ID_MATFX, matfxOpen, matfxClose);
}

void matfxDefaultRender(InstanceDataHeader *header, InstanceData *inst) {
    Material *m;
    RGBAf col;
    GLfloat surfProps[4];
    m = inst->material;

    simpleShader->use();

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
}

void calcEnvTexMatrix(Frame *f, float32 *mat) {
    Matrix cam;
    if (f) {
        // PS2 style - could be simplified by the shader
        cam = *((Camera *)engine->currentCamera)->getFrame()->getLTM();
        cam.pos = {0.0, 0.0, 0.0};
        cam.right.x = -cam.right.x;
        cam.right.y = -cam.right.y;
        cam.right.z = -cam.right.z;

        Matrix inv;
        Matrix::invert(&inv, f->getLTM());
        inv.pos = {-1.0, -1.0, -1.0};
        inv.right.x *= -0.5f;
        inv.right.y *= -0.5f;
        inv.right.z *= -0.5f;
        inv.up.x *= -0.5f;
        inv.up.y *= -0.5f;
        inv.up.z *= -0.5f;
        inv.at.x *= -0.5f;
        inv.at.y *= -0.5f;
        inv.at.z *= -0.5f;
        inv.pos.x *= -0.5f;
        inv.pos.y *= -0.5f;
        inv.pos.z *= -0.5f;

        Matrix m;
        Matrix::mult(&m, &cam, &inv);

        memcpy(mat, &m, 64);
        mat[3] = mat[7] = mat[11] = 0.0f;
        mat[15] = 1.0f;

    } else {
        // D3D - TODO: find out what PS2 does
        mat[0] = 0.5f;
        mat[1] = 0.0f;
        mat[2] = 0.0f;
        mat[3] = 0.0f;

        mat[4] = 0.0f;
        mat[5] = -0.5f;
        mat[6] = 0.0f;
        mat[7] = 0.0f;

        mat[8] = 0.0f;
        mat[9] = 0.0f;
        mat[10] = 1.0f;
        mat[11] = 0.0f;

        mat[12] = 0.5f;
        mat[13] = 0.5f;
        mat[14] = 0.0f;
        mat[15] = 0.0f;
    }
}

void matfxEnvRender(InstanceDataHeader *header, InstanceData *inst) {
    Material *m;
    RGBAf col;
    GLfloat surfProps[4];
    float32 texMat[16];
    m = inst->material;

    matfxDefaultRender(header, inst);

    MatFX *fx = MatFX::get(m);
    int32 idx = fx->getEffectIndex(MatFX::ENVMAP);
    MatFX::Env *env = &fx->fx[idx].env;

    envShader->use();

    convColor(&col, &m->color);
    glUniform4fv(U(u_matColor), 1, (GLfloat *)&col);

    surfProps[0] = m->surfaceProps.ambient;
    surfProps[1] = m->surfaceProps.specular;
    surfProps[2] = m->surfaceProps.diffuse;
    surfProps[3] = 0.0f;
    glUniform4fv(U(u_surfaceProps), 1, surfProps);

    glUniform1fv(U(u_coefficient), 1, &env->coefficient);

    calcEnvTexMatrix(env->frame, texMat);
    glUniformMatrix4fv(U(u_texMatrix), 1, GL_FALSE, texMat);

    setTexture(0, env->tex);

    rw::SetRenderState(VERTEXALPHA, 1);
    rw::SetRenderState(SRCBLEND, BLENDONE);
    rw::SetRenderState(DESTBLEND, BLENDONE);

    flushCache();
    glDrawElements(header->primType, inst->numIndex, GL_UNSIGNED_SHORT,
                   (void *)(uintptr)inst->offset);

    rw::SetRenderState(SRCBLEND, BLENDSRCALPHA);
    rw::SetRenderState(DESTBLEND, BLENDINVSRCALPHA);
}

void matfxRenderCB(Atomic *atomic, InstanceDataHeader *header) {
    setWorldMatrix(atomic->getFrame()->getLTM());
    lightingCB(!!(atomic->geometry->flags & Geometry::NORMALS));

    glBindBuffer(GL_ARRAY_BUFFER, header->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, header->ibo);
    setAttribPointers(header->attribDesc, header->numAttribs);

    InstanceData *inst = header->inst;
    int32 n = header->numMeshes;

    //	rw::SetRenderState(ALPHATESTFUNC, 1);
    //	rw::SetRenderState(ALPHATESTREF, 50);

    int32 fx;
    while (n--) {
        fx = MatFX::getEffects(inst->material);
        switch (fx) {
            case MatFX::ENVMAP:
                matfxEnvRender(header, inst);
                break;
            default:
                matfxDefaultRender(header, inst);
        }
        inst++;
    }
    disableAttribPointers(header->attribDesc, header->numAttribs);
}

ObjPipeline *makeMatFXPipeline(void) {
    ObjPipeline *pipe = new ObjPipeline(PLATFORM_GL3);
    pipe->instanceCB = defaultInstanceCB;
    pipe->uninstanceCB = defaultUninstanceCB;
    pipe->renderCB = matfxRenderCB;
    pipe->pluginID = ID_MATFX;
    pipe->pluginData = 0;
    return pipe;
}

#else

void initMatFX(void) {
}

#endif

}  // namespace gl3
}  // namespace rw
