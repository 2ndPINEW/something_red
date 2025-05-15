set(srcs "main.c"
         "led_matrix_control.c"
         "matrix_utils.c"
         "light_controller.c"
         "color_palette.c"
         "light_modes/light_mode_init.c"
         "light_modes/light_mode_off.c"
         "light_modes/light_mode_blue_flush.c"
         "light_modes/light_mode_blue_fire.c"
         )

set(include "."
            "include")

# In order for the cases defined by `TEST_CASE` to be linked into the final elf,
# the component can be registered as WHOLE_ARCHIVE
idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES
                       spi_flash
                       led_strip
                       bt
                       INCLUDE_DIRS "")
