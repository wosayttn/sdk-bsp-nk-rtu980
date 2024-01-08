/**************************************************************************//**
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author           Notes
* 2023-11-10      Wayne            First version
*
******************************************************************************/
#ifndef __DRV_EMAC_H___
#define __DRV_EMAC_H___

#include "rtconfig.h"

typedef enum
{
    EMAC_START = -1,
#if defined(BSP_USING_EMAC0)
    EMAC0_IDX,
#endif
#if defined(BSP_USING_EMAC1)
    EMAC1_IDX,
#endif
    EMAC_CNT
} EMAC_IDX;

#define NIOCTL_PRIV_CMD              0x1000
#define NIOCTL_PRIV_CHANGE_MAC       (NIOCTL_PRIV_CMD+1)

int nu_emac_assign_mac_addr(EMAC_IDX evIdx, uint8_t *pu8MacAddr);

#endif // __DRV_EMAC_H___
