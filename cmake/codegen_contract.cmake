# Validate generated board artifact compatibility contract.
#
# The generated file cmake/generated/board_metadata.cmake exports:
# - MICROCORE_GENERATED_CODEGEN_CONTRACT_*
# - MICROCORE_GENERATED_FRAMEWORK_VERSION_*
# - MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_*
#
# This module enforces that generated artifacts were produced for the same
# framework major/minor line and still satisfy the declared compatibility range.

function(microcore_validate_codegen_artifact_contract)
    set(_missing_vars "")
    foreach(_required
        MICROCORE_GENERATED_CODEGEN_CONTRACT_ID
        MICROCORE_GENERATED_CODEGEN_CONTRACT_VERSION
        MICROCORE_GENERATED_CODEGEN_VERSION
        MICROCORE_GENERATED_FRAMEWORK_VERSION
        MICROCORE_GENERATED_FRAMEWORK_VERSION_MAJOR
        MICROCORE_GENERATED_FRAMEWORK_VERSION_MINOR
        MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MAJOR
        MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MIN
        MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MAX
    )
        if(NOT DEFINED ${_required})
            list(APPEND _missing_vars ${_required})
        endif()
    endforeach()

    if(_missing_vars)
        message(FATAL_ERROR
            "Generated codegen contract metadata is incomplete.\n"
            "Missing variables: ${_missing_vars}\n"
            "Regenerate board artifacts:\n"
            "  python3 tools/codegen/scripts/generate_board_artifacts.py"
        )
    endif()

    # Pin artifacts to the current framework major/minor line.
    if(NOT "${PROJECT_VERSION_MAJOR}" STREQUAL "${MICROCORE_GENERATED_FRAMEWORK_VERSION_MAJOR}")
        message(FATAL_ERROR
            "Generated artifact/framework major mismatch.\n"
            "  Framework: ${PROJECT_VERSION}\n"
            "  Artifacts: ${MICROCORE_GENERATED_FRAMEWORK_VERSION}\n"
            "Regenerate board artifacts with current framework version."
        )
    endif()

    if(NOT "${PROJECT_VERSION_MINOR}" STREQUAL "${MICROCORE_GENERATED_FRAMEWORK_VERSION_MINOR}")
        message(FATAL_ERROR
            "Generated artifact/framework minor mismatch.\n"
            "  Framework: ${PROJECT_VERSION}\n"
            "  Artifacts: ${MICROCORE_GENERATED_FRAMEWORK_VERSION}\n"
            "Regenerate board artifacts with current framework version."
        )
    endif()

    # Validate against declared compatibility envelope.
    if(NOT "${PROJECT_VERSION_MAJOR}" STREQUAL "${MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MAJOR}")
        message(FATAL_ERROR
            "Framework major ${PROJECT_VERSION_MAJOR} is incompatible with "
            "generated codegen contract "
            "${MICROCORE_GENERATED_CODEGEN_CONTRACT_ID}@${MICROCORE_GENERATED_CODEGEN_CONTRACT_VERSION} "
            "(expects major ${MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MAJOR})."
        )
    endif()

    if(PROJECT_VERSION_MINOR LESS MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MIN
       OR PROJECT_VERSION_MINOR GREATER MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MAX)
        message(FATAL_ERROR
            "Framework minor ${PROJECT_VERSION_MINOR} is outside supported range "
            "[${MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MIN}, "
            "${MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MAX}] "
            "for generated codegen contract "
            "${MICROCORE_GENERATED_CODEGEN_CONTRACT_ID}@${MICROCORE_GENERATED_CODEGEN_CONTRACT_VERSION}."
        )
    endif()

    message(STATUS
        "Codegen artifact contract: "
        "${MICROCORE_GENERATED_CODEGEN_CONTRACT_ID}@${MICROCORE_GENERATED_CODEGEN_CONTRACT_VERSION} "
        "(generator ${MICROCORE_GENERATED_CODEGEN_VERSION}, "
        "framework ${MICROCORE_GENERATED_FRAMEWORK_VERSION})"
    )
endfunction()
