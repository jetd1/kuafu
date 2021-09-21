//
// Created by jet on 9/17/21.
//

#pragma once
#include <cuda.h>

namespace kuafu {

void kfCuDlOpen();
void kfCuDlClose();

CUresult kfCuInit(unsigned int Flags);
CUresult kfCuStreamCreate(CUstream* phStream, unsigned int Flags);
CUresult kfCuCtxCreate(CUcontext* pctx, unsigned int flags, CUdevice dev);
CUresult kfCuCtxDestroy(CUcontext ctx);

}
