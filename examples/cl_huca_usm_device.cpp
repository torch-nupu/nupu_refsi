#include <CL/cl.h>
#include <CL/cl_ext_intel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstdlib>

#define CHECK_CL_ERROR(X)                                \
  if (X != CL_SUCCESS) {                                 \
    printf("OpenCL error %d at line %d\n", X, __LINE__); \
    return -1;                                           \
  }

// OpenCL kernel using USM pointer
static const char *kernel_source = R"(
__kernel void vector_add(__global int* src1, __global int* src2, __global int* dst) {
    size_t gid = get_global_id(0);
    if (gid < 1024) {  // Add boundary check
        dst[gid] = src1[gid] + src2[gid];
    }
}
)";

#define ARRAY_SIZE 1024

// Add function pointer type definitions
typedef void *(CL_API_CALL *clDeviceMemAllocINTEL_fn)(
    cl_context context, cl_device_id device,
    const cl_mem_properties_intel *properties, size_t size, cl_uint alignment,
    cl_int *errcode_ret);

typedef cl_int(CL_API_CALL *clMemFreeINTEL_fn)(cl_context context, void *ptr);

typedef cl_int(CL_API_CALL *clEnqueueMemcpyINTEL_fn)(
    cl_command_queue queue, cl_bool blocking, void *dst_ptr,
    const void *src_ptr, size_t size, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event);

typedef cl_int(CL_API_CALL *clSetKernelArgMemPointerINTEL_fn)(
    cl_kernel kernel, cl_uint arg_index, const void *arg_value);

int main() {
  cl_int err;

  // Get platform and device
  cl_platform_id platform;
  cl_uint num_platforms;
  err = clGetPlatformIDs(1, &platform, &num_platforms);
  CHECK_CL_ERROR(err);

  // Get all devices
  cl_uint num_devices;
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
  CHECK_CL_ERROR(err);

  cl_device_id *devices =
      (cl_device_id *)malloc(num_devices * sizeof(cl_device_id));
  err =
      clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);
  CHECK_CL_ERROR(err);

  // Find RefSi M1 device
  cl_device_id refsi_device = NULL;
  for (cl_uint i = 0; i < num_devices; i++) {
    char device_name[1024];
    err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(device_name),
                          device_name, NULL);
    CHECK_CL_ERROR(err);
    printf("Found device: %s\n", device_name);

    if (strstr(device_name, "RefSi M1") != NULL) {
      refsi_device = devices[i];
      printf("Selected device: %s\n", device_name);
      break;
    }
  }

  if (!refsi_device) {
    printf("RefSi M1 device not found!\n");
    free(devices);
    return -1;
  }

  // Check if device supports USM
  size_t ext_size;
  err = clGetDeviceInfo(refsi_device, CL_DEVICE_EXTENSIONS, 0, NULL, &ext_size);
  CHECK_CL_ERROR(err);

  char *extensions = (char *)malloc(ext_size);
  err = clGetDeviceInfo(refsi_device, CL_DEVICE_EXTENSIONS, ext_size,
                        extensions, NULL);
  CHECK_CL_ERROR(err);
  printf("Device extensions: %s\n", extensions);

  if (strstr(extensions, "cl_intel_unified_shared_memory") == NULL) {
    printf("Device does not support USM!\n");
    free(extensions);
    free(devices);
    return -1;
  }
  free(extensions);

  // Check platform extensions
  err = clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 0, NULL, &ext_size);
  CHECK_CL_ERROR(err);

  extensions = (char *)malloc(ext_size);
  err = clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, ext_size,
                          extensions, NULL);
  CHECK_CL_ERROR(err);
  printf("Platform extensions: %s\n", extensions);
  free(extensions);

  // Use the found RefSi device
  cl_context context =
      clCreateContext(NULL, 1, &refsi_device, NULL, NULL, &err);
  CHECK_CL_ERROR(err);

  cl_command_queue queue = clCreateCommandQueue(context, refsi_device, 0, &err);
  CHECK_CL_ERROR(err);

  // Get all required function pointers
  clDeviceMemAllocINTEL_fn pfn_clDeviceMemAllocINTEL =
      (clDeviceMemAllocINTEL_fn)clGetExtensionFunctionAddressForPlatform(
          platform, "clDeviceMemAllocINTEL");
  if (!pfn_clDeviceMemAllocINTEL) {
    printf("Failed to get clDeviceMemAllocINTEL function\n");
    return -1;
  }

  clMemFreeINTEL_fn pfn_clMemFreeINTEL =
      (clMemFreeINTEL_fn)clGetExtensionFunctionAddressForPlatform(
          platform, "clMemFreeINTEL");
  if (!pfn_clMemFreeINTEL) {
    printf("Failed to get clMemFreeINTEL function\n");
    return -1;
  }

  clEnqueueMemcpyINTEL_fn pfn_clEnqueueMemcpyINTEL =
      (clEnqueueMemcpyINTEL_fn)clGetExtensionFunctionAddressForPlatform(
          platform, "clEnqueueMemcpyINTEL");
  if (!pfn_clEnqueueMemcpyINTEL) {
    printf("Failed to get clEnqueueMemcpyINTEL function\n");
    return -1;
  }

  clSetKernelArgMemPointerINTEL_fn pfn_clSetKernelArgMemPointerINTEL =
      (clSetKernelArgMemPointerINTEL_fn)
          clGetExtensionFunctionAddressForPlatform(
              platform, "clSetKernelArgMemPointerINTEL");
  if (!pfn_clSetKernelArgMemPointerINTEL) {
    printf("Failed to get clSetKernelArgMemPointerINTEL function\n");
    return -1;
  }

  // Modify memory allocation method
  printf("Starting memory allocation...\n");

  // Simplify memory allocation properties
  cl_mem_properties_intel *properties =
      nullptr;  // Try without special properties first

  // Use device memory allocation uniformly
  printf("Allocating device memory for src1...\n");
  int *src1 = (int *)pfn_clDeviceMemAllocINTEL(
      context, refsi_device, properties, ARRAY_SIZE * sizeof(int),
      128,  // Use 128 byte alignment
      &err);
  if (!src1 || err != CL_SUCCESS) {
    printf("Failed to allocate src1: error %d\n", err);
    return -1;
  }
  printf("src1 allocation successful\n");

  printf("Allocating device memory for src2...\n");
  int *src2 = (int *)pfn_clDeviceMemAllocINTEL(
      context, refsi_device, properties, ARRAY_SIZE * sizeof(int), 128, &err);
  if (!src2 || err != CL_SUCCESS) {
    printf("Failed to allocate src2: error %d\n", err);
    pfn_clMemFreeINTEL(context, src1);
    return -1;
  }
  printf("src2 allocation successful\n");

  printf("Allocating device memory for dst...\n");
  int *dst = (int *)pfn_clDeviceMemAllocINTEL(
      context, refsi_device, properties, ARRAY_SIZE * sizeof(int), 128, &err);
  if (!dst || err != CL_SUCCESS) {
    printf("Failed to allocate dst: error %d\n", err);
    pfn_clMemFreeINTEL(context, src1);
    pfn_clMemFreeINTEL(context, src2);
    return -1;
  }
  printf("dst allocation successful\n");

  // Initialize data
  int host_src1[ARRAY_SIZE];
  int host_src2[ARRAY_SIZE];
  for (int i = 0; i < ARRAY_SIZE; i++) {
    host_src1[i] = i;
    host_src2[i] = i * 2;
  }

  // Copy data to device
  err = pfn_clEnqueueMemcpyINTEL(queue, CL_TRUE, src1, host_src1,
                                 ARRAY_SIZE * sizeof(int), 0, NULL, NULL);
  CHECK_CL_ERROR(err);

  err = pfn_clEnqueueMemcpyINTEL(queue, CL_TRUE, src2, host_src2,
                                 ARRAY_SIZE * sizeof(int), 0, NULL, NULL);
  CHECK_CL_ERROR(err);

  // Create and compile kernel
  cl_program program =
      clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
  CHECK_CL_ERROR(err);

  err = clBuildProgram(program, 1, &refsi_device, NULL, NULL, NULL);
  CHECK_CL_ERROR(err);

  cl_kernel kernel = clCreateKernel(program, "vector_add", &err);
  CHECK_CL_ERROR(err);

  // Set kernel arguments
  err = pfn_clSetKernelArgMemPointerINTEL(kernel, 0, src1);
  CHECK_CL_ERROR(err);
  err = pfn_clSetKernelArgMemPointerINTEL(kernel, 1, src2);
  CHECK_CL_ERROR(err);
  err = pfn_clSetKernelArgMemPointerINTEL(kernel, 2, dst);
  CHECK_CL_ERROR(err);

  // Execute kernel
  size_t global_size = ARRAY_SIZE;
  size_t local_size = 64;  // Set work group size
  err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size,
                               &local_size, 0, NULL, NULL);
  CHECK_CL_ERROR(err);

  // Wait for completion
  err = clFinish(queue);
  CHECK_CL_ERROR(err);

  // Read back results
  int host_dst[ARRAY_SIZE];
  err = pfn_clEnqueueMemcpyINTEL(queue, CL_TRUE, host_dst, dst,
                                 ARRAY_SIZE * sizeof(int), 0, NULL, NULL);
  CHECK_CL_ERROR(err);

  // Verify results
  int errors = 0;
  for (int i = 0; i < ARRAY_SIZE; i++) {
    if (host_dst[i] != i + i * 2) {  // Verify calculation results
      printf("Error at index %d: %d != %d + %d\n", i, host_dst[i], i, i * 2);
      errors++;
      if (errors > 10) break;
    }
  }

  if (errors == 0) {
    printf("USM test on NUPU device completed successfully!\n");
  }

  // Cleanup
  pfn_clMemFreeINTEL(context, src1);
  pfn_clMemFreeINTEL(context, src2);
  pfn_clMemFreeINTEL(context, dst);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  free(devices);
  return errors ? -1 : 0;
}
