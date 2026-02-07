#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ -f .env ]]; then
  set -a
  source .env
  set +a
fi

HOST="${POMACE_WEB_HOST:-0.0.0.0}"
PORT="${POMACE_WEB_PORT:-8080}"

exec uvicorn app:app --host "$HOST" --port "$PORT"
