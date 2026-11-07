from fastapi import HTTPException

import server


def test_default_daily_quota_and_limit(tmp_path, monkeypatch):
    monkeypatch.setattr(server, "DATABASE", tmp_path / "quota.sqlite3")
    install_id = "a" * 32

    for expected in range(1, 101):
        used, limit = server.consume_request(install_id)
        assert used == expected
        assert limit == 100

    try:
        server.consume_request(install_id)
    except HTTPException as error:
        assert error.status_code == 429
    else:
        raise AssertionError("101st request should be rejected")


def test_seven_taps_unlock_bonus_allowance(tmp_path, monkeypatch):
    monkeypatch.setattr(server, "DATABASE", tmp_path / "quota.sqlite3")
    install_id = "b" * 32

    for expected in range(1, 8):
        result = server.bonus_tap(install_id)
        assert result["taps"] == expected
        assert result["bonus_unlocked"] is (expected == 7)

    assert result["daily_limit"] == 1000
    used, limit = server.consume_request(install_id)
    assert (used, limit) == (1, 1000)


def test_daily_reset_keeps_bonus_unlock(tmp_path, monkeypatch):
    monkeypatch.setattr(server, "DATABASE", tmp_path / "quota.sqlite3")
    current_day = ["2026-11-07"]
    monkeypatch.setattr(server, "today", lambda: current_day[0])
    install_id = "c" * 32

    for _ in range(7):
        server.bonus_tap(install_id)
    for _ in range(3):
        server.consume_request(install_id)

    current_day[0] = "2026-11-08"
    state = server.get_usage(install_id)
    assert state["requests"] == 0
    assert state["bonus"] == 1
    used, limit = server.consume_request(install_id)
    assert (used, limit) == (1, 1000)
