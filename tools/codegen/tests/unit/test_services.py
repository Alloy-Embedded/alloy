"""
Unit tests for CLI services (MCU, Board, Metadata, Config).
"""

import pytest
import yaml
import json
from pathlib import Path


# Import services
import sys
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.services.metadata_service import MetadataService, MetadataType, ValidationSeverity
from cli.loaders.config_loader import ConfigLoader
from cli.models.config import AlloyConfig


class TestMetadataService:
    """Test MetadataService validation and operations."""

    def test_validate_valid_mcu_yaml(self, sample_mcu_yaml):
        """Test validation of valid MCU YAML file."""
        service = MetadataService()
        result = service.validate_file(sample_mcu_yaml, metadata_type=MetadataType.MCU)

        assert not result.has_errors()
        assert result.error_count() == 0

    def test_validate_valid_board_yaml(self, sample_board_yaml):
        """Test validation of valid board YAML file."""
        service = MetadataService()
        result = service.validate_file(sample_board_yaml, metadata_type=MetadataType.BOARD)

        assert not result.has_errors()
        assert result.error_count() == 0

    def test_validate_missing_file(self, temp_dir):
        """Test validation of non-existent file."""
        service = MetadataService()
        result = service.validate_file(temp_dir / "nonexistent.yaml")

        assert result.has_errors()
        assert result.error_count() == 1

    def test_validate_invalid_yaml_syntax(self, temp_dir):
        """Test validation of file with YAML syntax error."""
        bad_yaml = temp_dir / "bad.yaml"
        with open(bad_yaml, 'w') as f:
            f.write("invalid: [unclosed\n")

        service = MetadataService()
        result = service.validate_file(bad_yaml)

        assert result.has_errors()

    def test_validate_missing_required_fields(self, temp_dir):
        """Test validation catches missing required fields."""
        incomplete = temp_dir / "incomplete.yaml"
        data = {
            "schema_version": "1.0",
            "family": {
                "id": "test"
                # Missing required: vendor, display_name
            }
        }
        with open(incomplete, 'w') as f:
            yaml.safe_dump(data, f)

        service = MetadataService()
        result = service.validate_file(incomplete, metadata_type=MetadataType.MCU)

        assert result.has_errors()
        assert result.error_count() >= 1

    def test_validate_strict_mode(self, temp_dir):
        """Test strict mode catches empty strings."""
        yaml_with_empty = temp_dir / "empty.yaml"
        data = {
            "schema_version": "1.0",
            "family": {
                "id": "test",
                "vendor": "",  # Empty string
                "display_name": "Test"
            }
        }
        with open(yaml_with_empty, 'w') as f:
            yaml.safe_dump(data, f)

        service = MetadataService()
        result = service.validate_file(yaml_with_empty, strict=True, metadata_type=MetadataType.MCU)

        assert result.warning_count() >= 1

    def test_create_mcu_template_yaml(self, temp_dir):
        """Test creating MCU template in YAML format."""
        service = MetadataService()
        output = temp_dir / "new_mcu.yaml"

        created = service.create_template(MetadataType.MCU, output_path=output, format="yaml")

        assert created.exists()
        assert created == output

        # Verify it's valid YAML
        with open(created, 'r') as f:
            data = yaml.safe_load(f)

        assert "schema_version" in data
        assert "family" in data
        assert "mcus" in data

    def test_create_board_template_json(self, temp_dir):
        """Test creating board template in JSON format."""
        service = MetadataService()
        output = temp_dir / "new_board.json"

        created = service.create_template(MetadataType.BOARD, output_path=output, format="json")

        assert created.exists()

        # Verify it's valid JSON
        with open(created, 'r') as f:
            data = json.load(f)

        assert "board" in data
        assert "mcu" in data

    def test_diff_identical_files(self, sample_mcu_yaml, temp_dir):
        """Test diff of two identical files."""
        # Copy file
        import shutil
        copy = temp_dir / "mcu_copy.yaml"
        shutil.copy(sample_mcu_yaml, copy)

        service = MetadataService()
        diff = service.diff_files(sample_mcu_yaml, copy)

        assert len(diff["added"]) == 0
        assert len(diff["removed"]) == 0
        assert len(diff["modified"]) == 0

    def test_diff_modified_files(self, temp_dir):
        """Test diff shows modifications."""
        file1 = temp_dir / "v1.yaml"
        file2 = temp_dir / "v2.yaml"

        data1 = {"version": 1, "name": "test"}
        data2 = {"version": 2, "name": "test"}  # version changed

        with open(file1, 'w') as f:
            yaml.safe_dump(data1, f)
        with open(file2, 'w') as f:
            yaml.safe_dump(data2, f)

        service = MetadataService()
        diff = service.diff_files(file1, file2)

        assert "version" in diff["modified"]

    def test_diff_added_removed_fields(self, temp_dir):
        """Test diff shows added and removed fields."""
        file1 = temp_dir / "v1.yaml"
        file2 = temp_dir / "v2.yaml"

        data1 = {"field1": "value1", "field2": "value2"}
        data2 = {"field1": "value1", "field3": "value3"}  # field2 removed, field3 added

        with open(file1, 'w') as f:
            yaml.safe_dump(data1, f)
        with open(file2, 'w') as f:
            yaml.safe_dump(data2, f)

        service = MetadataService()
        diff = service.diff_files(file1, file2)

        assert "field2" in diff["removed"]
        assert "field3" in diff["added"]

    def test_auto_detect_metadata_type_mcu(self, sample_mcu_yaml):
        """Test auto-detection of MCU metadata type."""
        service = MetadataService()
        with open(sample_mcu_yaml, 'r') as f:
            data = yaml.safe_load(f)

        mtype = service._detect_metadata_type(sample_mcu_yaml, data)
        assert mtype == MetadataType.MCU

    def test_auto_detect_metadata_type_board(self, sample_board_yaml):
        """Test auto-detection of board metadata type."""
        service = MetadataService()
        with open(sample_board_yaml, 'r') as f:
            data = yaml.safe_load(f)

        mtype = service._detect_metadata_type(sample_board_yaml, data)
        assert mtype == MetadataType.BOARD


class TestConfigLoader:
    """Test ConfigLoader hierarchical configuration."""

    def test_load_default_config(self):
        """Test loading default configuration."""
        config = AlloyConfig.create_default()

        assert config.general.output_format == "table"
        assert config.general.color is True
        assert config.paths.database == "./database"

    def test_load_config_from_file(self, sample_config_yaml):
        """Test loading config from YAML file."""
        loader = ConfigLoader(project_dir=sample_config_yaml.parent)
        config = loader.load()

        # Should have loaded project config
        assert config.general.verbose is True
        assert config.general.color is True

    def test_merge_configs(self):
        """Test merging two configs (higher priority wins)."""
        base = AlloyConfig.create_default()
        override = AlloyConfig(
            general={"verbose": True, "color": False}
        )

        merged = base.merge(override)

        assert merged.general.verbose is True
        assert merged.general.color is False
        # Other values should be from base
        assert merged.general.output_format == "table"

    def test_environment_variable_conversion(self):
        """Test environment variable type conversion."""
        loader = ConfigLoader()

        # Test boolean conversion
        assert loader._convert_env_value("true") is True
        assert loader._convert_env_value("1") is True
        assert loader._convert_env_value("false") is False
        assert loader._convert_env_value("0") is False

        # Test integer conversion
        assert loader._convert_env_value("42") == 42

        # Test string fallback
        assert loader._convert_env_value("test") == "test"

    def test_save_config(self, temp_dir):
        """Test saving configuration to file."""
        config = AlloyConfig.create_default()
        config.general.verbose = True

        loader = ConfigLoader(project_dir=temp_dir)
        saved_path = loader.save(config, scope="project")

        assert saved_path.exists()

        # Verify saved content
        with open(saved_path, 'r') as f:
            data = yaml.safe_load(f)

        assert data["general"]["verbose"] is True

    def test_find_project_config_current_dir(self, temp_dir, sample_config_yaml):
        """Test finding project config in current directory."""
        found = ConfigLoader.find_project_config(sample_config_yaml.parent)

        assert found is not None
        assert found == sample_config_yaml

    def test_find_project_config_not_found(self, temp_dir):
        """Test finding project config when none exists."""
        found = ConfigLoader.find_project_config(temp_dir)

        assert found is None

    def test_config_to_yaml_dict(self):
        """Test converting config to YAML dict (excludes None values)."""
        config = AlloyConfig.create_default()
        config.discovery.default_vendor = "st"

        yaml_dict = config.to_yaml_dict()

        assert "schema_version" in yaml_dict
        assert "discovery" in yaml_dict
        assert yaml_dict["discovery"]["default_vendor"] == "st"


# Marker for pytest
pytestmark = pytest.mark.unit
