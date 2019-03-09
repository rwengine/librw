#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Base.hpp"
#include "Engine.hpp"
#include "Error.hpp"
#include "Objects.hpp"
#include "Pipeline.hpp"
#include "Plg.hpp"
#include "UserData.hpp"

#define PLUGIN_ID ID_USERDATA

namespace rw {

UserDataGlobals userDataGlobals;

#define udMalloc(sz) rwMalloc(sz, MEMDUR_EVENT | ID_USERDATA)

void UserDataArray::setString(int32 n, const char *s) {
    int32 len;
    char **sp = &((char **)this->data)[n];
    rwFree(*sp);
    len = (int32)strlen(s) + 1;
    *sp = (char *)udMalloc(len);
    if (*sp) strncpy(*sp, s, len);
}

static void *createUserData(void *object, int32 offset, int32) {
    UserDataExtension *ext = PLUGINOFFSET(UserDataExtension, object, offset);
    ext->numArrays = 0;
    ext->arrays = nil;
    return object;
}

static void *destroyUserData(void *object, int32 offset, int32) {
    int32 i, j;
    char **strar;
    UserDataArray *a;
    UserDataExtension *ext = PLUGINOFFSET(UserDataExtension, object, offset);
    a = ext->arrays;
    for (i = 0; i < ext->numArrays; i++) {
        rwFree(a->name);
        switch (a->datatype) {
            case USERDATASTRING:
                strar = (char **)a->data;
                for (j = 0; j < a->numElements; j++) rwFree(strar[j]);
                /* fallthrough */
            case USERDATAINT:
            case USERDATAFLOAT:
                rwFree(a->data);
                break;
        }
        a++;
    }
    rwFree(ext->arrays);
    ext->numArrays = 0;
    ext->arrays = nil;
    return object;
}

static void *copyUserData(void *dst, void *src, int32 offset, int32) {
    int32 i, j;
    char **srcstrar, **dststrar;
    UserDataArray *srca, *dsta;
    UserDataExtension *srcext = PLUGINOFFSET(UserDataExtension, src, offset);
    UserDataExtension *dstext = PLUGINOFFSET(UserDataExtension, dst, offset);
    dstext->numArrays = srcext->numArrays;
    dstext->arrays =
        (UserDataArray *)udMalloc(dstext->numArrays * sizeof(UserDataArray));
    srca = srcext->arrays;
    dsta = srcext->arrays;
    for (i = 0; i < srcext->numArrays; i++) {
        int32 len = (int32)strlen(srca->name) + 1;
        dsta->name = (char *)udMalloc(len);
        strncpy(dsta->name, srca->name, len);
        dsta->datatype = srca->datatype;
        dsta->numElements = srca->numElements;
        switch (srca->datatype) {
            case USERDATAINT:
                dsta->data =
                    (int32 *)udMalloc(sizeof(int32) * dsta->numElements);
                memcpy(dsta->data, srca->data,
                       sizeof(int32) * dsta->numElements);
                break;
            case USERDATAFLOAT:
                dsta->data =
                    (float32 *)udMalloc(sizeof(float32) * dsta->numElements);
                memcpy(dsta->data, srca->data,
                       sizeof(float32) * dsta->numElements);
                break;
            case USERDATASTRING:
                dststrar =
                    (char **)udMalloc(sizeof(char *) * dsta->numElements);
                dsta->data = dststrar;
                srcstrar = (char **)srca->data;
                for (j = 0; j < dsta->numElements; j++) {
                    len = (int32)strlen(srcstrar[j]) + 1;
                    dststrar[j] = (char *)udMalloc(len);
                    strncpy(dststrar[j], srcstrar[j], len);
                }
                break;
        }
        srca++;
        dsta++;
    }
    return dst;
}

static Stream *readUserData(Stream *stream, int32, void *object, int32 offset,
                            int32) {
    int32 i, j;
    char **strar;
    UserDataArray *a;
    UserDataExtension *ext = PLUGINOFFSET(UserDataExtension, object, offset);
    ext->numArrays = stream->readI32();
    ext->arrays =
        (UserDataArray *)udMalloc(ext->numArrays * sizeof(UserDataArray));
    a = ext->arrays;
    for (i = 0; i < ext->numArrays; i++) {
        int32 len = stream->readI32();
        a->name = (char *)udMalloc(len);
        stream->read(a->name, len);
        a->datatype = stream->readU32();
        a->numElements = stream->readI32();
        switch (a->datatype) {
            case USERDATAINT:
                a->data = (int32 *)udMalloc(sizeof(int32) * a->numElements);
                stream->read(a->data, sizeof(int32) * a->numElements);
                break;
            case USERDATAFLOAT:
                a->data = (float32 *)udMalloc(sizeof(float32) * a->numElements);
                stream->read(a->data, sizeof(float32) * a->numElements);
                break;
            case USERDATASTRING:
                strar = (char **)udMalloc(sizeof(char *) * a->numElements);
                a->data = strar;
                for (j = 0; j < a->numElements; j++) {
                    len = stream->readI32();
                    strar[j] = (char *)udMalloc(len);
                    stream->read(strar[j], len);
                }
                break;
        }
        a++;
    }
    return stream;
}

static Stream *writeUserData(Stream *stream, int32, void *object, int32 offset,
                             int32) {
    int32 len;
    int32 i, j;
    char **strar;
    UserDataArray *a;
    UserDataExtension *ext = PLUGINOFFSET(UserDataExtension, object, offset);
    stream->writeI32(ext->numArrays);
    a = ext->arrays;
    for (i = 0; i < ext->numArrays; i++) {
        len = (int32)strlen(a->name) + 1;
        stream->writeI32(len);
        stream->write(a->name, len);
        stream->writeU32(a->datatype);
        stream->writeI32(a->numElements);
        switch (a->datatype) {
            case USERDATAINT:
                stream->write(a->data, sizeof(int32) * a->numElements);
                break;
            case USERDATAFLOAT:
                stream->write(a->data, sizeof(float32) * a->numElements);
                break;
            case USERDATASTRING:
                strar = (char **)a->data;
                for (j = 0; j < a->numElements; j++) {
                    len = (int32)strlen(strar[j]) + 1;
                    stream->writeI32(len);
                    stream->write(strar[j], len);
                }
                break;
        }
        a++;
    }
    return stream;
}

static int32 getSizeUserData(void *object, int32 offset, int32) {
    int32 len;
    int32 i, j;
    char **strar;
    int32 size;
    UserDataArray *a;
    UserDataExtension *ext = PLUGINOFFSET(UserDataExtension, object, offset);
    if (ext->numArrays == 0) return 0;
    size = 4;  // numArrays
    a = ext->arrays;
    for (i = 0; i < ext->numArrays; i++) {
        len = (int32)strlen(a->name) + 1;
        size += 4 + len + 4 + 4;  // name len, name, type, numElements
        switch (a->datatype) {
            case USERDATAINT:
                size += sizeof(int32) * a->numElements;
                break;
            case USERDATAFLOAT:
                size += sizeof(float32) * a->numElements;
                break;
            case USERDATASTRING:
                strar = (char **)a->data;
                for (j = 0; j < a->numElements; j++) {
                    len = (int32)strlen(strar[j]) + 1;
                    size += 4 + len;  // len and string
                }
                break;
        }
        a++;
    }
    return size;
}

static int32 add(UserDataExtension *ext, const char *name, int32 datatype,
                 int32 numElements) {
    int32 i;
    int32 len;
    int32 typesz;
    UserDataArray *a;
    // try to find empty slot
    for (i = 0; i < ext->numArrays; i++)
        if (ext->arrays[i].datatype == USERDATANA) goto alloc;
    // have to realloc
    a = (UserDataArray *)udMalloc((ext->numArrays + 1) * sizeof(UserDataArray));
    if (a == nil) return -1;
    memcpy(a, ext->arrays, ext->numArrays * sizeof(UserDataArray));
    rwFree(ext->arrays);
    ext->arrays = a;
    i = ext->numArrays++;
alloc:
    a = &ext->arrays[i];
    len = (int32)strlen(name) + 1;
    a->name = (char *)udMalloc(len + 1);
    assert(a->name);
    strncpy(a->name, name, len);
    a->datatype = datatype;
    a->numElements = numElements;
    typesz = datatype == USERDATAINT
                 ? sizeof(int32)
                 : datatype == USERDATAFLOAT
                       ? sizeof(float32)
                       : datatype == USERDATASTRING ? sizeof(char *) : 0;
    a->data = udMalloc(typesz * numElements);
    assert(a->data);
    memset(a->data, 0, typesz * numElements);
    return i;
}

static void remove(UserDataExtension *ext, int32 n) {
    int32 i;
    UserDataArray *a = &ext->arrays[n];
    if (a->name) {
        rwFree(a->name);
        a->name = nil;
    }
    if (a->datatype == USERDATASTRING)
        for (i = 0; i < a->numElements; i++) rwFree(((char **)a->data)[i]);
    if (a->data) {
        rwFree(a->data);
        a->data = nil;
    }
    a->datatype = USERDATANA;
    a->numElements = 0;
}

#define ACCESSOR(TYPE, NAME)                                                  \
    int32 UserDataArray::NAME##Add(TYPE *t, const char *name, int32 datatype, \
                                   int32 numElements) {                       \
        return add(                                                           \
            PLUGINOFFSET(UserDataExtension, t, userDataGlobals.NAME##Offset), \
            name, datatype, numElements);                                     \
    }                                                                         \
    void UserDataArray::NAME##Remove(TYPE *t, int32 n) {                      \
        remove(                                                               \
            PLUGINOFFSET(UserDataExtension, t, userDataGlobals.NAME##Offset), \
            n);                                                               \
    }                                                                         \
    int32 UserDataArray::NAME##GetCount(TYPE *t) {                            \
        return PLUGINOFFSET(UserDataExtension, t,                             \
                            userDataGlobals.NAME##Offset)                     \
            ->numArrays;                                                      \
    }                                                                         \
    UserDataArray *UserDataArray::NAME##Get(TYPE *t, int32 n) {               \
        if (n >=                                                              \
            PLUGINOFFSET(UserDataExtension, t, userDataGlobals.NAME##Offset)  \
                ->numArrays)                                                  \
            return nil;                                                       \
        return &PLUGINOFFSET(UserDataExtension, t,                            \
                             userDataGlobals.NAME##Offset)                    \
                    ->arrays[n];                                              \
    }                                                                         \
    int32 UserDataArray::NAME##FindIndex(TYPE *t, const char *name) {         \
        int32 i;                                                              \
        UserDataExtension *ext =                                              \
            PLUGINOFFSET(UserDataExtension, t, userDataGlobals.NAME##Offset); \
        for (i = 0; i < ext->numArrays; i++)                                  \
            if (strcmp(ext->arrays[i].name, name) == 0) return i;             \
        return -1;                                                            \
    }

ACCESSOR(Geometry, geometry)
ACCESSOR(Frame, frame)
ACCESSOR(Camera, camera)
ACCESSOR(Light, light)
ACCESSOR(Material, material)
ACCESSOR(Texture, texture)

void registerUserDataPlugin(void) {
    // TODO: World Sector

    userDataGlobals.geometryOffset =
        Geometry::registerPlugin(sizeof(UserDataExtension), ID_USERDATA,
                                 createUserData, destroyUserData, copyUserData);
    Geometry::registerPluginStream(ID_USERDATA, readUserData, writeUserData,
                                   getSizeUserData);

    userDataGlobals.frameOffset =
        Frame::registerPlugin(sizeof(UserDataExtension), ID_USERDATA,
                              createUserData, destroyUserData, copyUserData);
    Frame::registerPluginStream(ID_USERDATA, readUserData, writeUserData,
                                getSizeUserData);

    userDataGlobals.cameraOffset =
        Camera::registerPlugin(sizeof(UserDataExtension), ID_USERDATA,
                               createUserData, destroyUserData, copyUserData);
    Camera::registerPluginStream(ID_USERDATA, readUserData, writeUserData,
                                 getSizeUserData);

    userDataGlobals.lightOffset =
        Light::registerPlugin(sizeof(UserDataExtension), ID_USERDATA,
                              createUserData, destroyUserData, copyUserData);
    Light::registerPluginStream(ID_USERDATA, readUserData, writeUserData,
                                getSizeUserData);

    userDataGlobals.materialOffset =
        Material::registerPlugin(sizeof(UserDataExtension), ID_USERDATA,
                                 createUserData, destroyUserData, copyUserData);
    Material::registerPluginStream(ID_USERDATA, readUserData, writeUserData,
                                   getSizeUserData);

    userDataGlobals.textureOffset =
        Texture::registerPlugin(sizeof(UserDataExtension), ID_USERDATA,
                                createUserData, destroyUserData, copyUserData);
    Texture::registerPluginStream(ID_USERDATA, readUserData, writeUserData,
                                  getSizeUserData);
}

}  // namespace rw
