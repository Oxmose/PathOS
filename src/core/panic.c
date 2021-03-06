/*******************************************************************************
 *
 * File: panic.c
 *
 * Author: Alexy Torres Aurora Dugo
 *
 * Date: 16/12/2017
 *
 * Version: 1.0
 *
 * Kernel panic functions. Displays the CPU registers, the faulty instruction
 * and the interrupt ID and cause.
 ******************************************************************************/

#include "interrupts.h"          /* cpu_state_t, stack_state_t, PANIC_INT_LINE */
#include "kernel_output.h"       /* kernel_printf */
#include "../lib/stdint.h"       /* Generic int types */
#include "../cpu/cpu.h"          /* hlt cli */
#include "../drivers/vesa.h"     /* vesa_switch_vga_text */
#include "../drivers/graphic.h"  /* set_color_scheme */

/* Header file */
#include "panic.h"

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void panic(cpu_state_t* cpu_state, uint32_t int_id, stack_state_t* stack_state)
{
    /* Get EFLAGS one by one */
    int8_t cf_f = (stack_state->eflags & 0x1);
    int8_t pf_f = (stack_state->eflags & 0x4) >> 2;
    int8_t af_f = (stack_state->eflags & 0x10) >> 4;
    int8_t zf_f = (stack_state->eflags & 0x40) >> 6;
    int8_t sf_f = (stack_state->eflags & 0x80) >> 7;
    int8_t tf_f = (stack_state->eflags & 0x100) >> 8;
    int8_t if_f = (stack_state->eflags & 0x200) >> 9;
    int8_t df_f = (stack_state->eflags & 0x400) >> 10;
    int8_t of_f = (stack_state->eflags & 0x800) >> 11;
    int8_t nt_f = (stack_state->eflags & 0x4000) >> 14;
    int8_t rf_f = (stack_state->eflags & 0x10000) >> 16;
    int8_t vm_f = (stack_state->eflags & 0x20000) >> 17;
    int8_t ac_f = (stack_state->eflags & 0x40000) >> 18;
    int8_t id_f = (stack_state->eflags & 0x200000) >> 21;
    int8_t iopl0_f = (stack_state->eflags & 0x1000) >> 12;
    int8_t iopl1_f = (stack_state->eflags & 0x2000) >> 13;
    int8_t vif_f = (stack_state->eflags & 0x8000) >> 19;
    int8_t vip_f = (stack_state->eflags & 0x100000) >> 20;

    uint32_t CR0;
    uint32_t CR2;
    uint32_t CR3;
    uint32_t CR4;

    colorscheme_t panic_scheme;

    /* Fall back to VGA text display */
    vesa_switch_vga_text();

    panic_scheme.background = BG_RED;
    panic_scheme.foreground = FG_WHITE;
    panic_scheme.vga_color  = 1;

    set_color_scheme(panic_scheme);

    kernel_printf("#=============================    KERNEL PANIC    ============================#\n");
    kernel_printf("|                                                                             |\n");
    kernel_printf("| Reason: ");
    switch(int_id)
    {
        case 0:
            kernel_printf("Division by zero                        ");
            break;
        case 1:
            kernel_printf("Single-step interrupt                   ");
            break;
        case 2:
            kernel_printf("Non maskable interrupt                  ");
            break;
        case 3:
            kernel_printf("Breakpoint                              ");
            break;
        case 4:
            kernel_printf("Overflow                                ");
            break;
        case 5:
            kernel_printf("Bounds                                  ");
            break;
        case 6:
            kernel_printf("Invalid Opcode                          ");
            break;
        case 7:
            kernel_printf("Coprocessor not available               ");
            break;
        case 8:
            kernel_printf("Double fault                            ");
            break;
        case 9:
            kernel_printf("Coprocessor Segment Overrun             ");
            break;
        case 10:
            kernel_printf("Invalid Task State Segment              ");
            break;
        case 11:
            kernel_printf("Segment not present                     ");
            break;
        case 12:
            kernel_printf("Stack Fault                             ");
            break;
        case 13:
            kernel_printf("General protection fault                ");
            break;
        case 14:
            kernel_printf("Page fault                              ");
            break;
        case 16:
            kernel_printf("Math Fault                              ");
            break;
        case 17:
            kernel_printf("Alignment Check                         ");
            break;
        case 18:
            kernel_printf("Machine Check                           ");
            break;
        case 19:
            kernel_printf("SIMD Floating-Point Exception           ");
            break;
        case 20:
            kernel_printf("Virtualization Exception                ");
            break;
        case 21:
            kernel_printf("Control Protection Exception            ");
            break;
        case PANIC_INT_LINE:
            kernel_printf("Panic generated by the kernel           ");
            break;
        default:
            kernel_printf("Unknown                                 ");
    }

    __asm__ __volatile__("push %eax");
    __asm__ __volatile__("movl %cr0, %eax");
    __asm__ __volatile__("movl %%eax, %0" : "=rm"(CR0));
    __asm__ __volatile__("movl %cr2, %eax");
    __asm__ __volatile__("movl %%eax, %0" : "=rm"(CR2));
    __asm__ __volatile__("movl %cr3, %eax");
    __asm__ __volatile__("movl %%eax, %0" : "=rm"(CR3));
    __asm__ __volatile__("movl %cr4, %eax");
    __asm__ __volatile__("movl %%eax, %0" : "=rm"(CR4));
    __asm__ __volatile__("pop %eax");


    kernel_printf("INT ID: 0x%02x                |\n", int_id);
    kernel_printf("| Instruction [EIP]: 0x%08x                   Error code: 0x%08x      |\n", stack_state->eip, stack_state->error_code);
    kernel_printf("|                                                                             |\n");
    kernel_printf("|================================= CPU STATE =================================|\n");
    kernel_printf("|                                                                             |\n");
    kernel_printf("| EAX: 0x%08x  |  EBX: 0x%08x  |  ECX: 0x%08x  |  EDX: 0x%08x |\n", cpu_state->eax, cpu_state->ebx, cpu_state->ecx, cpu_state->edx);
    kernel_printf("| ESI: 0x%08x  |  EDI: 0x%08x  |  EBP: 0x%08x  |  ESP: 0x%08x |\n", cpu_state->esi, cpu_state->edi, cpu_state->ebp, cpu_state->esp);
    kernel_printf("| CR0: 0x%08x  |  CR2: 0x%08x  |  CR3: 0x%08x  |  CR4: 0x%08x |\n", CR0, CR2, CR3, CR4);
    kernel_printf("|                                                                             |\n");
    kernel_printf("|============================= SEGMENT REGISTERS =============================|\n");
    kernel_printf("|                                                                             |\n");
    kernel_printf("| CS: 0x%04x  |  DS: 0x%04x  |  SS: 0x%04x                                    |\n", stack_state->cs & 0xFFFF, cpu_state->ds & 0xFFFF, cpu_state->ss & 0xFFFF);
    kernel_printf("| ES: 0x%04x  |  FS: 0x%04x  |  GS: 0x%04x                                    |\n", cpu_state->es & 0xFFFF , cpu_state->fs & 0xFFFF , cpu_state->gs & 0xFFFF);
    kernel_printf("|                                                                             |\n");
    kernel_printf("|================================= EFLAGS REG ================================|\n");
    kernel_printf("|                                                                             |\n");
    kernel_printf("| CF: %d  |  PF: %d  |  AF: %d  |  ZF: %d  |  SF: %d  |  TF: %d  |  IF: %d  |  DF: %d |\n", cf_f, pf_f, af_f, zf_f, sf_f, tf_f, if_f, df_f);
    kernel_printf("| OF: %d  |  NT: %d  |  RF: %d  |  VM: %d  |  AC: %d  |  ID: %d                     |\n", of_f, nt_f, rf_f, vm_f, ac_f, id_f);
    kernel_printf("| IOPL: %d  |  VIF: %d  |  VIP: %d                                               |\n", (iopl0_f | iopl1_f << 1), vif_f, vip_f);
    kernel_printf("|                                                                             |\n");
    kernel_printf("|                                                                             |\n");
    kernel_printf("|                         LET'S HOPE IT WON'T EXPLODE                         |\n");
    kernel_printf("#=============================================================================#");

    /* We will never return from interrupt */
    while(1)
    {
        cli();
        hlt();
    }
}

void kernel_panic(void)
{
   __asm__ __volatile__("int %0" :: "i" (PANIC_INT_LINE));
}

/*********************************************************************************|
 *#=============================      OS PANIC      =============================#|
 *|                                                                              ||
 *| Reason: ###############################         INT ID: 0x##                 ||
 *|                                                                              ||
 *| Instruction: 0x########                                                      ||
 *|                                                                              ||
 *|================================= CPU STATE ==================================||
 *|                                                                              ||
 *| EAX: 0x########  |  EBX: 0x########  |  ECX: 0x########  |  EDX: 0x########  ||
 *| ESI: 0x########  |  EDI: 0x########  |  EBP: 0x########  |  ESP: 0x########  ||
 *|                                                                              ||
 *|============================= SEGMENT REGISTERS ==============================||
 *|                                                                              ||
 *| CS:  0x####  |  DS:  0x####  |  SS:  0x####                                  ||
 *| ES:  0x####  |  FS:  0x####  |  GS:  0x####                                  ||
 *|                                                                              ||
 *|================================= EFLAGS REG =================================||
 *|                                                                              ||
 *| CF: #  |  PF: #  |  AF: #  |  ZF: #  |  SF: #  |  TF: #  |  IF: #  |  DF: #  ||
 *| OF: #  |  NT: #  |  RF: #  |  VM: #  |  AC: #  |  ID: #                      ||
 *| IOPL: #  |  VIF: #  |  VIP: #                                                ||
 *|                                                                              ||
 *|                          LET'S HOPE IT WON'T EXPLODE                         ||
 *#==============================================================================#|
 *********************************************************************************|
 */
