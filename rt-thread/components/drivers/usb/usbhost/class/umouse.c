/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-01-03     Yi Qiu      first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/usb_host.h>
#include "hid.h"
#include "nu_disp.h"

#if defined(RT_USBH_HID) && defined(RT_USBH_HID_MOUSE)
static struct uprotocal mouse_protocal;

#define MOUSE_SCALING 0x02
static rt_bool_t lkey_down = RT_FALSE;

typedef struct
{
    union
    {
        uint8_t B;   // Button
        struct
        {
            uint8_t BL: 1;
            uint8_t BR: 1;
            uint8_t BM: 1;
            uint8_t DevSpecific: 5;
        } S_B;
    };
    uint8_t X;   // Displacement
    uint8_t Y;   // Displacement
    uint8_t D;   // Displacement
} S_UMOUSE_CXT;

typedef struct
{
    rt_uint16_t x;
    rt_uint16_t y;
} S_MOUSE;
static S_MOUSE emouse = {BSP_LCD_WIDTH/2, BSP_LCD_HEIGHT/2};

static uint32_t u32HaveMouse = 0;

void nu_touch_inputevent_cb(rt_int16_t x, rt_int16_t y, rt_uint8_t state);

void mouse_cursor_hide(void)
{
    DISP_Trigger(eLayer_Cursor, 0);
}

void mouse_cursor_show(void)
{
    DISP_Trigger(eLayer_Cursor, 1);
}

static rt_err_t rt_usbh_hid_mouse_callback(void *arg)
{
    S_UMOUSE_CXT *psUMouseCxt;

    struct uhid *hid;
    rt_uint16_t xoffset = 0;
    rt_uint16_t yoffset = 0;
    hid = (struct uhid *)arg;
    psUMouseCxt = (S_UMOUSE_CXT *)&hid->buffer[0];

    if (!u32HaveMouse)
    {
        mouse_cursor_show();
        u32HaveMouse = 1;
    }

    //rt_kprintf("hid 0x%x 0x%x 0x%x\n", psUMouseCxt->B, psUMouseCxt->X, psUMouseCxt->Y);

    if (psUMouseCxt->X != 0)
    {
        if (psUMouseCxt->X > 127)
        {
            xoffset = (256 - psUMouseCxt->X) * MOUSE_SCALING;
            if (emouse.x > xoffset)
            {
                emouse.x -= xoffset;
            }
            else
            {
                emouse.x = 0;
            }
        }
        else
        {
            xoffset = (psUMouseCxt->X) * MOUSE_SCALING;
            if ((emouse.x + xoffset) < BSP_LCD_WIDTH)
            {
                emouse.x += xoffset;
            }
            else
            {
                emouse.x = BSP_LCD_WIDTH;
            }
        }
    }
    if (psUMouseCxt->Y != 0)
    {
        if (psUMouseCxt->Y > 127)
        {
            yoffset = (256 - psUMouseCxt->Y) * MOUSE_SCALING;
            if (emouse.y > yoffset)
            {
                emouse.y -= yoffset;
            }
            else
            {
                emouse.y = 0;
            }
        }
        else
        {
            yoffset = psUMouseCxt->Y * MOUSE_SCALING;
            if (emouse.y + yoffset < BSP_LCD_HEIGHT)
            {
                emouse.y += yoffset;
            }
            else
            {
                emouse.y = BSP_LCD_HEIGHT;
            }
        }
    }
    if (xoffset != 0 || yoffset != 0)
    {
        DISP_SetCursorPosition(emouse.x, emouse.y);
    }
    else
    {
        /* Workaround: Send down event before up event */
        nu_touch_inputevent_cb(emouse.x, emouse.y, RT_TOUCH_EVENT_DOWN);
        rt_thread_mdelay(50);
        nu_touch_inputevent_cb(emouse.x, emouse.y, RT_TOUCH_EVENT_UP);
        lkey_down = RT_FALSE;
        return RT_EOK;
    }

    if (psUMouseCxt->S_B.BL)
    {
        if (lkey_down == RT_FALSE)
        {
            //rt_kprintf("mouse left key press down\n");
            lkey_down = RT_TRUE;
        }
        nu_touch_inputevent_cb(emouse.x, emouse.y, RT_TOUCH_EVENT_DOWN);
    }
    else if (lkey_down == RT_TRUE)
    {
        //rt_kprintf("mouse left key press up\n");
        lkey_down = RT_FALSE;
        nu_touch_inputevent_cb(emouse.x, emouse.y, RT_TOUCH_EVENT_UP);
    }

    return RT_EOK;
}

static void mouse_task(void *param)
{
    struct uhintf *intf = (struct uhintf *)param;

    while (1)
    {
        if (rt_usb_hcd_pipe_xfer(intf->device->hcd,
                                 ((struct uhid *)intf->user_data)->pipe_in,
                                 ((struct uhid *)intf->user_data)->buffer,
                                 ((struct uhid *)intf->user_data)->pipe_in->ep.wMaxPacketSize,
                                 USB_TIMEOUT_BASIC) == 0)
        {
            break;
        }

        rt_usbh_hid_mouse_callback(intf->user_data);
    }
}

static rt_err_t rt_usbh_hid_mouse_init(void *arg)
{
    rt_thread_t mouse_thread;
    struct uhintf *intf = (struct uhintf *)arg;

    RT_ASSERT(intf != RT_NULL);

    rt_usbh_hid_set_protocal(intf, 0);

    rt_usbh_hid_set_idle(intf, 0, 0);

    mouse_thread = rt_thread_create("mouse0", mouse_task, intf, 2048, 5, 100);
    rt_thread_startup(mouse_thread);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("start usb mouse\n"));
    return RT_EOK;
}

/**
 * This function will define the hid mouse protocal, it will be register to the protocal list.
 *
 * @return the keyboard protocal structure.
 */
uprotocal_t rt_usbh_hid_protocal_mouse(void)
{
    mouse_protocal.pro_id = USB_HID_MOUSE;
    mouse_protocal.init = rt_usbh_hid_mouse_init;
    mouse_protocal.callback = rt_usbh_hid_mouse_callback;

    return &mouse_protocal;
}

#endif

