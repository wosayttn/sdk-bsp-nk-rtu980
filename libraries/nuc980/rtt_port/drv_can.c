/**************************************************************************//**
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2020-12-12      Wayne        First version
*
******************************************************************************/

#include <rtconfig.h>

#if defined(BSP_USING_CAN)

#include <rtdevice.h>
#include <rthw.h>
#include "NuMicro.h"
#include <drv_sys.h>

/* Private Define ---------------------------------------------------------------*/
#define RX_MSG_ID_INDEX        16
#define IS_CAN_STDID(STDID)   ((STDID) <= 0x7FFU)
#define IS_CAN_EXTID(EXTID)   ((EXTID) <= 0x1FFFFFFFU)
#define IS_CAN_DLC(DLC)       ((DLC) <= 8U)

#define CAN_DISABLE_AUTO_RETRANSMISSION(can) ((can)->CON |= CAN_CON_DAR_Msk)
#define CAN_ENABLE_AUTO_RETRANSMISSION(can)  ((can)->CON &= ~CAN_CON_DAR_Msk)

/* Default config for serial_configure structure */
#define NU_CAN_CONFIG_DEFAULT                  \
{                                              \
    CAN1MBaud,           /* 1M bits/s       */ \
    RT_CANMSG_BOX_SZ,    /* message box max size */ \
    RT_CANSND_BOX_NUM,   /* message box number   */ \
    RT_CAN_MODE_NORMAL,  /* Normal mode     */ \
    0,                   /* privmode        */ \
    0,                   /* reserved        */ \
    100,                 /* Timeout Tick    */ \
}

enum
{
    CAN_START = -1,
#if defined(BSP_USING_CAN0)
    CAN0_IDX,
#endif
#if defined(BSP_USING_CAN1)
    CAN1_IDX,
#endif
#if defined(BSP_USING_CAN2)
    CAN2_IDX,
#endif
#if defined(BSP_USING_CAN3)
    CAN3_IDX,
#endif
    CAN_CNT,
};

/* Private Typedef --------------------------------------------------------------*/
struct nu_can
{
    struct rt_can_device dev;
    char *name;
    CAN_T *base;
    IRQn_Type irqn;
    E_SYS_IPRST rstidx;
    E_SYS_IPCLK clkidx;
    uint32_t int_flag;

    uint32_t u32LastReportTime;
    uint32_t u32LastSendPkg;
};
typedef struct nu_can *nu_can_t;

/* Private functions ------------------------------------------------------------*/
static rt_err_t nu_can_configure(struct rt_can_device *can, struct can_configure *cfg);
static rt_err_t nu_can_control(struct rt_can_device *can, int cmd, void *arg);
static int nu_can_sendmsg(struct rt_can_device *can, const void *buf, rt_uint32_t boxno);
static int nu_can_recvmsg(struct rt_can_device *can, void *buf, rt_uint32_t boxno);
static void nu_can_isr(int vector, void *param);

static struct nu_can nu_can_arr[] =
{
#if defined(BSP_USING_CAN0)
    {
        .name = "can0",
        .base = CAN0,
        .irqn =  IRQ_CAN0,
        .rstidx = CAN0RST,
        .clkidx = CAN0CKEN,
    },
#endif
#if defined(BSP_USING_CAN1)
    {
        .name = "can1",
        .base = CAN1,
        .irqn =  IRQ_CAN1,
        .rstidx = CAN1RST,
        .clkidx = CAN1CKEN,
    },
#endif
#if defined(BSP_USING_CAN2)
    {
        .name = "can2",
        .base = CAN2,
        .irqn =  IRQ_CAN2,
        .rstidx = CAN2RST,
        .clkidx = CAN2CKEN,
    },
#endif
#if defined(BSP_USING_CAN3)
    {
        .name = "can3",
        .base = CAN3,
        .irqn =  IRQ_CAN3,
        .rstidx = CAN3RST,
        .clkidx = CAN3CKEN,
    },
#endif
}; /* struct nu_can */

/* Public functions ------------------------------------------------------------*/

/* Private variables ------------------------------------------------------------*/
static const struct rt_can_ops nu_can_ops =
{
    .configure = nu_can_configure,
    .control = nu_can_control,
    .sendmsg = nu_can_sendmsg,
    .recvmsg = nu_can_recvmsg,
};

static const struct can_configure nu_can_default_config = NU_CAN_CONFIG_DEFAULT;

static const char *szLEC[] =
{
    /*
    0: No Error
    */
    "No Error",

    /*
    1: More than 5 equal bits in a sequence have occurred in a part of a received message where this is not allowed.
    */
    "Stuff Error",

    /*
    2: A fixed format part of a received frame has the wrong format.
    */
    "Form Error",

    /*
    3: The message this CAN Core transmitted was not acknowledged by another node.
    */
    "Ack Error",

    /*
    4: During the transmission of a message (with the exception of the arbitration field), the device wanted to send a recessive level
       (bit of logical value ????, but the monitored bus value was dominant
    */
    "Bit1 Error",

    /*
    5: During the transmission of a message (or acknowledge bit, or active error flag, or overload flag),
       though the device wanted to send a dominant level (data or identifier bit logical value ????, but the monitored Bus
       value was recessive. During bus-off recovery, this status is set each time a sequence of 11 recessive bits has been
       monitored. This enables the CPU to monitor the proceedings of the bus-off recovery sequence (indicating the bus is not
       stuck at dominant or continuously disturbed).
    */
    "Bit0 Error",

    /*
    6: The CRC check sum was incorrect in the message received, the CRC received for an
       incoming message does not match with the calculated CRC for the received data.
    */
    "CRC Error",

    /*
    7: When the LEC shows the value ???? no CAN bus event was detected since the CPU wrote this value to the LEC.
    */
    "Unused"
};

static void nu_can_ie(nu_can_t psNuCAN)
{
    uint32_t u32CanIE = CAN_CON_IE_Msk;

    if (psNuCAN->int_flag & (RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX))
    {
        u32CanIE |= CAN_CON_SIE_Msk;
    }
    else
    {
        u32CanIE &= ~CAN_CON_SIE_Msk;
    }

    if (psNuCAN->int_flag & RT_DEVICE_CAN_INT_ERR)
    {
        u32CanIE |= CAN_CON_EIE_Msk;
    }
    else
    {
        u32CanIE &= ~CAN_CON_EIE_Msk;
    }

    if (u32CanIE & (CAN_CON_SIE_Msk | CAN_CON_EIE_Msk))
    {
        CAN_EnableInt(psNuCAN->base, u32CanIE);

        /* Enable interrupt. */
        rt_hw_interrupt_umask(psNuCAN->irqn);
    }
    else
    {
        u32CanIE |= (CAN_CON_IE_Msk | CAN_CON_SIE_Msk);
        CAN_DisableInt(psNuCAN->base, u32CanIE);

        /* Disable interrupt. */
        rt_hw_interrupt_mask(psNuCAN->irqn);
    }
}

static void nu_can_busoff_recovery(CAN_T *can)
{
    uint32_t regCon, regTest;

    rt_tick_t bBegin;

    regCon = can->CON;
    regTest = can->TEST;

    /* To release busoff pin */
    CAN_EnterInitMode(can, regCon & ~(CAN_CON_IE_Msk | CAN_CON_SIE_Msk | CAN_CON_EIE_Msk));

    if (regCon & CAN_CON_TEST_Msk)
        CAN_EnterTestMode(can, regTest);

    CAN_LeaveInitMode(can);

#define DEF_BUSOFF_RECOVERY_TIMEOUT     2
    bBegin = rt_tick_get();
    while ((bBegin + DEF_BUSOFF_RECOVERY_TIMEOUT) > rt_tick_get())
    {
        if (can->ERR == 0)
        {
            CAN_EnableInt(can, regCon & (CAN_CON_IE_Msk | CAN_CON_SIE_Msk | CAN_CON_EIE_Msk));
            break;
        }
    }
}

rt_err_t nu_can_status_cb(struct rt_can_device *can, void *user_data)
{
#define DEF_TIMEOUT_COUNTER  3       // 10 * can->config.ticks
    nu_can_t psNuCAN = (nu_can_t)can;
    uint32_t u32Status = CAN_GET_INT_STATUS(psNuCAN->base);

    /* Catch bus-off exception */
    if (u32Status & CAN_STATUS_BOFF_Msk)
    {
        rt_kprintf("Catch busoff, recovery....\n");

        /* Do bus-off recovery */
        nu_can_busoff_recovery(psNuCAN->base);
    }

    /* Anti-zombie */
    if ((psNuCAN->u32LastSendPkg == can->status.sndpkg) && (can->status.sndchange == 1))
    {
        psNuCAN->u32LastReportTime++;

        if (psNuCAN->u32LastReportTime >= DEF_TIMEOUT_COUNTER)
        {
            rt_kprintf("Catch %s\n", szLEC[u32Status & CAN_STATUS_LEC_Msk]);

            can->status.sndchange = 0;
            psNuCAN->u32LastReportTime = 0;

            rt_hw_can_isr(can, RT_CAN_EVENT_TX_FAIL);
        }
    }

    psNuCAN->u32LastSendPkg = can->status.sndpkg;

    return RT_EOK;
}

/* Interrupt Handle Function  ----------------------------------------------------*/
static void nu_can_isr(int vector, void *param)
{
    nu_can_t psNuCAN  = (nu_can_t)param;

    /* Get base address of CAN register */
    CAN_T *base = psNuCAN->base;

    /* Get interrupt event */
    uint32_t u32IIDRstatus = CAN_GET_INT_PENDING_STATUS(base) & CAN_IIDR_INTID_Msk;

    /* Check Status Interrupt Flag (Error status Int and Status change Int) */
    if (u32IIDRstatus == 0x00008000)
    {
        /**************************/
        /* Status Change interrupt*/
        /**************************/
        uint32_t u32Status = CAN_GET_INT_STATUS(base);
        if (u32Status & CAN_STATUS_TXOK_Msk)
        {
            base->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/
#ifndef RT_CAN_USING_HDR
            if (psNuCAN->int_flag & RT_DEVICE_FLAG_INT_TX)
            {
                psNuCAN->dev.status.sndchange = 0;
                /*Using as Lisen,Loopback,Loopback+Lisen mode*/
                rt_hw_can_isr(&psNuCAN->dev, RT_CAN_EVENT_TX_DONE);
            }
#endif
        }

        if (u32Status & CAN_STATUS_RXOK_Msk)
        {
            base->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/
#ifndef RT_CAN_USING_HDR
            if (psNuCAN->int_flag & RT_DEVICE_FLAG_INT_RX)
            {
                /*Using as Listen,Loopback,Loopback+Listen mode*/
                rt_hw_can_isr(&psNuCAN->dev, RT_CAN_EVENT_RX_IND);
            }
#endif
        }
        /**************************/
        /* Error Status interrupt */
        /**************************/
        if (u32Status & CAN_STATUS_EWARN_Msk)
        {
            struct rt_can_status *psCANStatus = &psNuCAN->dev.status;
            uint32_t u32LEC = u32Status & CAN_STATUS_LEC_Msk;
            switch (u32LEC)
            {
            case RT_CAN_BUS_NO_ERR: //No Error
                return;

            /* For transmitting exceptions */
            case RT_CAN_BUS_ACK_ERR: //ACK ERROR
                psCANStatus->ackerrcnt++;
                /* To avoid stuck in ISR by enabling Disable Auto-Retransmission(DAR). */
                CAN_DISABLE_AUTO_RETRANSMISSION(base);
                break;

            case RT_CAN_BUS_IMPLICIT_BIT_ERR: //Bit1
            case RT_CAN_BUS_EXPLICIT_BIT_ERR: //Bit0
                psCANStatus->biterrcnt++;
                break;

            /* For receiving exceptions */
            case RT_CAN_BUS_BIT_PAD_ERR: //Stuff Error
                psCANStatus->bitpaderrcnt++;
                break;

            case RT_CAN_BUS_FORMAT_ERR: //Form Error
                psCANStatus->formaterrcnt++;
                break;

            case RT_CAN_BUS_CRC_ERR: //CRC Error
                psCANStatus->crcerrcnt++;
                break;

            default:
                break;
            }
            //rt_kprintf("[%s]EWARN INT(LEC:%s)\n", psNuCAN->name, szLEC[u32LEC]);
        }

        if (u32Status & CAN_STATUS_BOFF_Msk)
        {
            //rt_kprintf("[%s]Got Bus-off\n", psNuCAN->name);
        }

    }
#ifdef RT_CAN_USING_HDR
    /*IntId: 0x0001-0x0020, Number of Message Object which caused the interrupt.*/
    else if (u32IIDRstatus > 0 && u32IIDRstatus <= 32)
    {
        u32IIDRstatus--; // Get index.

        if ((psNuCAN->int_flag & RT_DEVICE_FLAG_INT_TX) &&
                (u32IIDRstatus < RX_MSG_ID_INDEX))
        {
            /*Message RAM 0~RX_MSG_ID_INDEX for CAN Tx using*/
            psNuCAN->dev.status.sndchange = 0;
            rt_hw_can_isr(&psNuCAN->dev, RT_CAN_EVENT_TX_DONE | ((u32IIDRstatus) << 8));
        }
        else if (psNuCAN->int_flag & RT_DEVICE_FLAG_INT_RX)
        {
            /*Message RAM RX_MSG_ID_INDEX~31 for CAN Rx using*/
            rt_hw_can_isr(&psNuCAN->dev, (RT_CAN_EVENT_RX_IND | ((u32IIDRstatus) << 8)));
        }
        CAN_CLR_INT_PENDING_BIT(base, u32IIDRstatus);     /* Clear Interrupt Pending */
    }
#endif
}

static rt_err_t nu_can_configure(struct rt_can_device *can, struct can_configure *cfg)
{
    nu_can_t psNuCAN  = (nu_can_t)can;
    uint32_t u32CANMode;

    RT_ASSERT(can);
    RT_ASSERT(cfg);

    /* Get base address of CAN register */
    CAN_T *base = psNuCAN->base;

    /* Reset this module */
    nu_sys_ip_reset(psNuCAN->rstidx);

    u32CANMode = (cfg->mode == RT_CAN_MODE_NORMAL) ? CAN_NORMAL_MODE : CAN_BASIC_MODE;

    /*Set the CAN Bit Rate and Operating mode*/
    if (CAN_Open(base, cfg->baud_rate, u32CANMode) != cfg->baud_rate)
        goto exit_nu_can_configure;

    switch (cfg->mode)
    {
    case RT_CAN_MODE_NORMAL:
#ifdef RT_CAN_USING_HDR
        CAN_LeaveTestMode(base);
#else
        CAN_EnterTestMode(base, CAN_TEST_BASIC_Msk);
#endif
        break;
    case RT_CAN_MODE_LISTEN:
        CAN_EnterTestMode(base, CAN_TEST_BASIC_Msk | CAN_TEST_SILENT_Msk);
        break;
    case RT_CAN_MODE_LOOPBACK:
        CAN_EnterTestMode(base, CAN_TEST_BASIC_Msk | CAN_TEST_LBACK_Msk);
        break;
    case RT_CAN_MODE_LOOPBACKANLISTEN:
        CAN_EnterTestMode(base, CAN_TEST_BASIC_Msk | CAN_TEST_SILENT_Msk | CAN_TEST_LBACK_Msk);
        break;
    default:
        rt_kprintf("Unsupported Operating mode");
        goto exit_nu_can_configure;
    }

    /* register status indication function */
    can->status_indicate.ind = nu_can_status_cb;
    can->status_indicate.args = RT_NULL;

    nu_can_ie(psNuCAN);

    return RT_EOK;

exit_nu_can_configure:

    CAN_Close(base);

    return -(RT_ERROR);
}

static rt_err_t nu_can_control(struct rt_can_device *can, int cmd, void *arg)
{
    rt_uint32_t argval = (rt_uint32_t)arg;
    nu_can_t psNuCAN = (nu_can_t)can;

    RT_ASSERT(can);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_SET_INT:
        psNuCAN->int_flag |= argval;
        nu_can_ie(psNuCAN);
        break;

    case RT_DEVICE_CTRL_CLR_INT:
        psNuCAN->int_flag &= ~argval;
        nu_can_ie(psNuCAN);
        break;

    case RT_CAN_CMD_SET_FILTER:
    {
        struct rt_can_filter_config *filter_cfg = (struct rt_can_filter_config *)arg;

        for (int i = 0; i < filter_cfg->count; i++)
        {
            /*set the filter message object*/
            if (filter_cfg->items[i].mode == 1)
            {

                if (CAN_SetRxMsgObjAndMsk(psNuCAN->base,
                                          MSG((filter_cfg->items[i].hdr_bank == -1) ? RX_MSG_ID_INDEX : RX_MSG_ID_INDEX + filter_cfg->items[i].hdr_bank),
                                          filter_cfg->items[i].ide,
                                          filter_cfg->items[i].id,
                                          filter_cfg->items[i].mask,
                                          FALSE) == FALSE)
                {
                    return -(RT_ERROR);
                }

            }
            else
            {

                /*set the filter message object*/
                if (CAN_SetRxMsgAndMsk(psNuCAN->base,
                                       MSG((filter_cfg->items[i].hdr_bank == -1) ? RX_MSG_ID_INDEX : RX_MSG_ID_INDEX + filter_cfg->items[i].hdr_bank),
                                       filter_cfg->items[i].ide,
                                       filter_cfg->items[i].id,
                                       filter_cfg->items[i].mask) == FALSE)
                {
                    return -(RT_ERROR);
                }

            } // if (filter_cfg->items[i].mode == 1)

        } // for (int i = 0; i < filter_cfg->count; i++)

    } // case RT_CAN_CMD_SET_FILTER:
    break;

    case RT_CAN_CMD_SET_MODE:
        if ((argval == RT_CAN_MODE_NORMAL) ||
                (argval == RT_CAN_MODE_LISTEN) ||
                (argval == RT_CAN_MODE_LOOPBACK) ||
                (argval == RT_CAN_MODE_LOOPBACKANLISTEN))
        {
            if (argval != can->config.mode)
            {
                can->config.mode = argval;
                return nu_can_configure(can, &can->config);
            }
        }
        else
        {
            return -(RT_ERROR);
        }
        break;

    case RT_CAN_CMD_SET_BAUD:
    {
        if ((argval == CAN1MBaud) ||
                (argval == CAN800kBaud) ||
                (argval == CAN500kBaud) ||
                (argval == CAN250kBaud) ||
                (argval == CAN125kBaud) ||
                (argval == CAN100kBaud) ||
                (argval == CAN50kBaud) ||
                (argval == CAN20kBaud) ||
                (argval == CAN10kBaud))
        {
            if (argval != can->config.baud_rate)
            {
                can->config.baud_rate = argval;
                return nu_can_configure(can, &can->config);
            }
        }
        else
        {
            return -(RT_ERROR);
        }
    }
    break;

    case RT_CAN_CMD_SET_PRIV:
        if (argval != RT_CAN_MODE_PRIV &&
                argval != RT_CAN_MODE_NOPRIV)
        {
            return -(RT_ERROR);
        }
        if (argval != can->config.privmode)
        {
            can->config.privmode = argval;
            return nu_can_configure(can, &can->config);
        }
        break;

    case RT_CAN_CMD_GET_STATUS:
    {
        rt_uint32_t errtype, status;
        RT_ASSERT(arg);

        errtype = psNuCAN->base->ERR;
        status = CAN_GET_INT_STATUS(psNuCAN->base);

        /*Receive Error Counter, return value is with Receive Error Passive.*/
        can->status.rcverrcnt = (errtype >> 8);

        /*Transmit Error Counter*/
        can->status.snderrcnt = (errtype & 0xFF);

        /*Last Error Type*/
        can->status.lasterrtype = status & CAN_STATUS_LEC_Msk;

        /*Status error code*/
        can->status.errcode = (status & CAN_STATUS_BOFF_Msk) ? 4 :   // 4~7
                              (status & CAN_STATUS_EWARN_Msk) ? 1 :  // 1
                              (status & CAN_STATUS_EPASS_Msk) ? 2 :  // 2~3
                              0;                                     // 0

        rt_memcpy(arg, &can->status, sizeof(struct rt_can_status));
    }
    break;

    default:
        return -(RT_EINVAL);

    }

    return RT_EOK;
}

static int nu_can_sendmsg(struct rt_can_device *can, const void *buf, rt_uint32_t boxno)
{
    STR_CANMSG_T tMsg = {0};
    struct rt_can_msg *pmsg;
    nu_can_t psNuCAN = (nu_can_t)can;

    RT_ASSERT(can);
    RT_ASSERT(buf);

    pmsg = (struct rt_can_msg *) buf;

    if (pmsg->ide == RT_CAN_STDID && IS_CAN_STDID(pmsg->id))
    {
        /* Standard ID (11 bits)*/
        tMsg.IdType = CAN_STD_ID;
        tMsg.Id  = pmsg->id ;
    }
    else if (pmsg->ide == RT_CAN_EXTID && IS_CAN_EXTID(pmsg->id))
    {
        /* Extended ID (29 bits)*/
        tMsg.IdType = CAN_EXT_ID;
        tMsg.Id = pmsg->id ;
    }
    else
    {
        goto exit_nu_can_sendmsg;
    }

    if (pmsg->rtr == RT_CAN_DTR)
    {
        /* Data frame */
        tMsg.FrameType = CAN_DATA_FRAME;
    }
    else if (pmsg->rtr == RT_CAN_RTR)
    {
        /* Remote frame */
        tMsg.FrameType = CAN_REMOTE_FRAME;
    }
    else
    {
        goto exit_nu_can_sendmsg;
    }

    /* Check the parameters */
    if (IS_CAN_DLC(pmsg->len))
    {
        tMsg.DLC = pmsg->len;
    }
    else
    {
        goto exit_nu_can_sendmsg;
    }

    if (pmsg->len > 0)
    {
        rt_memcpy(&tMsg.Data[0], &pmsg->data[0], pmsg->len);
    }

    /* Configure Msg RAM and send the Msg in the RAM. */
    CAN_ENABLE_AUTO_RETRANSMISSION(psNuCAN->base);
    if (CAN_Transmit(psNuCAN->base, boxno, &tMsg) == FALSE)
    {
        rt_kprintf("Failed transmit TX Msg in %d box.\n", boxno);

        goto exit_nu_can_sendmsg;
    }

    return RT_EOK;

exit_nu_can_sendmsg:

    return -(RT_ERROR);
}

static int nu_can_recvmsg(struct rt_can_device *can, void *buf, rt_uint32_t boxno)
{
    STR_CANMSG_T tMsg = {0};
    struct rt_can_msg *pmsg;
    nu_can_t psNuCAN = (nu_can_t)can;

    RT_ASSERT(can);
    RT_ASSERT(buf);

    pmsg = (struct rt_can_msg *) buf;

    /* get data */
    if (CAN_Receive(psNuCAN->base, boxno, &tMsg) == FALSE)
    {
        rt_kprintf("No available RX Msg in %d box.\n", boxno);
        return -(RT_ERROR);
    }

#ifdef RT_CAN_USING_HDR
    /* Hardware filter messages are valid */
    pmsg->hdr_index = boxno - RX_MSG_ID_INDEX;
    can->hdr[pmsg->hdr_index].connected = 1;
#endif

    pmsg->ide = (tMsg.IdType == CAN_STD_ID) ? RT_CAN_STDID : RT_CAN_EXTID;
    pmsg->rtr = (tMsg.FrameType == CAN_DATA_FRAME) ? RT_CAN_DTR : RT_CAN_RTR;
    pmsg->id  = tMsg.Id;
    pmsg->len = tMsg.DLC ;

    if (pmsg->len > 0)
        rt_memcpy(&pmsg->data[0], &tMsg.Data[0], pmsg->len);

    return RT_EOK;
}

/**
 * Hardware CAN Initialization
 */
static int rt_hw_can_init(void)
{
    int i;
    rt_err_t ret = RT_EOK;

    for (i = (CAN_START + 1); i < CAN_CNT; i++)
    {
        nu_can_arr[i].dev.config = nu_can_default_config;

#ifdef RT_CAN_USING_HDR
        nu_can_arr[i].dev.config.maxhdr = RT_CANMSG_BOX_SZ;
#endif

        /* Register CAN ISR */
        rt_hw_interrupt_install(nu_can_arr[i].irqn, nu_can_isr, &nu_can_arr[i], nu_can_arr[i].name);

        /* Enable IP engine clock */
        nu_sys_ipclk_enable(nu_can_arr[i].clkidx);

        /* Register can device */
        ret = rt_hw_can_register(&nu_can_arr[i].dev, nu_can_arr[i].name, &nu_can_ops, NULL);
        RT_ASSERT(ret == RT_EOK);
    }

    return (int)ret;
}
INIT_DEVICE_EXPORT(rt_hw_can_init);
#endif  //#if defined(BSP_USING_CAN)
