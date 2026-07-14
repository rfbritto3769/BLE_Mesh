# The following functions contains all the flags passed to the different build stages.

set(PACK_REPO_PATH "C:/Users/X44455/.mchp_packs" CACHE PATH "Path to the root of a pack repository.")

function(multilink_wbz451_wbz451_wbz451_XC32_assemble_rule target)
    set(options
        "-g"
        "${ASSEMBLER_PRE}"
        "-mprocessor=WBZ451"
        "-Wa,--defsym=__MPLAB_BUILD=1${MP_EXTRA_AS_POST},--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--gdwarf-2,-I${CMAKE_CURRENT_SOURCE_DIR}/../../../multilink_wbz451.X"
        "-mdfp=${PACK_REPO_PATH}/Microchip/PIC32CX-BZ_DFP/1.4.243/WBZ451")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target} PRIVATE "__DEBUG=1")
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../multilink_wbz451.X")
endfunction()
function(multilink_wbz451_wbz451_wbz451_XC32_assembleWithPreprocess_rule target)
    set(options
        "-x"
        "assembler-with-cpp"
        "-g"
        "${MP_EXTRA_AS_PRE}"
        "${DEBUGGER_NAME_AS_MACRO}"
        "-mdfp=${PACK_REPO_PATH}/Microchip/PIC32CX-BZ_DFP/1.4.243/WBZ451"
        "-mprocessor=WBZ451"
        "-Wa,--defsym=__MPLAB_BUILD=1${MP_EXTRA_AS_POST},--defsym=__MPLAB_DEBUG=1,--gdwarf-2,--defsym=__DEBUG=1,-I${CMAKE_CURRENT_SOURCE_DIR}/../../../multilink_wbz451.X")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG=1"
        PRIVATE "XPRJ_wbz451=wbz451")
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../multilink_wbz451.X")
endfunction()
function(multilink_wbz451_wbz451_wbz451_XC32_compile_rule target)
    set(options
        "-g"
        "${CC_PRE}"
        "-x"
        "c"
        "-c"
        "-mprocessor=WBZ451"
        "-ffunction-sections"
        "-fdata-sections"
        "-O1"
        "-fcommon"
        "-Wall"
        "-mdfp=${PACK_REPO_PATH}/Microchip/PIC32CX-BZ_DFP/1.4.243/WBZ451")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG"
        PRIVATE "HAVE_CONFIG_H"
        PRIVATE "WOLFSSL_IGNORE_FILE_WARN"
        PRIVATE "XPRJ_wbz451=wbz451")
    target_include_directories(${target}
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/app_ble"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/app_ble/handlers"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/config/wbz451"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/config/wbz451/ble/lib/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/config/wbz451/ble/middleware_ble"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/config/wbz451/ble/profile_ble"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/config/wbz451/ble/service_ble"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/config/wbz451/driver/pds/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/packs/CMSIS"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/packs/CMSIS/CMSIS/Core/Include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/packs/WBZ451_DFP"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/third_party/rtos/FreeRTOS/Source/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/third_party/rtos/FreeRTOS/Source/portable/GCC/SAM/ARM_CM4F"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/third_party/wolfssl"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/third_party/wolfssl/wolfssl"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../multilink_wbz451.X"
        PRIVATE "${PACK_REPO_PATH}/ARM/CMSIS/5.8.0/CMSIS/Core/Include")
endfunction()
function(multilink_wbz451_wbz451_wbz451_XC32_compile_cpp_rule target)
    set(options
        "-g"
        "${CC_PRE}"
        "${DEBUGGER_NAME_AS_MACRO}"
        "-mprocessor=WBZ451"
        "-frtti"
        "-fexceptions"
        "-fno-check-new"
        "-fenforce-eh-specs"
        "-ffunction-sections"
        "-O1"
        "-fno-common"
        "-mdfp=${PACK_REPO_PATH}/Microchip/PIC32CX-BZ_DFP/1.4.243/WBZ451")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target}
        PRIVATE "__DEBUG"
        PRIVATE "XPRJ_wbz451=wbz451")
    target_include_directories(${target}
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/config/wbz451"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/packs/CMSIS"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/packs/CMSIS/CMSIS/Core/Include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/packs/WBZ451_DFP"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/third_party/rtos/FreeRTOS/Source/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/third_party/rtos/FreeRTOS/Source/portable/GCC/SAM/ARM_CM4F"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../multilink_wbz451.X"
        PRIVATE "${PACK_REPO_PATH}/ARM/CMSIS/5.8.0/CMSIS/Core/Include")
endfunction()
function(multilink_wbz451_wbz451_dependentObject_rule target)
    set(options
        "-mprocessor=WBZ451"
        "-mdfp=${PACK_REPO_PATH}/Microchip/PIC32CX-BZ_DFP/1.4.243/WBZ451")
    list(REMOVE_ITEM options "")
    target_compile_options(${target} PRIVATE "${options}")
endfunction()
function(multilink_wbz451_wbz451_link_rule target)
    set(options
        "-g"
        "${MP_EXTRA_LD_PRE}"
        "${DEBUGGER_OPTION_TO_LINKER}"
        "${DEBUGGER_NAME_AS_MACRO}"
        "-mprocessor=WBZ451"
        "-mno-device-startup-code"
        "-Wl,--defsym=__MPLAB_BUILD=1${MP_EXTRA_LD_POST},--script=${multilink_wbz451_wbz451_LINKER_SCRIPT},--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=_min_heap_size=512,--gc-sections,-L${CMAKE_CURRENT_SOURCE_DIR}/../../../multilink_wbz451.X,-Map=mem.map,--memorysummary,memoryfile.xml"
        "-mdfp=${PACK_REPO_PATH}/Microchip/PIC32CX-BZ_DFP/1.4.243/WBZ451")
    list(REMOVE_ITEM options "")
    target_link_options(${target} PRIVATE "${options}")
    target_compile_definitions(${target} PRIVATE "XPRJ_wbz451=wbz451")
endfunction()
function(multilink_wbz451_wbz451_bin2hex_rule target)
    add_custom_target(
        multilink_wbz451_wbz451_Bin2Hex ALL
        COMMAND ${MP_BIN2HEX} ${multilink_wbz451_wbz451_image_name}
        WORKING_DIRECTORY ${multilink_wbz451_wbz451_output_dir}
        BYPRODUCTS "${multilink_wbz451_wbz451_output_dir}/${multilink_wbz451_wbz451_image_base_name}.hex"
        COMMENT "Convert build file to .hex")
    add_dependencies(multilink_wbz451_wbz451_Bin2Hex ${target})
endfunction()
