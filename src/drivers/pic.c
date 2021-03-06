/*******************************************************************************
 *
 * File: pic.c
 *
 * Author: Alexy Torres Aurora Dugo
 *
 * Date: 17/12/2017
 *
 * Version: 1.0
 *
 * PIC (programmable interrupt controler) driver.
 * Allows to remmap the PIC IRQ, set the IRQs mask, manage EoI.
 ******************************************************************************/

#include "../sync/lock.h"          /* spinlock */
#include "../cpu/cpu.h"            /* outb */
#include "../lib/stdint.h"         /* Generic int types */
#include "../lib/stddef.h"         /* OS_RETURN_E, NULL */
#include "../core/kernel_output.h" /* kernel_success */

#include "../debug.h"      /* kernel_serial_debug */

/* Header file */
#include "pic.h"

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

static lock_t pic_lock;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

OS_RETURN_E init_pic(void)
{
    /* Initialize the master, remap IRQs */
    outb(PIC_ICW1_ICW4 | PIC_ICW1_INIT, PIC_MASTER_COMM_PORT);
    outb(PIC0_BASE_INTERRUPT_LINE, PIC_MASTER_DATA_PORT);
    outb(0x4,  PIC_MASTER_DATA_PORT);
    outb(0x1,  PIC_MASTER_DATA_PORT);

    /* Initialize the slave, remap IRQs */
    outb(PIC_ICW1_ICW4 | PIC_ICW1_INIT, PIC_SLAVE_COMM_PORT);
    outb(PIC1_BASE_INTERRUPT_LINE, PIC_SLAVE_DATA_PORT);
    outb(0x2,  PIC_SLAVE_DATA_PORT);
    outb(0x1,  PIC_SLAVE_DATA_PORT);

    /* Set EOI for both PICs. */
    outb(PIC_EOI, PIC_MASTER_COMM_PORT);
    outb(PIC_EOI, PIC_SLAVE_COMM_PORT);

    /* Disable all IRQs */
    outb(0xFF, PIC_MASTER_DATA_PORT);
    outb(0xFF, PIC_SLAVE_DATA_PORT);

    spinlock_init(&pic_lock);

    return OS_NO_ERR;
}

OS_RETURN_E set_IRQ_PIC_mask(const uint32_t irq_number, const uint8_t enabled)
{
    uint8_t  init_mask;
    uint32_t cascading_number;

    if(irq_number > PIC_MAX_IRQ_LINE)
    {
        return OS_ERR_NO_SUCH_IRQ_LINE;
    }

    spinlock_lock(&pic_lock);

    /* Manage master PIC */
    if(irq_number < 8)
    {
        /* Retrieve initial mask */
        init_mask = inb(PIC_MASTER_DATA_PORT);

        /* Set new mask value */
        if(!enabled)
        {
            init_mask |= 1 << irq_number;
        }
        else
        {
            init_mask &= ~(1 << irq_number);
        }

        /* Set new mask */
        outb(init_mask, PIC_MASTER_DATA_PORT);
    }

    /* Manage slave PIC. WARNING, cascading has to be enabled */
    if(irq_number > 7)
    {
        /* Set new IRQ number */
        cascading_number = irq_number - 8;

        /* Retrieve initial mask */
        init_mask = inb(PIC_SLAVE_DATA_PORT);

        /* Set new mask value */
        if(!enabled)
        {
            init_mask |= 1 << cascading_number;
        }
        else
        {
            init_mask &= ~(1 << cascading_number);
        }

        /* Set new mask */
        outb(init_mask, PIC_SLAVE_DATA_PORT);
    }

    #ifdef DEBUG_PIC
    kernel_serial_debug("PIC mask IRQ %d: %d\n", irq_number, enabled);
    #endif

    spinlock_unlock(&pic_lock);

    return OS_NO_ERR;
}

OS_RETURN_E set_IRQ_PIC_EOI(const uint32_t irq_number)
{
    if(irq_number > PIC_MAX_IRQ_LINE)
    {
        return OS_ERR_NO_SUCH_IRQ_LINE;
    }

    /* End of interrupt signal */
    if(irq_number > 7)
    {
        outb(PIC_EOI, PIC_SLAVE_COMM_PORT);
    }
    outb(PIC_EOI, PIC_MASTER_COMM_PORT);

    #ifdef DEBUG_PIC
    kernel_serial_debug("PIC EOI IRQ %d\n", irq_number);
    #endif

    return OS_NO_ERR;
}
