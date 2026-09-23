include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/googletest-src
)

# MSVC: link gtest against the shared (/MD) CRT to match the test executable's default.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
