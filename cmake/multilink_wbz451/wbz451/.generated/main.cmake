include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(multilink_wbz451_wbz451_library_list )

# Handle files with suffix s, for group wbz451-XC32
if(multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_assemble)
add_library(multilink_wbz451_wbz451_wbz451_XC32_assemble OBJECT ${multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_assemble})
    multilink_wbz451_wbz451_wbz451_XC32_assemble_rule(multilink_wbz451_wbz451_wbz451_XC32_assemble)
    list(APPEND multilink_wbz451_wbz451_library_list "$<TARGET_OBJECTS:multilink_wbz451_wbz451_wbz451_XC32_assemble>")

endif()

# Handle files with suffix S, for group wbz451-XC32
if(multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_assembleWithPreprocess)
add_library(multilink_wbz451_wbz451_wbz451_XC32_assembleWithPreprocess OBJECT ${multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_assembleWithPreprocess})
    multilink_wbz451_wbz451_wbz451_XC32_assembleWithPreprocess_rule(multilink_wbz451_wbz451_wbz451_XC32_assembleWithPreprocess)
    list(APPEND multilink_wbz451_wbz451_library_list "$<TARGET_OBJECTS:multilink_wbz451_wbz451_wbz451_XC32_assembleWithPreprocess>")

endif()

# Handle files with suffix [cC], for group wbz451-XC32
if(multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_compile)
add_library(multilink_wbz451_wbz451_wbz451_XC32_compile OBJECT ${multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_compile})
    multilink_wbz451_wbz451_wbz451_XC32_compile_rule(multilink_wbz451_wbz451_wbz451_XC32_compile)
    list(APPEND multilink_wbz451_wbz451_library_list "$<TARGET_OBJECTS:multilink_wbz451_wbz451_wbz451_XC32_compile>")

endif()

# Handle files with suffix cpp, for group wbz451-XC32
if(multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_compile_cpp)
add_library(multilink_wbz451_wbz451_wbz451_XC32_compile_cpp OBJECT ${multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_compile_cpp})
    multilink_wbz451_wbz451_wbz451_XC32_compile_cpp_rule(multilink_wbz451_wbz451_wbz451_XC32_compile_cpp)
    list(APPEND multilink_wbz451_wbz451_library_list "$<TARGET_OBJECTS:multilink_wbz451_wbz451_wbz451_XC32_compile_cpp>")

endif()

# Handle files with suffix [cC], for group wbz451-XC32
if(multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_dependentObject)
add_library(multilink_wbz451_wbz451_wbz451_XC32_dependentObject OBJECT ${multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_dependentObject})
    multilink_wbz451_wbz451_wbz451_XC32_dependentObject_rule(multilink_wbz451_wbz451_wbz451_XC32_dependentObject)
    list(APPEND multilink_wbz451_wbz451_library_list "$<TARGET_OBJECTS:multilink_wbz451_wbz451_wbz451_XC32_dependentObject>")

endif()


# Main target for this project
add_executable(multilink_wbz451_wbz451_image_c9pycGeW ${multilink_wbz451_wbz451_library_list})

set_target_properties(multilink_wbz451_wbz451_image_c9pycGeW PROPERTIES
    OUTPUT_NAME "wbz451"
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY "${multilink_wbz451_wbz451_output_dir}")
target_link_libraries(multilink_wbz451_wbz451_image_c9pycGeW PRIVATE ${multilink_wbz451_wbz451_wbz451_XC32_FILE_TYPE_link})

# Add the link options from the rule file.
multilink_wbz451_wbz451_link_rule( multilink_wbz451_wbz451_image_c9pycGeW)

# Add bin2hex target for converting built file to a .hex file.
string(REGEX REPLACE [.]elf$ .hex multilink_wbz451_wbz451_image_name_hex ${multilink_wbz451_wbz451_image_name})
add_custom_target(multilink_wbz451_wbz451_Bin2Hex ALL
    COMMAND ${MP_BIN2HEX} \"${multilink_wbz451_wbz451_output_dir}/${multilink_wbz451_wbz451_image_name}\"
    BYPRODUCTS ${multilink_wbz451_wbz451_output_dir}/${multilink_wbz451_wbz451_image_name_hex}
    COMMENT "Convert built file to .hex")
add_dependencies(multilink_wbz451_wbz451_Bin2Hex multilink_wbz451_wbz451_image_c9pycGeW)



