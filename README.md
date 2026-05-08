Spike RISC-V ISA Simulator
==========================

Preface
-------
### Purpose of this fork

  This fork of [riscv-isa-sim](https://github.com/riscv-software-src/riscv-isa-sim) (aka Spike) enables the use of [SimPoint](https://cseweb.ucsd.edu/~calder/simpoint/) to efficiently create representative traces of very large workloads. These traces serve as input to performance models and microarchitectural simulators, enabling detailed analysis of large workloads and benchmark suites where full execution on cycle-level simulators is impractical. This approach makes it possible to study workload behavior and estimate performance with reasonable turnaround.

### Key features

  The key features in this fork are:

  - SimPoint-compatible Basic Block Vector (BBV) tracing
  - STF trace generation with precise trace window selection
  - Checkpoint save/restore for accelerated trace collection
  - Single-process and ROI-aware tracing
  - Multi-trace collection in a single execution

### Typical workflow

  A typical SimPoint tracing workflow using this fork is:

  1. Run the workload with BBV generation and periodic checkpointing enabled.
  2. Run SimPoint analysis on the generated BBV files to identify representative intervals.
  3. Use the tracks file to determine the absolute instruction ranges corresponding to the selected intervals.
  4. Restore execution from the checkpoint nearest the desired interval.
  5. Generate STF traces for the selected intervals.

### Contributors reference

  See [CONTRIBUTORS.md](CONTRIBUTORS.md).

BBV generation
--------------
### Overview
This version of riscv-isa-sim is capable of generating [BBV files](https://valgrind.org/docs/manual/bbv-manual.html#bbv-manual.fileformat) for use with the SimPoint application.
```shell
spike --en_bbv --bb_file bbv.spike --simpoint_size 5000000 binary.riscv
```
Option `--en_bbv` enables BBV tracing. `--simpoint_size` is used to choose the interval window size for each vector.

BBV tracing produces two output files per core -- the basic block vector file, and a "tracks" file, which records information about each interval. The `--bb_file` option is used to pass the base name of the output files. The base output name is appended with `_cpu<n>` to form the BBV file name; and with `_<n>_tracks.log` to form the tracks file name.

### Region-of-interest and single-process tracing
Oftentimes, one wishes to use SimPoint to analyze a single process, and only the compute-intensive portion of that process, ignoring OS overhead, context switching, application initialization, and the like. This fork extends Spike with BBV and STF tracing options that allow precise configuration to isolate a single process's region of interest (ROI).

A dummy CSR with address `0x8c2` has been added, which software uses to signal the beginning and end of the region of interest (by writing one or zero to the register, respectively).

### `--bbv_umode_only` option
When the CSR is written to indicate the beginning of the ROI, the physical page number (PPN) in the SATP register is recorded. Ordinarily, the BBV control CSR will be written by a user-mode application, and the SATP PPN field will hold a value which is unique to that process and constant over the life of the process. When the `--bbv_umode_only` option is selected, BBV tracing will consider only user-mode instructions that are executed under the same PPN as the CSR instruction which enabled the region of interest (i.e., are part of the same process). Absent this option, all instructions of all privilege levels are traced.


### Tracks file
The tracks file provides information about each interval, to facilitate trace collection after SimPoint is run.

When the `--bbv_umode_only` option is enabled, the BBV/SimPoint intervals include only instructions from a single process. STF trace, however, is enabled and disabled via absolute instruction counts, inclusive of all instructions. Therefore, tracing a selected interval requires knowing the absolute instruction counts for the start and end of the single-process interval. The tracks file provides this information by recording various metadata about the start and end of each interval, as seen by BBV trace and SimPoint.

There is one line in the tracks file for each interval that is traced. (That is, the BBV trace and the tracks file always have the same number of lines.) The format of a line in the tracks file is:

```
<interval start instruction track> <interval end instruction track>
```

The format of a "track" is:

```
<total instructions executed> <total user-mode instructions executed> <total ROI user-mode instructions executed> <instruction address>
```

This information is used to configure STF trace to capture a selected interval. More information is provided in the STF trace section.


### Warmup region and --warmup\_size option
Captured traces may be used as stimulus to simulators that model caches, branch predictors, and other stateful design elements. Thus it is often useful to have a "warmup region" of instructions preceding the trace region of interest, which serves to initialize models to a reasonable state. To capture a fixed-size warmup region, one must know the absolute instruction number marking its beginning. When the desired warmup region size is provided via the `--warmup_size` argument, an additional track, recording the beginning of each interval's warmup region, is prepended to each line of the tracks file.

Thus, when the `--warmup_size` argument is provided, the format of the tracks file becomes:

```
<warmup window start instruction track> <interval start instruction track> <interval end instruction track>
```



STF tracing
-----------
### Overview
This fork extends Spike with support for generating traces in the [STF](https://github.com/sparcians/stf_spec) format, including fine-grained control over trace window selection, full exception and interrupt tracing, and options to generate correct traces of a single process's execution.

### Trace window selection
Trace window selection can be controlled via two methods -- instruction counts, provided via the command line, or enable/disable macros embedded in the application code.

To enable STF trace, provide a trace file name via the `--stf_trace` option (with either a `.stf` or `.zstf` extension, as desired). Then, select either `--stf_insn_num_tracing` or `--stf_macro_tracing`.

### Instruction-count tracing
When `--stf_insn_num_tracing` is selected, the beginning and end of the trace window can be controlled via the `--stf_insn_start` (default 0) and `--stf_insn_count` (default unlimited) arguments. These values are **always** inclusive of all instructions executed, regardless of privilege level. The counts are **not** affected by `--stf_priv_modes` or `--stf_proc_ppn` instruction filtering. (The only exception is via the `--stf_count_from_bbv_roi` flag. When this option is selected, instruction counting starts from the CSR instruction that enables the BBV ROI, whether BBV tracing is enabled or not.)

### Macro-based tracing
When `--stf_macro_tracing` is selected, trace capture will start when the first `xor x0, x0, x0` no-op instruction is executed. Trace capture will end when a `xor x0, x1, x1` instruction is executed. If `--stf_include_macros` is selected (default false), the enable/disable macros will be included in the trace stream.

### Additional options
With either trace capture method, `--stf_trace_register_state` and/or `--stf_trace_memory_records` may be optionally included to trace register updates and memory operations per instruction.

### Single-process tracing
The target of SimPoint analysis is often a single process. Two options are provided to support generating correct traces from a single process.

The `--stf_priv_modes` argument accepts any combination of M, H, S, and U to specify which of machine, hypervisor, supervisor, and user-mode instructions should be included in the trace.

The `--stf_proc_ppn` argument is used to specify the SATP CSR physical page number field that corresponds to the process of interest. When the `--stf_priv_modes` option specifies *only* user-mode tracing (`--stf_priv_modes U`), only user-mode instructions executed under the PPN value provided by `--stf_proc_ppn` will be traced.

If `--stf_priv_modes U` is provided and `--stf_proc_ppn` is not, the application PPN will be determined in one of two ways. First, if the CSR instruction to enable the BBV ROI is executed (regardless of `--en_bbv` setting), the PPN value at the time of that instruction will be used. Alternatively, if trace is enabled and a target PPN is not set, the PPN value at the time of the first traced instruction will be selected.

### Multiple STF traces
A single Spike run can collect multiple STF traces with different parameters. Simply specify a new `--stf_trace <file>` argument; all of the STF trace options following it will apply to that trace file (up to the next `--stf_trace` argument, if there is one).

For example, to simultaneously generate a trace of all instructions, and a trace of only user-mode instructions:

```shell
spike --stf_trace trace_all.stf --stf_insn_num_tracing --stf_trace trace_user.stf --stf_insn_num_tracing --stf_priv_modes U binary.riscv
```

Checkpointing
---------------------------
### Motivation
SimPoint tracing is generally a two-step process: first, BBV trace is collected and SimPoint analysis is run; then the workload is run a second time with STF tracing enabled to trace the selected intervals.

For very long workloads, this can be time-consuming or prohibitive. By periodically collecting checkpoints during the BBV generation phase, the STF tracing phase can be accelerated by restarting the simulation from the checkpoint nearest each interval of interest.

### Checkpoint generation
Checkpoints can be generated in three ways: by specifying an absolute instruction number; via instruction macros embedded in the application code; or at periodic SimPoint intervals.

- **Instruction Number**:
To generate a checkpoint at a specific instruction, use the `--checkpoint_instruction` argument. Multiple `--checkpoint_instruction` arguments may be provided. Checkpoint files of the form `checkpoint_<insn_num>.json` will be generated. As with STF trace, instruction counting is inclusive of all instructions of all privilege levels. A checkpoint at instruction `n` represents platform state immediately after execution of instruction `n`.

- **Macro-based**:
To generate checkpoints via instruction macro, provide the `--checkpoint_macro_enable` argument and embed `xor x0, x2, x2` no-op instructions in the application code wherever a checkpoint should be generated. The checkpoint naming convention will be the same as above (`checkpoint_<insn_num>.json`), where `insn_num` is the absolute instruction count of the no-op macro, and the checkpoint represents platform state immediately after execution of the no-op.

- **Periodic intervals**:
To checkpoint periodically during BBV generation, use the `--checkpoint_interval` argument, usually in conjunction with the `--simpoint_size` argument. This will generate a checkpoint after every `n` SimPoint intervals. The naming convention will be `checkpoint_<interval_num>_<insn_num>.json`, where `interval_num` is a multiple of the argument provided to `--checkpoint_interval`, and `insn_num` is the absolute instruction count of the last instruction of the preceding interval. The checkpoint represents the state of the platform immediately after execution of `insn_num`, and immediately before the execution of the first instruction of interval `interval_num`.

### Restoring from checkpoints
To resume execution from a saved checkpoint, use the `--restore_checkpoint` option. When resuming from a checkpoint, providing the original executable is not required, since the memory image is part of the checkpoint and not reloaded from the program source. However, symbols such as tohost and fromhost are not stored in the checkpoint. Provide the original executable source when reloading from a checkpoint to reload symbols used by Spike. (Symbol preservation will be added as a future enhancement.)

Parallelization of BBV generation
---------------------------------
- **Motivation**:
  Since BBV tracing results in a significant slowdown over vanilla Spike execution (50% or more), the operation can be accelerated by parallelization via checkpointing. Additional options are provided to facilitate this.

- **Workflow with checkpoints**:
  First, periodic checkpoints would be created in a non-tracing (vanilla) Spike run, by providing the `--simpoint_size` and `--checkpoint_interval` options; but **not** the `--en_bbv` option.

  As checkpoints are created, each would be used to collect a portion of the BBV trace, up to the next checkpoint. This can be accomplished with the `--max_intervals` option, which, combined with `--en_bbv`, stops execution after the desired number of intervals are traced. (A `--start_interval` option is also provided. The `--start_interval` value is relative to the starting point. When a checkpoint is reloaded, the first interval traced is number 0, which is the default value of `--start_interval`.)

- **Basic block ID encoding**:
Some implementations of BBV tracing assign a sequential basic block ID to each basic block. However, this prevents BBV collection in piecemeal fashion, since the global map of basic block IDs must always be known. To overcome this, basic blocks are, by default, assigned an ID equal to the address of the last instruction in the basic block. To encode basic block IDs as sequential integers, the option `--encode_bb_ids` is provided.

Branches
--------
The `spike_stf` branch is the stable public branch for this fork.

This repository is maintained using linear patch-stack branches on selected upstream Spike revisions, with integration commits on `spike_stf`. See [MAINTAINERS.md](MAINTAINERS.md) for details.


About
-------------

Spike, the RISC-V ISA Simulator, implements a functional model of one or more
RISC-V harts.  It is named after the golden spike used to celebrate the
completion of the US transcontinental railway.

Spike supports the following RISC-V ISA features:
  - RV32I and RV64I base ISAs, v2.1
  - RV32E and RV64E base ISAs, v1.9
  - Zifencei extension, v2.0
  - Zicsr extension, v2.0
  - Zicntr extension, v2.0
  - M extension, v2.0
  - A extension, v2.1
  - B extension, v1.0
  - F extension, v2.2
  - D extension, v2.2
  - Q extension, v2.2
  - C extension, v2.0
  - Zbkb, Zbkc, Zbkx, Zknd, Zkne, Zknh, Zksed, Zksh scalar cryptography extensions (Zk, Zkn, and Zks groups), v1.0
  - Zkr virtual entropy source emulation, v1.0
  - V extension, v1.0 (_requires a 64-bit host_)
  - P extension, v0.9.2
  - Zba extension, v1.0
  - Zbb extension, v1.0
  - Zbc extension, v1.0
  - Zbs extension, v1.0
  - Zfh and Zfhmin half-precision floating-point extensions, v1.0
  - Zfinx extension, v1.0
  - Zmmul integer multiplication extension, v1.0
  - Zicbom, Zicbop, Zicboz cache-block maintenance extensions, v1.0
  - Conformance to both RVWMO and RVTSO (Spike is sequentially consistent)
  - Machine, Supervisor, and User modes, v1.11
  - Hypervisor extension, v1.0
  - Svnapot extension, v1.0
  - Svpbmt extension, v1.0
  - Svinval extension, v1.0
  - Svadu extension, v1.0
  - Sdext extension, v1.0-STABLE
  - Sdtrig extension, v1.0-STABLE
  - Smepmp extension v1.0
  - Smstateen extension, v1.0
  - Smdbltrp extension, v1.0
  - Sscofpmf v0.5.2
  - Ssdbltrp extension, v1.0
  - Ssqosid extension, v1.0
  - Zaamo extension, v1.0
  - Zalrsc extension, v1.0
  - Zabha extension, v1.0
  - Zacas extension, v1.0
  - Zawrs extension, v1.0
  - Zicfiss extension, v1.0
  - Zicfilp extension, v1.0
  - Zca extension, v1.0
  - Zcb extension, v1.0
  - Zcf extension, v1.0
  - Zcd extension, v1.0
  - Zcmp extension, v1.0
  - Zcmt extension, v1.0
  - Zfbfmin extension, v0.6
  - Zvfbfmin extension, v0.6
  - Zvfbfwma extension, v0.6
  - Zvbb extension, v1.0
  - Zvbc extension, v1.0
  - Zvkg extension, v1.0
  - Zvkned extension, v1.0
  - Zvknha, Zvknhb extension, v1.0
  - Zvksed extension, v1.0
  - Zvksh extension, v1.0
  - Zvkt  extension, v1.0
  - Zvkn, Zvknc, Zvkng extension, v1.0
  - Zvks, Zvksc, Zvksg extension, v1.0 
  - Zicond extension, v1.0
  - Zilsd extension, v0.10
  - Zclsd extension, v0.10

Versioning and APIs
-------------------

Projects are versioned primarily to indicate when the API has been extended or
rendered incompatible.  In that spirit, Spike aims to follow the
[SemVer](https://semver.org/spec/v2.0.0.html) versioning scheme, in which
major version numbers are incremented when backwards-incompatible API changes
are made; minor version numbers are incremented when new APIs are added; and
patch version numbers are incremented when bugs are fixed in
a backwards-compatible manner.

Spike's principal public API is the RISC-V ISA.  _The C++ interface to Spike's
internals is **not** considered a public API at this time_, and
backwards-incompatible changes to this interface _will_ be made without
incrementing the major version number.

Build Steps 
---------------
Use the build steps above to build this fork. The instructions below are kept 
for reference.

We assume that the RISCV environment variable is set to the RISC-V tools
install path.

```
    $ apt-get install device-tree-compiler libboost-regex-dev libboost-system-dev
    $ mkdir build
    $ cd build
    $ ../configure --prefix=$RISCV
    $ make
    $ [sudo] make install
```

If your system uses the `yum` package manager, you can substitute
`yum install dtc` for the first step.


Build Steps on OpenBSD
----------------------

Install bash, gmake, dtc, and use clang.

    $ pkg_add bash gmake dtc
    $ exec bash
    $ export CC=cc; export CXX=c++
    $ mkdir build
    $ cd build
    $ ../configure --prefix=$RISCV
    $ gmake
    $ [doas] make install

Compiling and Running a Simple C Program
-------------------------------------------

Install spike (see Build Steps), riscv-gnu-toolchain, and riscv-pk.

Write a short C program and name it hello.c.  Then, compile it into a RISC-V
ELF binary named hello:

    $ riscv64-unknown-elf-gcc -o hello hello.c

Now you can simulate the program atop the proxy kernel:

    $ spike pk hello

Simulating a New Instruction
------------------------------------

Adding an instruction to the simulator requires two steps:

  1.  Describe the instruction's functional behavior in the file
      riscv/insns/<new_instruction_name>.h.  Examine other instructions
      in that directory as a starting point.

  2.  Add the opcode and opcode mask to riscv/opcodes.h.  Alternatively,
      add it to the riscv-opcodes package, and it will do so for you:
        ```
         $ cd ../riscv-opcodes
         $ vi opcodes       // add a line for the new instruction
         $ make install
        ```

  3.  Add the instruction to riscv/riscv.mk.in. Otherwise, the instruction
      will not be included in the build and will be treated as an illegal instruction.

  4.  Rebuild the simulator.

Interactive Debug Mode
---------------------------

To invoke interactive debug mode, launch spike with -d:

    $ spike -d pk hello

To see the contents of an integer register (0 is for core 0):

    : reg 0 a0

To see the contents of a floating point register:

    : fregs 0 ft0

or:

    : fregd 0 ft0

depending upon whether you wish to print the register as single- or double-precision.

To see the contents of a memory location (physical address in hex):

    : mem 2020

To see the contents of memory with a virtual address (0 for core 0):

    : mem 0 2020

You can advance by one instruction by pressing the enter key. You can also
execute until a desired equality is reached:

    : until pc 0 2020                   (stop when pc=2020)
    : until reg 0 mie a                 (stop when register mie=0xa)
    : until mem 2020 50a9907311096993   (stop when mem[2020]=50a9907311096993)

Alternatively, you can execute as long as an equality is true:

    : while mem 2020 50a9907311096993

You can continue execution indefinitely by:

    : r

At any point during execution (even without -d), you can enter the
interactive debug mode with `<control>-<c>`.

To end the simulation from the debug prompt, press `<control>-<c>` or:

    : q

Debugging With Gdb
------------------

An alternative to interactive debug mode is to attach using gdb. Because spike
tries to be like real hardware, you also need OpenOCD to do that. OpenOCD
doesn't currently know about address translation, so it's not possible to
easily debug programs that are run under `pk`. We'll use the following test
program:
```
$ cat rot13.c 
char text[] = "Vafgehpgvba frgf jnag gb or serr!";

// Don't use the stack, because sp isn't set up.
volatile int wait = 1;

int main()
{
    while (wait)
        ;

    // Doesn't actually go on the stack, because there are lots of GPRs.
    int i = 0;
    while (text[i]) {
        char lower = text[i] | 32;
        if (lower >= 'a' && lower <= 'm')
            text[i] += 13;
        else if (lower > 'm' && lower <= 'z')
            text[i] -= 13;
        i++;
    }

done:
    while (!wait)
        ;
}
$ cat spike.lds 
OUTPUT_ARCH( "riscv" )

SECTIONS
{
  . = 0x10110000;
  .text : { *(.text) }
  .data : { *(.data) }
}
$ riscv64-unknown-elf-gcc -g -Og -o rot13-64.o -c rot13.c
$ riscv64-unknown-elf-gcc -g -Og -T spike.lds -nostartfiles -o rot13-64 rot13-64.o
```

To debug this program, first run spike telling it to listen for OpenOCD:
```
$ spike --rbb-port=9824 -m0x10100000:0x20000 rot13-64
Listening for remote bitbang connection on port 9824.
```

In a separate shell run OpenOCD with the appropriate configuration file:
```
$ cat spike.cfg 
adapter driver remote_bitbang
remote_bitbang host localhost
remote_bitbang port 9824

set _CHIPNAME riscv
jtag newtap $_CHIPNAME cpu -irlen 5 -expected-id 0xdeadbeef

set _TARGETNAME $_CHIPNAME.cpu
target create $_TARGETNAME riscv -chain-position $_TARGETNAME

gdb_report_data_abort enable

init
halt
$ openocd -f spike.cfg
Open On-Chip Debugger 0.10.0-dev-00002-gc3b344d (2017-06-08-12:14)
...
riscv.cpu: target state: halted
```

In yet another shell, start your gdb debug session:
```
tnewsome@compy-vm:~/SiFive/spike-test$ riscv64-unknown-elf-gdb rot13-64
GNU gdb (GDB) 8.0.50.20170724-git
Copyright (C) 2017 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "--host=x86_64-pc-linux-gnu --target=riscv64-unknown-elf".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
<http://www.gnu.org/software/gdb/documentation/>.
For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from rot13-64...done.
(gdb) target remote localhost:3333
Remote debugging using localhost:3333
0x0000000010010004 in main () at rot13.c:8
8	    while (wait)
(gdb) print wait
$1 = 1
(gdb) print wait=0
$2 = 0
(gdb) print text
$3 = "Vafgehpgvba frgf jnag gb or serr!"
(gdb) b done 
Breakpoint 1 at 0x10110064: file rot13.c, line 22.
(gdb) c
Continuing.
Disabling abstract command writes to CSRs.

Breakpoint 1, main () at rot13.c:23
23	    while (!wait)
(gdb) print wait
$4 = 0
(gdb) print text
...
```
