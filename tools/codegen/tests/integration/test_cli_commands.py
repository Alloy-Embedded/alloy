"""
Integration tests for CLI commands.
"""

import pytest
from pathlib import Path
from typer.testing import CliRunner

# Import CLI app
import sys
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.main import app

runner = CliRunner()


class TestCLICommands:
    """Test CLI commands end-to-end."""

    def test_version_command(self):
        """Test alloy version command."""
        result = runner.invoke(app, ["version"])

        assert result.exit_code == 0
        assert "Alloy CLI" in result.stdout
        assert "v2.0.0" in result.stdout

    def test_config_show_command(self):
        """Test alloy config show command."""
        result = runner.invoke(app, ["config", "show"])

        # Should not error
        assert result.exit_code == 0

    def test_config_path_command(self):
        """Test alloy config path command."""
        result = runner.invoke(app, ["config", "path"])

        assert result.exit_code == 0
        assert "User" in result.stdout or "Project" in result.stdout

    def test_metadata_create_command(self, temp_dir):
        """Test alloy metadata create command."""
        output = temp_dir / "test_mcu.yaml"
        result = runner.invoke(app, ["metadata", "create", "mcu", "--output", str(output)])

        assert result.exit_code == 0
        assert output.exists()

    def test_metadata_validate_valid_file(self, sample_mcu_yaml):
        """Test alloy metadata validate with valid file."""
        result = runner.invoke(app, ["metadata", "validate", str(sample_mcu_yaml)])

        assert result.exit_code == 0
        assert "✓" in result.stdout or "passed" in result.stdout.lower()

    def test_metadata_validate_missing_file(self):
        """Test alloy metadata validate with missing file."""
        result = runner.invoke(app, ["metadata", "validate", "nonexistent.yaml"])

        assert result.exit_code == 1

    def test_metadata_diff_command(self, sample_mcu_yaml, temp_dir):
        """Test alloy metadata diff command."""
        # Copy file
        import shutil
        copy = temp_dir / "mcu_copy.yaml"
        shutil.copy(sample_mcu_yaml, copy)

        result = runner.invoke(app, ["metadata", "diff", str(sample_mcu_yaml), str(copy)])

        assert result.exit_code == 0
        assert "identical" in result.stdout.lower() or "0 changes" in result.stdout.lower()

    def test_config_init_command(self, temp_dir):
        """Test alloy config init command."""
        # Change to temp dir
        import os
        old_cwd = os.getcwd()
        os.chdir(temp_dir)

        try:
            result = runner.invoke(app, ["config", "init"])

            assert result.exit_code == 0
            assert (temp_dir / ".alloy.yaml").exists()
        finally:
            os.chdir(old_cwd)


# Marker for pytest
pytestmark = pytest.mark.integration
