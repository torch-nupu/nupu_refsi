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

#include <CL/cl.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#define CL_CHECK(ERROR)                            \
  if (ERROR) {                                     \
    std::cerr << "OpenCL error: " << ERROR << "\n" \
              << "at line: " << __LINE__ << "\n";  \
    std::exit(1);                                  \
  }

#define IS_CL_SUCCESS(X)                                                   \
  {                                                                        \
    const cl_int ret_val = X;                                              \
    if (CL_SUCCESS != ret_val) {                                           \
      (void)fprintf(stderr, "OpenCL error occurred: %s returned %d\n", #X, \
                    ret_val);                                              \
      exit(1);                                                             \
    }                                                                      \
  }

/// @brief Print help message on executable usage
///
/// @param arg0 Name of executable
void printUsage(const char *arg0) {
  printf("usage: %s [-h] [--platform <name>] [--device <name>]\n", arg0);
}

/// @brief Parse executable arguments for platform and device name
///
/// Found platform and device names are returned as C-string output
/// parameters. If --help / -h is passed as an argument the help
/// message is printed and the application exits with success.
///
/// @param[in] argc Number of arguments propagated from main()
/// @param[in] argv Command-line arguments propagated from main()
/// @param[out] platform_name Platform name found from --platform argument
/// @param[out] device_name Device name found from --device argument
void parseArguments(const int argc, const char **argv,
                    const char **platform_name, const char **device_name) {
  for (int argi = 1; argi < argc; argi++) {
    if (0 == strcmp("-h", argv[argi]) || 0 == strcmp("--help", argv[argi])) {
      printUsage(argv[0]);
      exit(0);
    } else if (0 == strcmp("--platform", argv[argi])) {
      argi++;
      if (argi == argc) {
        printUsage(argv[0]);
        (void)fprintf(stderr, "expected platform name\n");
        exit(1);
      }
      *platform_name = argv[argi];
    } else if (0 == strcmp("--device", argv[argi])) {
      argi++;
      if (argi == argc) {
        printUsage(argv[0]);
        (void)fprintf(stderr, "error: expected device name\n");
        exit(1);
      }
      *device_name = argv[argi];
    } else {
      printUsage(argv[0]);
      (void)fprintf(stderr, "error: invalid argument: %s\n", argv[argi]);
      exit(1);
    }
  }
}

/// @brief Select the OpenCL platform
///
/// If a platform name string is passed on the command-line this is used to
/// select the platform, otherwise if only one platform exists this is chosen.
/// If neither of these cases apply the user is asked which platform to use.
///
/// @param platform_name_arg String of platform name passed on command-line
///
/// @return OpenCL platform selected
cl_platform_id selectPlatform(const char *platform_name_arg) {
  cl_uint num_platforms;
  IS_CL_SUCCESS(clGetPlatformIDs(0, NULL, &num_platforms));

  if (0 == num_platforms) {
    (void)fprintf(stderr, "No OpenCL platforms found, exiting\n");
    exit(1);
  }

  cl_platform_id *platforms =
      (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
  if (NULL == platforms) {
    (void)fprintf(stderr, "\nCould not allocate memory for platform ids\n");
    exit(1);
  }
  IS_CL_SUCCESS(clGetPlatformIDs(num_platforms, platforms, NULL));

  printf("Available platforms are:\n");

  unsigned selected_platform = 0;
  for (cl_uint i = 0; i < num_platforms; ++i) {
    size_t platform_name_size;
    IS_CL_SUCCESS(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL,
                                    &platform_name_size));

    if (0 == platform_name_size) {
      printf("  %u. Nameless platform\n", i + 1);
    } else {
      char *platform_name = (char *)malloc(platform_name_size);
      if (NULL == platform_name) {
        (void)fprintf(stderr,
                      "\nCould not allocate memory for platform name\n");
        exit(1);
      }
      IS_CL_SUCCESS(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME,
                                      platform_name_size, platform_name, NULL));
      printf("  %u. %s\n", i + 1, platform_name);
      if (platform_name_arg && 0 == strcmp(platform_name, platform_name_arg)) {
        selected_platform = i + 1;
      }
      free(platform_name);
    }
  }

  if (platform_name_arg != NULL && selected_platform == 0) {
    (void)fprintf(stderr, "Platform name matching '--platform %s' not found\n",
                  platform_name_arg);
    exit(1);
  }

  if (1 == num_platforms) {
    printf("\nSelected platform 1\n");
    selected_platform = 1;
  } else if (0 != selected_platform) {
    printf("\nSelected platform %u by '--platform %s'\n", selected_platform,
           platform_name_arg);
  } else {
    printf("\nPlease select a platform: ");
    if (1 != scanf("%u", &selected_platform)) {
      (void)fprintf(stderr, "\nCould not parse provided input, exiting\n");
      exit(1);
    }
  }

  selected_platform -= 1;

  if (num_platforms <= selected_platform) {
    (void)fprintf(stderr, "\nSelected unknown platform, exiting\n");
    exit(1);
  } else {
    printf("\nRunning example on platform %u\n", selected_platform + 1);
  }

  cl_platform_id selected_platform_id = platforms[selected_platform];
  free((void *)platforms);
  return selected_platform_id;
}

/// @brief Select the OpenCL device
///
/// If a device name string is passed on the command-line this is used to
/// select the device in the platform, otherwise if only one device exists in
/// the platform this is chosen. If neither of these cases apply the user is
/// asked which device to use from the platform.
///
/// @param selected_platform OpenCL platform to use
/// @param device_name_arg String of device name passed on command-line
///
/// @return OpenCL device selected
cl_device_id selectDevice(cl_platform_id selected_platform,
                          const char *device_name_arg) {
  cl_uint num_devices;

  IS_CL_SUCCESS(clGetDeviceIDs(selected_platform, CL_DEVICE_TYPE_ALL, 0, NULL,
                               &num_devices));

  if (0 == num_devices) {
    (void)fprintf(stderr, "No OpenCL devices found, exiting\n");
    exit(1);
  }

  cl_device_id *devices =
      (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices);
  if (NULL == devices) {
    (void)fprintf(stderr, "\nCould not allocate memory for device ids\n");
    exit(1);
  }
  IS_CL_SUCCESS(clGetDeviceIDs(selected_platform, CL_DEVICE_TYPE_ALL,
                               num_devices, devices, NULL));

  printf("Available devices are:\n");

  unsigned selected_device = 0;
  for (cl_uint i = 0; i < num_devices; ++i) {
    size_t device_name_size;
    IS_CL_SUCCESS(clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL,
                                  &device_name_size));

    if (0 == device_name_size) {
      printf("  %u. Nameless device\n", i + 1);
    } else {
      char *device_name = (char *)malloc(device_name_size);
      if (NULL == device_name) {
        (void)fprintf(stderr, "\nCould not allocate memory for device name\n");
        exit(1);
      }
      IS_CL_SUCCESS(clGetDeviceInfo(devices[i], CL_DEVICE_NAME,
                                    device_name_size, device_name, NULL));
      printf("  %u. %s\n", i + 1, device_name);
      if (device_name_arg && 0 == strcmp(device_name, device_name_arg)) {
        selected_device = i + 1;
      }
      free(device_name);
    }
  }

  if (device_name_arg != NULL && selected_device == 0) {
    (void)fprintf(stderr, "Device name matching '--device %s' not found\n",
                  device_name_arg);
    exit(1);
  }

  if (1 == num_devices) {
    printf("\nSelected device 1\n");
    selected_device = 1;
  } else if (0 != selected_device) {
    printf("\nSelected device %u by '--device %s'\n", selected_device,
           device_name_arg);
  } else {
    printf("\nPlease select a device: ");
    if (1 != scanf("%u", &selected_device)) {
      (void)fprintf(stderr, "\nCould not parse provided input, exiting\n");
      exit(1);
    }
  }

  selected_device -= 1;

  if (num_devices <= selected_device) {
    (void)fprintf(stderr, "\nSelected unknown device, exiting\n");
    exit(1);
  } else {
    printf("\nRunning example on device %u\n", selected_device + 1);
  }

  cl_device_id selected_device_id = devices[selected_device];

  cl_bool device_compiler_available;
  IS_CL_SUCCESS(clGetDeviceInfo(selected_device_id,
                                CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool),
                                &device_compiler_available, NULL));
  if (!device_compiler_available) {
    printf("compiler not available for selected device, skipping example.\n");
    exit(0);
  }

  free((void *)devices);
  return selected_device_id;
}


int main(const int argc, const char **argv) {
  const char *platform_name = NULL;
  const char *device_name = NULL;
  parseArguments(argc, argv, &platform_name, &device_name);

  cl_platform_id platform = selectPlatform(platform_name);
  cl_device_id device = selectDevice(platform, device_name);

  // Sub-groups were introduced in 2.X.
  size_t device_version_length = 0;
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_VERSION, 0, nullptr,
                           &device_version_length));
  std::string device_version(device_version_length, '\0');
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_VERSION, device_version_length,
                           device_version.data(), nullptr));

  // The device version string must be of the form
  // OpenCL<space><major_version.minor_version><space><vendor-specific
  // information>.
  auto dot_position = device_version.find('.');
  auto major_version = device_version[dot_position - 1] - '0';

  // Skip the example if the OpenCL driver is early than 2.X since sub-groups
  // didn't exist.
  if (major_version < 2) {
    std::cerr << "Sub-groups are not an OpenCL feature before OpenCL 2.0, "
                 "skipping sub-group example.\n";
    return 0;
  }

  // Sub-groups were made optional in OpenCL 3.0, so check that they are
  // supported if we have a 3.0 driver or later.
  if (major_version >= 3) {
    cl_uint max_num_sub_groups = 0;
    CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_MAX_NUM_SUB_GROUPS,
                             sizeof(cl_uint), &max_num_sub_groups, nullptr));
    if (0 == max_num_sub_groups) {
      std::cerr << "Sub-groups are not supported on this device, "
                   "skipping sub-group example.\n";
      return 0;
    }
  }

  // A compiler is required to compile the example kernel, if there isn't one
  // skip.
  cl_bool device_compiler_available;
  CL_CHECK(clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE,
                           sizeof(cl_bool), &device_compiler_available,
                           nullptr));
  if (!device_compiler_available) {
    std::cerr << "compiler not available for the device, skipping sub-group"
                 "example.\n";
    return 0;
  }

  auto error = CL_SUCCESS;
  auto context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
  CL_CHECK(error);

  const char *code = R"OPENCLC(
kernel void reduction(global int *in, local int *tmp, global int *out) {
  const size_t gid = get_global_id(0);
  const size_t lid = get_local_id(0);
  const size_t wgid = get_group_id(0);
  const size_t sgid = get_sub_group_id();
  const size_t sg_count = get_num_sub_groups();

  int partial_reduction = sub_group_reduce_add(in[gid]);
  tmp[sgid] = partial_reduction;

  barrier(CLK_LOCAL_MEM_FENCE);

  for (unsigned i = sg_count / 2; i != 0; i /= 2) {
    if (lid < i) {
      tmp[lid] = tmp[lid] + tmp[lid + i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  if (lid == 0) {
    out[wgid] = *tmp;
  }
}
)OPENCLC";
  const auto code_length = std::strlen(code);

  auto program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  CL_CHECK(error);

  CL_CHECK(
      clBuildProgram(program, 1, &device,
                     major_version == 2 ? "-cl-std=CL2.0" : "-cl-std=CL3.0",
                     nullptr, nullptr));

  auto kernel = clCreateKernel(program, "reduction", &error);
  CL_CHECK(error);

  constexpr size_t global_size = 1024;
  constexpr size_t local_size = 32;
  constexpr size_t work_group_count = global_size / local_size;
  size_t sub_group_count = 0;
  CL_CHECK(clGetKernelSubGroupInfo(
      kernel, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE, sizeof(size_t),
      &local_size, sizeof(size_t), &sub_group_count, nullptr));
  std::cout << "Sub-group count for local size (" << local_size
            << ", 1, 1): " << sub_group_count << '\n';
  constexpr size_t input_buffer_size = global_size * sizeof(cl_int);
  constexpr size_t output_buffer_size = work_group_count * sizeof(cl_int);
  const size_t local_buffer_size = sub_group_count * sizeof(cl_int);

  std::random_device random_device;
  std::mt19937 random_engine{random_device()};
  std::uniform_int_distribution<cl_int> random_distribution{
      std::numeric_limits<cl_int>::min() / static_cast<cl_int>(global_size),
      std::numeric_limits<cl_int>::max() / static_cast<cl_int>(global_size)};
  std::vector<cl_int> input_data(global_size);
  std::generate(std::begin(input_data), std::end(input_data),
                [&]() { return random_distribution(random_engine); });

  auto input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                     input_buffer_size, nullptr, &error);
  CL_CHECK(error);

  auto output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                      output_buffer_size, nullptr, &error);
  CL_CHECK(error);

  auto command_queue = clCreateCommandQueue(context, device, 0, &error);
  CL_CHECK(error);

  CL_CHECK(clEnqueueWriteBuffer(command_queue, input_buffer, CL_FALSE, 0,
                                input_buffer_size, input_data.data(), 0,
                                nullptr, nullptr));

  CL_CHECK(clSetKernelArg(kernel, 0, sizeof(input_buffer),
                          static_cast<void *>(&input_buffer)));
  CL_CHECK(clSetKernelArg(kernel, 1, local_buffer_size, nullptr));
  CL_CHECK(clSetKernelArg(kernel, 2, sizeof(output_buffer),
                          static_cast<void *>(&output_buffer)));

  CL_CHECK(clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr,
                                  &global_size, &local_size, 0, nullptr,
                                  nullptr));
  std::vector<cl_int> output_data(work_group_count, 0);
  CL_CHECK(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                               output_buffer_size, output_data.data(), 0,
                               nullptr, nullptr));

  auto result =
      std::accumulate(std::begin(output_data), std::end(output_data), 0);
  auto expected =
      std::accumulate(std::begin(input_data), std::end(input_data), 0);

  CL_CHECK(clReleaseCommandQueue(command_queue));
  CL_CHECK(clReleaseMemObject(input_buffer));
  CL_CHECK(clReleaseMemObject(output_buffer));
  CL_CHECK(clReleaseKernel(kernel));
  CL_CHECK(clReleaseProgram(program));
  CL_CHECK(clReleaseContext(context));

  if (result != expected) {
    std::cerr << "Result did not validate, expected: " << expected
              << " but got: " << result << " exiting...\n";
    std::exit(1);
  }

  std::cout
      << "Result validated, sub-groups example ran successfully, exiting...\n";
  return 0;
}
