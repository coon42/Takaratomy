#include <stdio.h>
#include "takaratomy.h"

static struct usb_device* openUsbDevice(int vendorId, int productId, unsigned int devNum) {
  usb_init();
  usb_find_busses();
  usb_find_devices();

  for(struct usb_bus* pBus = usb_get_busses(); pBus; pBus = pBus->next) {
    for(struct usb_device* pDev = pBus->devices; pDev; pDev = pDev->next) {
      if((pDev->descriptor.idVendor == vendorId) && (pDev->descriptor.idProduct == productId)) {
        if(devNum == 0)
          return pDev;
        else
          devNum--;
      }
    }
  }

  return NULL;
}

struct usb_dev_handle* openButton(unsigned int devNum) {
  struct usb_device* pDev = openUsbDevice(VENDORID, BUTTON_PRODUCTID, devNum);

  if(!pDev) {
    fprintf(stderr, "ERROR: USB button was not found!\n");
    printf("Did you perhaps forget to sudo or add a udev rule?\n");

    return NULL;
  }

  struct usb_dev_handle* hDev = usb_open(pDev);

  if(!hDev) {
    fprintf(stderr, "ERROR: USB button could not be opened!\n");
    printf("Did you perhaps forget to sudo or add a udev rule?\n");

    return NULL;
  }

  usb_set_debug(1);

  if(usb_claim_interface(hDev, 0) < 0) {
    fprintf(stderr, "ERROR: Cannot claim interface!\n");

    return NULL;
  }

  return hDev;
}

static int sendUsbCommand(struct usb_dev_handle* hDev, unsigned char cmd) {
  int ec;

  if((ec = usb_interrupt_write(hDev, 0x02, &cmd, 1, 10)) < 0) {
    printf("Error writing to USB device (%d): %s\n", ec, usb_strerror());
    return 2;
  }

  return 0;
}

static int requestButtonState(struct usb_dev_handle* hDev) {
  return sendUsbCommand(hDev, CMD_REQUEST_STATE);
}

int getButtonState(struct usb_dev_handle* hDev, enum ButtonState* pState) {
  int error;
  unsigned char data;
  static unsigned char last = 0;
  static enum ButtonState lastState = IDLE;

  error = usb_interrupt_read(hDev, 0x81, &data, 1, 10);
  int bytesRead = error >= 0 ? error : 0;

  if(bytesRead) {
    if(last != data) {
      if((data & 0x0F) == 5)
        lastState = BUTTON_PRESSED;
      if(data & 0x02)
        lastState = OPEN_BUTTON_PRESSED;
      if(data == 0x68)
        lastState = OPENING;
      if(data == 0x74)
        lastState = CLOSING;
      if(data == 0x44 && last == 0x60)
        lastState = OPEN;
      if(data == 0x58)
        lastState = CLOSED;

      last = data;

      switch(lastState) {
        case BUTTON_PRESSED: printf("button pressed\n"); break;
        case OPEN_BUTTON_PRESSED: printf("open button pressed\n"); break;
        case OPENING: printf("opening\n"); break;
        case CLOSING: printf("closing\n"); break;
        case OPEN: printf("open\n"); break;
        case CLOSED: printf("closed\n"); break;
      }
    }

    if(error = requestButtonState(hDev))
      return error;
  }

  *pState = lastState;

  return 0;
}

int openButtonLid(struct usb_dev_handle* hDev) {
  return sendUsbCommand(hDev, CMD_OPEN);
}

int closeButtonLid(struct usb_dev_handle* hDev) {
  return sendUsbCommand(hDev, CMD_CLOSE);
}

// quick hack for LED matrix:
// 0: maybe command: 0x80 = Light up all LEDS
// 1: unknown
// 2: LED brightness
// 3: unknown
// 4: unknown
// 5: unknown
// 6: unknown
// 7: unknown

// unsigned char pBuf[8] = { 0x80, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00 };

// if((ec = usb_interrupt_write(hDev, 0x02, pBuf, 8, 10)) < 0) {
//   printf("Error writing to USB device (%d): %s\n", ec, usb_strerror());
//   return 2;
// }
// --

