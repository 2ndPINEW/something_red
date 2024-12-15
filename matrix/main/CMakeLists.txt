set(srcs "main.c"
         "back_matrix_control.c"
         "front_matrix_control.c"
         "matrix_utils.c"
         "color_palette.c"
         "light_controller.c"
         "light_mode_rainbow_scroll.c"
         "light_mode_off.c"
         "light_mode_blink_red.c"
         "light_mode_init.c"
         "light_mode_pachinko.c"
         )

set(include "."
            "include")

# In order for the cases defined by `TEST_CASE` to be linked into the final elf,
# the component can be registered as WHOLE_ARCHIVE
idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES
                       spi_flash
                       led_strip
                       INCLUDE_DIRS "")
