idf_component_register(
    SRCS
        "test_main.c"
       
        "../../main/path_utils.c"
    INCLUDE_DIRS
        "."
        "../../main"
    REQUIRES
        unity
)
