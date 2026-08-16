cmake_minimum_required(VERSION 3.31.10)
project(md)

# Needed for _BitInt support.
set(CMAKE_C_STANDARD 23)

#
# In clang with std=23 on arm64, _BitInt width is limited to 128; on x86_64 it is 655535.
# In gcc, _BitInt widths up to 655535 on arm64 are supported.
# The experimental flag below gives support for bigger integers on Mac (we need 256).
#
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Xclang -fexperimental-max-bitint-width=256)
endif()

#
# Custom build type: UNIT — optimized code with unit-test hooks compiled in
#
set(CMAKE_C_FLAGS_UNIT "-O2 -g -DUNIT"
    CACHE STRING "Flags used by the C compiler during UNIT builds." FORCE)
set(CMAKE_CXX_FLAGS_UNIT "-O2 -g -DUNIT"
    CACHE STRING "Flags used by the C++ compiler during UNIT builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_UNIT "" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_UNIT "" CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_UNIT "" CACHE STRING "" FORCE)
mark_as_advanced(
    CMAKE_C_FLAGS_UNIT
    CMAKE_CXX_FLAGS_UNIT
    CMAKE_EXE_LINKER_FLAGS_UNIT
    CMAKE_SHARED_LINKER_FLAGS_UNIT
    CMAKE_MODULE_LINKER_FLAGS_UNIT
)
# Advertise UNIT to multi-config generators (Xcode). Harmless on Ninja/Make.
if(CMAKE_CONFIGURATION_TYPES)
    list(APPEND CMAKE_CONFIGURATION_TYPES UNIT)
    list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}"
        CACHE STRING "Available build types" FORCE)
endif()

#
# Makes the compiler respect DEBUG blocks inside of C files.
#
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")

# max-pretarget.cmake sets minimum CMAKE_OSX_DEPLOYMENT_TARGET to 10.11
#set(CMAKE_OSX_DEPLOYMENT_TARGET "26")

set(MAX_VERSIONS
    "8"
    "9"
)

set(MAX_SDK_BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/max-test/source/max-sdk-base")

set(C74_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/externals)

#############################################################
# MAX EXTERNAL
#############################################################

include(${MAX_SDK_BASE_DIRECTORY}/script/max-pretarget.cmake)

include_directories( 
    "${MAX_SDK_INCLUDES}"
    "${MAX_SDK_MSP_INCLUDES}"
    "${MAX_SDK_JIT_INCLUDES}"
)

file(GLOB PROJECT_SRC
    "*.h"
    "*.c"
)

add_library( 
    ${PROJECT_NAME} 
    MODULE
    ${PROJECT_SRC}
)

#
# Add mlib (https://github.com/P-p-H-d/mlib/) for dict.h native C hashmap functionality.
#
#message("The path is to mlib is: ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/mlib")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/mlib")

include(${MAX_SDK_BASE_DIRECTORY}/script/max-posttarget.cmake)

#############################################################

#
# On MacOS, all officially distributed apps have to be signed with an Apple-issued certificate.
# Since this is build is for local use, we do an "ad-hoc" signing (no certificate) to bypass security restrictions.
#
if (APPLE)
    message("path to the binary that needs to be signed (from max-posttarget.cmake):")
    message("${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${${PROJECT_NAME}_EXTERN_OUTPUT_NAME}.mxo")
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND codesign --force --deep -s - "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${${PROJECT_NAME}_EXTERN_OUTPUT_NAME}.mxo"
        COMMENT "Ad-hoc signing ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${${PROJECT_NAME}_EXTERN_OUTPUT_NAME}.mxo"
    )
endif()

#
# Post-build, this moves all the contents from the above subdirectories into the Max Packages directory for use by Max.
# - Works with both Unix Makefiles and build button in Xcode.
# - Will create the path (e.g ~/Documents/Max 8/Packages/) if it doesn't exist.
# - Will overwrite files of the same name, but on Mac the OS doesn't seem to update the file modified dates. 
# - Max picks up externals by default from the Packages directory; no need to add it to the Max search path.
#
if (APPLE)
    set(EXPANDED_PATH "$ENV{HOME}/Documents")
    #message(${EXPANDED_PATH})
elseif (WIN32)
    #TODO: test cmake in Windows
    set(EXPANDED_PATH "$ENV{USERPROFILE}/Documents")
endif()
foreach(VERSION IN ITEMS ${MAX_VERSIONS})
    install(DIRECTORY . DESTINATION "${EXPANDED_PATH}/Max ${VERSION}/Packages/md" # put subdirs into an md dir
        PATTERN ".DS_Store" EXCLUDE
        PATTERN "*.h" EXCLUDE
        PATTERN "*.c" EXCLUDE
        PATTERN "*.txt" EXCLUDE
    )
endforeach()
