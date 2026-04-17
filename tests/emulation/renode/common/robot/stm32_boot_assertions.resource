*** Settings ***
Resource    ${COMMON_BOOT_RESOURCE}

*** Keywords ***
Create STM32 USART2 Boot Smoke Machine
    [Arguments]    ${performance_mips}=125
    Create Boot Smoke Machine    sysbus.usart2    ${performance_mips}

Assert STM32 USART2 Boot Smoke
    [Arguments]
    ...    ${boot_banner}
    ...    ${boot_stage}
    ...    ${boot_marker}
    ...    ${uart_bytes}
    ...    ${gpio_moder_addr}
    ...    ${gpio_moder_mask}
    ...    ${gpio_moder_expected}
    ...    ${gpio_afrl_addr}
    ...    ${gpio_afrl_mask}
    ...    ${gpio_afrl_expected}
    ...    ${usart_brr_addr}
    ...    ${usart_brr_expected}
    ...    ${usart_cr1_addr}
    ...    ${usart_cr1_mask}
    ...    ${usart_cr1_expected}
    Start Emulation
    Wait For Boot Banner    ${boot_banner}
    Boot Stage Should Be    ${BOOT_STAGE_ADDR}    ${boot_stage}
    Boot Marker Should Be    ${BOOT_MARKER_ADDR}    ${boot_marker}
    Double Word Should Be    ${BOOT_UART_BYTES_ADDR}    ${uart_bytes}
    Masked Double Word Should Be    ${gpio_moder_addr}    ${gpio_moder_mask}    ${gpio_moder_expected}
    Masked Double Word Should Be    ${gpio_afrl_addr}    ${gpio_afrl_mask}    ${gpio_afrl_expected}
    Double Word Should Be    ${usart_brr_addr}    ${usart_brr_expected}
    Masked Double Word Should Be    ${usart_cr1_addr}    ${usart_cr1_mask}    ${usart_cr1_expected}
