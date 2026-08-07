#include <dlfcn.h>

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: layer_exports_test <shared-library>\n";
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        std::cerr << "dlopen failed: " << dlerror() << '\n';
        return 1;
    }

    const char* const symbols[] = {
        "layer_vkGetInstanceProcAddr",
        "layer_vkGetDeviceProcAddr",
    };
    for (const char* symbol : symbols) {
        if (dlsym(library, symbol) == nullptr) {
            std::cerr << "missing exported layer entry point: " << symbol << '\n';
            dlclose(library);
            return 1;
        }
    }

    dlclose(library);
    return 0;
}
