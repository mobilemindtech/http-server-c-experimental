cmake_minimum_required(VERSION 3.14)

project(server VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra")

find_package(PkgConfig REQUIRED)

# sua lib liburing (como já tinha)
pkg_check_modules(LIBURING liburing)
include_directories(${LIBURING_INCLUDE_DIRS})
link_directories(${LIBURING_LIBRARY_DIRS})

# ---------------------------------------------------------
# Aqui criamos uma biblioteca com a lógica testável
# ---------------------------------------------------------
# suponha que você tenha: src/server_core.c + src/server_core.h
# e um src/server_main.c que contenha main() e apenas chama funções da lib
add_library(http_parser STATIC
    src/http_parser.h
    src/http_parser.c
    src/http_core.h
    src/http_core.c
    src/util.h
    src/util.c
    src/data.h
    src/data.c
    # adicione outros .c testáveis aqui
)

target_include_directories(http_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(http_parser PRIVATE ${LIBURING_LIBRARIES})

# executável final (usa a biblioteca)
add_executable(http_server src/http_server.c)   # renomeei para http_server
target_link_libraries(http_server PRIVATE http_parser ${LIBURING_LIBRARIES})

# Permite ao usuário ligar/desligar a build dos testes
option(BUILD_TESTS "Build unit tests" ON)

if (BUILD_TESTS)
    enable_testing()
    # exporta a variável para subdiretórios (ou você pode procurar CUnit no tests/CMakeLists)
    add_subdirectory(tests)
endif()

# Cria um alvo customizado chamado "check"
add_custom_target(check
    COMMAND ${CMAKE_COMMAND} --build .            # equivalente ao "make"
    COMMAND ${CMAKE_CTEST_COMMAND} --rerun-failed --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Build e reexecuta apenas os testes que falharam"
)
