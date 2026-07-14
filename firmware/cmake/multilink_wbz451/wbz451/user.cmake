# Add BLE Transparent Profile Server and Provisioning source files
target_sources(multilink_wbz451_wbz451_wbz451_XC32_compile PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/config/wbz451/ble/profile_ble/ble_trsps/ble_trsps.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/provisioning.c"
)
