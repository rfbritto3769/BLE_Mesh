set(DEPENDENT_MP_BIN2HEXmultilink_wbz451_wbz451_c9pycGeW "c:/Program Files/Microchip/xc32/v5.10/bin/xc32-bin2hex.exe")
set(DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_c9pycGeW ${CMAKE_CURRENT_LIST_DIR}/../../../../out/multilink_wbz451/wbz451.elf)
set(DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_c9pycGeW ${CMAKE_CURRENT_LIST_DIR}/../../../../out/multilink_wbz451)
set(DEPENDENT_BYPRODUCTSmultilink_wbz451_wbz451_c9pycGeW ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_c9pycGeW}/${sourceFileNamemultilink_wbz451_wbz451_c9pycGeW}.c)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_c9pycGeW}/${sourceFileNamemultilink_wbz451_wbz451_c9pycGeW}.c
    COMMAND ${DEPENDENT_MP_BIN2HEXmultilink_wbz451_wbz451_c9pycGeW} --image ${DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_c9pycGeW} --image-generated-c ${sourceFileNamemultilink_wbz451_wbz451_c9pycGeW}.c --image-generated-h ${sourceFileNamemultilink_wbz451_wbz451_c9pycGeW}.h --image-copy-mode ${modemultilink_wbz451_wbz451_c9pycGeW} --image-offset ${addressmultilink_wbz451_wbz451_c9pycGeW} 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_c9pycGeW}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_c9pycGeW})
add_custom_target(
    dependent_produced_source_artifactmultilink_wbz451_wbz451_c9pycGeW 
    DEPENDS ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_c9pycGeW}/${sourceFileNamemultilink_wbz451_wbz451_c9pycGeW}.c
    )
