#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/lock.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

/*
 * ========== Memory Map ==========
 */
#define SRAM_BASE 0x20000000
#define SRAM_SIZE (20 * 1024) /* 20KB SRAM */
#define STACK_TOP (SRAM_BASE + SRAM_SIZE)

/* Entry point */
extern int main(void);

/*
 * ========== Vector Table ==========
 */

void Reset_Handler(void);
void Default_Handler(void);

__attribute__((section(".vectors"))) const void *vector_table[] = {
    (void *)STACK_TOP,       /* Initial stack pointer */
    (void *)Reset_Handler,   /* Reset handler */
    (void *)Default_Handler, /* NMI */
    (void *)Default_Handler, /* HardFault */
    (void *)Default_Handler, /* MemManage */
    (void *)Default_Handler, /* BusFault */
    (void *)Default_Handler, /* UsageFault */
};

/*
 * ========== Main Program ==========
 */

void Default_Handler(void)
{
    while (1)
    {
        ;
    }
}

/* Weak alias for SystemInit */
void SystemInit(void) __attribute__((weak));
void SystemInit(void)
{
    /* Default SystemInit does nothing */
    /* Can be overridden to initialize clocks, etc. */
}

/* Initialize semihosting (required by rdimon) */
extern void initialise_monitor_handles(void);

void Reset_Handler(void)
{
    /* Initialize .data section (copy from flash to RAM) */
    extern char _sidata, _sdata, _edata;
    char *src = &_sidata;
    char *dst = &_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    /* Zero .bss section */
    extern char _sbss, _ebss;
    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0;
    }

    /* Call SystemInit if provided */
    SystemInit();

    /* Initialize semihosting for I/O - THIS IS CRITICAL! */
    initialise_monitor_handles();

    /* Call main */
    main();

    // /* Test printf with various outputs */
    // printf("Hello from C++ printf!\n");

    // /* Test standard printf features */
    // printf("Test number: %d\n", 42);
    // printf("Test string: %s\n", "working");

    // /* Test fprintf to stderr */
    // fprintf(stderr, "Stderr message\n");

    /* If main returns, exit via semihosting */
    __asm volatile("movs r0, #0x18\n"   /* SYS_EXIT */
                   "ldr r1, =0x20026\n" /* ADP_Stopped_ApplicationExit */
                   "bkpt #0xAB\n"       /* Semihosting breakpoint */
    );

    /* Should never reach here, but loop just in case */
    exit(0);
}
