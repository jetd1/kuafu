//
// Created by jet on 9/17/21.
//

#ifdef KUAFU_OPTIX_DENOISER

#include <cuda.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace kuafu {

void *libcuda = nullptr;

static std::optional<std::string> findFile(std::vector<std::string>const&paths, std::string_view filename) {
    for (auto path: paths) {
        auto filepath = std::filesystem::path(path) / filename;
        if (std::filesystem::exists(filepath)) {
            return filepath.string();
        }
    }
    return {};
}

void kfCuDlOpen() {
    auto file = findFile({"/usr/lib/x86_64-linux-gnu", "/usr/lib", "/usr/local/lib"}, "libcuda.so");
    if (file) {
        libcuda = dlopen(file.value().c_str(), RTLD_NOW);
    }
    if (!libcuda)
        global::logger->critical("Failed to load CUDA driver lib for the denoiser: {}."
                                 "Hint: have you installed the latest NVIDIA driver?",
                                 dlerror());
  dlerror();
}

CUresult kfCuInit(unsigned int Flags) {
  if (!libcuda)
    global::logger->critical("CUDA driver lib is not loaded!");
  typedef decltype(kfCuInit) FuncType;
  try {
    auto func = reinterpret_cast<FuncType *>(dlsym(libcuda, "cuInit"));
    return func(Flags);
  } catch (std::exception &e) {
    throw std::runtime_error(std::string("kfCuInit failed: ") + e.what());
  }
}

CUresult kfCuStreamCreate(CUstream *phStream, unsigned int Flags) {
  if (!libcuda)
    global::logger->critical("CUDA driver lib is not loaded!");
  typedef decltype(kfCuStreamCreate) FuncType;
  try {
    auto func = reinterpret_cast<FuncType *>(dlsym(libcuda, "cuStreamCreate"));
    return func(phStream, Flags);
  } catch (std::exception &e) {
    throw std::runtime_error(std::string("kfCuStreamCreate failed: ") + e.what());
  }
}

CUresult kfCuCtxCreate(CUcontext *pctx, unsigned int flags, CUdevice dev) {
  if (!libcuda)
    global::logger->critical("CUDA driver lib is not loaded!");
  typedef decltype(kfCuCtxCreate) FuncType;
  try {
    auto func = reinterpret_cast<FuncType *>(dlsym(libcuda, "cuCtxCreate_v2"));
    return func(pctx, flags, dev);
  } catch (std::exception &e) {
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
  } catch (std::exception &e) {
    throw std::runtime_error(std::string("kfCuCtxDestroy failed: ") + e.what());
  }
}

void kfCuDlClose() {
  if (libcuda)
    dlclose(libcuda);
}

} // namespace kuafu

#endif
