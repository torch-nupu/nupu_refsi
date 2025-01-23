include_directories("${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-CLHPP/include")
add_compile_options("-Wno-error=cast-qual")

add_ca_cl_executable(cl_nupu_vector_add_c
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_vector_add.c)
add_ca_cl_executable(cl_nupu_vector_add_2d_c
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_vector_add_2d.c)
add_ca_cl_executable(cl_nupu_nop ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_nop.c)
add_ca_cl_executable(cl_nupu_multi_devices
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_multi_devices.c)
add_ca_cl_executable(cl_nupu_control_flow
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_control_flow.c)
add_ca_cl_executable(cl_nupu_reduce
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_reduce.cpp)

add_ca_cl_executable(cl_nupu_usm_device ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_usm_device.cpp)
add_ca_cl_executable(cl_nupu_usm_host ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_usm_host.cpp)
add_ca_cl_executable(cl_nupu_absf
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_absf.cpp)
add_ca_cl_executable(cl_nupu_usm_memcpy ${CMAKE_CURRENT_SOURCE_DIR}/cl_nupu_usm_memcpy.cpp)

install(TARGETS cl_nupu_vector_add_c RUNTIME DESTINATION bin)
install(TARGETS cl_nupu_vector_add_2d_c RUNTIME DESTINATION bin)
install(TARGETS cl_nupu_nop RUNTIME DESTINATION bin)
install(TARGETS cl_nupu_multi_devices RUNTIME DESTINATION bin)

install(TARGETS cl_nupu_usm_device RUNTIME DESTINATION bin)
install(TARGETS cl_nupu_usm_host RUNTIME DESTINATION bin)
install(TARGETS cl_nupu_control_flow RUNTIME DESTINATION bin)
install(TARGETS cl_nupu_reduce RUNTIME DESTINATION bin)
install(TARGETS cl_nupu_absf RUNTIME DESTINATION bin)
install(TARGETS cl_nupu_usm_memcpy RUNTIME DESTINATION bin)
