/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include "rtthread.h"

#define CHERRYUSB_VERSION 0x001000

/* ================ USB common Configuration ================ */

#define CONFIG_USB_PRINTF     rt_kprintf

#define usb_malloc(size) rt_malloc(size)
#define usb_free(ptr)    rt_free(ptr)

/* Fix unaligned issue */
#define memcpy rt_memcpy
#define memset rt_memset

#ifndef CONFIG_USB_DBG_LEVEL
    #define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#endif

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE

/* data align size when use dma */
#ifndef CONFIG_USB_ALIGN_SIZE
    #define CONFIG_USB_ALIGN_SIZE 64
#endif

/* attribute data into no cache ram */
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))

/* ================= USB Device Stack Configuration ================ */

/* Ep0 max transfer buffer, specially for receiving data from ep0 out */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 256

/* Setup packet log for debug */
//#define CONFIG_USBDEV_SETUP_LOG_PRINT

/* Check if the input descriptor is correct */
//#define CONFIG_USBDEV_DESC_CHECK

/* Enable test mode */
// #define CONFIG_USBDEV_TEST_MODE

#define CONFIG_USBDEV_TX_THREAD
#define CONFIG_USBDEV_RX_THREAD

#ifdef CONFIG_USBDEV_TX_THREAD
    #ifndef CONFIG_USBDEV_TX_PRIO
        #define CONFIG_USBDEV_TX_PRIO 4
    #endif
    #ifndef CONFIG_USBDEV_TX_STACKSIZE
        #define CONFIG_USBDEV_TX_STACKSIZE 4096
    #endif
#endif

#ifdef CONFIG_USBDEV_RX_THREAD
    #ifndef CONFIG_USBDEV_RX_PRIO
        #define CONFIG_USBDEV_RX_PRIO 4
    #endif
    #ifndef CONFIG_USBDEV_RX_STACKSIZE
        #define CONFIG_USBDEV_RX_STACKSIZE 4096
    #endif
#endif

#ifndef CONFIG_USBDEV_MSC_BLOCK_SIZE
    #define CONFIG_USBDEV_MSC_BLOCK_SIZE 512
#endif

#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
    #define CONFIG_USBDEV_MSC_MANUFACTURER_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
    #define CONFIG_USBDEV_MSC_PRODUCT_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
    #define CONFIG_USBDEV_MSC_VERSION_STRING "0.01"
#endif

#ifndef CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE
    #define CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE 156
#endif

#ifndef CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE
    #define CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE 1536
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_ID
    #define CONFIG_USBDEV_RNDIS_VENDOR_ID 0x0000ffff
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_DESC
    #define CONFIG_USBDEV_RNDIS_VENDOR_DESC "CherryUSB"
#endif

#define CONFIG_USBDEV_RNDIS_USING_LWIP

/* ================ USB HOST Stack Configuration ================== */

#define CONFIG_USBHOST_MAX_RHPORTS          2
#define CONFIG_USBHOST_MAX_EXTHUBS          8
#define CONFIG_USBHOST_MAX_EHPORTS          16
#define CONFIG_USBHOST_MAX_INTERFACES       16
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 16
#define CONFIG_USBHOST_MAX_ENDPOINTS        16

#define CONFIG_USBHOST_MAX_CDC_ACM_CLASS 4
#define CONFIG_USBHOST_MAX_HID_CLASS     4
#define CONFIG_USBHOST_MAX_MSC_CLASS     2
#define CONFIG_USBHOST_MAX_AUDIO_CLASS   1
#define CONFIG_USBHOST_MAX_VIDEO_CLASS   1
#define CONFIG_USBHOST_MAX_RNDIS_CLASS   1

#define CONFIG_USBHOST_DEV_NAMELEN 16

#ifndef CONFIG_USBHOST_PSC_PRIO
    #define CONFIG_USBHOST_PSC_PRIO 4
#endif
#ifndef CONFIG_USBHOST_PSC_STACKSIZE
    #define CONFIG_USBHOST_PSC_STACKSIZE 4096
#endif

//#define CONFIG_USBHOST_GET_STRING_DESC

/* Ep0 max transfer buffer */
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN 512

#ifndef CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT
    #define CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT 500
#endif

#ifndef CONFIG_USBHOST_MSC_TIMEOUT
    #define CONFIG_USBHOST_MSC_TIMEOUT 5000
#endif

/* ================ USB Device Port Configuration ================*/

#ifndef USBD_IRQHandler
    #define USBD_IRQHandler HSUSBD_IRQHandler
#endif

#ifndef USB_BASE
    #define USB_BASE (0x40200000UL)
#endif

/* Don't change below settings. */
/* Internal storage 3073*35 bit */
#define CONFIG_USB_DWC2_RX_FIFO_SIZE  (1024)
#define CONFIG_USB_DWC2_TX0_FIFO_SIZE (128)
#define CONFIG_USB_DWC2_TX1_FIFO_SIZE (1024)
#define CONFIG_USB_DWC2_TX2_FIFO_SIZE (1024)
#define CONFIG_USB_DWC2_TX3_FIFO_SIZE (1024)
#define CONFIG_USB_DWC2_TX4_FIFO_SIZE (1024)
#define CONFIG_USB_DWC2_TX5_FIFO_SIZE (1024)
#define CONFIG_USB_DWC2_TX6_FIFO_SIZE (1024)
#define CONFIG_USB_DWC2_TX7_FIFO_SIZE (1024)
#define CONFIG_USB_DWC2_TX8_FIFO_SIZE (1024)

#ifndef USB_NUM_BIDIR_ENDPOINTS
    #define USB_NUM_BIDIR_ENDPOINTS 9
#endif

#ifndef CONFIG_USB_HS
    #define CONFIG_USB_HS
#endif

#ifndef CONFIG_USB_DWC2_PORT
    #define CONFIG_USB_DWC2_PORT    1
#endif

/* ================ USB Host Port Configuration ==================*/

#define CONFIG_USBHOST_PIPE_NUM 10

#define USBH_IRQHandler   CherryUSB_EHCI_IRQHandler

/* ================ EHCI Configuration ================ */

#define CONFIG_USB_EHCI_HCCR_BASE       (0xB0015000)
#define CONFIG_USB_EHCI_HCOR_BASE       (CONFIG_USB_EHCI_HCCR_BASE + 0x20)
#define CONFIG_USB_EHCI_FRAME_LIST_SIZE 1024
#define CONFIG_USB_EHCI_INFO_ENABLE
//#define CONFIG_USB_ECHI_HCOR_RESERVED_DISABLE
#define CONFIG_USB_EHCI_CONFIGFLAG
#define CONFIG_USB_EHCI_PORT_POWER

#endif
