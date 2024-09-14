add_ca_cl_executable(cl_huca_vector_add_c
                     ${CMAKE_CURRENT_SOURCE_DIR}/cl_huca_vector_add.c)

# TODO: add CPP examples for better UX

install(TARGETS cl_huca_vector_add_c RUNTIME DESTINATION bin)
