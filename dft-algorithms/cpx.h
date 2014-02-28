#ifndef DFTALGORITHMS_CPX_H
#define DFTALGORITHMS_CPX_H

#include "clhelpers/cl.h"

#include <complex>

using namespace std;

typedef complex<float> cpx;

// dimensione dei dati sul device
template <typename HostType> static cl_uint cl_deviceDataSize(int N = 1);
template <> cl_uint cl_deviceDataSize<float>(int N) { return N * sizeof(cl_float); }
template <> cl_uint cl_deviceDataSize<cpx>(int N) { return N * sizeof(cl_float2); }

// channel order per texture
template <typename HostType> static cl_channel_order cl_channelOrder();
template <> cl_channel_order cl_channelOrder<float>() { return CL_R; }
template <> cl_channel_order cl_channelOrder<cpx>() { return CL_RG; }

#endif // DFTALGORITHMS_CPX_H
