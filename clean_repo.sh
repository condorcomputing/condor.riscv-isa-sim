#!/bin/bash

# Ensure we're at the repo root
cd "$(git rev-parse --show-toplevel)" || exit 1

echo "Cleaning build directories..."
rm -rf build/* install/*

echo "Cleaning submodules..."
git submodule deinit -f stf_lib riscv-tests
rm -rf .git/modules/stf_lib .git/modules/riscv-tests
git submodule update --init --recursive

