"""
Configuration models for Alloy CLI.

Defines the structure and validation for .alloy.yaml configuration files.
Configuration hierarchy: CLI args > env vars > project > user > system > defaults
"""

from typing import Optional, List
from pydantic import BaseModel, Field
from pathlib import Path


class GeneralConfig(BaseModel):
    """General CLI settings."""
    output_format: str = Field(default="table", description="Output format (table, json, yaml, compact)")
    color: bool = Field(default=True, description="Enable colored output")
    verbose: bool = Field(default=False, description="Enable verbose logging")
    editor: str = Field(default="vim", description="Editor for 'alloy config edit'")


class PathsConfig(BaseModel):
    """Path configuration."""
    database: str = Field(default="./database", description="Database directory")
    templates: str = Field(default="./database/templates", description="Templates directory")
    svd: str = Field(default="./svd", description="SVD files directory")
    output: str = Field(default="./generated", description="Output directory for generated code")

    def resolve_database(self) -> Path:
        """Resolve database path, expanding environment variables."""
        return Path(self.database).expanduser().resolve()

    def resolve_templates(self) -> Path:
        """Resolve templates path."""
        return Path(self.templates).expanduser().resolve()

    def resolve_svd(self) -> Path:
        """Resolve SVD path."""
        return Path(self.svd).expanduser().resolve()

    def resolve_output(self) -> Path:
        """Resolve output path."""
        return Path(self.output).expanduser().resolve()


class DiscoveryConfig(BaseModel):
    """Discovery settings."""
    default_vendor: Optional[str] = Field(default=None, description="Default vendor filter")
    default_family: Optional[str] = Field(default=None, description="Default MCU family")
    show_deprecated: bool = Field(default=False, description="Show deprecated MCUs/boards")
    max_results: int = Field(default=50, description="Maximum results in list commands")
    default_sort: str = Field(default="part_number", description="Default sort order")


class BuildConfig(BaseModel):
    """Build system settings."""
    system: str = Field(default="cmake", description="Build system (cmake, meson)")
    build_type: str = Field(default="Debug", description="Build type")
    jobs: int = Field(default=0, description="Parallel jobs (0=auto)")
    toolchain: Optional[str] = Field(default=None, description="Toolchain file path")
    cmake_generator: Optional[str] = Field(default=None, description="CMake generator")


class ValidationConfig(BaseModel):
    """Validation settings."""
    strict: bool = Field(default=False, description="Enable strict validation")
    syntax: bool = Field(default=True, description="Enable syntax validation")
    semantic: bool = Field(default=True, description="Enable semantic validation")
    compile: bool = Field(default=False, description="Enable compilation check")
    clang_path: str = Field(default="clang++", description="Clang path")
    gcc_arm_path: str = Field(default="arm-none-eabi-gcc", description="GCC ARM path")


class MetadataConfig(BaseModel):
    """Metadata settings."""
    format: str = Field(default="yaml", description="Preferred metadata format (yaml, json)")
    auto_format: bool = Field(default=True, description="Auto-format on save")
    include_comments: bool = Field(default=True, description="Include comments in metadata")


class ProjectConfig(BaseModel):
    """Project-specific settings (only in project .alloy.yaml)."""
    name: Optional[str] = Field(default=None, description="Project name")
    board: Optional[str] = Field(default=None, description="Target board")
    mcu: Optional[str] = Field(default=None, description="Target MCU")
    peripherals: List[str] = Field(default_factory=list, description="Configured peripherals")
    build_dir: str = Field(default="build", description="Build directory")


class AlloyConfig(BaseModel):
    """Root configuration object for Alloy CLI."""

    schema_version: str = Field(default="1.0", description="Config schema version")

    general: GeneralConfig = Field(default_factory=GeneralConfig)
    paths: PathsConfig = Field(default_factory=PathsConfig)
    discovery: DiscoveryConfig = Field(default_factory=DiscoveryConfig)
    build: BuildConfig = Field(default_factory=BuildConfig)
    validation: ValidationConfig = Field(default_factory=ValidationConfig)
    metadata: MetadataConfig = Field(default_factory=MetadataConfig)
    project: ProjectConfig = Field(default_factory=ProjectConfig)

    @classmethod
    def create_default(cls) -> "AlloyConfig":
        """Create default configuration with all defaults."""
        return cls()

    def merge(self, other: "AlloyConfig") -> "AlloyConfig":
        """
        Merge another config into this one.
        Values from 'other' take precedence over 'self'.
        """
        # Create new config by merging field by field
        merged_data = self.model_dump()
        other_data = other.model_dump()

        # Merge nested dictionaries
        for section, values in other_data.items():
            if section == "schema_version":
                continue
            if isinstance(values, dict) and section in merged_data:
                # Merge nested config sections
                for key, value in values.items():
                    if value is not None:  # Only override if explicitly set
                        merged_data[section][key] = value

        return AlloyConfig(**merged_data)

    def to_yaml_dict(self) -> dict:
        """
        Convert config to YAML-serializable dictionary.
        Excludes None values and empty lists for cleaner output.
        """
        data = self.model_dump(exclude_none=True)

        # Remove empty sections
        cleaned = {}
        for section, values in data.items():
            if section == "schema_version":
                cleaned[section] = values
                continue

            if isinstance(values, dict):
                # Remove None and empty values
                section_data = {
                    k: v for k, v in values.items()
                    if v is not None and v != [] and v != ""
                }
                if section_data:  # Only include non-empty sections
                    cleaned[section] = section_data

        return cleaned
