"""Unit tests for the NDJSON serial monitor.

The framing is the contract an editor depends on, and it is the kind of thing
that looks right until a device splits a line across two reads. These drive a
fake port that does exactly that.
"""

from __future__ import annotations

import io
import json
import sys

import pytest

from alloy_cli import monitor as mon


class _FakePort:
    """A serial port that hands back a scripted sequence of reads."""

    def __init__(self, chunks: list[bytes]) -> None:
        self._chunks = list(chunks)
        self.written = bytearray()
        self.closed = False

    def read(self, _n: int) -> bytes:
        if self._chunks:
            return self._chunks.pop(0)
        raise OSError("device gone")   # ends the loop the way unplugging does

    def write(self, data: bytes) -> int:
        self.written.extend(data)
        return len(data)

    def close(self) -> None:
        self.closed = True


def _run(monkeypatch, chunks: list[bytes], stdin: str = "") -> list[dict]:
    port = _FakePort(chunks)
    monkeypatch.setattr(mon, "_open_port", lambda _b: (port, "/dev/fake", 115200))
    monkeypatch.setattr(sys, "stdin", io.StringIO(stdin))
    out = io.StringIO()
    monkeypatch.setattr(sys, "stdout", out)
    mon.monitor_ndjson({"id": "fake"})
    monkeypatch.undo()
    return [json.loads(line) for line in out.getvalue().splitlines() if line.strip()]


def test_it_announces_the_link_and_its_close(monkeypatch) -> None:
    events = _run(monkeypatch, [b"hi\n"])
    assert events[0] == {"event": "open", "port": "/dev/fake", "baud": 115200}
    assert events[-1] == {"event": "closed"}


def test_one_object_per_line(monkeypatch) -> None:
    events = _run(monkeypatch, [b"alpha\nbeta\n"])
    lines = [e["line"] for e in events if "line" in e]
    assert lines == ["alpha", "beta"]


def test_a_line_split_across_reads_is_reassembled(monkeypatch) -> None:
    """The common case that breaks naive framing: a device writing faster than
    the host reads."""
    events = _run(monkeypatch, [b"tempe", b"rature=2", b"1.5\n"])
    assert [e["line"] for e in events if "line" in e] == ["temperature=21.5"]


def test_carriage_returns_do_not_survive(monkeypatch) -> None:
    """Firmware prints \\r\\n; a stray \\r makes every line look mismatched."""
    events = _run(monkeypatch, [b"ready\r\n"])
    assert [e["line"] for e in events if "line" in e] == ["ready"]


def test_a_prompt_without_a_newline_is_not_swallowed(monkeypatch) -> None:
    """A bootloader that prints "> " and waits would otherwise be invisible."""
    events = _run(monkeypatch, [b"boot ok\n> "])
    tail = [e for e in events if e.get("partial")]
    assert tail and tail[0]["line"] == "> "


def test_timestamps_are_milliseconds_from_the_open(monkeypatch) -> None:
    """Wall clock is not what a device log is read for — the interval is."""
    events = _run(monkeypatch, [b"a\n", b"b\n"])
    stamps = [e["t"] for e in events if "line" in e]
    assert all(isinstance(t, int) and t >= 0 for t in stamps)
    assert stamps == sorted(stamps)


def test_a_read_failure_is_reported_not_hidden(monkeypatch) -> None:
    events = _run(monkeypatch, [])
    assert any(e.get("event") == "error" for e in events)
    assert events[-1] == {"event": "closed"}


def test_stdin_lines_reach_the_device(monkeypatch) -> None:
    port = _FakePort([b"ok\n"])
    monkeypatch.setattr(mon, "_open_port", lambda _b: (port, "/dev/fake", 115200))
    monkeypatch.setattr(sys, "stdin", io.StringIO("reset\n"))
    monkeypatch.setattr(sys, "stdout", io.StringIO())
    mon.monitor_ndjson({"id": "fake"})
    monkeypatch.undo()
    assert bytes(port.written) == b"reset\n"
    assert port.closed, "the port must be released even on an abrupt end"


def test_undecodable_bytes_do_not_kill_the_stream(monkeypatch) -> None:
    """Line noise at the wrong baud is a normal thing to see, not a crash."""
    events = _run(monkeypatch, [b"\xff\xfe ok\n"])
    assert any("ok" in e.get("line", "") for e in events)


@pytest.mark.parametrize("baud,ok", [(115200, True), (12345, False)])
def test_the_baud_whitelist_still_applies(baud: int, ok: bool) -> None:
    board = {"id": "b", "roles": {"debug_uart": {"baud": baud}}}
    if ok:
        assert mon._resolve_baud(board) == baud
    else:
        with pytest.raises(Exception, match="unsupported baud"):
            mon._resolve_baud(board)
