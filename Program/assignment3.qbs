import qbs

CppApplication {
    consoleApplication: true
    Group {
        name: "imgui"
        files: [
            "imconfig.h",
            "imgui.cpp",
            "imgui.h",
            "imgui_demo.cpp",
            "imgui_draw.cpp",
            "imgui_impl_sdl_gl3.cpp",
            "imgui_impl_sdl_gl3.h",
            "imgui_internal.h"
        ]
    }

    Group {
        name: "opengl"
        files: [
            "opengl.cpp",
            "opengl.h",
            "shaderset.cpp",
            "shaderset.h"
        ]
    }

    Group {
        name: "sdl"
        files: [
            "mysdl_dpi.cpp",
            "mysdl_dpi.h"
        ]
    }

    Group {
        name: "tinyobjloader"
        files: [
            "tiny_obj_loader.cc",
            "tiny_obj_loader.h"
        ]
    }

    Group {
        name: "stb"
        files: [
            "stb_image.c",
            "stb_image.h",
            "stb_rect_pack.h",
            "stb_textedit.h",
            "stb_truetype.h"
        ]
    }

    Group {
        name: "containers"
        files: [
            "packed_freelist.h"
        ]
    }

    Group {
        name: "cameras"
        files: [
            "flythrough_camera.h"
        ]
    }

    Group {
        name: "shaders"
        files: [
            "cloud.frag",
            "cloud.vert",
            "cloud_ray.frag",
            "cloud_ray.vert",
            "depthvis.frag",
            "depthvis.vert",
            "preamble.glsl",
            "scene.vert",
            "scene.frag",
        ]
    }

    files: [
        "main.cpp",
        "renderer.cpp",
        "renderer.h",
        "scene.cpp",
        "scene.h",
        "simulation.cpp",
        "simulation.h"
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }

    // Avoid linking to the system's SDL because the system's SDL is probably too old and lacks DPI functionality
    // For some reason Qbs can't link to locally built frameworks (it keeps trying to get the system one), but it works with the lib
    // I assume this is because there is no libSDL2 in /usr/lib
    // soooooo meta. Qbs is yet another example of why meta build systems are so universally a bad idea.
    // Anyways, this links to a custom-built statically linked SDL2. Because of that, all its dependencies need to be listed out manually too.
    cpp.frameworks: ["Cocoa", "CoreAudio", "AudioToolbox", "CoreVideo", "ForceFeedback", "IOKit", "Carbon"]
    cpp.includePaths: ["include", "OSX/include"]
    cpp.libraryPaths: ["OSX/lib"]
    cpp.staticLibraries: ["SDL2"]
    cpp.dynamicLibraries: ["iconv"]

    cpp.cxxLanguageVersion: "c++14"

    Properties {
        condition: qbs.buildVariant == "debug"
        cpp.defines: ["_DEBUG"]
    }
}
