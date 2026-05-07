#! /usr/bin/env bash
set -e
CPM_SPIKE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${CPM_SPIKE_DIR}/scripts/shared_settings.sh"

echo
echo "Downloading bare-metal compiler tar file"
echo
wget "$BM_TARBALL_URL"
echo
echo "Extracting bare-metal compiler tar file (lengthy)"
echo
tar xf "$BM_TARBALL"
echo
echo "Creating bare-metal riscv64 links"
echo
#FIXME: magic string
for f in riscv-embecosm-embedded-ubuntu2204-20250309/bin/riscv32-unknown-elf-*; do ln -s "$(basename "$f")" "riscv-embecosm-embedded-ubuntu2204-20250309/bin/$(basename "$f" | sed 's/riscv32/riscv64/')"; done
echo
echo "Done"
