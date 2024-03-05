
# Build the executable based on the source files
set(TARGET_NAME Surf5_SNTP_Library)

add_executable(${TARGET_NAME}

  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/Surf5_SNTP_Library/main.c 
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/Surf5_SNTP_Library/W7500x_it.c
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/Surf5_SNTP_Library/retarget.c
  ${CMAKE_SOURCE_DIR}/Libraries/CMSIS/Device/WIZnet/W7500/Source/GCC/startup_w7500.s
  )

target_include_directories(${TARGET_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/Surf5_SNTP_Library
  )

target_link_libraries(${TARGET_NAME} PRIVATE
    SYSTEM_W7500X_FILES  
    GPIO_FILES
    UART_FILES
    ADC_FILES
    CRG_FILES
    DUALTIMER_FILES
    EXTI_FILES
    FLASH_FILES
    MIIM_FILES
    MISC_FILES
    PWM_FILES
    RNG_FILES
    RTC_FILES
    SSP_FILES
    WDT_FILES
    WZTOE_FILES   
    WIZCHIP_CONF_FILES
    LOOPBACK_FILES
    SNTP_FILES
    DHCP_FILES
)

complie_link_option(${TARGET_NAME})
create_binary_file(${TARGET_NAME})
create_elf_file(${TARGET_NAME})

