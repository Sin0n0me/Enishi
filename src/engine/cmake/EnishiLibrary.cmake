include_guard(GLOBAL)

function(enishi_add_library target)
    set(sources ${ARGN})
    set(has_compilation_unit FALSE)

    foreach(source IN LISTS sources)
        get_filename_component(extension "${source}" LAST_EXT)
        if(extension MATCHES "^\\.(c|cc|cpp|cxx|m|mm)$")
            set(has_compilation_unit TRUE)
            break()
        endif()
    endforeach()

    if(has_compilation_unit)
        add_library(${target} ${sources})
    else()
        add_library(${target} INTERFACE)
    endif()
endfunction()

function(enishi_configure_library target)
    cmake_parse_arguments(ARG "" "" "INCLUDE_DIRECTORIES;DEPENDENCIES" ${ARGN})
    get_target_property(target_type ${target} TYPE)

    if(target_type STREQUAL "INTERFACE_LIBRARY")
        target_include_directories(${target} INTERFACE ${ARG_INCLUDE_DIRECTORIES})
        target_link_libraries(${target} INTERFACE ${ARG_DEPENDENCIES})
        target_compile_features(${target} INTERFACE cxx_std_23)
    else()
        target_include_directories(${target} PUBLIC ${ARG_INCLUDE_DIRECTORIES})
        target_link_libraries(${target} PRIVATE ${ARG_DEPENDENCIES})
        set_target_properties(${target} PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED ON
        )
    endif()
endfunction()
