"""
Configuration loader with hierarchical support.

Loads and merges configuration from multiple sources in priority order:
1. CLI arguments (highest priority)
2. Environment variables (ALLOY_*)
3. Project config (.alloy.yaml in current directory)
4. User config (~/.config/alloy/config.yaml)
5. System config (/etc/alloy/config.yaml)
6. Defaults (lowest priority)

Example:
    loader = ConfigLoader()
    config = loader.load()

    # Access config
    print(config.general.verbose)
    print(config.paths.database)
"""

import os
import yaml
from pathlib import Path
from typing import Optional, Dict, Any
from functools import lru_cache

from ..models.config import AlloyConfig


class ConfigLoader:
    """Load and merge Alloy configuration from multiple sources."""

    # Standard config file locations
    SYSTEM_CONFIG = Path("/etc/alloy/config.yaml")
    USER_CONFIG = Path.home() / ".config" / "alloy" / "config.yaml"
    PROJECT_CONFIG = Path(".alloy.yaml")

    def __init__(
        self,
        project_dir: Optional[Path] = None,
        cli_overrides: Optional[Dict[str, Any]] = None
    ):
        """
        Initialize config loader.

        Args:
            project_dir: Project directory to search for .alloy.yaml (default: cwd)
            cli_overrides: Dictionary of CLI argument overrides
        """
        self.project_dir = project_dir or Path.cwd()
        self.cli_overrides = cli_overrides or {}

    @lru_cache(maxsize=1)
    def load(self) -> AlloyConfig:
        """
        Load configuration from all sources and merge hierarchically.

        Returns:
            Merged AlloyConfig instance
        """
        # Start with defaults
        config = AlloyConfig.create_default()

        # Load and merge in priority order (lowest to highest)
        # 1. System config
        config = self._merge_file(config, self.SYSTEM_CONFIG)

        # 2. User config
        config = self._merge_file(config, self.USER_CONFIG)

        # 3. Project config
        project_config_path = self.project_dir / self.PROJECT_CONFIG.name
        config = self._merge_file(config, project_config_path)

        # 4. Environment variables
        config = self._merge_env(config)

        # 5. CLI overrides
        config = self._merge_cli(config)

        return config

    def _merge_file(self, base: AlloyConfig, path: Path) -> AlloyConfig:
        """
        Load YAML config file and merge into base config.

        Args:
            base: Base configuration
            path: Path to config file

        Returns:
            Merged configuration
        """
        if not path.exists():
            return base

        try:
            with open(path, 'r', encoding='utf-8') as f:
                data = yaml.safe_load(f)

            if not data:
                return base

            # Create config from file data
            file_config = AlloyConfig(**data)

            # Merge (file_config takes precedence)
            return base.merge(file_config)

        except Exception as e:
            # Log warning but don't fail
            print(f"Warning: Failed to load config from {path}: {e}")
            return base

    def _merge_env(self, base: AlloyConfig) -> AlloyConfig:
        """
        Merge environment variables into config.

        Environment variables use ALLOY_ prefix:
        - ALLOY_VERBOSE=1 -> general.verbose = True
        - ALLOY_DATABASE_PATH=/path -> paths.database = "/path"
        - ALLOY_OUTPUT_FORMAT=json -> general.output_format = "json"

        Args:
            base: Base configuration

        Returns:
            Merged configuration
        """
        env_data = {}

        # Map environment variables to config paths
        env_mappings = {
            # General
            "ALLOY_OUTPUT_FORMAT": ("general", "output_format"),
            "ALLOY_COLOR": ("general", "color"),
            "ALLOY_VERBOSE": ("general", "verbose"),
            "ALLOY_EDITOR": ("general", "editor"),

            # Paths
            "ALLOY_DATABASE_PATH": ("paths", "database"),
            "ALLOY_TEMPLATES_PATH": ("paths", "templates"),
            "ALLOY_SVD_PATH": ("paths", "svd"),
            "ALLOY_OUTPUT_PATH": ("paths", "output"),

            # Discovery
            "ALLOY_DEFAULT_VENDOR": ("discovery", "default_vendor"),
            "ALLOY_DEFAULT_FAMILY": ("discovery", "default_family"),
            "ALLOY_SHOW_DEPRECATED": ("discovery", "show_deprecated"),
            "ALLOY_MAX_RESULTS": ("discovery", "max_results"),

            # Build
            "ALLOY_BUILD_SYSTEM": ("build", "system"),
            "ALLOY_BUILD_TYPE": ("build", "build_type"),
            "ALLOY_BUILD_JOBS": ("build", "jobs"),

            # Validation
            "ALLOY_VALIDATION_STRICT": ("validation", "strict"),
            "ALLOY_CLANG_PATH": ("validation", "clang_path"),
            "ALLOY_GCC_ARM_PATH": ("validation", "gcc_arm_path"),

            # Metadata
            "ALLOY_METADATA_FORMAT": ("metadata", "format"),
        }

        # Build nested dict from environment variables
        for env_var, (section, key) in env_mappings.items():
            value = os.getenv(env_var)
            if value is not None:
                if section not in env_data:
                    env_data[section] = {}

                # Convert string to appropriate type
                env_data[section][key] = self._convert_env_value(value)

        if not env_data:
            return base

        # Create config from env data
        env_config = AlloyConfig(**env_data)

        # Merge
        return base.merge(env_config)

    def _merge_cli(self, base: AlloyConfig) -> AlloyConfig:
        """
        Merge CLI overrides into config.

        Args:
            base: Base configuration

        Returns:
            Merged configuration
        """
        if not self.cli_overrides:
            return base

        # Create config from CLI overrides
        cli_config = AlloyConfig(**self.cli_overrides)

        # Merge (CLI takes precedence)
        return base.merge(cli_config)

    @staticmethod
    def _convert_env_value(value: str) -> Any:
        """
        Convert environment variable string to appropriate Python type.

        Args:
            value: String value from environment

        Returns:
            Converted value (bool, int, or str)
        """
        # Boolean conversion
        if value.lower() in ("1", "true", "yes", "on"):
            return True
        if value.lower() in ("0", "false", "no", "off"):
            return False

        # Integer conversion
        try:
            return int(value)
        except ValueError:
            pass

        # String (default)
        return value

    def save(self, config: AlloyConfig, scope: str = "user") -> Path:
        """
        Save configuration to file.

        Args:
            config: Configuration to save
            scope: Where to save ("user", "project", "system")

        Returns:
            Path where config was saved

        Raises:
            ValueError: If scope is invalid
        """
        if scope == "user":
            path = self.USER_CONFIG
        elif scope == "project":
            path = self.project_dir / self.PROJECT_CONFIG.name
        elif scope == "system":
            path = self.SYSTEM_CONFIG
        else:
            raise ValueError(f"Invalid scope: {scope}. Must be 'user', 'project', or 'system'")

        # Ensure directory exists
        path.parent.mkdir(parents=True, exist_ok=True)

        # Convert to YAML dict and save
        data = config.to_yaml_dict()

        with open(path, 'w', encoding='utf-8') as f:
            yaml.safe_dump(
                data,
                f,
                default_flow_style=False,
                sort_keys=False,
                allow_unicode=True
            )

        return path

    @staticmethod
    def find_project_config(start_dir: Optional[Path] = None) -> Optional[Path]:
        """
        Search for .alloy.yaml in current directory and parent directories.

        Args:
            start_dir: Starting directory (default: cwd)

        Returns:
            Path to .alloy.yaml if found, None otherwise
        """
        current = start_dir or Path.cwd()

        # Search up to root
        for directory in [current] + list(current.parents):
            config_path = directory / ".alloy.yaml"
            if config_path.exists():
                return config_path

        return None


# Global config instance (lazy loaded)
_global_config: Optional[AlloyConfig] = None


def get_config(reload: bool = False) -> AlloyConfig:
    """
    Get global configuration instance.

    Args:
        reload: Force reload configuration from disk

    Returns:
        Global AlloyConfig instance
    """
    global _global_config

    if _global_config is None or reload:
        loader = ConfigLoader()
        _global_config = loader.load()

    return _global_config


def set_config(config: AlloyConfig):
    """
    Set global configuration instance.

    Args:
        config: Configuration to set as global
    """
    global _global_config
    _global_config = config
