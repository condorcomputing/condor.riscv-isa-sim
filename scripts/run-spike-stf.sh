#!/usr/bin/env bash
set -e
CPM_SPIKE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${CPM_SPIKE_DIR}/scripts/shared_settings.sh"

# Description:
# Run the Spike simulator with a given STF trace and ELF binary.
# Outputs logs and traces to the 'output/' directory unless the user specifies full paths.

# Default input arguments
ELF_FILE="${1:-dhrystone/bin/dhrystone_opt1.1000.gcc.bare.riscv}"
TRACE_FILE="${2:-dhrystone_opt1.1000.gcc.bare.riscv.zstf}"

# Output directory
OUTDIR="trace_out"
mkdir -p "$OUTDIR"

# Check ELF file existence
if [[ ! -f "$ELF_FILE" ]]; then
  echo "Error: ELF file '$ELF_FILE' not found." >&2
  exit 1
fi

# Determine output trace file path
if [[ "$TRACE_FILE" == */* ]]; then
  OUT_TRACE="$TRACE_FILE"
else
  OUT_TRACE="$OUTDIR/$TRACE_FILE"
fi

# Run Spike
LD_LIBRARY_PATH=./build ./build/spike \
  --isa="${SPIKE_ISA}" \
  --stf_macro_tracing --stf_trace "$OUT_TRACE" \
  "$ELF_FILE"
