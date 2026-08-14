"""The anti-drift pair for the bus registry, pinned the standard way:

    P1  validate finds no errors  =>  resolve + emission succeed
    P2  resolve/emission fails    =>  validate found an error

There is only one oracle (bus.message_issues), so unlike the product pair
these cannot drift apart in SEMANTICS — what the properties guard here is
the plumbing: validate_report's envelope, the located fields, and the
file-level failures (absent, unparseable, wrong schema) that arrive through
load_bus instead of the oracle.
"""

from __future__ import annotations

import pytest
from alloy_cli.bus import BUS_SCHEMA, load_bus, resolve
from alloy_cli.bus_validate import validate_bus, validate_report
from alloy_cli.emit.bus import emit_bus_header
from alloy_cli.emit.common import EmitError

VALID = {
    "schema": BUS_SCHEMA,
    "messages": {
        "ping": {"id": 0x0301, "fields": [{"name": "token", "type": "u32"}]},
        "legacy": {"id": 0x0300, "retired": True},
    },
}

BROKEN = [
    {"schema": BUS_SCHEMA, "messages": {"m": {"fields": []}}},
    {"schema": BUS_SCHEMA, "messages": {"m": {"id": 0xFFFF,
                                              "fields": [{"name": "x", "type": "u8"}]}}},
    {"schema": BUS_SCHEMA, "messages": {"9bad": {"id": 1,
                                                 "fields": [{"name": "x", "type": "u8"}]}}},
    {"schema": BUS_SCHEMA, "extra": 1, "messages": {}},
]


def test_p1_no_errors_means_emission_succeeds() -> None:
    assert validate_bus(VALID) == []
    text = emit_bus_header(resolve(VALID))
    assert "struct ping_wire" in text


@pytest.mark.parametrize("data", BROKEN)
def test_p2_emission_failure_was_predicted(data) -> None:
    with pytest.raises(EmitError):
        resolve(data)
    issues = validate_bus(data)
    assert any(i["level"] == "error" for i in issues)


def test_issues_are_located() -> None:
    issues = validate_bus({"schema": BUS_SCHEMA,
                           "messages": {"m": {"id": 0,
                                              "fields": [{"name": "x", "type": "u8"}]}}})
    assert issues[0]["field"] == "messages.m.id"


def test_report_envelope_on_a_real_project(tmp_path) -> None:
    (tmp_path / "bus.toml").write_text(
        f'schema = "{BUS_SCHEMA}"\n\n'
        "[messages.ping]\n"
        "id = 0x0301\n"
        'fields = [ { name = "token", type = "u32" } ]\n')
    report = validate_report(tmp_path)
    assert report["schema"] == "alloy.bus_validate.v1"
    assert report["ok"] is True
    assert report["issues"] == []


def test_report_names_a_missing_file_and_a_wrong_schema(tmp_path) -> None:
    report = validate_report(tmp_path)
    assert report["ok"] is False
    assert "no bus.toml" in report["issues"][0]["message"]

    (tmp_path / "bus.toml").write_text('schema = "alloy.bus.v999"\n')
    report = validate_report(tmp_path)
    assert report["ok"] is False
    assert "schema" in report["issues"][0]["message"]


def test_load_refuses_unparseable_toml(tmp_path) -> None:
    (tmp_path / "bus.toml").write_text("[messages.\n")
    with pytest.raises(EmitError):
        load_bus(tmp_path)
