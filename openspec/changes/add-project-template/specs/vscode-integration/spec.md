# Spec: VSCode Integration

## ADDED Requirements

### Requirement: Build Tasks
**ID**: TEMPLATE-VSCODE-001
**Priority**: P1 (High)

The system SHALL provide VSCode tasks for build, flash, and clean.

#### Scenario: Build from VSCode
```json
// .vscode/tasks.json
{
    "label": "Build Application",
    "type": "shell",
    "command": "./scripts/build.sh",
    "args": ["application", "stm32f407vg"]
}
```

---

### Requirement: Debug Configuration
**ID**: TEMPLATE-VSCODE-002
**Priority**: P1 (High)

The system SHALL provide debug launch configurations for all platforms.

#### Scenario: Debug with OpenOCD
```json
// .vscode/launch.json
{
    "name": "Debug STM32F407",
    "type": "cortex-debug",
    "request": "launch",
    "executable": "build/application/application.elf",
    "servertype": "openocd"
}
```

---

### Requirement: IntelliSense Configuration
**ID**: TEMPLATE-VSCODE-003
**Priority**: P2 (Medium)

The system SHALL configure IntelliSense for Alloy headers.

#### Scenario: Code completion works
```json
// .vscode/settings.json
{
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/external/alloy/src",
        "${workspaceFolder}/common/include"
    ]
}
```

## MODIFIED Requirements
None.

## REMOVED Requirements
None.
