set(DEPENDENT_MP_BIN2HEXmultilink_wbz451_wbz451_qG1qnUzK "c:/Program Files/Microchip/xc32/v5.10/bin/xc32-bin2hex.exe")
set(DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_qG1qnUzK ${CMAKE_CURRENT_LIST_DIR}/../../../../out/multilink_wbz451/wbz451.elf)
set(DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_qG1qnUzK ${CMAKE_CURRENT_LIST_DIR}/../../../../out/multilink_wbz451)
set(DEPENDENT_BYPRODUCTSmultilink_wbz451_wbz451_qG1qnUzK ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_qG1qnUzK}/${sourceFileNamemultilink_wbz451_wbz451_qG1qnUzK}.c)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_qG1qnUzK}/${sourceFileNamemultilink_wbz451_wbz451_qG1qnUzK}.c
    COMMAND ${DEPENDENT_MP_BIN2HEXmultilink_wbz451_wbz451_qG1qnUzK} --image ${DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_qG1qnUzK} --image-generated-c ${sourceFileNamemultilink_wbz451_wbz451_qG1qnUzK}.c --image-generated-h ${sourceFileNamemultilink_wbz451_wbz451_qG1qnUzK}.h --image-copy-mode ${modemultilink_wbz451_wbz451_qG1qnUzK} --image-offset ${addressmultilink_wbz451_wbz451_qG1qnUzK} 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_qG1qnUzK}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_qG1qnUzK})
add_custom_target(
    dependent_produced_source_artifactmultilink_wbz451_wbz451_qG1qnUzK 
    DEPENDS ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_qG1qnUzK}/${sourceFileNamemultilink_wbz451_wbz451_qG1qnUzK}.c
    )
