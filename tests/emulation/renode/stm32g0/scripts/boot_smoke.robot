*** Settings ***
Suite Setup       Setup
Suite Teardown    Teardown
Test Setup        Setup Emulation
Resource          ${COMMON_BOOT_RESOURCE}
Resource          ${COMMON_STM32_BOOT_RESOURCE}

*** Variables ***
${EXPECTED_BOOT_BANNER}    alloy renode stm32g0 boot ok
${EXPECTED_BOOT_STAGE}     0x4
${EXPECTED_BOOT_MARKER}    0xA1100001
${EXPECTED_UART_BYTES}     29
${RCC_CR_ADDR}             0x40021000
${RCC_CFGR_ADDR}           0x40021008
${FLASH_ACR_ADDR}          0x40022000
${GPIOA_MODER_ADDR}        0x50000000
${GPIOA_AFRL_ADDR}         0x50000020
${USART2_BRR_ADDR}         0x4000440C
${USART2_CR1_ADDR}         0x40004400

*** Keywords ***
Setup Emulation
    Create STM32 USART2 Boot Smoke Machine    125

*** Test Cases ***
STM32G0 boot smoke reaches main and emits UART banner
    Assert STM32 USART2 Boot Smoke
    ...    ${EXPECTED_BOOT_BANNER}
    ...    ${EXPECTED_BOOT_STAGE}
    ...    ${EXPECTED_BOOT_MARKER}
    ...    ${EXPECTED_UART_BYTES}
    ...    ${GPIOA_MODER_ADDR}
    ...    0xCF0
    ...    0x4A0
    ...    ${GPIOA_AFRL_ADDR}
    ...    0xFF00
    ...    0x1100
    ...    ${USART2_BRR_ADDR}
    ...    556
    ...    ${USART2_CR1_ADDR}
    ...    0xD
    ...    0xD
    Masked Double Word Should Be    ${RCC_CR_ADDR}        0x2000500  0x2000500
    Masked Double Word Should Be    ${RCC_CFGR_ADDR}      0x38       0x10
    Masked Double Word Should Be    ${FLASH_ACR_ADDR}     0x7        0x2
