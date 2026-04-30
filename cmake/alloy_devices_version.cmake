# ==============================================================================
# alloy-devices version pin
# ==============================================================================
#
# Override ALLOY_DEVICES_VERSION on the cmake command line to pin a specific
# release:
#   cmake -DALLOY_DEVICES_VERSION=1.2.0 ...
#
# CI can override ALLOY_DEVICES_BASE_URL to point at a staging server or
# a local HTTP mirror without touching this file.
#

set(ALLOY_DEVICES_VERSION "1.0.0"
    CACHE STRING "alloy-devices release version to fetch from the package registry")
mark_as_advanced(ALLOY_DEVICES_VERSION)

set(ALLOY_DEVICES_BASE_URL
    "https://github.com/alloy-rs/alloy-devices/releases/download/alloy-devices-v${ALLOY_DEVICES_VERSION}"
    CACHE STRING "Base URL for alloy-devices package downloads (no trailing slash)")
mark_as_advanced(ALLOY_DEVICES_BASE_URL)
