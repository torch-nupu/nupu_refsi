// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cassert>

#define CL_HPP_TARGET_OPENCL_VERSION 300
#include "CL/opencl.hpp"

namespace {
const size_t elem_size = 4;
const size_t buffer_size = elem_size * sizeof(float);
const std::vector<float> vec_in = {-1.0f, -2.0f, -3.0f, -4.0f};
std::vector<float> vec_out = {0.0f, 0.0f, 0.0f, 0.0f};
const size_t dim_in = {elem_size};

const char *kerenel_name = "absf";
const char *program_source = R"(
kernel void absf(global const float *in, global float *out, global int *dim) {
  int gid = get_global_id(0);
  if (gid < *dim) {
    out[gid] = in[gid] * -1;
  }
}
)";
}  // namespace

int main() {
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  if (platforms.empty()) {
    printf("No OpenCL platform found.\n");
    return 0;
  }
  cl::Platform platform = cl::Platform::setDefault(platforms.front());
  auto context = cl::Context::getDefault();
  std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
  if (devices.empty()) {
    printf("No OpenCL device found.\n");
    return 0;
  }
  const auto &device = devices.front();
  auto program = cl::Program(context, program_source, false);
  program.build(device);

  auto buffer_in = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              buffer_size, (void *)vec_in.data());
  auto buffer_out = cl::Buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                               buffer_size, (void *)vec_out.data());
  auto buffer_dim = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                               sizeof(size_t), (void *)&dim_in);

  cl::Kernel kernel(program, kerenel_name);
  kernel.setArg(0, buffer_in);
  kernel.setArg(1, buffer_out);
  kernel.setArg(2, buffer_dim);

  cl::CommandQueue queue(context, device);
  cl::Event event_enq;
  queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(elem_size),
                             cl::NullRange, NULL, &event_enq);
  cl::Event event_map;
  queue.enqueueMapBuffer(buffer_out, CL_TRUE, CL_MAP_READ, 0, buffer_size, NULL,
                         &event_map);
  cl::Event event_unmap;
  queue.enqueueUnmapMemObject(buffer_out, (void *)vec_out.data(), NULL,
                              &event_unmap);
  queue.finish();

  // check results
  for (size_t i = 0; i < vec_in.size(); ++i) {
    assert(!(vec_in[i] + vec_out[i]));
  }

  return 0;
}
