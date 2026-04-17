*** Settings ***
Suite Setup       Setup
Suite Teardown    Teardown
Test Setup        Setup Emulation
Resource          ${COMMON_BOOT_RESOURCE}
Resource          ${COMMON_STM32_BOOT_RESOURCE}

*** Variables ***
${EXPECTED_BOOT_BANNER}    alloy renode stm32f4 boot ok
${EXPECTED_BOOT_STAGE}     0x4
${EXPECTED_BOOT_MARKER}    0xA1144001
${EXPECTED_UART_BYTES}     29
${RCC_AHB1ENR_ADDR}        0x40023830
${RCC_APB1ENR_ADDR}        0x40023840
${GPIOA_MODER_ADDR}        0x40020000
${GPIOA_AFRL_ADDR}         0x40020020
${USART2_BRR_ADDR}         0x40004408
${USART2_CR1_ADDR}         0x4000440C

*** Keywords ***
Setup Emulation
    Create STM32 USART2 Boot Smoke Machine    125

*** Test Cases ***
STM32F4 boot smoke reaches main and emits UART banner
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
    ...    0x7700
    ...    ${USART2_BRR_ADDR}
    ...    365
    ...    ${USART2_CR1_ADDR}
    ...    0x200C
    ...    0x200C
    Masked Double Word Should Be    ${RCC_AHB1ENR_ADDR}    0x1        0x1
    Masked Double Word Should Be    ${RCC_APB1ENR_ADDR}    0x20000    0x20000
