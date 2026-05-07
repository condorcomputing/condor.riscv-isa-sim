Spike-STF RISC-V ISA Simulator
============================

This is a fork of Spike which supports STF and BBV generation maintained 
by [Condor Computing](https://condorcomputing.com/).

-------------
# Preface

This fork of riscv-isa-sim (aka Spike) has support for STF trace and BBV 
generation.  This fork integrates riscv-tests as a submodule supporting 
regression.

The default branch is `spike_stf`. All new features are maintained on 
this branch.

See also USAGE.md.

See CONTRIBUTORS.md.

---------------
# Clone Steps

To clone:
```
git clone https://github.com/condorcomputing/condor.riscv-isa-sim.git --recurse-submodules
```

---------------
# Prerequisites

Before you build Spike-STF you must have a baremetal cross compiler 
(riscv64-unknown-elf-gcc) in your path.

If you need to install a compiler you can use these steps. This example shows 
the Ubuntu 22.x download. Other operating systems are available, 
see [Embecosm Download](https://embecosm.com/downloads/tool-chain-downloads/#risc-v-embedded-stable-release-compilers)

There is a script which downloads and extracts the compiler tarball and
adds links to the toolchain elements so names are compatible with riscv-tests.

adds the compiler bin dir to your path and adds links to toolchain elements so names are 
compatible with riscv-tests

```
    $ cd condor.riscv-isa-sim
    $ bash scripts/download-bm-compiler.sh
```
The downloaded compiler supports both RV32 and RV64.

The addition to your PATH is left as a manual step.  To update your path a typical command would be:
```
    $ export PATH=`pwd`/riscv-embecosm-embedded-ubuntu2204-20250309/bin:$PATH
    $ which riscv64-unknown-elf-gcc       #verify compiler is in your path
```

## Ubuntu support packages

Spike-STF requires some standard packages. The Ubuntu command is this:
```
    $ sudo apt-get install device-tree-compiler libboost-regex-dev libboost-system-dev
```

---------------
# Spike-STF Build Steps
These steps assumes the Ubuntu packages have been previously installed and you have riscv64-unknown-elf-gcc in your path.

See **Prerequisites** above.

```
    $ mkdir -p build install
    $ cd build
    $ ../configure --prefix=`pwd`/../install
    $ make -j$(nproc)
    $ make regress
    $ make install
```

The make regress target builds and executes the riscv-test suite.

---------------
# Further information:

A discussion on using Spike-STF for trace generation under linux and baremetal, 
as well as discussions of automation and BBV generation is found in USAGE.md

  - how to generate traces for bare metal and linux
  - how to generate the dhrystone traces
      - discussion optimization levels of the dhrystone example
  - building and running linux on spike
  - how to generate bbv's
  - how to automate linux trace generation

See : [USAGE.md](https://github.com/condorcomputing/condor.riscv-isa-sim/blob/spike_stf/USAGE.md)
