*** Settings ***
Resource    ${RENODEKEYWORDS}

*** Keywords ***
Create Boot Smoke Machine
    [Arguments]    ${uart_path}=${EMPTY}    ${performance_mips}=125
    Execute Command    mach create
    Execute Command    machine LoadPlatformDescription @${PLATFORM_REPL}
    Execute Command    cpu PerformanceInMips ${performance_mips}
    Execute Command    sysbus LoadELF @${ELF}
    IF    '${uart_path}' != '${EMPTY}'
        Create Terminal Tester    ${uart_path}
    END

Read Double Word
    [Arguments]    ${address}
    ${value}=    Execute Command    sysbus ReadDoubleWord ${address}
    ${value}=    Get Line    ${value}    0
    RETURN    ${value}

Double Word Should Be
    [Arguments]    ${address}    ${expected}
    ${value}=    Read Double Word    ${address}
    Should Be Equal As Integers    ${value}    ${expected}

Masked Double Word Should Be
    [Arguments]    ${address}    ${mask}    ${expected}
    ${value}=    Read Double Word    ${address}
    ${masked}=    Evaluate    int(${value}) & int(${mask})
    Should Be Equal As Integers    ${masked}    ${expected}

Boot Marker Should Be
    [Arguments]    ${address}    ${expected}
    ${value}=    Read Double Word    ${address}
    Should Be Equal As Integers    ${value}    ${expected}

Boot Stage Should Be
    [Arguments]    ${address}    ${expected}
    ${value}=    Read Double Word    ${address}
    Should Be Equal As Integers    ${value}    ${expected}

Wait For Boot Banner
    [Arguments]    ${banner}
    Wait For Line On Uart    ${banner}
