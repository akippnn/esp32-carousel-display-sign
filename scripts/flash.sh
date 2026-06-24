#!/usr/bin/env bash
#
# flash.sh -- Compile and upload the ESP32 Carousel Display firmware via PlatformIO
#
# Usage:
#   ./scripts/flash.sh               # Auto-detect port
#   ./scripts/flash.sh /dev/cu.SLAB_USBtoUART   # Specify port
#

set -euo pipefail

# Navigate to project root (scripts/ is one level below root)
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

# Auto-detect port if not provided
if [ $# -ge 1 ]; then
  PORT="$1"
else
  echo "=== Auto-detecting ESP32 port ==="
  if [[ "$(uname -s)" == "Darwin" ]]; then
    PORT=$(ls /dev/cu.SLAB_USBtoUART /dev/cu.usbserial-* /dev/cu.Serial* 2>/dev/null | head -1 || true)
  else
    PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -1 || true)
  fi

  if [ -z "${PORT:-}" ]; then
    echo "Error: No ESP32 serial port detected."
    echo "Please specify the port manually:"
    echo "  ./scripts/flash.sh /dev/cu.SLAB_USBtoUART"
    exit 1
  fi

  echo "Detected port: $PORT"
fi

echo ""
echo "=== Building and uploading via PlatformIO ==="

pio run -t upload --upload-port "$PORT"

echo ""
echo "=== Done! ==="
