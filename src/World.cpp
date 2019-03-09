#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Base.hpp"
#include "Engine.hpp"
#include "Error.hpp"
#include "Objects.hpp"
#include "Pipeline.hpp"
#include "Plg.hpp"

#define PLUGIN_ID ID_WORLD

namespace rw {

PluginList World::s_plglist = {sizeof(World), sizeof(World), nil, nil};

World *World::create(void) {
    World *world = (World *)rwMalloc(s_plglist.size, MEMDUR_EVENT | ID_WORLD);
    if (world == nil) {
        RWERROR((ERR_ALLOC, s_plglist.size));
        return nil;
    }
    world->object.init(World::ID, 0);
    world->lights.init();
    world->directionalLights.init();
    return world;
}

void World::addLight(Light *light) {
    light->world = this;
    if (light->getType() < Light::POINT) {
        this->directionalLights.append(&light->inWorld);
    } else {
        this->lights.append(&light->inWorld);
        if (light->getFrame()) light->getFrame()->updateObjects();
    }
}

void World::addCamera(Camera *cam) {
    cam->world = this;
    if (cam->getFrame()) cam->getFrame()->updateObjects();
}

}  // namespace rw
