#ifndef RWLIB_ENGINE_
#define RWLIB_ENGINE_

#include "Base.hpp"
#include "Plg.hpp"

namespace rw {

enum DeviceReq {
    // Device/Context creation
    DEVICEOPEN,
    // Device/Context shutdown
    DEVICECLOSE,

    // Device initialization before Engine/Driver plugins are opened
    DEVICEINIT,
    // Device de-initialization after Engine/Driver plugins are closed
    DEVICETERM,

    // Device initialization after Engine/Driver plugins are opened
    DEVICEFINALIZE
    // TODO? counterpart to FINALIZE?
};

typedef int DeviceSystem(DeviceReq req, void *arg0);

struct Camera;
struct Image;
struct Texture;
struct Raster;
class ObjPipeline;

// This is for the render device, we only have one
struct Device {
    float32 zNear, zFar;
    void (*beginUpdate)(Camera *);
    void (*endUpdate)(Camera *);
    void (*clearCamera)(Camera *, RGBA *col, uint32 mode);
    void (*showRaster)(Raster *raster);
    void (*setRenderState)(int32 state, void *value);
    void *(*getRenderState)(int32 state);

    void (*im2DRenderLine)(void *, int32, int32, int32);
    void (*im2DRenderTriangle)(void *, int32, int32, int32, int32);
    void (*im2DRenderPrimitive)(PrimitiveType, void *, int32);
    void (*im2DRenderIndexedPrimitive)(PrimitiveType, void *, int32, void *,
                                       int32);

    // Not sure if this will stay...
    void (*im3DTransform)(void *vertices, int32 numVertices, Matrix *world);
    void (*im3DRenderIndexed)(PrimitiveType primType, void *indices,
                              int32 numIndices);
    void (*im3DEnd)(void);

    DeviceSystem *system;
};

// This is for platform-dependent but portable things
// so the engine has one for every platform
struct Driver {
    ObjPipeline *defaultPipeline;
    int32 rasterNativeOffset;

    void (*rasterCreate)(Raster *);
    uint8 *(*rasterLock)(Raster *, int32 level);
    void (*rasterUnlock)(Raster *, int32 level);
    int32 (*rasterNumLevels)(Raster *);
    void (*rasterFromImage)(Raster *, Image *);
    Image *(*rasterToImage)(Raster *);

    static PluginList s_plglist[NUM_PLATFORMS];
    static int32 registerPlugin(int32 platform, int32 size, uint32 id,
                                Constructor ctor, Destructor dtor) {
        return s_plglist[platform].registerPlugin(size, id, ctor, dtor, nil);
    }
};

struct EngineStartParams;

enum MemHint {
    MEMDUR_NA = 0,
    // used inside function
    MEMDUR_FUNCTION = 0x10000,
    // used for one frame
    MEMDUR_FRAME = 0x20000,
    // used for longer time
    MEMDUR_EVENT = 0x30000,
    // used while the engine is running
    MEMDUR_GLOBAL = 0x40000
};

struct MemoryFunctions {
    void *(*rwmalloc)(size_t sz, uint32 hint);
    void *(*rwrealloc)(void *p, size_t sz, uint32 hint);
    void (*rwfree)(void *p);
    // These are temporary until we have better error handling
    // TODO: Maybe don't put them here since they shouldn't really be switched
    // out
    void *(*rwmustmalloc)(size_t sz, uint32 hint);
    void *(*rwmustrealloc)(void *p, size_t sz, uint32 hint);
};

// This is for platform independent things
// TODO: move more stuff into this
struct Engine {
    enum State { Dead = 0, Initialized, Opened, Started };
    void *currentCamera;
    void *currentWorld;
    LinkList frameDirtyList;

    // Dynamically allocated because of plugins
    Driver *driver[NUM_PLATFORMS];
    Device device;

    // These must always be available
    static MemoryFunctions memfuncs;
    static State state;

    static bool32 init(void);
    static bool32 open(void);
    static bool32 start(EngineStartParams *);
    static void term(void);
    static void close(void);
    static void stop(void);

    static PluginList s_plglist;
    static int32 registerPlugin(int32 size, uint32 id, Constructor ctor,
                                Destructor dtor) {
        return s_plglist.registerPlugin(size, id, ctor, dtor, nil);
    }
};

extern Engine *engine;

// These must be macros because we might want to pass __FILE__ and __LINE__
// later
#define rwMalloc(s, h) rw::Engine::memfuncs.rwmalloc(s, h)
#define rwMallocT(t, s, h) \
    (t *)rw::Engine::memfuncs.rwmalloc((s) * sizeof(t), h)
#define rwRealloc(p, s, h) rw::Engine::memfuncs.rwrealloc(p, s, h)
#define rwReallocT(t, p, s, h) \
    (t *)rw::Engine::memfuncs.rwrealloc(p, (s) * sizeof(t), h)
#define rwFree(p) rw::Engine::memfuncs.rwfree(p)
#define rwNew(s, h) rw::Engine::memfuncs.rwmustmalloc(s, h)
#define rwNewT(t, s, h) \
    (t *)rw::Engine::memfuncs.rwmustmalloc((s) * sizeof(t), h)
#define rwResize(p, s, h) rw::Engine::memfuncs.rwmustrealloc(p, s, h)
#define rwResizeT(t, p, s, h) \
    (t *)rw::Engine::memfuncs.rwmustrealloc(p, (s) * sizeof(t), h)

namespace null {
void beginUpdate(Camera *);
void endUpdate(Camera *);
void clearCamera(Camera *, RGBA *col, uint32 mode);
void showRaster(Raster *);

void setRenderState(int32 state, void *value);
void *getRenderState(int32 state);

void rasterCreate(Raster *);
uint8 *rasterLock(Raster *, int32 level);
void rasterUnlock(Raster *, int32 level);
int32 rasterNumLevels(Raster *);
void rasterFromImage(Raster *, Image *);
Image *rasterToImage(Raster *);

void im2DRenderLine(void *, int32, int32, int32);
void im2DRenderTriangle(void *, int32, int32, int32, int32);
void im2DRenderPrimitive(PrimitiveType, void *, int32);
void im2DRenderIndexedPrimitive(PrimitiveType, void *, int32, void *, int32);

void im3DTransform(void *vertices, int32 numVertices, Matrix *world);
void im3DRenderIndexed(PrimitiveType primType, void *indices, int32 numIndices);
void im3DEnd(void);

int deviceSystem(DeviceReq req, void *);

extern Device renderdevice;
}  // namespace null

}  // namespace rw

#endif
