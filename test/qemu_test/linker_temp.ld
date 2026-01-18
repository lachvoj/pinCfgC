/*
 * Linker Script
 *
 * Memory layout: for tests
 *   - 65KB Flash @ 0x08000000
 *   - 20KB  SRAM  @ 0x20000000
 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 32K
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector table at start of flash */
    .vectors : {
        KEEP(*(.vectors))
    } > FLASH

    /* Code section */
    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        
        /* ARM exception index (needed for C++) */
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* Initialized data (loaded into flash, copied to RAM at startup) */
    _sidata = LOADADDR(.data);
    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    /* Uninitialized data */
    .bss : {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = .;
    } > SRAM

    /* Heap for malloc (between .bss and stack) */
    .heap : {
        . = ALIGN(8);
        _heap_start = .;
        end = .;              /* Symbol required by rdimon _sbrk */
        _end = .;             /* Alternative name */
        . = . + 8K;  /* 8KB heap */
        . = ALIGN(8);
        _heap_end = .;
    } > SRAM

    /* Stack (grows downward from end of SRAM) */
    .stack : {
        . = ALIGN(8);
        _estack = .;
    } > SRAM

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.ARM.attributes*)
        *(.comment)
    }
}
