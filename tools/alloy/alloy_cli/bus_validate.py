"""Bus registry validation — every problem at once, located, with a way out.

The bus twin of product_validate. The rules are NOT duplicated here: bus.py's
message_issues() is the single oracle, and this module only re-shapes its
(field, problem) pairs as the located-issue dicts a form or CI gate consumes,
plus the file-level problems (absent, unparseable, wrong schema) that
load_bus() reports as one hard error.

test_bus_validate pins the pair the standard way:

    P1  validate finds no errors  =>  resolve + emission succeed
    P2  resolve/emission fails    =>  validate found an error
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .bus import load_bus, message_issues
from .emit.common import EmitError

MAX_SUGGESTIONS = 6


def _issue(level: str, message: str, *, field: str | None = None,
           suggestions: list[str] | None = None) -> dict[str, Any]:
    return {"level": level, "field": field, "message": message,
            "suggestions": (suggestions or [])[:MAX_SUGGESTIONS]}


def validate_bus(data: dict[str, Any]) -> list[dict[str, Any]]:
    """Located issues for an already-parsed registry."""
    return [_issue("error", msg, field=field)
            for field, msg in message_issues(data)]


def validate_report(project_root: Path) -> dict[str, Any]:
    """The alloy.bus_validate.v1 envelope for `alloy bus validate`."""
    try:
        data = load_bus(project_root)
        issues = validate_bus(data)
    except EmitError as exc:
        issues = [_issue("error", str(exc))]
    return {"schema": "alloy.bus_validate.v1",
            "ok": not any(i["level"] == "error" for i in issues),
            "issues": issues}
