#!/usr/bin/env python3
"""
List all available SVD files (upstream + custom)

Usage:
    python3 list_svds.py
    python3 list_svds.py --vendor STMicro
    python3 list_svds.py --custom-only
"""

import argparse
from collections import defaultdict
from svd_discovery import discover_all_svds, Colors

def main():
    parser = argparse.ArgumentParser(description="List all available SVD files")
    parser.add_argument("--vendor", help="Filter by vendor name")
    parser.add_argument("--custom-only", action="store_true", help="Show only custom SVDs")
    parser.add_argument("--upstream-only", action="store_true", help="Show only upstream SVDs")
    args = parser.parse_args()

    # Discover all SVDs
    all_svds = discover_all_svds()

    # Filter by source
    if args.custom_only:
        all_svds = {name: svd for name, svd in all_svds.items() if svd.source == "custom-svd"}
    elif args.upstream_only:
        all_svds = {name: svd for name, svd in all_svds.items() if svd.source == "upstream"}

    # Filter by vendor
    if args.vendor:
        all_svds = {name: svd for name, svd in all_svds.items()
                   if args.vendor.lower() in svd.vendor.lower()}

    # Group by source and vendor
    upstream_by_vendor = defaultdict(list)
    custom_by_vendor = defaultdict(list)

    for device_name, svd in sorted(all_svds.items()):
        if svd.source == "upstream":
            upstream_by_vendor[svd.vendor].append((device_name, svd))
        else:
            custom_by_vendor[svd.vendor].append((device_name, svd))

    # Print results
    print("\n📦 Available SVD Files:\n")

    # Print upstream SVDs
    if upstream_by_vendor and not args.custom_only:
        print(f"{Colors.INFO}Upstream (cmsis-svd-data):{Colors.ENDC}")
        upstream_count = 0
        for vendor in sorted(upstream_by_vendor.keys()):
            devices = upstream_by_vendor[vendor]
            print(f"\n  {vendor}:")
            for device_name, svd in devices[:5]:  # Show first 5 per vendor
                print(f"    ✓ {device_name}")
                upstream_count += 1

            if len(devices) > 5:
                print(f"    ... ({len(devices) - 5} more)")
                upstream_count += len(devices) - 5

        print(f"\n  Total upstream: {sum(len(d) for d in upstream_by_vendor.values())}")

    # Print custom SVDs
    if custom_by_vendor and not args.upstream_only:
        if upstream_by_vendor:
            print()  # Separator
        print(f"{Colors.INFO}Custom (custom-svd/):{Colors.ENDC}")
        for vendor in sorted(custom_by_vendor.keys()):
            devices = custom_by_vendor[vendor]
            print(f"\n  {vendor}:")
            for device_name, svd in devices:
                # Check if it overrides upstream
                upstream_exists = device_name in [d[0] for d in upstream_by_vendor.get(vendor, [])]
                override_marker = f" {Colors.WARNING}[Overrides upstream]{Colors.ENDC}" if upstream_exists else " [New device]"
                print(f"    ✓ {device_name}{override_marker}")

        print(f"\n  Total custom: {sum(len(d) for d in custom_by_vendor.values())}")

    # Summary
    total_count = len(all_svds)
    upstream_total = sum(len(d) for d in upstream_by_vendor.values())
    custom_total = sum(len(d) for d in custom_by_vendor.values())

    print(f"\n{Colors.INFO}Total: {total_count} SVDs ", end="")
    if not args.custom_only and not args.upstream_only:
        overrides = sum(1 for svd in all_svds.values()
                       if svd.source == "custom-svd" and svd.device_name in
                       [s.device_name for s in all_svds.values() if s.source == "upstream"])
        print(f"({upstream_total} upstream, {custom_total} custom", end="")
        if overrides > 0:
            print(f", {overrides} overrides", end="")
        print(f"){Colors.ENDC}")
    else:
        print(f"{Colors.ENDC}")

    if custom_total == 0 and not args.upstream_only:
        print(f"\n{Colors.INFO}ℹ  No custom SVDs found. Add SVD files to tools/codegen/custom-svd/vendors/{Colors.ENDC}")

if __name__ == "__main__":
    main()
