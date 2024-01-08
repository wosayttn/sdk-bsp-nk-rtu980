/**************************************************************************//**
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author           Notes
* 2020-11-11      Wayne            First version
*
******************************************************************************/

#include <rtthread.h>
#include <rthw.h>
#include "board.h"
#include "drv_uart.h"
#include "drv_sys.h"

#if defined(BSP_USING_MMU)

static struct mem_desc hw_mem_desc[] =
{
    {
        /* BOARD_SDRAM_SIZE cached DDR memory */
        0x00000000,
        BOARD_SDRAM_SIZE - BOARD_SDRAM_NCNB_SIZE - 1,
        0x00000000,
        RW_CB
    },
    {
        /* Non-cacheable memory */
        BOARD_SDRAM_SIZE - BOARD_SDRAM_NCNB_SIZE,
        0xFFFFFFFF - (BOARD_SDRAM_SIZE - BOARD_SDRAM_NCNB_SIZE) + 1,
        BOARD_SDRAM_SIZE - BOARD_SDRAM_NCNB_SIZE,
        RW_NCNB
    },
};
#endif

/**
 * This function will initial M487 board.
 */
RT_WEAK void rt_hw_board_init(void)
{
    /* initialize base clock */
    nu_clock_base_init();

    /* initialize peripheral pin function */
    nu_pin_init();

#if defined(BSP_USING_MMU)
    /* initialize mmu */
    rt_hw_mmu_init(&hw_mem_desc[0], sizeof(hw_mem_desc) / sizeof(hw_mem_desc[0]));
#else
    /* disable I/D cache */
    mmu_disable_dcache();
    mmu_disable_icache();
    mmu_disable();
    mmu_invalidate_tlb();
#endif

    /* initialize hardware interrupt */
    rt_hw_interrupt_init();

    /* initialize systick */
    rt_hw_systick_init();

#ifdef RT_USING_HEAP
    /* init memory system */
    rt_system_heap_init((void *)BOARD_HEAP_START, (void *)BOARD_HEAP_END);
#endif

    /* initialize uart */
    rt_hw_uart_init();

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
