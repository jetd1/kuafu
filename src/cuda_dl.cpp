//
// Created by jet on 9/17/21.
//
#include <cuda.h>
#include <dlfcn.h>

namespace kuafu {

void* libcuda = nullptr;

void kfCuDlOpen() {
    libcuda = dlopen("/usr/lib/x86_64-linux-gnu/libcuda.so", RTLD_NOW);    // Do PATH lookup
    if (!libcuda)
        global::logger->critical("Failed to load CUDA driver lib for the denoiser: {}."
                                 "Hint: have you installed the latest NVIDIA driver?", dlerror());
    dlerror();
}

CUresult kfCuInit(unsigned int Flags) {
    if (!libcuda)
        global::logger->critical("CUDA driver lib is not loaded!");
    typedef decltype(kfCuInit) FuncType;
    try {
        auto func = reinterpret_cast<FuncType*>(dlsym(libcuda, "cuInit"));
        return func(Flags);
    } catch (std::exception& e) {
        throw std::runtime_error(std::string("kfCuInit failed: ") + e.what());
    }
}

CUresult kfCuStreamCreate(CUstream* phStream, unsigned int Flags) {
    if (!libcuda)
        global::logger->critical("CUDA driver lib is not loaded!");
    typedef decltype(kfCuStreamCreate) FuncType;
    try {
        auto func = reinterpret_cast<FuncType*>(dlsym(libcuda, "cuStreamCreate"));
        return func(phStream, Flags);
    } catch (std::exception& e) {
        throw std::runtime_error(std::string("kfCuStreamCreate failed: ") + e.what());
    }
}

CUresult kfCuCtxCreate(CUcontext* pctx, unsigned int flags, CUdevice dev) {
    if (!libcuda)
        global::logger->critical("CUDA driver lib is not loaded!");
    typedef decltype(kfCuCtxCreate) FuncType;
    try {
        auto func = reinterpret_cast<FuncType *>(dlsym(libcuda, "cuCtxCreate_v2"));
        return func(pctx, flags, dev);
    } catch (std::exception& e) {
        throw std::runtime_error(std::string("kfCuCtxCreate failed: ") + e.what());
    }
}

CUresult kfCuCtxDestroy(CUcontext ctx) {
    if (!libcuda)
        global::logger->critical("CUDA driver lib is not loaded!");
    typedef decltype(kfCuCtxDestroy) FuncType;
    try {
        auto func = reinterpret_cast<FuncType *>(dlsym(libcuda, "cuCtxDestroy_v2"));
        return func(ctx);
    } catch (std::exception& e) {
        throw std::runtime_error(std::string("kfCuCtxDestroy failed: ") + e.what());
    }
}

void kfCuDlClose() {
    if (libcuda)
        dlclose(libcuda);
}

}

