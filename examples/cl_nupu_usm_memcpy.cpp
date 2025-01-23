#include <CL/cl.h>
#include <CL/cl_ext_intel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cstdlib>

#define CHECK_CL_ERROR(X) \
    if (X != CL_SUCCESS) { \
        printf("OpenCL error %d at line %d\n", X, __LINE__); \
        return -1; \
    }

// OpenCL kernel using USM pointer
static const char* kernel_source = R"(
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

typedef cl_int (CL_API_CALL *clMemFreeINTEL_fn)(
    cl_context context,
    void* ptr);

typedef cl_int (CL_API_CALL *clSetKernelArgMemPointerINTEL_fn)(
    cl_kernel kernel,
    cl_uint arg_index,
    const void *arg_value);

typedef void* (CL_API_CALL *clHostMemAllocINTEL_fn)(
    cl_context context,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret);

typedef cl_int CL_API_CALL
clEnqueueMemcpyINTEL_t(
    cl_command_queue command_queue,
    cl_bool blocking,
    void* dst_ptr,
    const void* src_ptr,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

typedef clEnqueueMemcpyINTEL_t *
clEnqueueMemcpyINTEL_fn ;

typedef void* CL_API_CALL
clDeviceMemAllocINTEL_t(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret);

typedef clDeviceMemAllocINTEL_t *
clDeviceMemAllocINTEL_fn ;

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

    cl_device_id* devices = (cl_device_id*)malloc(num_devices * sizeof(cl_device_id));
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);
    CHECK_CL_ERROR(err);

    // Find RefSi M1 device
    cl_device_id refsi_device = NULL;
    for (cl_uint i = 0; i < num_devices; i++) {
        char device_name[1024];
        err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
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
    err = clGetDeviceInfo(refsi_device,
                         CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
                         sizeof(host_caps),
                         &host_caps,
                         NULL);

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

    char* extensions = (char*)malloc(ext_size);
    err = clGetDeviceInfo(refsi_device, CL_DEVICE_EXTENSIONS, ext_size, extensions, NULL);
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

    extensions = (char*)malloc(ext_size);
    err = clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, ext_size, extensions, NULL);
    CHECK_CL_ERROR(err);
    printf("Platform extensions: %s\n", extensions);
    free(extensions);

    // Use the found RefSi device
    cl_context context = clCreateContext(NULL, 1, &refsi_device, NULL, NULL, &err);
    CHECK_CL_ERROR(err);

    cl_command_queue queue = clCreateCommandQueue(context, refsi_device, 0, &err);
    CHECK_CL_ERROR(err);

    // Get USM function pointers
    printf("Getting USM function pointers...\n");

    // Get USM allocation function pointer
    clHostMemAllocINTEL_fn pfn_clHostMemAllocINTEL =
        (clHostMemAllocINTEL_fn)clGetExtensionFunctionAddressForPlatform(
            platform, "clHostMemAllocINTEL");
    printf("clHostMemAllocINTEL function pointer: %p\n", (void*)pfn_clHostMemAllocINTEL);
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
        (clSetKernelArgMemPointerINTEL_fn)clGetExtensionFunctionAddressForPlatform(
            platform, "clSetKernelArgMemPointerINTEL");
    if (!pfn_clSetKernelArgMemPointerINTEL) {
        printf("Failed to get clSetKernelArgMemPointerINTEL function pointer\n");
        return -1;
    }

    clDeviceMemAllocINTEL_fn pfn_clDeviceMemAllocINTEL =
        (clDeviceMemAllocINTEL_fn)clGetExtensionFunctionAddressForPlatform(
            platform, "clDeviceMemAllocINTEL");
    printf("clDeviceMemAllocINTEL function pointer: %p\n", (void*)pfn_clDeviceMemAllocINTEL);
    if (!pfn_clDeviceMemAllocINTEL) {
        printf("Failed to get clDeviceMemAllocINTEL function pointer\n");
        return -1;
    }

    clEnqueueMemcpyINTEL_fn pfn_clEnqueueMemcpyINTEL =
        (clEnqueueMemcpyINTEL_fn)clGetExtensionFunctionAddressForPlatform(
            platform, "clEnqueueMemcpyINTEL");
    printf("clEnqueueMemcpyINTEL function pointer: %p\n", (void*)pfn_clEnqueueMemcpyINTEL);
    if (!pfn_clEnqueueMemcpyINTEL) {
        printf("Failed to get clEnqueueMemcpyINTEL function pointer\n");
        return -1;
    }

    // Modify memory allocation part
    printf("Starting memory allocation...\n");
    printf("Context: %p\n", (void*)context);

    // Use static properties array
    cl_mem_properties_intel properties[] = {0};  // Empty properties list

    printf("Calling clHostMemAllocINTEL with:\n");
    printf("- context: %p\n", (void*)context);
    printf("- properties: %p\n", (void*)properties);
    printf("- size: %zu\n", ARRAY_SIZE * sizeof(int));
    printf("- alignment: 128\n");  // Use 128 byte alignment

    cl_int alloc_err = CL_SUCCESS;
    int* src1 = NULL;

    // Try using try-catch block to catch potential crashes
    src1 = (int *)malloc(ARRAY_SIZE * sizeof(int));
    if (!src1) {
        printf("Allocation returned NULL pointer\n");
        return -1;
    }
    printf("src1 allocation successful\n");

    printf("Allocating host memory for src2...\n");
    alloc_err = CL_SUCCESS;
    int* src2 = (int*)pfn_clHostMemAllocINTEL(context,
                                             NULL,
                                             ARRAY_SIZE * sizeof(int),
                                             ARRAY_SIZE * sizeof(int),
                                             &alloc_err);
    if (!src2 || alloc_err != CL_SUCCESS) {
        printf("Failed to allocate src2: error %d\n", alloc_err);
        return -1;
    }
    printf("src2 allocation successful\n");

    printf("Allocating host memory for dst...\n");
    alloc_err = CL_SUCCESS;
    int* dst = (int*)pfn_clHostMemAllocINTEL(context,
                                            NULL,
                                            ARRAY_SIZE * sizeof(int),
                                            ARRAY_SIZE * sizeof(int),
                                            &alloc_err);
    if (!dst || alloc_err != CL_SUCCESS) {
        printf("Failed to allocate dst: error %d\n", alloc_err);
        free(src1);
        pfn_clMemFreeINTEL(context, src2);
        return -1;
    }
    printf("dst allocation successful\n");

    // Initialize data - write directly, no memcpy needed
    for (int i = 0; i < ARRAY_SIZE; i++) {
        src1[i] = i;
        src2[i] = i * 2;
    }

    size_t bytes = ARRAY_SIZE * sizeof(int);

    int *d_src1 =
        (int *)pfn_clDeviceMemAllocINTEL(context, refsi_device, nullptr, bytes, 0, &err);
    CHECK_CL_ERROR(err);

    int *d_src2 =
        (int *)pfn_clDeviceMemAllocINTEL(context, refsi_device, nullptr, bytes, 0, &err);
    CHECK_CL_ERROR(err);

    int *d_dst =
        (int *)pfn_clDeviceMemAllocINTEL(context, refsi_device, nullptr, bytes, 0, &err);
    CHECK_CL_ERROR(err);

    CHECK_CL_ERROR(pfn_clEnqueueMemcpyINTEL(queue, 0, d_src1, src1, bytes, 0, nullptr, nullptr));
    CHECK_CL_ERROR(pfn_clEnqueueMemcpyINTEL(queue, 0, d_src2, src2, bytes, 0, nullptr, nullptr));

    // Create and compile kernel
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source,
                                                 NULL, &err);
    CHECK_CL_ERROR(err);

    err = clBuildProgram(program, 1, &refsi_device, NULL, NULL, NULL);
    CHECK_CL_ERROR(err);

    cl_kernel kernel = clCreateKernel(program, "vector_add", &err);
    CHECK_CL_ERROR(err);

    // Set kernel arguments
    err = pfn_clSetKernelArgMemPointerINTEL(kernel, 0, d_src1);
    CHECK_CL_ERROR(err);
    err = pfn_clSetKernelArgMemPointerINTEL(kernel, 1, d_src2);
    CHECK_CL_ERROR(err);
    err = pfn_clSetKernelArgMemPointerINTEL(kernel, 2, d_dst);
    CHECK_CL_ERROR(err);

    // Execute kernel
    size_t global_size = ARRAY_SIZE;
    size_t local_size = 64;  // Set workgroup size
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size,
                                0, NULL, NULL);
    CHECK_CL_ERROR(err);

    CHECK_CL_ERROR(pfn_clEnqueueMemcpyINTEL(queue, 0, dst, d_dst, bytes, 0, nullptr, nullptr));

    // Wait for completion
    err = clFinish(queue);
    CHECK_CL_ERROR(err);

    // Verify results
    int errors = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (dst[i] != i + i * 2) {
            printf("Error at index %d: %d != %d + %d\n",
                   i, dst[i], i, i * 2);
            errors++;
            if (errors > 10) break;
        }
    }

    if (errors == 0) {
        printf("USM memcpy test on NUPU device completed successfully!\n");
    }

    // Cleanup
    pfn_clMemFreeINTEL(context, d_src1);
    pfn_clMemFreeINTEL(context, d_src2);
    pfn_clMemFreeINTEL(context, d_dst);
    free(src1);
    pfn_clMemFreeINTEL(context, src2);
    pfn_clMemFreeINTEL(context, dst);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(devices);
    return errors ? -1 : 0;
}
