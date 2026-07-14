# Add custom mesh/provisioning source files to the build
target_sources(multilink_wbz451_wbz451_wbz451_XC32_compile PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../firmware/src/config/wbz451/peripheral/tcc/plib_tcc1.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../firmware/src/app_ble/handlers/app_trsps_handler.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../firmware/src/mesh_routing.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../firmware/src/mesh_conn_mgr.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../firmware/src/led_dimmer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../firmware/src/provisioning.c"
)
