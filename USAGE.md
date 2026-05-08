# Usage of the Condor Spike Fork (aka Spike-STF)

## TOC

1. [Building the Bare Metal Dhrystone Example](#building-the-bare-metal-dhrystone-example)

    1. [Dhrystone Optimization Discussion](#dhrystone-optimization-discussion)


1. [Trace Generation for Bare Metal Applications](#trace-generation-for-bare-metal-applications)

1. [Booting Linux on the Condor Spike Fork](#booting-linux-on-the-condor-spike-fork)

    1. [Building the Linux Collateral](#building-the-linux-collateral)

    1. [Running Linux on the Condor Spike Fork](#running-linux-on-the-condor-spike-fork)

1. [Building the Linux Dhrystone Example](#building-the-linux-dhrystone-example)

    1. [Updating the Root File System](#updating-the-root-file-system)

1. [Trace Generation for Linux Applications](#trace-generation-for-linux-applications)

1. [(planned) Automating Linux Tracing with initd](#automating-linux-tracing-with-initd)

--------

## Building the Bare Metal Dhrystone Example

You must have a baremetal crosscompiler in your path. To test:
```
which riscv64-unknown-elf-gcc
```

If you need to add a compiler, these steps are copied from the master readme.
```
cd condor.riscv-isa-sim  # the top of this repo's install
bash scripts/download-bm-compiler.sh
export PATH=`pwd`/riscv-embecosm-embedded-ubuntu2204-20250309/bin:$PATH
```
Then 
```
make -C dhrystone
```
This builds 3 elfs with increasing levels of optimization running 1000 iterations of dhrystone.

Usually there is nothign to be done for the default target. The baremetal 
versions are built during ./configure.


### Dhrystone Optimization Discussion
There are three dhrystone ELFS created by the default make target, labeled opt1, opt2, and op3.

  - OPT1: module level compiles, no in-lining, -O2 opt level (aka ground rules)
  - OPT2: top level compile, inlined functions, -O3 opt level
  - OPT3: top level compile, inlined functions, -O3 opt level with LTO

You can print the main compiler switches to the console by using the help-? 
make target

```
cd dhrystone
make help-BM_OPT1
make help-BM_OPT2
make help-BM_OPT3
```

## Trace Generation for Bare Metal Applications

You must have previously built spike, see the README.md.

The command to create an STF using spike is contained in the script
condor.riscv-isa-sim/run\_spike\_stf.sh.

Basic usage: `bash scripts/run-spike-stf.sh <ELF> <TRACEFILE>`

```
cd condor.riscv-isa-sim
bash scripts/run-spike-stf.sh dhrystone/bin/dhrystone_opt1.1000.gcc.bare.riscv dhrystone_opt1.1000.gcc.bare.riscv.zstf
```

This runs very quickly and creates a compressed (.zstf) trace file in
`condor.riscv-isa-sim/trace_out`

## Building the Linux Dhrystone Example
--------
You must have a linux cross compiler in your path. Try
```
which  riscv64-unknown-linux-gnu-gcc
```

If you need to download a compiler:
```
cd condor.riscv-isa-sim
bash scripts/download-lnx-compiler.sh
export PATH=`pwd`/riscv64-embecosm-linux-gcc-ubuntu2204-20240407/bin:$PATH
which  riscv64-unknown-linux-gnu-gcc
```
Once you have the linux compiler in your path
```
cd condor.riscv-isa-sim
make -C dhrystone bin-linux
```

`dhrystone/bin` will now contain the linux versions of the three optimization
levels for dhrystone.

## Booting Linux on the Condor Spike Fork

Three components are used in this example to boot linux on spike:

- a kernel image (kernel.org)
- a root file system (buildroot) 
- a boot loader (OpenSBI)

Building rootfs and the kernel is a lengthy process. 

### Building the Linux Collateral
Once again you must have a linux cross compiler in your path.  See above.
```
which  riscv64-unknown-linux-gnu-gcc
```
Downloading/building the kernel, root files system and OpenSBI boot loader 
is done in one script.

```
cd condor.riscv-isa-sim
bash scripts/build-linux-collateral.sh
```
This will take some time.

### Running Linux on the Condor Spike Fork
With the linux components built, boot linux using the helper script:
```
cd condor.riscv-isa-sim
bash scripts/boot-linux.sh
```
The credentials are root/root. 

Hitting control-c a few times will exit the simulator. Depending on what is
running under linux it can sometimes be necessary to kill the spike PID.

### Updating the Root File System
To trace applications under linux it is necessary to add them to the root
file system. This makes them available from the spike/linux console.

In this example the 3 versions of dhrystone linux are built into the root
file system. 

This script builds the `dhrystone_optN.1000.gcc.linux.riscv` elfs, adds them to 
the buildroot source tree, rebuilds rootfs, and copies the image to riscv-linux
for use in the next section.

```
bash scripts/build-trace-rootfs.sh
```

## Trace Generation for Linux Applications

Once the new rootfs is built we boot linux on spike with tracing enabled 
from the command line. We use the boot-linux.sh script with two additional
arguments. The first specifes the new rootfs and the second specifies the path 
for the STF trace output.

```
cd condor.riscv-isa-sim
bash scripts/boot-linux.sh --rootfs    ./riscv-linux/trace_rootfs.cpio \
                           --trace_out ./trace_out/linux_trace.zstf
```
Once linux boots, enter the root/root credentials, then from the ash shell
cd to `trace_elfs and run the dhrystone_opt3.1000.gcc.linux.riscv.zstf

A sample session:

```
Welcome to Buildroot
buildroot login: root
root
Password: root

# cd trace_elfs
cd trace_elfs
# ls
ls
dhrystone_opt1.1000.gcc.linux.riscv  dhrystone_opt3.1000.gcc.linux.riscv
dhrystone_opt2.1000.gcc.linux.riscv
# ./dhrystone_opt3.1000.gcc.linux.riscv

...snip...
-I: traced 241546 instructions
...snip...
Str_2_Loc:           DHRYSTONE PROGRAM, 2'ND STRING
        should be:   DHRYSTONE PROGRAM, 2'ND STRING

Measured time too small to obtain meaningful results
Please increase number of runs

# <control-c>
# ^C(spike) quit
```

Once you have exited spike and are returned to the main o/s, the trace_out 
directory will hold the trace file.
```
-rw-r--r-- 1 random agroup 12097 Jan 1 00:00 linux_trace.zstf
```

Note any trace enabled ELF you run while in spike linux will be added to the
trace file. 

This is useful in cases. The next section discusses how to configure this
system to allow hands-free automation of linux based applications, like SPEC,
coremark, coremark-pro.

## Automating Linux Tracing with initd

Automatic tracing of a linux based is done using by passing
the name of the elf through boot args. An init.d script called S99runprogram
unpacks the boot args and executes the specified elf

To show this the previous boot-linux.sh script is used with an addition
argument, --auto_trace. We can reuse the existing trace rootfs and the
elfs previously installed.

```
cd condor.riscv-isa-sim
bash scripts/boot-linux.sh --rootfs     ./riscv-linux/trace_rootfs.cpio   \
                           --trace_out  ./trace_out/linux_auto_trace.zstf \
                           --auto_trace dhrystone_opt3.1000.gcc.linux.riscv
```

This script will boot linux, execute the dhrystone opt3 elf, create a
compressed trace file and exit.

### Initd details

During execution of `scripts/build-trace-rootfs.sh` the script installs
the trace enabled elfs as well the init.d script `S99runprogram`. The
script performs an incremental compile of the file system and moves it
to the common area, `riscv-linux`.

The script `scripts/boot-linux.sh` appends an additional boot arg to 
the default boot args, the argument is trace_elf=${AUTO_TRACE}
Where AUTO_TRACE is the file name supplied by --auto_trace.

The trace_elf boot arg is expected to be a trace enabled ELF name within the
/root/trace_elfs subdirectory. The path is not included, only the name
of the ELF.

The S99runprogram script runs after all init.d scripts labeled S98 and lower. 
S99 parses the boot args for the name supplied with trace_elf, forms the
expected path and tries to execute it.

In the example above the --stf_exit_on_stop_opc is used to automatically
exit linux and spike. 

The combination of these settings provides the automatic execution of
linux, tracing of the target elf and shut down of the simulator.

