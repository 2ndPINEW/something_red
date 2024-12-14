set(srcs "main.c"
         "back_matrix_control.c"
         "front_matrix_control.c"
         "matrix_utils.c")

set(include "."
            "include")

# In order for the cases defined by `TEST_CASE` to be linked into the final elf,
# the component can be registered as WHOLE_ARCHIVE
idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES
                       spi_flash
                       led_strip
                       INCLUDE_DIRS "")
