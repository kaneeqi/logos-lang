cmake_minimum_required(VERSION 3.20)

set(CMAKE_BUILD_TYPE Debug)
project(Logos)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ========================================
# Configuracion de ANTLR4
# ========================================
set(MY_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(ANTLR_EXECUTABLE "${MY_BASE_DIR}/antlr-4.13.2-complete.jar")
set(ANTLR_INCLUDE_DIR "/usr/include/antlr4-runtime")
list(APPEND CMAKE_MODULE_PATH ${MY_BASE_DIR})

# Descargar ANTLR JAR si no existe
if(NOT EXISTS "${ANTLR_EXECUTABLE}")
    message(STATUS "Descargando ANTLR JAR...")
    file(DOWNLOAD
        "https://www.antlr.org/download/antlr-4.13.2-complete.jar"
        "${ANTLR_EXECUTABLE}"
        STATUS DOWNLOAD_STATUS TIMEOUT 60)
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Error descargando ANTLR JAR")
    endif()
endif()

# Descargar FindANTLR.cmake si no existe
if(NOT EXISTS "${MY_BASE_DIR}/FindANTLR.cmake")
    message(STATUS "Descargando FindANTLR.cmake...")
    set(URL1 "https://raw.githubusercontent.com/antlr/antlr4/")
    set(URL2 "refs/heads/dev/runtime/Cpp/cmake/FindANTLR.cmake")
    file(DOWNLOAD
        "${URL1}${URL2}"
        "${MY_BASE_DIR}/FindANTLR.cmake"
        STATUS DOWNLOAD_STATUS TIMEOUT 60)
endif()

find_package(ANTLR REQUIRED)
message(STATUS "ANTLR encontrado: ${ANTLR_VERSION}")

# Generar codigo desde gramatica
file(GLOB GFOURS "*.g4")
antlr_target(${PROJECT_NAME} ${GFOURS} LEXER PARSER VISITOR)
message(STATUS "ANTLR genero archivos en: ${ANTLR_${PROJECT_NAME}_OUTPUT_DIR}")

# ========================================
# Configuracion de LLVM
# ========================================
find_package(LLVM REQUIRED CONFIG)
message(STATUS "LLVM encontrado: ${LLVM_PACKAGE_VERSION}")
message(STATUS "Usando LLVMConfig.cmake en: ${LLVM_DIR}")

# ========================================
# Compilar ejecutable
# ========================================
file(GLOB SOURCES "*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES} ${ANTLR_${PROJECT_NAME}_CXX_OUTPUTS})

target_link_libraries(${PROJECT_NAME} PRIVATE antlr4-runtime LLVM)
target_include_directories(
    ${PROJECT_NAME} PRIVATE
    ${ANTLR_INCLUDE_DIR}
    ${ANTLR_${PROJECT_NAME}_OUTPUT_DIR}
    ${LLVM_INCLUDE_DIRS}
)

add_definitions(${LLVM_DEFINITIONS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Suprimir warnings de LLVM headers
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-comment)

# vim: set ts=4 sw=4 et sts=0 :
