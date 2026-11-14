#!/bin/sh

set -u

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <media-url>" >&2
    exit 2
fi

if [ -z "${RIALTO_SOCKET_PATH:-}" ]; then
    echo "RIALTO_SOCKET_PATH must point to the active Rialto server socket" >&2
    exit 2
fi

player=${RIALTO_REFERENCE_PLAYER:-rialto_reference_player}
if ! command -v "$player" >/dev/null 2>&1; then
    echo "Reference-player executable not found: $player" >&2
    exit 2
fi

media_url=$1
timeout_ms=${REFPLAYER_METRIC_TIMEOUT_MS:-30000}
failures=0

for type in VIDEO_FRAME_DROP VIDEO_DECODE_ERROR VIDEO_FRAME_REPEAT BUFFER_UNDERFLOW; do
    printf '\n=== %s ===\n' "$type"
    if "$player" --url "$media_url" --inject-fault "$type" --expect-metric "$type" \
        --timeout-ms "$timeout_ms"; then
        printf 'PASS: received expected %s metric\n' "$type"
    else
        status=$?
        printf 'FAIL: %s run exited with status %s\n' "$type" "$status" >&2
        failures=$((failures + 1))
    fi
done

if [ "$failures" -ne 0 ]; then
    printf '\n%s of 4 metric integration runs failed\n' "$failures" >&2
    exit 1
fi

echo "All 4 metric integration runs passed"