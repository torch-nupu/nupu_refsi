add_ca_cl_executable(cl_huca_vector_add_c
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_vector_add.c)
add_ca_cl_executable(cl_huca_vector_add_2d_c
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_vector_add_2d.c)
add_ca_cl_executable(cl_huca_nop ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_nop.c)
add_ca_cl_executable(cl_huca_multi_devices
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_multi_devices.c)

# TODO: add CPP examples for better UX

install(TARGETS cl_huca_vector_add_c RUNTIME DESTINATION bin)
install(TARGETS cl_huca_vector_add_2d_c RUNTIME DESTINATION bin)
install(TARGETS cl_huca_nop RUNTIME DESTINATION bin)
install(TARGETS cl_huca_multi_devices RUNTIME DESTINATION bin)
