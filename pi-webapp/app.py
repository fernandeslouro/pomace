#!/usr/bin/env python3
import os
import re
import threading
import time
from pathlib import Path
from typing import Literal, Optional

import serial
from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field

SERIAL_PORT = os.getenv("POMACE_SERIAL_PORT", "/dev/ttyUSB0")
SERIAL_BAUD = int(os.getenv("POMACE_SERIAL_BAUD", "115200"))
SERIAL_TIMEOUT_S = float(os.getenv("POMACE_SERIAL_TIMEOUT_S", "0.10"))
COMMAND_TIMEOUT_S = float(os.getenv("POMACE_COMMAND_TIMEOUT_S", "1.60"))
COMMAND_TAIL_WAIT_S = float(os.getenv("POMACE_COMMAND_TAIL_WAIT_S", "0.22"))

STATIC_DIR = Path(__file__).resolve().parent / "static"
SUMMARY_PAIR_RE = re.compile(r"([A-Z_]+)=([^ ]+)")
SAFE_RAW_RE = re.compile(r"[A-Za-z0-9_ ]+")

VALID_RESET_TARGETS = {
    "ALL",
    "SAFETY",
    "BOILER",
    "SF",
    "PH",
    "PWH",
    "SB",
    "FSG",
    "FC",
    "STORAGE",
    "CRUSHER",
}

VALID_MOTOR_NAMES = {
    "SF",
    "PH",
    "PWH",
    "SB",
    "FSG",
    "FC",
    "STORAGE",
    "CRUSHER",
}


class ModePayload(BaseModel):
    mode: Literal["AUTO", "OFF", "REMOTE"]


class ThermostatPayload(BaseModel):
    state: Literal["AUTO", "0", "1"]


class FlamePayload(BaseModel):
    state: Literal["AUTO", "ON"]


class StagePayload(BaseModel):
    stage: Literal["AUTO", "0", "1", "2", "3"]


class FanPayload(BaseModel):
    value: str = Field(min_length=1, max_length=8)


class ResetPayload(BaseModel):
    target: str = Field(min_length=1, max_length=16)


class MotorCommandPayload(BaseModel):
    motor: str = Field(min_length=1, max_length=16)
    state: Literal["ON", "OFF", "AUTO"]


class JamPayload(BaseModel):
    motor: str = Field(min_length=1, max_length=16)


class RawCommandPayload(BaseModel):
    command: str = Field(min_length=1, max_length=64)


class SerialBridge:
    def __init__(self, port: str, baud: int, timeout_s: float):
        self._port = port
        self._baud = baud
        self._timeout_s = timeout_s
        self._lock = threading.Lock()
        self._serial: Optional[serial.Serial] = None

    def _ensure_serial(self) -> serial.Serial:
        if self._serial is not None and self._serial.is_open:
            return self._serial
        self._serial = serial.Serial(self._port, self._baud, timeout=self._timeout_s)
        return self._serial

    @property
    def is_connected(self) -> bool:
        return self._serial is not None and self._serial.is_open

    def close(self) -> None:
        if self._serial is not None:
            self._serial.close()

    def query(self, command: str) -> list[str]:
        with self._lock:
            ser = self._ensure_serial()
            ser.reset_input_buffer()
            ser.reset_output_buffer()

            payload = (command.strip() + "\n").encode("ascii", errors="ignore")
            ser.write(payload)
            ser.flush()

            lines: list[str] = []
            deadline = time.monotonic() + COMMAND_TIMEOUT_S
            while time.monotonic() < deadline:
                raw = ser.readline()
                if not raw:
                    continue

                line = raw.decode("utf-8", errors="ignore").strip()
                if not line:
                    continue

                lines.append(line)
                deadline = time.monotonic() + COMMAND_TAIL_WAIT_S

            if not lines:
                raise TimeoutError(f"No response for command: {command}")

            return lines


bridge = SerialBridge(SERIAL_PORT, SERIAL_BAUD, SERIAL_TIMEOUT_S)
app = FastAPI(title="Pomace Boiler Web API", version="0.1.0")
app.mount("/static", StaticFiles(directory=str(STATIC_DIR)), name="static")


def _coerce_value(raw: str):
    if raw in {"0", "1"}:
        return raw == "1"
    try:
        return int(raw)
    except ValueError:
        pass
    try:
        return float(raw)
    except ValueError:
        return raw


def _parse_status(lines: list[str]) -> dict:
    summary: dict[str, object] = {}
    motors: list[dict[str, object]] = []

    for line in lines:
        if line.startswith("MODE="):
            for key, raw in SUMMARY_PAIR_RE.findall(line):
                summary[key.lower()] = _coerce_value(raw)
            continue

        if line.startswith("MOTOR "):
            parts = line.split()
            if len(parts) < 5:
                continue
            motor = {
                "name": parts[1],
                "on": parts[2].split("=", 1)[-1] == "1",
                "fault": parts[3].split("=", 1)[-1] == "1",
                "recovery": parts[4].split("=", 1)[-1] == "1",
            }
            motors.append(motor)

    return {"summary": summary, "motors": motors, "lines": lines}


def _exec_command(command: str) -> dict:
    try:
        lines = bridge.query(command)
    except (serial.SerialException, TimeoutError, OSError) as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc

    return {"command": command, "lines": lines}


def _normalize_motor_name(motor: str) -> str:
    candidate = motor.strip().upper()
    if candidate not in VALID_MOTOR_NAMES:
        raise HTTPException(status_code=422, detail=f"Invalid motor name: {motor}")
    return candidate


@app.get("/")
def index() -> FileResponse:
    return FileResponse(STATIC_DIR / "index.html")


@app.get("/api/health")
def health() -> dict:
    return {
        "ok": True,
        "serial_port": SERIAL_PORT,
        "serial_baud": SERIAL_BAUD,
        "serial_connected": bridge.is_connected,
    }


@app.get("/api/status")
def status() -> dict:
    result = _exec_command("STATUS")
    parsed = _parse_status(result["lines"])
    mode = str(parsed.get("summary", {}).get("mode", ""))
    parsed["authority"] = {
        "app_full_control": mode == "REMOTE",
        "required_mode_for_full_control": "REMOTE",
        "effective_mode": mode,
        "note": "Panel selector decides effective mode. Full remote authority requires REMOTE.",
    }
    return {"ok": True, **parsed}


@app.post("/api/mode")
def set_mode(payload: ModePayload) -> dict:
    command_result = _exec_command(f"MODE {payload.mode}")
    status_result = status()
    effective_mode = str(status_result.get("summary", {}).get("mode", ""))
    return {
        **command_result,
        "requested_mode": payload.mode,
        "effective_mode": effective_mode,
        "full_app_control": effective_mode == "REMOTE",
    }


@app.post("/api/thermostat")
def set_thermostat(payload: ThermostatPayload) -> dict:
    return _exec_command(f"THERMOSTAT {payload.state}")


@app.post("/api/flame")
def set_flame(payload: FlamePayload) -> dict:
    return _exec_command(f"FLAME {payload.state}")


@app.post("/api/stage")
def set_stage(payload: StagePayload) -> dict:
    return _exec_command(f"STAGE {payload.stage}")


@app.post("/api/fan")
def set_fan(payload: FanPayload) -> dict:
    value = payload.value.strip().upper()
    if value == "AUTO":
        return _exec_command("FAN AUTO")

    if not value.isdigit():
        raise HTTPException(status_code=422, detail="Fan value must be AUTO or 0-100")

    fan_value = int(value)
    if fan_value < 0 or fan_value > 100:
        raise HTTPException(status_code=422, detail="Fan value must be AUTO or 0-100")

    return _exec_command(f"FAN {fan_value}")


@app.post("/api/motor")
def set_motor(payload: MotorCommandPayload) -> dict:
    motor = _normalize_motor_name(payload.motor)
    return _exec_command(f"SET {motor} {payload.state}")


@app.post("/api/jam")
def jam_motor(payload: JamPayload) -> dict:
    motor = _normalize_motor_name(payload.motor)
    return _exec_command(f"JAM {motor}")


@app.post("/api/reset")
def reset(payload: ResetPayload) -> dict:
    target = payload.target.strip().upper()
    if target not in VALID_RESET_TARGETS:
        raise HTTPException(status_code=422, detail=f"Invalid reset target: {target}")
    return _exec_command(f"RESET {target}")


@app.post("/api/raw")
def raw(payload: RawCommandPayload) -> dict:
    command = payload.command.strip()
    if not SAFE_RAW_RE.fullmatch(command):
        raise HTTPException(status_code=422, detail="Command must match [A-Za-z0-9_ ]")
    return _exec_command(command)


@app.on_event("shutdown")
def on_shutdown() -> None:
    bridge.close()
