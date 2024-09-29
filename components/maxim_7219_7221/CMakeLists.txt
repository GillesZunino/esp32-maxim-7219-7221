set(srcs
    "src/maxim-7219-7221.c"
)

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "include"
		    REQUIRES "driver")


if(CONFIG_MAXIM7219_7221_SANITIZER)
    idf_component_get_property(lib maxim-7219-7221 COMPONENT_LIB)
    target_compile_options(${lib} PRIVATE "-fsanitize=undefined" "-fno-sanitize=shift-base")
endif()