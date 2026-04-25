"""alloy-cli: user-facing entry point for the Alloy multi-vendor runtime."""

# `_version.py` is generated at build/install time by hatch-vcs from the most
# recent `vX.Y.Z` tag on the alloy repo. The fallback only kicks in when the
# package is imported from a checkout that has never been built or installed
# (which should be rare; even `pip install -e .` runs the hook).
try:
    from ._version import __version__  # type: ignore[no-redef]
except ImportError:  # pragma: no cover
    __version__ = "0.0.0+unknown"
