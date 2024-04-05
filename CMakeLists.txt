cmake_minimum_required(VERSION 3.26)
project(ComputerGraphicProject VERSION 0.0.1)

find_package(glew CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(imgui CONFIG REQUIRED)

set(CG_LIBRARY GLEW::glew imgui::imgui glfw glm::glm OpenGL::GL)
set(CG_INCLUDE_DIR Utility/Header ${Stb_INCLUDE_DIR})


file(GLOB_RECURSE CG_FILES CGProject/*.cpp)
add_executable(CGProject ${CG_FILES} CGProject/GlobalSpace.h)
target_include_directories(CGProject PRIVATE ${CG_INCLUDE_DIR})
target_link_libraries(CGProject PRIVATE ${CG_LIBRARY})

# Se il risultato non cambia cancellare la cache di cmake (Tools->Cmake->Delete Cache and Reload)
option(COMPILE_PROF_CODE "Compile prof code" OFF)

if (COMPILE_PROF_CODE)
    add_definitions(-DCOMPILE_PROF_CODE)
    file(GLOB_RECURSE SRC_FILES src/*.cpp)
    add_library(ProfCode Utility/Header/source.h ${SRC_FILES})
    target_link_libraries(ProfCode PRIVATE ${CG_LIBRARY})
    target_include_directories(ProfCode PRIVATE ${CG_INCLUDE_DIR})
    target_link_libraries(CGProject PRIVATE ProfCode)
endif ()

