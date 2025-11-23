"""
Board Configuration Generator

Generates C++ board_config.hpp from YAML board configuration files.
Uses Jinja2 templates and JSON schema validation.
"""

from pathlib import Path
from typing import Optional
import jinja2
from ..loaders.board_yaml_loader import BoardYAMLLoader, BoardConfig
from ..core.logger import logger, print_success, print_error, print_warning, print_info


class BoardGenerator:
    """
    Generate C++ board configuration from YAML

    Usage:
        generator = BoardGenerator()
        generator.generate_board_config(
            yaml_path="boards/nucleo_f401re/board.yaml",
            output_path="boards/nucleo_f401re/board_config.hpp"
        )
    """

    def __init__(self, template_dir: Optional[Path] = None):
        """
        Initialize generator

        Args:
            template_dir: Directory containing Jinja2 templates
                         (default: tools/codegen/templates)
        """
        if template_dir is None:
            template_dir = Path(__file__).parents[2] / "templates"

        self.template_dir = template_dir
        self.loader = BoardYAMLLoader()

        # Setup Jinja2 environment
        self.jinja_env = jinja2.Environment(
            loader=jinja2.FileSystemLoader(str(template_dir)),
            trim_blocks=True,
            lstrip_blocks=True,
            keep_trailing_newline=True
        )

    def generate_board_config(
        self,
        yaml_path: Path | str,
        output_path: Optional[Path | str] = None
    ) -> str:
        """
        Generate board_config.hpp from YAML

        Args:
            yaml_path: Path to board.yaml file
            output_path: Output path for board_config.hpp
                        (default: same directory as yaml_path)

        Returns:
            Generated C++ code as string

        Raises:
            FileNotFoundError: If YAML file not found
            ValueError: If YAML validation fails
        """
        yaml_path = Path(yaml_path)

        # Load and validate YAML
        print_info(f"Loading board configuration: {yaml_path}")
        board_config = self.loader.load_board(yaml_path)

        # Generate C++ code
        print_info(f"Generating board_config.hpp for {board_config.board.name}")
        generated_code = self._render_template(board_config)

        # Write to file if output_path specified
        if output_path is None:
            output_path = yaml_path.parent / "board_config.hpp"
        else:
            output_path = Path(output_path)

        print_info(f"Writing to: {output_path}")
        output_path.write_text(generated_code)

        print_success(f"Generated {output_path}")
        return generated_code

    def _render_template(self, board_config: BoardConfig) -> str:
        """Render Jinja2 template with board configuration"""
        template = self.jinja_env.get_template("board_config.hpp.j2")
        return template.render(board=board_config)

    def validate_yaml(self, yaml_path: Path | str) -> bool:
        """
        Validate YAML file without generating code

        Args:
            yaml_path: Path to board.yaml file

        Returns:
            True if valid, False otherwise
        """
        try:
            yaml_path = Path(yaml_path)
            print_info(f"Validating: {yaml_path}")
            self.loader.load_board(yaml_path)
            print_success("Valid board configuration")
            return True
        except Exception as e:
            print_error(f"Validation failed: {e}")
            return False

    def generate_all_boards(self, boards_dir: Path | str) -> int:
        """
        Generate board_config.hpp for all boards with board.yaml

        Args:
            boards_dir: Root boards directory (e.g., "boards/")

        Returns:
            Number of boards successfully generated
        """
        boards_dir = Path(boards_dir)
        count = 0

        # Find all board.yaml files
        yaml_files = list(boards_dir.glob("*/board.yaml"))

        if not yaml_files:
            print_warning(f"No board.yaml files found in {boards_dir}")
            return 0

        print_info(f"Found {len(yaml_files)} board configurations")

        for yaml_file in yaml_files:
            try:
                self.generate_board_config(yaml_file)
                count += 1
            except Exception as e:
                print_error(f"Failed to generate {yaml_file.parent.name}: {e}")

        print_success(f"\nGenerated {count}/{len(yaml_files)} boards")
        return count


def main():
    """CLI entry point for board generator"""
    import argparse

    parser = argparse.ArgumentParser(
        description="Generate C++ board configuration from YAML"
    )
    parser.add_argument(
        "yaml_path",
        type=Path,
        help="Path to board.yaml file or boards directory"
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        help="Output path for board_config.hpp (default: same as YAML)"
    )
    parser.add_argument(
        "--validate-only",
        "-v",
        action="store_true",
        help="Only validate YAML without generating code"
    )
    parser.add_argument(
        "--all",
        "-a",
        action="store_true",
        help="Generate all boards in directory"
    )

    args = parser.parse_args()

    generator = BoardGenerator()

    try:
        if args.validate_only:
            # Validation only
            valid = generator.validate_yaml(args.yaml_path)
            exit(0 if valid else 1)

        elif args.all:
            # Generate all boards
            count = generator.generate_all_boards(args.yaml_path)
            exit(0 if count > 0 else 1)

        else:
            # Generate single board
            generator.generate_board_config(args.yaml_path, args.output)
            exit(0)

    except Exception as e:
        print_error(f"Error: {e}")
        exit(1)


if __name__ == "__main__":
    main()
