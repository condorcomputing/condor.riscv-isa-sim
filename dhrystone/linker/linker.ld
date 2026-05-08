/* there is no big endian support so all 3 names would be the same */
OUTPUT_FORMAT("elf64-littleriscv")
OUTPUT_ARCH( "riscv" )
ENTRY(_start)

SECTIONS
{

  /* set loc counter */
  . = 0x80000000;

  /* elf header marker */
  __ehdr_start = .;

  /* text: test code section */
  .text.init : { 
    *(.text.init)
  }

  /* Allocate 4KB each for to/fromhost in case we want to do more  */
  /* sophisticated communication between simulator and environment */

  . = ALIGN(0x1000);
  .tohost   : { 
    *(.tohost)
    . = . + 0x1000;
  }

  .fromhost : { 
    *(.fromhost)
    . = . + 0x1000;
  }

  /* Allocate 4KB each for to/fromhost in case we want to do more  */
  /* sophisticated communication between simulator and environment */
  .conio : { 
    *(.conio)
    . = . + 0x1000;
  }

  /* text segment */
  .text : {
    *(.text)
    *(.rodata)
  }

  /* small data closer to text than .data */
  .sdata : {
    __global_pointer$ = . + 0x800;
    *(.srodata.cst16) 
    *(.srodata.cst8) 
    *(.srodata.cst4) 
    *(.srodata.cst2) 
    *(.srodata*)
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  }

  /* data segment */
  .data : {
    *(.data) 
  }

  /* read only */
  .rodata : {
    *(.rodata)
  }

  /* small data bss segment */
  .sbss : {
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
  }

  /* all init to zero */
  .bss : { 
    __bss_start = .;
    *(.bss)
    __bss_end = .;
    PROVIDE(__bss_start = __bss_start);
  }

  /* thread-local data segment */
  /* There is redundancy here to satisfy a few different */
  /* compilation/linkage domains                         */
  .tdata :
  {
    _tls_data = .;
    _tdata_begin = .;
    *(.tdata.begin)
    *(.tdata)
    *(.tdata.end)
    _tdata_end = .;
  }

  .tbss :
  {
    _tbss_begin = .;
    *(.tbss)
    *(.tbss.end)
    _tbss_end = .;
  }

  STACK = ALIGN(16) + 0x1000;

  /* End of uninitalized data segement */
  _end = .;
  PROVIDE(_edata = _end);

}

