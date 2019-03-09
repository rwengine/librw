if(LIBRW_PLATFORM_GL3)
    set(OpenGL_GL_PREFERENCE GLVND)
    #find_package(OpenGL REQUIRED)
    #find_package(SDL2 REQUIRED)
    find_package(GLEW REQUIRED)
endif()

add_library(librw
    "${PROJECT_SOURCE_DIR}/args.h"
    "${PROJECT_SOURCE_DIR}/rw.h"

    Animation.cpp
    Animation.hpp
    Base.cpp
    Base.hpp
    Base.err
    Engine.cpp
    Engine.hpp
    Error.hpp
    Objects.hpp
    Pipeline.hpp
    Plg.hpp
    rwplugins.h
    Render.hpp
    UserData.hpp
    Bmp.cpp
    Camera.cpp
    Clump.cpp
    Error.cpp
    Frame.cpp
    Geometry.cpp
    Geoplg.cpp
    Hanim.cpp
    Image.cpp
    Light.cpp
    MatFx.cpp
    Pipeline.cpp
    Plg.cpp
    Prim.cpp
    Render.cpp
    Skin.cpp
    Texture.cpp
    Tga.cpp
    Tristrip.cpp
    UserData.cpp
    UvAnim.cpp
    World.cpp

    d3d/rwd3d8.h
    d3d/rwd3d9.h
    d3d/rwd3d.h
    d3d/rwd3dimpl.h
    d3d/rwxbox.h
    d3d/rwxboximpl.h
    d3d/d3d8.cpp
    d3d/d3d8matfx.cpp
    d3d/d3d8render.cpp
    d3d/d3d8skin.cpp
    d3d/d3d9.cpp
    d3d/d3d9matfx.cpp
    d3d/d3d9render.cpp
    d3d/d3d9skin.cpp
    d3d/d3d.cpp
    d3d/d3ddevice.cpp
    d3d/d3dimmed.cpp
    d3d/d3drender.cpp
    d3d/xbox.cpp
    d3d/xboxmatx.cpp
    d3d/xboxskin.cpp
    d3d/xboxvfmt.cpp

    gl/rwgl3.h
    gl/rwgl3impl.h
    gl/rwgl3plg.h
    gl/rwgl3shader.h
    gl/rwwdgl.h
    gl/gl3.cpp
    gl/gl3device.cpp
    gl/gl3immed.cpp
    gl/gl3matfx.cpp
    gl/gl3pipe.cpp
    gl/gl3raster.cpp
    gl/gl3render.cpp
    gl/gl3shader.cpp
    gl/gl3skin.cpp
    gl/wdgl.cpp

    ps2/rwps2.h
    ps2/rwps2impl.h
    ps2/rwps2plg.h
    ps2/pds.cpp
    ps2/ps2.cpp
    ps2/ps2device.cpp
    ps2/ps2matfx.cpp
    ps2/ps2raster.cpp
    ps2/ps2skin.cpp
    )

target_include_directories(librw
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    )

target_compile_definitions(librw
    PRIVATE
        "$<IF:$<CONFIG:DEBUG>,DEBUG,NDEBUG>"
    PUBLIC
        "RW_${LIBRW_PLATFORM}"
        "$<$<BOOL:LIBRW_PLATFORM_GL3>:LIBRW_SDL2>"
    )

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options(librw
        PRIVATE
            "-Wall"
        )
    if (NOT LIBRW_PLATFORM_PS2)
        target_compile_options(librw
            PRIVATE

                "-Wdouble-promotion"
            )
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(librw
        PUBLIC
            /wd4996 /wd4244
        )
endif()

set_target_properties(librw
    PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS OFF
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        PREFIX ""
    )

if(LIBRW_PLATFORM_GL3)
    target_link_libraries(librw
        PUBLIC
            SDL2
            GLEW::GLEW
            OpenGL::GL
        )
elseif(LIBRW_PLATFORM_D3D9)
    target_link_libraries(librw
        PUBLIC
            d3d9
            xinput
        )
endif()

if(LIBRW_INSTALL)
    target_include_directories(librw
        INTERFACE
            $<INSTALL_INTERFACE:${LIBRW_INSTALL_INCLUDEDIR}>
        )

    install(
        FILES
            "${PROJECT_SOURCE_DIR}/args.h"
            "${PROJECT_SOURCE_DIR}/rw.h"
        DESTINATION "${LIBRW_INSTALL_INCLUDEDIR}"
        )
    install(
        FILES
            base.err
            Base.hpp
            Error.hpp
            Plg.hpp
            Render.hpp
            Engine.hpp
            Pipeline.hpp
            Objects.hpp
            Animation.hpp
            rwplugins.h
            UserData.hpp
        DESTINATION "${LIBRW_INSTALL_INCLUDEDIR}/src"
        )
    install(
        FILES
            d3d/rwxbox.h
            d3d/rwd3d.h
            d3d/rwd3d8.h
            d3d/rwd3d9.h
        DESTINATION "${LIBRW_INSTALL_INCLUDEDIR}/src/d3d"
        )
    install(
        FILES
            ps2/rwps2.h
            ps2/rwps2plg.h
        DESTINATION "${LIBRW_INSTALL_INCLUDEDIR}/src/ps2"
        )
    install(
        FILES
            gl/rwwdgl.h
            gl/rwgl3.h
            gl/rwgl3shader.h
        DESTINATION "${LIBRW_INSTALL_INCLUDEDIR}/src/gl"
        )

    install(
        TARGETS librw
        EXPORT librw-targets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        )
endif()
