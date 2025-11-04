#!/usr/bin/env python3
"""
SVD Discovery Module - Multi-source SVD file discovery with merge policy

This module discovers SVD files from multiple sources (upstream and custom-svd)
and merges them according to the merge policy.
"""

import json
from pathlib import Path
from typing import Dict, List, Optional
from dataclasses import dataclass
import xml.etree.ElementTree as ET

CODEGEN_DIR = Path(__file__).parent.parent.parent
UPSTREAM_SVD_DIR = CODEGEN_DIR / "upstream" / "cmsis-svd-data" / "data"
CUSTOM_SVD_DIR = CODEGEN_DIR / "custom-svd" / "vendors"
MERGE_POLICY_FILE = CODEGEN_DIR / "custom-svd" / "merge_policy.json"


@dataclass
class SVDFile:
    """Information about an SVD file"""
    device_name: str
    file_path: Path
    source: str  # "upstream" or "custom-svd"
    priority: int  # Higher number = higher priority
    vendor: str
    version: Optional[str] = None


class Colors:
    WARNING = '\033[93m'
    INFO = '\033[96m'
    ENDC = '\033[0m'


def load_merge_policy() -> dict:
    """Load merge policy from JSON file"""
    if not MERGE_POLICY_FILE.exists():
        # Default policy if file doesn't exist
        return {
            "priority": {
                "order": ["custom-svd", "upstream"]
            },
            "conflict_resolution": {
                "duplicate_files": {"action": "prefer_custom"}
            },
            "validation": {
                "require_valid_xml": {"enabled": True}
            },
            "logging": {
                "show_duplicate_warnings": True
            }
        }

    with open(MERGE_POLICY_FILE, 'r') as f:
        return json.load(f)


def extract_device_name(svd_path: Path) -> Optional[str]:
    """Extract device name from SVD file"""
    try:
        tree = ET.parse(svd_path)
        root = tree.getroot()

        # Find <device><name> element
        device_elem = root.find('device')
        if device_elem is not None:
            name_elem = device_elem.find('name')
            if name_elem is not None and name_elem.text:
                return name_elem.text.strip()

        # Fallback: use filename without extension
        return svd_path.stem
    except Exception:
        # If XML parsing fails, use filename
        return svd_path.stem


def extract_version(svd_path: Path) -> Optional[str]:
    """Extract version from SVD file if available"""
    try:
        tree = ET.parse(svd_path)
        root = tree.getroot()
        device_elem = root.find('device')
        if device_elem is not None:
            version_elem = device_elem.find('version')
            if version_elem is not None and version_elem.text:
                return version_elem.text.strip()
    except Exception:
        pass
    return None


def validate_svd(svd_path: Path, policy: dict) -> tuple[bool, Optional[str]]:
    """Validate SVD file according to policy

    Returns:
        (is_valid, error_message)
    """
    validation = policy.get("validation", {})

    # Check XML validity
    if validation.get("require_valid_xml", {}).get("enabled", True):
        try:
            ET.parse(svd_path)
        except ET.ParseError as e:
            return False, f"Invalid XML: {e}"

    # Check device name
    if validation.get("require_device_name", {}).get("enabled", True):
        device_name = extract_device_name(svd_path)
        if not device_name:
            return False, "Missing <device><name> element"

    # Check file size
    size_check = validation.get("check_file_size", {})
    if size_check.get("enabled", False):
        size = svd_path.stat().st_size
        min_size = size_check.get("min_bytes", 1024)
        max_size = size_check.get("max_bytes", 10485760)
        if size < min_size:
            return False, f"File too small ({size} bytes < {min_size} bytes)"
        if size > max_size:
            return False, f"File too large ({size} bytes > {max_size} bytes)"

    return True, None


def scan_upstream_svds() -> List[SVDFile]:
    """Scan upstream cmsis-svd-data for SVD files"""
    svds = []

    if not UPSTREAM_SVD_DIR.exists():
        return svds

    # Scan all vendor directories
    for vendor_dir in UPSTREAM_SVD_DIR.iterdir():
        if not vendor_dir.is_dir():
            continue

        vendor_name = vendor_dir.name

        # Find all .svd files
        for svd_file in vendor_dir.glob("*.svd"):
            device_name = extract_device_name(svd_file)
            version = extract_version(svd_file)

            if device_name:
                svds.append(SVDFile(
                    device_name=device_name,
                    file_path=svd_file,
                    source="upstream",
                    priority=1,  # Lower priority
                    vendor=vendor_name,
                    version=version
                ))

    return svds


def scan_custom_svds(policy: dict) -> List[SVDFile]:
    """Scan custom-svd directory for SVD files"""
    svds = []

    if not CUSTOM_SVD_DIR.exists():
        return svds

    # Scan all vendor subdirectories
    for vendor_dir in CUSTOM_SVD_DIR.iterdir():
        if not vendor_dir.is_dir():
            continue

        vendor_name = vendor_dir.name

        # Find all .svd files
        for svd_file in vendor_dir.glob("*.svd"):
            # Validate SVD file
            is_valid, error_msg = validate_svd(svd_file, policy)
            if not is_valid:
                if policy.get("logging", {}).get("show_duplicate_warnings", True):
                    print(f"{Colors.WARNING}⚠{Colors.ENDC} Skipping {svd_file.name}: {error_msg}")
                continue

            device_name = extract_device_name(svd_file)
            version = extract_version(svd_file)

            if device_name:
                svds.append(SVDFile(
                    device_name=device_name,
                    file_path=svd_file,
                    source="custom-svd",
                    priority=2,  # Higher priority
                    vendor=vendor_name,
                    version=version
                ))

    return svds


def merge_svd_lists(upstream: List[SVDFile], custom: List[SVDFile], policy: dict) -> Dict[str, SVDFile]:
    """Merge upstream and custom SVD lists according to policy

    Returns:
        Dictionary mapping device_name to selected SVDFile
    """
    merged = {}
    show_warnings = policy.get("logging", {}).get("show_duplicate_warnings", True)

    # Add upstream SVDs first (lower priority)
    for svd in upstream:
        merged[svd.device_name] = svd

    # Add custom SVDs (higher priority, may override)
    for svd in custom:
        if svd.device_name in merged:
            # Conflict detected
            upstream_svd = merged[svd.device_name]

            # Check version-based resolution
            conflict_policy = policy.get("conflict_resolution", {})
            version_policy = conflict_policy.get("version_mismatch", {}).get("action")

            selected_svd = svd  # Default: prefer custom

            if version_policy == "use_newer_version" and upstream_svd.version and svd.version:
                # Compare versions (simple string comparison)
                if upstream_svd.version > svd.version:
                    selected_svd = upstream_svd

            if show_warnings:
                print(f"\n{Colors.WARNING}⚠  WARNING: Duplicate SVD detected for {svd.device_name}{Colors.ENDC}")
                print(f"   - Upstream: {upstream_svd.file_path.relative_to(SCRIPT_DIR)}")
                print(f"   - Custom:   {svd.file_path.relative_to(SCRIPT_DIR)}")
                print(f"   Using: {selected_svd.source} (priority={selected_svd.priority})\n")

            merged[svd.device_name] = selected_svd
        else:
            # No conflict, add custom SVD
            merged[svd.device_name] = svd

    return merged


def discover_all_svds() -> Dict[str, SVDFile]:
    """Main entry point: discover all SVD files from all sources

    Returns:
        Dictionary mapping device_name to SVDFile
    """
    # Load merge policy
    policy = load_merge_policy()

    # Scan both sources
    upstream_svds = scan_upstream_svds()
    custom_svds = scan_custom_svds(policy)

    # Merge according to policy
    merged_svds = merge_svd_lists(upstream_svds, custom_svds, policy)

    return merged_svds


def get_svd_by_device(device_name: str) -> Optional[SVDFile]:
    """Get SVD file for a specific device"""
    all_svds = discover_all_svds()
    return all_svds.get(device_name)


def list_all_devices() -> List[str]:
    """Get list of all supported device names"""
    all_svds = discover_all_svds()
    return sorted(all_svds.keys())


if __name__ == "__main__":
    # Simple test/demo
    print("Discovering SVD files...")
    svds = discover_all_svds()

    print(f"\nTotal devices found: {len(svds)}")

    # Count by source
    upstream_count = sum(1 for svd in svds.values() if svd.source == "upstream")
    custom_count = sum(1 for svd in svds.values() if svd.source == "custom-svd")

    print(f"  Upstream: {upstream_count}")
    print(f"  Custom:   {custom_count}")

    # Show first 10 devices
    print("\nFirst 10 devices:")
    for i, device_name in enumerate(sorted(svds.keys())[:10]):
        svd = svds[device_name]
        print(f"  {i+1}. {device_name} ({svd.source}, vendor={svd.vendor})")
