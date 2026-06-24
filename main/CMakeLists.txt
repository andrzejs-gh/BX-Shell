file(GLOB MY_SOURCES "*.c")

idf_component_register(
    SRCS ${MY_SOURCES}
    INCLUDE_DIRS "."
)
