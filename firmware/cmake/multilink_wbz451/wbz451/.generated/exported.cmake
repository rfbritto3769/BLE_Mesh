set(DEPENDENT_MP_BIN2HEXmultilink_wbz451_wbz451_5F3HyYmM "c:/Program Files/Microchip/xc32/v4.60/bin/xc32-bin2hex.exe")
set(DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_5F3HyYmM ${CMAKE_CURRENT_LIST_DIR}/../../../../out/multilink_wbz451/wbz451.elf)
set(DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_5F3HyYmM ${CMAKE_CURRENT_LIST_DIR}/../../../../out/multilink_wbz451)
set(DEPENDENT_BYPRODUCTSmultilink_wbz451_wbz451_5F3HyYmM ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_5F3HyYmM}/${sourceFileNamemultilink_wbz451_wbz451_5F3HyYmM}.c)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_5F3HyYmM}/${sourceFileNamemultilink_wbz451_wbz451_5F3HyYmM}.c
    COMMAND ${DEPENDENT_MP_BIN2HEXmultilink_wbz451_wbz451_5F3HyYmM} --image ${DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_5F3HyYmM} --image-generated-c ${sourceFileNamemultilink_wbz451_wbz451_5F3HyYmM}.c --image-generated-h ${sourceFileNamemultilink_wbz451_wbz451_5F3HyYmM}.h --image-copy-mode ${modemultilink_wbz451_wbz451_5F3HyYmM} --image-offset ${addressmultilink_wbz451_wbz451_5F3HyYmM} 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_5F3HyYmM}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFmultilink_wbz451_wbz451_5F3HyYmM})
add_custom_target(
    dependent_produced_source_artifactmultilink_wbz451_wbz451_5F3HyYmM 
    DEPENDS ${DEPENDENT_TARGET_DIRmultilink_wbz451_wbz451_5F3HyYmM}/${sourceFileNamemultilink_wbz451_wbz451_5F3HyYmM}.c
    )
