//
// Created by jet on 9/17/21.
//
#include <cuda.h>
#include <dlfcn.h>
#include <driver_types.h>

namespace kuafu {

void* libcuda = nullptr;

void kfCuDlOpen() {
    libcuda = dlopen("/usr/lib/x86_64-linux-gnu/libcuda.so", RTLD_NOW);    // Do PATH lookup
}

CUresult kfCuInit(unsigned int Flags) {
    if (!libcuda)
        global::logger->critical("CUDA driver lib is not loaded!");
    typedef decltype(kfCuInit) FuncType;
    auto func = reinterpret_cast<FuncType*>(dlsym(libcuda, "cuInit"));
    return func(Flags);
}

CUresult kfCuStreamCreate(CUstream* phStream, unsigned int Flags) {
    if (!libcuda)
        global::logger->critical("CUDA driver lib is not loaded!");
    typedef decltype(kfCuStreamCreate) FuncType;
    auto func = reinterpret_cast<FuncType*>(dlsym(libcuda, "cuStreamCreate"));
    return func(phStream, Flags);
}

CUresult kfCuCtxCreate(CUcontext* pctx, unsigned int flags, CUdevice dev) {
    if (!libcuda)
        global::logger->critical("CUDA driver lib is not loaded!");
    typedef decltype(kfCuCtxCreate) FuncType;
    auto func = reinterpret_cast<FuncType*>(dlsym(libcuda, "cuCtxCreate_v2"));
    return func(pctx, flags, dev);
}

CUresult kfCuCtxDestroy(CUcontext ctx) {
    if (!libcuda)
        global::logger->critical("CUDA driver lib is not loaded!");
    typedef decltype(kfCuCtxDestroy) FuncType;
    auto func = reinterpret_cast<FuncType*>(dlsym(libcuda, "cuCtxDestroy_v2"));
    return func(ctx);
}

void kfCuDlClose() {
    if (libcuda)
        dlclose(libcuda);
}

}

