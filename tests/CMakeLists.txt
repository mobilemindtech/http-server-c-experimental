# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.14)

# procuramos CUnit via pkg-config (mesma estratégia)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CUNIT QUIET cunit)

if (NOT CUNIT_FOUND)
    message(STATUS "CUnit not found via pkg-config, trying manual find...")
    find_path(CUNIT_INCLUDE_DIR NAMES CUnit/Basic.h)
    find_library(CUNIT_LIBRARY NAMES cunit CUnit)
    if (CUNIT_INCLUDE_DIR AND CUNIT_LIBRARY)
        set(CUNIT_FOUND TRUE)
        set(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR})
        set(CUNIT_LIBRARIES ${CUNIT_LIBRARY})
    endif()
endif()

if (NOT CUNIT_FOUND)
    message(FATAL_ERROR "CUnit not found. Install libunit-dev / libcunit (pkg-config) or disable BUILD_TESTS")
endif()

include_directories(${CUNIT_INCLUDE_DIRS})
link_directories(${CUNIT_LIBRARY_DIRS})

# Adiciona o executável de teste, linkando com a lib que contém a lógica
add_executable(test_http_parser
    test_http_parser.c
)

target_include_directories(test_http_parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(test_http_parser PRIVATE http_parser ${CUNIT_LIBRARIES} ${LIBURING_LIBRARIES})

# registra no CTest (permite 'ctest' / 'ctest -V')
add_test(NAME server_unit_tests COMMAND test_http_parser)
