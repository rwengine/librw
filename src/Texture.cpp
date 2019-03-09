#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Base.hpp"
#include "Engine.hpp"
#include "Objects.hpp"
#include "d3d/rwd3d.h"
#include "d3d/rwd3d8.h"
#include "d3d/rwd3d9.h"
#include "d3d/rwxbox.h"
#include "ps2/rwps2.h"
#include "Error.hpp"
#include "Pipeline.hpp"
#include "Plg.hpp"

#define PLUGIN_ID 0

// TODO: maintain a global list of all texdicts

namespace rw {

PluginList TexDictionary::s_plglist = {sizeof(TexDictionary),
                                       sizeof(TexDictionary), nil, nil};
PluginList Texture::s_plglist = {sizeof(Texture), sizeof(Texture), nil, nil};
PluginList Raster::s_plglist = {sizeof(Raster), sizeof(Raster), nil, nil};

struct TextureGlobals {
    TexDictionary *initialTexDict;
    TexDictionary *currentTexDict;
    // load textures from files
    bool32 loadTextures;
    // create dummy textures to store just names
    bool32 makeDummies;
};
int32 textureModuleOffset;

#define TEXTUREGLOBAL(v) \
    (PLUGINOFFSET(TextureGlobals, engine, textureModuleOffset)->v)

static void *textureOpen(void *object, int32 offset, int32 size) {
    TexDictionary *texdict;
    textureModuleOffset = offset;
    texdict = TexDictionary::create();
    TEXTUREGLOBAL(initialTexDict) = texdict;
    TexDictionary::setCurrent(texdict);
    TEXTUREGLOBAL(loadTextures) = 1;
    TEXTUREGLOBAL(makeDummies) = 0;
    return object;
}
static void *textureClose(void *object, int32 offset, int32 size) {
    TEXTUREGLOBAL(initialTexDict)->destroy();
    return object;
}

void Texture::registerModule(void) {
    Engine::registerPlugin(sizeof(TextureGlobals), ID_TEXTUREMODULE,
                           textureOpen, textureClose);
}

void Texture::setLoadTextures(bool32 b) {
    TEXTUREGLOBAL(loadTextures) = b;
}

void Texture::setCreateDummies(bool32 b) {
    TEXTUREGLOBAL(makeDummies) = b;
}

//
// TexDictionary
//

TexDictionary *TexDictionary::create(void) {
    TexDictionary *dict = (TexDictionary *)rwMalloc(
        s_plglist.size, MEMDUR_EVENT | ID_TEXDICTIONARY);
    if (dict == nil) {
        RWERROR((ERR_ALLOC, s_plglist.size));
        return nil;
    }
    dict->object.init(TexDictionary::ID, 0);
    dict->textures.init();
    s_plglist.construct(dict);
    return dict;
}

void TexDictionary::destroy(void) {
    if (TEXTUREGLOBAL(currentTexDict) == this)
        TEXTUREGLOBAL(currentTexDict) = nil;
    FORLIST(lnk, this->textures)
    Texture::fromDict(lnk)->destroy();
    s_plglist.destruct(this);
    rwFree(this);
}

void TexDictionary::add(Texture *t) {
    if (t->dict) t->inDict.remove();
    t->dict = this;
    this->textures.append(&t->inDict);
}

void TexDictionary::addFront(Texture *t) {
    if (t->dict) t->inDict.remove();
    t->dict = this;
    this->textures.add(&t->inDict);
}

Texture *TexDictionary::find(const char *name) {
    FORLIST(lnk, this->textures) {
        Texture *tex = Texture::fromDict(lnk);
        if (strncmp_ci(tex->name, name, 32) == 0) return tex;
    }
    return nil;
}

TexDictionary *TexDictionary::streamRead(Stream *stream) {
    if (!findChunk(stream, ID_STRUCT, nil, nil)) {
        RWERROR((ERR_CHUNK, "STRUCT"));
        return nil;
    }
    int32 numTex = stream->readI16();
    stream->readI16();  // device id (0 = unknown, 1 = d3d8, 2 = d3d9,
                        // 3 = gcn, 4 = null, 5 = opengl,
                        // 6 = ps2, 7 = softras, 8 = xbox, 9 = psp)
    TexDictionary *txd = TexDictionary::create();
    if (txd == nil) return nil;
    Texture *tex;
    for (int32 i = 0; i < numTex; i++) {
        if (!findChunk(stream, ID_TEXTURENATIVE, nil, nil)) {
            RWERROR((ERR_CHUNK, "TEXTURENATIVE"));
            goto fail;
        }
        tex = Texture::streamReadNative(stream);
        if (tex == nil) goto fail;
        Texture::s_plglist.streamRead(stream, tex);
        txd->add(tex);
    }
    if (s_plglist.streamRead(stream, txd)) return txd;
fail:
    txd->destroy();
    return nil;
}

void TexDictionary::streamWrite(Stream *stream) {
    writeChunkHeader(stream, ID_TEXDICTIONARY, this->streamGetSize());
    writeChunkHeader(stream, ID_STRUCT, 4);
    int32 numTex = this->count();
    stream->writeI16(numTex);
    stream->writeI16(0);
    FORLIST(lnk, this->textures) {
        Texture *tex = Texture::fromDict(lnk);
        uint32 sz = tex->streamGetSizeNative();
        sz += 12 + Texture::s_plglist.streamGetSize(tex);
        writeChunkHeader(stream, ID_TEXTURENATIVE, sz);
        tex->streamWriteNative(stream);
        Texture::s_plglist.streamWrite(stream, tex);
    }
    s_plglist.streamWrite(stream, this);
}

uint32 TexDictionary::streamGetSize(void) {
    uint32 size = 12 + 4;
    FORLIST(lnk, this->textures) {
        Texture *tex = Texture::fromDict(lnk);
        size += 12 + tex->streamGetSizeNative();
        size += 12 + Texture::s_plglist.streamGetSize(tex);
    }
    size += 12 + s_plglist.streamGetSize(this);
    return size;
}

void TexDictionary::setCurrent(TexDictionary *txd) {
    PLUGINOFFSET(TextureGlobals, engine, textureModuleOffset)->currentTexDict =
        txd;
}

TexDictionary *TexDictionary::getCurrent(void) {
    return PLUGINOFFSET(TextureGlobals, engine, textureModuleOffset)
        ->currentTexDict;
}

//
// Texture
//

static Texture *defaultFindCB(const char *name);
static Texture *defaultReadCB(const char *name, const char *mask);

Texture *(*Texture::findCB)(const char *name) = defaultFindCB;
Texture *(*Texture::readCB)(const char *name, const char *mask) = defaultReadCB;

Texture *Texture::create(Raster *raster) {
    Texture *tex =
        (Texture *)rwMalloc(s_plglist.size, MEMDUR_EVENT | ID_TEXTURE);
    if (tex == nil) {
        RWERROR((ERR_ALLOC, s_plglist.size));
        return nil;
    }
    tex->dict = nil;
    tex->inDict.init();
    memset(tex->name, 0, 32);
    memset(tex->mask, 0, 32);
    tex->filterAddressing = (WRAP << 12) | (WRAP << 8) | NEAREST;
    tex->raster = raster;
    tex->refCount = 1;
    s_plglist.construct(tex);
    return tex;
}

void Texture::destroy(void) {
    this->refCount--;
    if (this->refCount <= 0) {
        s_plglist.destruct(this);
        if (this->dict) this->inDict.remove();
        if (this->raster) this->raster->destroy();
        rwFree(this);
    }
}

static Texture *defaultFindCB(const char *name) {
    if (TEXTUREGLOBAL(currentTexDict))
        return TEXTUREGLOBAL(currentTexDict)->find(name);
    // TODO: RW searches *all* TXDs otherwise
    return nil;
}

// TODO: actually read the mask!
static Texture *defaultReadCB(const char *name, const char *mask) {
    Texture *tex;
    Image *img;
    char *n = (char *)rwMalloc(strlen(name) + 5, MEMDUR_FUNCTION | ID_TEXTURE);
    strcpy(n, name);
    strcat(n, ".tga");
    img = readTGA(n);
    rwFree(n);
    if (img) {
        tex = Texture::create(Raster::createFromImage(img));
        strncpy(tex->name, name, 32);
        if (mask) strncpy(tex->mask, mask, 32);
        img->destroy();
        return tex;
    } else
        return nil;
}

Texture *Texture::read(const char *name, const char *mask) {
    (void)mask;
    Raster *raster = nil;
    Texture *tex;

    if (tex = Texture::findCB(name), tex) {
        tex->refCount++;
        return tex;
    }
    if (TEXTUREGLOBAL(loadTextures)) {
        tex = Texture::readCB(name, mask);
        if (tex == nil) goto dummytex;
    } else
    dummytex:
        if (TEXTUREGLOBAL(makeDummies)) {
            // printf("missing texture %s %s\n", name ? name : "", mask ? mask :
            // "");
            tex = Texture::create(nil);
            if (tex == nil) return nil;
            strncpy(tex->name, name, 32);
            if (mask) strncpy(tex->mask, mask, 32);
            raster = Raster::create(0, 0, 0, Raster::DONTALLOCATE);
            tex->raster = raster;
        }
    if (tex && TEXTUREGLOBAL(currentTexDict)) {
        if (tex->dict) tex->inDict.remove();
        TEXTUREGLOBAL(currentTexDict)->add(tex);
    }
    return tex;
}

Texture *Texture::streamRead(Stream *stream) {
    uint32 length;
    char name[128], mask[128];
    if (!findChunk(stream, ID_STRUCT, nil, nil)) {
        RWERROR((ERR_CHUNK, "STRUCT"));
        return nil;
    }
    uint32 filterAddressing = stream->readU32();
    // TODO: if V addressing is 0, copy U
    // if using mipmap filter mode, set automipmapping,
    // if 0x10000 is set, set mipmapping

    if (!findChunk(stream, ID_STRING, &length, nil)) {
        RWERROR((ERR_CHUNK, "STRING"));
        return nil;
    }
    stream->read(name, length);

    if (!findChunk(stream, ID_STRING, &length, nil)) {
        RWERROR((ERR_CHUNK, "STRING"));
        return nil;
    }
    stream->read(mask, length);

    Texture *tex = Texture::read(name, mask);
    if (tex == nil) {
        s_plglist.streamSkip(stream);
        return nil;
    }
    if (tex->refCount == 1) tex->filterAddressing = filterAddressing;

    if (s_plglist.streamRead(stream, tex)) return tex;

    tex->destroy();
    return nil;
}

bool Texture::streamWrite(Stream *stream) {
    int size;
    char buf[36];
    writeChunkHeader(stream, ID_TEXTURE, this->streamGetSize());
    writeChunkHeader(stream, ID_STRUCT, 4);
    stream->writeU32(this->filterAddressing);

    memset(buf, 0, 36);
    strncpy(buf, this->name, 32);
    size = strlen(buf) + 4 & ~3;
    writeChunkHeader(stream, ID_STRING, size);
    stream->write(buf, size);

    memset(buf, 0, 36);
    strncpy(buf, this->mask, 32);
    size = strlen(buf) + 4 & ~3;
    writeChunkHeader(stream, ID_STRING, size);
    stream->write(buf, size);

    s_plglist.streamWrite(stream, this);
    return true;
}

uint32 Texture::streamGetSize(void) {
    uint32 size = 0;
    size += 12 + 4;
    size += 12 + 12;
    size += strlen(this->name) + 4 & ~3;
    size += strlen(this->mask) + 4 & ~3;
    size += 12 + s_plglist.streamGetSize(this);
    return size;
}

Texture *Texture::streamReadNative(Stream *stream) {
    if (!findChunk(stream, ID_STRUCT, nil, nil)) {
        RWERROR((ERR_CHUNK, "STRUCT"));
        return nil;
    }
    uint32 platform = stream->readU32();
    stream->seek(-16);
    if (platform == FOURCC_PS2) return ps2::readNativeTexture(stream);
    if (platform == PLATFORM_D3D8) return d3d8::readNativeTexture(stream);
    if (platform == PLATFORM_D3D9) return d3d9::readNativeTexture(stream);
    if (platform == PLATFORM_XBOX) return xbox::readNativeTexture(stream);
    assert(0 && "unsupported platform");
    return nil;
}

void Texture::streamWriteNative(Stream *stream) {
    if (this->raster->platform == PLATFORM_PS2)
        ps2::writeNativeTexture(this, stream);
    else if (this->raster->platform == PLATFORM_D3D8)
        d3d8::writeNativeTexture(this, stream);
    else if (this->raster->platform == PLATFORM_D3D9)
        d3d9::writeNativeTexture(this, stream);
    else if (this->raster->platform == PLATFORM_XBOX)
        xbox::writeNativeTexture(this, stream);
    else
        assert(0 && "unsupported platform");
}

uint32 Texture::streamGetSizeNative(void) {
    if (this->raster->platform == PLATFORM_PS2)
        return ps2::getSizeNativeTexture(this);
    if (this->raster->platform == PLATFORM_D3D8)
        return d3d8::getSizeNativeTexture(this);
    if (this->raster->platform == PLATFORM_D3D9)
        return d3d9::getSizeNativeTexture(this);
    if (this->raster->platform == PLATFORM_XBOX)
        return xbox::getSizeNativeTexture(this);
    assert(0 && "unsupported platform");
    return 0;
}

//
// Raster
//

Raster *Raster::create(int32 width, int32 height, int32 depth, int32 format,
                       int32 platform) {
    // TODO: pass arguments through to the driver and create the raster there
    Raster *raster = (Raster *)rwMalloc(s_plglist.size, MEMDUR_EVENT);  // TODO
    assert(raster != nil);
    raster->parent = raster;
    raster->offsetX = 0;
    raster->offsetY = 0;
    raster->platform = platform ? platform : rw::platform;
    raster->type = format & 0x7;
    raster->flags = format & 0xF8;
    raster->format = format & 0xFF00;
    raster->width = width;
    raster->height = height;
    raster->depth = depth;
    raster->pixels = raster->palette = nil;
    s_plglist.construct(raster);

    //	printf("%d %d %d %d\n", raster->type, raster->width, raster->height,
    //raster->depth);
    engine->driver[raster->platform]->rasterCreate(raster);
    return raster;
}

void Raster::subRaster(Raster *parent, Rect *r) {
    if ((this->flags & DONTALLOCATE) == 0) return;
    this->width = r->w;
    this->height = r->h;
    this->offsetX += r->x;
    this->offsetY += r->y;
    this->parent = parent->parent;
}

void Raster::destroy(void) {
    s_plglist.destruct(this);
    //	delete[] this->texels;
    //	delete[] this->palette;
    rwFree(this);
}

uint8 *Raster::lock(int32 level) {
    return engine->driver[this->platform]->rasterLock(this, level);
}

void Raster::unlock(int32 level) {
    engine->driver[this->platform]->rasterUnlock(this, level);
}

int32 Raster::getNumLevels(void) {
    return engine->driver[this->platform]->rasterNumLevels(this);
}

int32 Raster::calculateNumLevels(int32 width, int32 height) {
    int32 size = width >= height ? width : height;
    int32 n;
    for (n = 0; size != 0; n++) size /= 2;
    return n;
}

bool Raster::formatHasAlpha(int32 format) {
    return (format & 0xF00) == Raster::C8888 ||
           (format & 0xF00) == Raster::C1555 ||
           (format & 0xF00) == Raster::C4444;
}

Raster *Raster::createFromImage(Image *image, int32 platform) {
    Raster *raster = Raster::create(image->width, image->height, image->depth,
                                    TEXTURE | DONTALLOCATE, platform);
    engine->driver[raster->platform]->rasterFromImage(raster, image);
    return raster;
}

Image *Raster::toImage(void) {
    return engine->driver[this->platform]->rasterToImage(this);
}

}  // namespace rw
