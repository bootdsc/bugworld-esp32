file(GLOB JET_SRC "src/*.cpp")

idf_component_register(
    SRCS ${JET_SRC}
    INCLUDE_DIRS "src"
    REQUIRES main
)
