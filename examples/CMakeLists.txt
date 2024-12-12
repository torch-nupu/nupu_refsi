include_directories("${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-CLHPP/include")
add_compile_options("-Wno-error=cast-qual")

add_ca_cl_executable(cl_huca_vector_add_c
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_vector_add.c)
add_ca_cl_executable(cl_huca_vector_add_2d_c
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_vector_add_2d.c)
add_ca_cl_executable(cl_huca_nop ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_nop.c)
add_ca_cl_executable(cl_huca_multi_devices
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_multi_devices.c)
add_ca_cl_executable(cl_huca_control_flow
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_control_flow.c)
add_ca_cl_executable(cl_huca_reduce
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_reduce.cpp)

add_ca_cl_executable(cl_huca_absf
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_absf.cpp)

install(TARGETS cl_huca_vector_add_c RUNTIME DESTINATION bin)
install(TARGETS cl_huca_vector_add_2d_c RUNTIME DESTINATION bin)
install(TARGETS cl_huca_nop RUNTIME DESTINATION bin)
install(TARGETS cl_huca_multi_devices RUNTIME DESTINATION bin)
install(TARGETS cl_huca_control_flow RUNTIME DESTINATION bin)
install(TARGETS cl_huca_reduce RUNTIME DESTINATION bin)
install(TARGETS cl_huca_absf RUNTIME DESTINATION bin)
