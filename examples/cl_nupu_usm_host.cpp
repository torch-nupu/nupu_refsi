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
        int val1 = src1[gid];  // Read step by step
        int val2 = src2[gid];
        int result = val1 + val2;  // Calculate step by step
        dst[gid] = result;  // Write step by step
    }
}
)";

#define ARRAY_SIZE 1024

typedef cl_int(CL_API_CALL *clMemFreeINTEL_fn)(cl_context context, void *ptr);

typedef cl_int(CL_API_CALL *clSetKernelArgMemPointerINTEL_fn)(
    cl_kernel kernel, cl_uint arg_index, const void *arg_value);

typedef void *(CL_API_CALL *clHostMemAllocINTEL_fn)(
    cl_context context, const cl_mem_properties_intel *properties, size_t size,
    cl_uint alignment, cl_int *errcode_ret);

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

  // Check host allocation capabilities
  cl_device_unified_shared_memory_capabilities_intel host_caps;
  err = clGetDeviceInfo(refsi_device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
                        sizeof(host_caps), &host_caps, NULL);

  printf("Host memory capabilities: 0x%lx\n", (unsigned long)host_caps);

  // Check if host allocation is supported
  if (!(host_caps & (CL_UNIFIED_SHARED_MEMORY_ACCESS_INTEL |
                     CL_UNIFIED_SHARED_MEMORY_ATOMIC_ACCESS_INTEL))) {
    printf("Device does not support USM host allocations\n");
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

  // Get USM function pointers
  printf("Getting USM function pointers...\n");

  // Get USM allocation function pointer
  clHostMemAllocINTEL_fn pfn_clHostMemAllocINTEL =
      (clHostMemAllocINTEL_fn)clGetExtensionFunctionAddressForPlatform(
          platform, "clHostMemAllocINTEL");
  printf("clHostMemAllocINTEL function pointer: %p\n",
         (void *)pfn_clHostMemAllocINTEL);
  if (!pfn_clHostMemAllocINTEL) {
    printf("Failed to get clHostMemAllocINTEL function pointer\n");
    return -1;
  }

  // Get USM free function pointer
  clMemFreeINTEL_fn pfn_clMemFreeINTEL =
      (clMemFreeINTEL_fn)clGetExtensionFunctionAddressForPlatform(
          platform, "clMemFreeINTEL");
  if (!pfn_clMemFreeINTEL) {
    printf("Failed to get clMemFreeINTEL function pointer\n");
    return -1;
  }

  // Get USM kernel argument setting function pointer
  clSetKernelArgMemPointerINTEL_fn pfn_clSetKernelArgMemPointerINTEL =
      (clSetKernelArgMemPointerINTEL_fn)
          clGetExtensionFunctionAddressForPlatform(
              platform, "clSetKernelArgMemPointerINTEL");
  if (!pfn_clSetKernelArgMemPointerINTEL) {
    printf("Failed to get clSetKernelArgMemPointerINTEL function pointer\n");
    return -1;
  }

  // Modify memory allocation part
  printf("Starting memory allocation...\n");
  printf("Context: %p\n", (void *)context);

  // Use static properties array
  cl_mem_properties_intel properties[] = {0};  // Empty properties list

  printf("Calling clHostMemAllocINTEL with:\n");
  printf("- context: %p\n", (void *)context);
  printf("- properties: %p\n", (void *)properties);
  printf("- size: %zu\n", ARRAY_SIZE * sizeof(int));
  printf("- alignment: 128\n");  // Use 128 byte alignment

  cl_int alloc_err = CL_SUCCESS;
  int *src1 = NULL;

  // Try using try-catch block to catch potential crashes
  src1 =
      (int *)pfn_clHostMemAllocINTEL(context,
                                     properties,  // Use empty properties list
                                     ARRAY_SIZE * sizeof(int),
                                     128,  // Use 128 byte alignment
                                     &alloc_err);

  printf("clHostMemAllocINTEL returned: %d\n", alloc_err);
  if (alloc_err != CL_SUCCESS) {
    printf("Failed to allocate src1: error %d\n", alloc_err);
    return -1;
  }
  if (!src1) {
    printf("Allocation returned NULL pointer\n");
    return -1;
  }
  printf("src1 allocation successful\n");

  printf("Allocating host memory for src2...\n");
  alloc_err = CL_SUCCESS;
  int *src2 = (int *)pfn_clHostMemAllocINTEL(
      context, NULL, ARRAY_SIZE * sizeof(int), 0, &alloc_err);
  if (!src2 || alloc_err != CL_SUCCESS) {
    printf("Failed to allocate src2: error %d\n", alloc_err);
    pfn_clMemFreeINTEL(context, src1);
    return -1;
  }
  printf("src2 allocation successful\n");

  printf("Allocating host memory for dst...\n");
  alloc_err = CL_SUCCESS;
  int *dst = (int *)pfn_clHostMemAllocINTEL(
      context, NULL, ARRAY_SIZE * sizeof(int), 0, &alloc_err);
  if (!dst || alloc_err != CL_SUCCESS) {
    printf("Failed to allocate dst: error %d\n", alloc_err);
    pfn_clMemFreeINTEL(context, src1);
    pfn_clMemFreeINTEL(context, src2);
    return -1;
  }
  printf("dst allocation successful\n");

  // Initialize data - write directly, no memcpy needed
  for (int i = 0; i < ARRAY_SIZE; i++) {
    src1[i] = i;
    src2[i] = i * 2;
  }

  // Add memory verification before kernel execution
  printf("Verifying memory access...\n");

  // Verify src1
  printf("Testing src1 access:\n");
  for (int i = 0; i < 10; i++) {
    src1[i] = i;
    printf("src1[%d] = %d\n", i, src1[i]);
  }

  // Verify src2
  printf("Testing src2 access:\n");
  for (int i = 0; i < 10; i++) {
    src2[i] = i * 2;
    printf("src2[%d] = %d\n", i, src2[i]);
  }

  // Verify dst
  printf("Testing dst access:\n");
  for (int i = 0; i < 10; i++) {
    dst[i] = 0;
    printf("dst[%d] = %d\n", i, dst[i]);
  }
  // Print three pointers
  printf("src1: %p\n", (void *)src1);
  printf("src2: %p\n", (void *)src2);
  printf("dst: %p\n", (void *)dst);

  // Add memory synchronization
  err = clFinish(queue);
  CHECK_CL_ERROR(err);

  printf("Memory verification complete\n");

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
  size_t local_size = 64;  // Set workgroup size
  err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size,
                               &local_size, 0, NULL, NULL);
  CHECK_CL_ERROR(err);

  // Wait for completion
  err = clFinish(queue);
  CHECK_CL_ERROR(err);

  // Verify results
  int errors = 0;
  for (int i = 0; i < ARRAY_SIZE; i++) {
    if (dst[i] != i + i * 2) {
      printf("Error at index %d: %d != %d + %d\n", i, dst[i], i, i * 2);
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
