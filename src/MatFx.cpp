#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Animation.hpp"

#include "Base.hpp"
#include "Engine.hpp"
#include "Objects.hpp"
#include "d3d/rwd3d8.h"
#include "d3d/rwd3d9.h"
#include "d3d/rwxbox.h"
#include "gl/rwgl3.h"
#include "gl/rwgl3plg.h"
#include "gl/rwwdgl.h"
#include "ps2/rwps2.h"
#include "ps2/rwps2plg.h"
#include "Error.hpp"
#include "Pipeline.hpp"
#include "Plg.hpp"
#include "rwplugins.h"

#define PLUGIN_ID ID_MATFX

namespace rw {

// Atomic

static void *createAtomicMatFX(void *object, int32 offset, int32) {
    *PLUGINOFFSET(int32, object, offset) = 0;
    return object;
}

static void *copyAtomicMatFX(void *dst, void *src, int32 offset, int32) {
    // don't call seteffects, it will override the pipeline
    if (*PLUGINOFFSET(int32, src, offset))
        *PLUGINOFFSET(int32, dst, offset) = 1;
    return dst;
}

static Stream *readAtomicMatFX(Stream *stream, int32, void *object, int32,
                               int32) {
    if (stream->readI32()) MatFX::enableEffects((Atomic *)object);
    return stream;
}

static Stream *writeAtomicMatFX(Stream *stream, int32, void *object,
                                int32 offset, int32) {
    stream->writeI32(*PLUGINOFFSET(int32, object, offset));
    return stream;
}

static int32 getSizeAtomicMatFX(void *object, int32 offset, int32) {
    int32 flag = *PLUGINOFFSET(int32, object, offset);
    // TODO: not sure which version
    return flag || rw::version < 0x34000 ? 4 : 0;
}

// Material

MatFXGlobals matFXGlobals = {0, 0, {nil}};

// TODO: Frames and Matrices?
static void clearMatFX(MatFX *matfx) {
    for (int i = 0; i < 2; i++) switch (matfx->fx[i].type) {
            case MatFX::BUMPMAP:
                if (matfx->fx[i].bump.bumpedTex)
                    matfx->fx[i].bump.bumpedTex->destroy();
                if (matfx->fx[i].bump.tex) matfx->fx[i].bump.tex->destroy();
                break;

            case MatFX::ENVMAP:
                if (matfx->fx[i].env.tex) matfx->fx[i].env.tex->destroy();
                break;

            case MatFX::DUAL:
                if (matfx->fx[i].dual.tex) matfx->fx[i].dual.tex->destroy();
                break;
        }
    memset(matfx, 0, sizeof(MatFX));
}

void MatFX::setEffects(Material *mat, uint32 type) {
    MatFX *matfx;

    matfx = MatFX::get(mat);
    if (matfx == nil) {
        matfx = rwNewT(MatFX, 1, MEMDUR_EVENT | ID_MATFX);
        memset(matfx, 0, sizeof(MatFX));
        *PLUGINOFFSET(MatFX *, mat, matFXGlobals.materialOffset) = matfx;
    }

    if (matfx->type != 0 && matfx->type != type) clearMatFX(matfx);
    matfx->type = type;
    switch (type) {
        case BUMPMAP:
        case ENVMAP:
        case DUAL:
        case UVTRANSFORM:
            matfx->fx[0].type = type;
            matfx->fx[1].type = NOTHING;
            break;

        case BUMPENVMAP:
            matfx->fx[0].type = BUMPMAP;
            matfx->fx[1].type = ENVMAP;
            break;

        case DUALUVTRANSFORM:
            matfx->fx[0].type = UVTRANSFORM;
            matfx->fx[1].type = DUAL;
            break;
    }
}

uint32 MatFX::getEffects(Material *m) {
    MatFX *fx = *PLUGINOFFSET(MatFX *, m, matFXGlobals.materialOffset);
    if (fx) return fx->type;
    return 0;
}

MatFX *MatFX::get(Material *m) {
    return *PLUGINOFFSET(MatFX *, m, matFXGlobals.materialOffset);
}

int32 MatFX::getEffectIndex(uint32 type) {
    for (int i = 0; i < 2; i++)
        if (this->fx[i].type == type) return i;
    return -1;
}

void MatFX::setBumpTexture(Texture *t) {
    int32 i = this->getEffectIndex(BUMPMAP);
    if (i >= 0) this->fx[i].bump.tex = t;
}

void MatFX::setBumpCoefficient(float32 coef) {
    int32 i = this->getEffectIndex(BUMPMAP);
    if (i >= 0) this->fx[i].bump.coefficient = coef;
}

Texture *MatFX::getBumpTexture(void) {
    int32 i = this->getEffectIndex(BUMPMAP);
    if (i >= 0) return this->fx[i].bump.tex;
    return nil;
}

float32 MatFX::getBumpCoefficient(void) {
    int32 i = this->getEffectIndex(BUMPMAP);
    if (i >= 0) return this->fx[i].bump.coefficient;
    return 0.0f;
}

void MatFX::setEnvTexture(Texture *t) {
    int32 i = this->getEffectIndex(ENVMAP);
    if (i >= 0) this->fx[i].env.tex = t;
}

void MatFX::setEnvFrame(Frame *f) {
    int32 i = this->getEffectIndex(ENVMAP);
    if (i >= 0) this->fx[i].env.frame = f;
}

void MatFX::setEnvCoefficient(float32 coef) {
    int32 i = this->getEffectIndex(ENVMAP);
    if (i >= 0) this->fx[i].env.coefficient = coef;
}

Texture *MatFX::getEnvTexture(void) {
    int32 i = this->getEffectIndex(ENVMAP);
    if (i >= 0) return this->fx[i].env.tex;
    return nil;
}

Frame *MatFX::getEnvFrame(void) {
    int32 i = this->getEffectIndex(ENVMAP);
    if (i >= 0) return this->fx[i].env.frame;
    return nil;
}

float32 MatFX::getEnvCoefficient(void) {
    int32 i = this->getEffectIndex(ENVMAP);
    if (i >= 0) return this->fx[i].env.coefficient;
    return 0.0f;
}

void MatFX::setDualTexture(Texture *t) {
    int32 i = this->getEffectIndex(DUAL);
    if (i >= 0) this->fx[i].dual.tex = t;
}

void MatFX::setDualSrcBlend(int32 blend) {
    int32 i = this->getEffectIndex(DUAL);
    if (i >= 0) this->fx[i].dual.srcBlend = blend;
}

void MatFX::setDualDestBlend(int32 blend) {
    int32 i = this->getEffectIndex(DUAL);
    if (i >= 0) this->fx[i].dual.dstBlend = blend;
}

Texture *MatFX::getDualTexture(void) {
    int32 i = this->getEffectIndex(DUAL);
    if (i >= 0) return this->fx[i].dual.tex;
    return nil;
}

int32 MatFX::getDualSrcBlend(void) {
    int32 i = this->getEffectIndex(DUAL);
    if (i >= 0) return this->fx[i].dual.srcBlend;
    return 0;
}

int32 MatFX::getDualDestBlend(void) {
    int32 i = this->getEffectIndex(DUAL);
    if (i >= 0) return this->fx[i].dual.dstBlend;
    return 0;
}

void MatFX::setUVTransformMatrices(Matrix *base, Matrix *dual) {
    int32 i = this->getEffectIndex(UVTRANSFORM);
    if (i >= 0) {
        this->fx[i].uvtransform.baseTransform = base;
        this->fx[i].uvtransform.dualTransform = dual;
    }
}

void MatFX::getUVTransformMatrices(Matrix **base, Matrix **dual) {
    int32 i = this->getEffectIndex(UVTRANSFORM);
    if (i >= 0) {
        if (base) *base = this->fx[i].uvtransform.baseTransform;
        if (dual) *dual = this->fx[i].uvtransform.dualTransform;
        return;
    }
    if (base) *base = nil;
    if (dual) *dual = nil;
}

static void *createMaterialMatFX(void *object, int32 offset, int32) {
    *PLUGINOFFSET(MatFX *, object, offset) = nil;
    return object;
}

static void *destroyMaterialMatFX(void *object, int32 offset, int32) {
    MatFX *matfx = *PLUGINOFFSET(MatFX *, object, offset);
    if (matfx) {
        clearMatFX(matfx);
        rwFree(matfx);
    }
    return object;
}

static void *copyMaterialMatFX(void *dst, void *src, int32 offset, int32) {
    MatFX *srcfx = *PLUGINOFFSET(MatFX *, src, offset);
    if (srcfx == nil) return dst;
    MatFX *dstfx = rwNewT(MatFX, 1, MEMDUR_EVENT | ID_MATFX);
    *PLUGINOFFSET(MatFX *, dst, offset) = dstfx;
    memcpy(dstfx, srcfx, sizeof(MatFX));
    for (int i = 0; i < 2; i++) switch (dstfx->fx[i].type) {
            case MatFX::BUMPMAP:
                if (dstfx->fx[i].bump.bumpedTex)
                    dstfx->fx[i].bump.bumpedTex->refCount++;
                if (dstfx->fx[i].bump.tex) dstfx->fx[i].bump.tex->refCount++;
                break;

            case MatFX::ENVMAP:
                if (dstfx->fx[i].env.tex) dstfx->fx[i].env.tex->refCount++;
                break;

            case MatFX::DUAL:
                if (dstfx->fx[i].dual.tex) dstfx->fx[i].dual.tex->refCount++;
                break;
        }
    return dst;
}

static Stream *readMaterialMatFX(Stream *stream, int32, void *object,
                                 int32 offset, int32) {
    Material *mat;
    MatFX *matfx;
    Texture *tex, *bumpedTex;
    float coefficient;
    int32 fbAlpha;
    int32 srcBlend, dstBlend;
    int32 idx;

    mat = (Material *)object;
    MatFX::setEffects(mat, stream->readU32());
    matfx = MatFX::get(mat);

    for (int i = 0; i < 2; i++) {
        uint32 type = stream->readU32();
        switch (type) {
            case MatFX::BUMPMAP:
                coefficient = stream->readF32();
                bumpedTex = tex = nil;
                if (stream->readI32()) {
                    if (!findChunk(stream, ID_TEXTURE, nil, nil)) {
                        RWERROR((ERR_CHUNK, "TEXTURE"));
                        return nil;
                    }
                    bumpedTex = Texture::streamRead(stream);
                }
                if (stream->readI32()) {
                    if (!findChunk(stream, ID_TEXTURE, nil, nil)) {
                        RWERROR((ERR_CHUNK, "TEXTURE"));
                        return nil;
                    }
                    tex = Texture::streamRead(stream);
                }
                idx = matfx->getEffectIndex(type);
                assert(idx >= 0);
                matfx->fx[idx].bump.bumpedTex = bumpedTex;
                matfx->fx[idx].bump.tex = tex;
                matfx->fx[idx].bump.coefficient = coefficient;
                break;

            case MatFX::ENVMAP:
                coefficient = stream->readF32();
                fbAlpha = stream->readI32();
                tex = nil;
                if (stream->readI32()) {
                    if (!findChunk(stream, ID_TEXTURE, nil, nil)) {
                        RWERROR((ERR_CHUNK, "TEXTURE"));
                        return nil;
                    }
                    tex = Texture::streamRead(stream);
                }
                idx = matfx->getEffectIndex(type);
                assert(idx >= 0);
                matfx->fx[idx].env.tex = tex;
                matfx->fx[idx].env.fbAlpha = fbAlpha;
                matfx->fx[idx].env.coefficient = coefficient;
                break;

            case MatFX::DUAL:
                srcBlend = stream->readI32();
                dstBlend = stream->readI32();
                tex = nil;
                if (stream->readI32()) {
                    if (!findChunk(stream, ID_TEXTURE, nil, nil)) {
                        RWERROR((ERR_CHUNK, "TEXTURE"));
                        return nil;
                    }
                    tex = Texture::streamRead(stream);
                }
                idx = matfx->getEffectIndex(type);
                assert(idx >= 0);
                matfx->fx[idx].dual.tex = tex;
                matfx->fx[idx].dual.srcBlend = srcBlend;
                matfx->fx[idx].dual.dstBlend = dstBlend;
                break;
        }
    }
    return stream;
}

static Stream *writeMaterialMatFX(Stream *stream, int32, void *object,
                                  int32 offset, int32) {
    MatFX *matfx = *PLUGINOFFSET(MatFX *, object, offset);

    stream->writeU32(matfx->type);
    for (int i = 0; i < 2; i++) {
        stream->writeU32(matfx->fx[i].type);
        switch (matfx->fx[i].type) {
            case MatFX::BUMPMAP:
                stream->writeF32(matfx->fx[i].bump.coefficient);
                stream->writeI32(matfx->fx[i].bump.bumpedTex != nil);
                if (matfx->fx[i].bump.bumpedTex)
                    matfx->fx[i].bump.bumpedTex->streamWrite(stream);
                stream->writeI32(matfx->fx[i].bump.tex != nil);
                if (matfx->fx[i].bump.tex)
                    matfx->fx[i].bump.tex->streamWrite(stream);
                break;

            case MatFX::ENVMAP:
                stream->writeF32(matfx->fx[i].env.coefficient);
                stream->writeI32(matfx->fx[i].env.fbAlpha);
                stream->writeI32(matfx->fx[i].env.tex != nil);
                if (matfx->fx[i].env.tex)
                    matfx->fx[i].env.tex->streamWrite(stream);
                break;

            case MatFX::DUAL:
                stream->writeI32(matfx->fx[i].dual.srcBlend);
                stream->writeI32(matfx->fx[i].dual.dstBlend);
                stream->writeI32(matfx->fx[i].dual.tex != nil);
                if (matfx->fx[i].dual.tex)
                    matfx->fx[i].dual.tex->streamWrite(stream);
                break;
        }
    }
    return stream;
}

static int32 getSizeMaterialMatFX(void *object, int32 offset, int32) {
    MatFX *matfx = *PLUGINOFFSET(MatFX *, object, offset);
    if (matfx == nil) return -1;
    int32 size = 4 + 4 + 4;

    for (int i = 0; i < 2; i++) {
        switch (matfx->fx[i].type) {
            case MatFX::BUMPMAP:
                size += 4 + 4 + 4;
                if (matfx->fx[i].bump.bumpedTex)
                    size += 12 + matfx->fx[i].bump.bumpedTex->streamGetSize();
                if (matfx->fx[i].bump.tex)
                    size += 12 + matfx->fx[i].bump.tex->streamGetSize();
                break;

            case MatFX::ENVMAP:
                size += 4 + 4 + 4;
                if (matfx->fx[i].env.tex)
                    size += 12 + matfx->fx[i].env.tex->streamGetSize();
                break;

            case MatFX::DUAL:
                size += 4 + 4 + 4;
                if (matfx->fx[i].dual.tex)
                    size += 12 + matfx->fx[i].dual.tex->streamGetSize();
                break;
        }
    }
    return size;
}

void MatFX::enableEffects(Atomic *atomic) {
    *PLUGINOFFSET(int32, atomic, matFXGlobals.atomicOffset) = 1;
    atomic->pipeline = matFXGlobals.pipelines[rw::platform];
}

// This prevents setting the pipeline on clone
void MatFX::disableEffects(Atomic *atomic) {
    *PLUGINOFFSET(int32, atomic, matFXGlobals.atomicOffset) = 0;
}

bool32 MatFX::getEffects(Atomic *atomic) {
    return *PLUGINOFFSET(int32, atomic, matFXGlobals.atomicOffset);
}

static void *matfxOpen(void *o, int32, int32) {
    // init dummy pipelines
    ObjPipeline *defpipe = new ObjPipeline(PLATFORM_NULL);
    defpipe->pluginID = 0;  // ID_MATFX;
    defpipe->pluginData = 0;
    for (uint i = 0; i < nelem(matFXGlobals.pipelines); i++)
        matFXGlobals.pipelines[i] = defpipe;
    return o;
}

static void *matfxClose(void *o, int32, int32) {
    return o;
}

void registerMatFXPlugin(void) {
    Driver::registerPlugin(PLATFORM_NULL, 0, ID_MATFX, matfxOpen, matfxClose);
    ps2::initMatFX();
    xbox::initMatFX();
    d3d8::initMatFX();
    d3d9::initMatFX();
    wdgl::initMatFX();
    gl3::initMatFX();

    matFXGlobals.atomicOffset = Atomic::registerPlugin(
        sizeof(int32), ID_MATFX, createAtomicMatFX, nil, copyAtomicMatFX);
    Atomic::registerPluginStream(ID_MATFX, readAtomicMatFX, writeAtomicMatFX,
                                 getSizeAtomicMatFX);

    matFXGlobals.materialOffset =
        Material::registerPlugin(sizeof(MatFX *), ID_MATFX, createMaterialMatFX,
                                 destroyMaterialMatFX, copyMaterialMatFX);
    Material::registerPluginStream(ID_MATFX, readMaterialMatFX,
                                   writeMaterialMatFX, getSizeMaterialMatFX);
}

}  // namespace rw
