function (bin_spi0_flash)
    set (name "app_spi0_flash")
    
    set (src 
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/System.cpp")

    set (link
        ${STARTUP_LIB_NAME}
        ${DEVICE_LIB_NAME}
        ${STDLIB_STUB_LIB_NAME}
        "gcc")

    add_executable(${name} ${src})
    target_link_libraries (${name} ${link})
    link_app (${name})
endfunction ()

#################################################################

bin_spi0_flash()