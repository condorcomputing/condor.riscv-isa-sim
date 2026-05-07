#! /usr/bin/env bash
set -e
CPM_SPIKE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${CPM_SPIKE_DIR}/scripts/shared_settings.sh"

echo
echo "Downloading linux compiler tar file"
echo
#wget https://buildbot.embecosm.com/job/riscv64-linux-gcc-ubuntu2204/20/artifact/riscv64-embecosm-linux-gcc-ubuntu2204-20240407.tar.gz
wget "${LNX_TARBALL_URL}"
echo
echo "Extracting linux compiler tar file (lengthy)"
echo
#tar xf riscv64-embecosm-linux-gcc-ubuntu2204-20240407.tar.gz
tar xf "${LNX_TARBALL}"
echo
echo "Done"
