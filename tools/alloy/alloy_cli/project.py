"""Project, framework and database discovery.

A project is a directory with an ``alloy.toml``. The framework root (this
repo) is found by walking up from the project (in-repo examples) or via
``ALLOY_ROOT``. The device database defaults to the sibling ``alloy-devices``
checkout or ``ALLOY_DEVICES_ROOT``. Wheel-based distribution replaces this
discovery later without changing callers.
"""

from __future__ import annotations

import json
import os
import tomllib
from dataclasses import dataclass
from pathlib import Path
from typing import Any


class ProjectError(RuntimeError):
    pass


@dataclass(frozen=True)
class Project:
    root: Path
    name: str
    board_id: str
    alloy_root: Path
    devices_root: Path
    # The selected PRODUCT (products/<id>.toml), or None for a project with no
    # product dimension. Resolved HERE — in load_project, from [product] name
    # or --product — never in per-verb code, so build/size/frame-audit/
    # debug-info always agree about which product a tree belongs to (the same
    # lesson apply_project_overrides records: per-verb resolution once let the
    # header say 48 MHz while the panel said 64).
    product_id: str | None = None

    # Per-configuration trees: switching boards must never reuse another
    # board's CMake cache (cached CPU flags) or stale generated headers — and
    # switching PRODUCTS must never serve product A's product.hpp or compiled
    # objects as product B's firmware, so the product joins the key. No
    # product -> the bare board id, so existing trees stay valid.
    @property
    def _tree_key(self) -> str:
        return self.board_id if self.product_id is None \
            else f"{self.board_id}+{self.product_id}"

    @property
    def gen_dir(self) -> Path:
        return self.root / ".alloy" / "generated" / self._tree_key

    @property
    def build_dir(self) -> Path:
        return self.root / ".alloy" / "build-tree" / self._tree_key

    @property
    def board_json(self) -> Path:
        # A project may carry its OWN board (a custom/clean board scaffolded from
        # a chosen MCU): ./boards/<id>/board.json wins over a framework board of
        # the same id, so users can build for any chip without touching the
        # framework. Falls back to the curated framework board.
        local = self.root / "boards" / self.board_id / "board.json"
        return local if local.exists() else self.alloy_root / "boards" / self.board_id / "board.json"

    def lib_includes(self) -> list[Path]:
        """Include dirs of libraries listed in [libs].

        `alloy lib add` copies a library into ./libs/<name> and records it in
        alloy.toml; the build adds each one's include/ dir so `#include <name.hpp>`
        resolves. A project WITHOUT a vendored copy falls back to the framework's
        own libs/<name> — the same local-wins-framework-covers rule board_json
        uses above, and what lets the in-tree examples consume a library without
        duplicating it into every example directory. Entries found in neither
        place are skipped (a stale toml never breaks the build).
        """
        toml_path = self.root / "alloy.toml"
        if not toml_path.exists():
            return []
        data = tomllib.loads(toml_path.read_text())
        out: list[Path] = []
        for name in data.get("libs", {}):
            for base in (self.root / "libs" / name, self.alloy_root / "libs" / name):
                dirs = self._manifest_include_dirs(base)
                if dirs:
                    out += dirs
                    break
        return out

    @staticmethod
    def _manifest_include_dirs(base: Path) -> list[Path]:
        """The include dirs one library copy contributes: what its own
        alloy.lib.toml [headers].include declares (the manifest field was
        previously ignored — "include" was hard-coded), defaulting to
        ["include"] when the manifest or the field is absent. Only dirs that
        exist count, so a copy with no headers yields [] and the caller falls
        through to the next candidate."""
        if not base.is_dir():
            return []
        dirs = ["include"]
        manifest = base / "alloy.lib.toml"
        if manifest.exists():
            headers = tomllib.loads(manifest.read_text()).get("headers", {})
            declared = headers.get("include", dirs)
            if isinstance(declared, list) and declared:
                dirs = [str(d) for d in declared]
        return [base / d for d in dirs if (base / d).is_dir()]

    def ota_options(self) -> dict[str, Any]:
        """The optional ``[ota]`` table from alloy.toml. `public_key` (a path to
        an `alloy keygen` .pub, or 64 hex chars inline) turns on signed-image
        verification: codegen bakes the key into alloy/ota_key.hpp and the build
        pulls in the Ed25519 verifier. Absent -> integrity-only v1 behaviour.

        `public_keys = [...]` is the rotation form: a positional ring the image
        header's key_id indexes, with ``"retired"`` marking a slot whose key must
        never authenticate anything again. See emit/ota_key.py."""
        toml_path = self.root / "alloy.toml"
        if not toml_path.exists():
            return {}
        return tomllib.loads(toml_path.read_text()).get("ota", {})

    def net_options(self) -> dict[str, Any]:
        """The optional ``[net]`` table from alloy.toml (lwIP feature/pool policy).

        Drives the generated ``lwipopts.h`` — features (dhcp/dns/udp), static pool
        sizes, MTU, checksum offload. Empty when absent, in which case the
        generator falls back to defaults that match the v1 hand-written config, so
        a project that never mentions [net] is byte-for-byte unchanged.
        """
        toml_path = self.root / "alloy.toml"
        if not toml_path.exists():
            return {}
        return tomllib.loads(toml_path.read_text()).get("net", {})

    def budget_options(self) -> dict[str, Any]:
        """The optional ``[budget]`` table from alloy.toml: a byte ceiling per
        output section, checked after every link.

            [budget]
            ".fastcode" = 2048     # the hot path must stay in fast RAM AND small
            ".bss"      = 8192

        A budget is how "this must not grow" stops being a code-review habit.
        The section names are the linker's, not friendly aliases, because the
        thing being bounded is what the linker actually produced — and a name
        that does not appear in the image FAILS rather than passes, since an
        absent `.fastcode` is exactly the state where nothing was placed there
        and every other number still looks right.
        """
        toml_path = self.root / "alloy.toml"
        if not toml_path.exists():
            return {}
        return tomllib.loads(toml_path.read_text()).get("budget", {})

    def load_board(self) -> dict[str, Any]:
        if not self.board_json.exists():
            boards_dir = self.alloy_root / "boards"
            known = sorted(p.name for p in boards_dir.iterdir() if (p / "board.json").exists()) \
                if boards_dir.exists() else []
            raise ProjectError(
                f"unknown board '{self.board_id}' — known boards: {', '.join(known) or '(none)'}"
            )
        board = json.loads(self.board_json.read_text())
        if board.get("schema") != "alloy.board.v1":
            raise ProjectError(f"{self.board_json}: expected schema alloy.board.v1")
        if board.get("id") != self.board_id:
            raise ProjectError(f"{self.board_json}: id '{board.get('id')}' != directory '{self.board_id}'")
        return self.apply_overrides(board)

    def project_settings(self) -> dict[str, Any]:
        """`[roles.*]` and `[clock]` from alloy.toml — what THIS project chose."""
        return read_project_settings(self.root)

    def apply_overrides(self, board: dict[str, Any]) -> dict[str, Any]:
        return apply_project_overrides(board, self.project_settings(),
                                       self.devices_root)


def apply_project_overrides(board: dict[str, Any], settings: dict[str, Any],
                            devices_root: Path) -> dict[str, Any]:
    """The board as a project uses it.

    A board.json value is a DEFAULT for the fields a project may choose — a baud
    rate, a watchdog timeout, how much flash to reserve, the clock. Overriding
    one from alloy.toml means a framework board keeps receiving upstream fixes
    instead of being forked by `board-clone` just to change a number.

    A module function, not a method, because every consumer must apply it: the
    emitter reads the board through Project, but `board-info` resolves the file
    itself, and the first version of this let those two disagree — the header
    came out at 48 MHz while the panel still said 64.

    What may be overridden is declared in roles.py. Anything else is a property
    of the hardware, and board-validate refuses it with that reason.
    """
    from .roles import ROLES  # noqa: PLC0415

    roles = settings.get("roles") or {}
    clock = settings.get("clock") or {}
    if not roles and not clock:
        return board

    board = json.loads(json.dumps(board))  # never mutate the caller's dict
    for role, overrides in roles.items():
        spec = ROLES.get(role)
        if spec is None or role not in board.get("roles", {}):
            continue
        for key, value in (overrides or {}).items():
            if key in spec.project_fields:
                board["roles"][role][key] = value

    if clock.get("profile"):
        board.pop("clock", None)
        board["clock_profile"] = clock["profile"]
    elif clock.get("mhz"):
        from .clock_solver import solve  # noqa: PLC0415
        from .devices import load_chip  # noqa: PLC0415

        chip = load_chip(devices_root, board["chip"])
        hse = board.get("hse") or {}
        solved = solve(chip.get("family"),
                       int(round(float(clock["mhz"]) * 1_000_000)),
                       external_hz=hse.get("hz") if clock.get("hse", bool(hse)) else None)
        board["clock"] = solved["profile"]
        board["clock_profile"] = "custom"
    return board


def read_project_settings(project_root: Path | None) -> dict[str, Any]:
    """`[roles.*]` and `[clock]` from a project's alloy.toml, or nothing."""
    if project_root is None:
        return {}
    toml_path = project_root / "alloy.toml"
    if not toml_path.exists():
        return {}
    data = tomllib.loads(toml_path.read_text())
    return {"roles": data.get("roles", {}), "clock": data.get("clock", {})}


def packaged_alloy_root() -> Path | None:
    """Framework payload embedded in the installed wheel, if any."""
    payload = Path(__file__).resolve().parent / "payload"
    if (payload / "boards").is_dir() and (payload / "src" / "alloy").is_dir():
        return payload
    return None


def _packaged_devices_root() -> Path | None:
    try:
        from alloy_devices.loader import data_root
    except ImportError:
        return None
    root = data_root()
    return root if (root / "chips").is_dir() else None


def _find_alloy_root(start: Path) -> Path:
    if env := os.environ.get("ALLOY_ROOT"):
        return Path(env).resolve()
    for candidate in [start, *start.parents]:
        if (candidate / "NORTH_STAR.md").exists() and (candidate / "boards").is_dir():
            return candidate
    if packaged := packaged_alloy_root():
        return packaged
    raise ProjectError(
        "could not find the alloy framework — run inside the repo, set ALLOY_ROOT, "
        "or install the alloy package (the wheel embeds the framework)"
    )


def _find_devices_root(alloy_root: Path, project_root: Path | None = None) -> Path:
    # A project's [devices] path wins over the environment, exactly as its
    # [alloy] root does: a shipped product says where its facts come from, and
    # an operator's shell does not get to move them silently. What DOES catch
    # such a move is the version/digest pin, which is checked against whatever
    # this returns — see devicedb.check_pin.
    if project_root is not None:
        from .devicedb import PinError, read_pin  # noqa: PLC0415

        if pinned := read_pin(project_root).get("path"):
            root = Path(pinned).expanduser()
            if not root.is_absolute():
                root = (project_root / root).resolve()
            if not (root / "chips").is_dir():
                raise PinError(
                    f"{project_root / 'alloy.toml'}: [devices] path = {pinned} "
                    f"is not a chip database ({root} has no chips/ dir)"
                )
            return root
    if env := os.environ.get("ALLOY_DEVICES_ROOT"):
        return Path(env).resolve()
    sibling = alloy_root.parent / "alloy-devices"
    if (sibling / "chips").is_dir():
        return sibling
    if packaged := _packaged_devices_root():
        return packaged
    raise ProjectError(
        "could not find the alloy-devices database — set ALLOY_DEVICES_ROOT, "
        f"clone it next to the framework ({sibling}), or install the "
        "alloy-devices package"
    )


def load_project(project_dir: Path, board_override: str | None = None,
                 product_override: str | None = None) -> Project:
    root = project_dir.resolve()
    toml_path = root / "alloy.toml"
    if not toml_path.exists():
        raise ProjectError(f"{root} is not an alloy project (no alloy.toml)")
    data = tomllib.loads(toml_path.read_text())
    try:
        name = data["project"]["name"]
        board_id = board_override or data["board"]["id"]
    except KeyError as exc:
        raise ProjectError(f"{toml_path}: missing required key {exc}") from exc

    # [product] name selects one of products/<name>.toml; --product overrides
    # it for a one-shot build, exactly like --board over [board] id. Existence
    # is NOT checked here (load_project runs for verbs that never touch
    # products); generation and product-validate check it and name the
    # products that do exist.
    product_table = data.get("product") or {}
    product_id = product_override or product_table.get("name")
    if product_id is not None and not isinstance(product_id, str):
        raise ProjectError(f"{toml_path}: [product] name must be a string")
    if product_table and "name" not in product_table and not product_override:
        raise ProjectError(f"{toml_path}: [product] carries no name = \"…\"")

    # Out-of-repo projects: `alloy new` records the framework root it was
    # scaffolded against; in-repo examples keep discovery by walking up.
    if recorded_root := data.get("alloy", {}).get("root"):
        alloy_root = Path(recorded_root).expanduser().resolve()
        if not (alloy_root / "boards").is_dir():
            raise ProjectError(
                f"{toml_path}: [alloy] root = {recorded_root} does not look like "
                "an alloy checkout (no boards/ dir)"
            )
    else:
        alloy_root = _find_alloy_root(root)
    devices_root = _find_devices_root(alloy_root, root)
    # Every verb that loads a project checks the pin, so there is no route that
    # builds, sizes, flashes or signs against a database the project did not
    # agree to. A project with no [devices] table pays nothing.
    #
    # PinError is deliberately NOT a ProjectError: two callers (cmd_boards and
    # _roots) catch ProjectError to mean "not inside a project, carry on with
    # the framework defaults". Wrapping the pin failure in one would let those
    # two verbs degrade into working against exactly the database the project
    # refused — the failure this pin exists to prevent, quietly reintroduced.
    from .devicedb import check_pin  # noqa: PLC0415

    check_pin(root, devices_root)
    return Project(
        root=root,
        name=name,
        board_id=board_id,
        alloy_root=alloy_root,
        devices_root=devices_root,
        product_id=product_id,
    )
