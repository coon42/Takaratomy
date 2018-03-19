#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <usb.h>
#include <unistd.h>
#include <fcntl.h>

#include "button.h"
#include "takaratomy.h"

int main(int argc, char **argv) {
	int ec = 0;
	struct usb_dev_handle* pDev = initPanel(0);

	if(!pDev) {
		fprintf(stderr, "ERROR: Couldn't initialize USB button!\n");

		return 2;
	}

	if(argc == 2) {
		unsigned char cmd = 64;

		if (!strcmp("close", argv[1]))
			cmd = 96; // alternatively: 224
		else if(!strcmp("open", argv[1]))
			cmd = 80; // alternatively: 208
		else {
		  printf("Please specify a command (open/close)\n");

			return 1;
		}

    if((ec = usb_interrupt_write(pDev, 0x02, &cmd, 1, 10)) < 0) {
	    printf("Error writing to USB device (%d): %s\n", ec, usb_strerror());
      return 2;
    }
  }
  else if(argc == 1) {
    unsigned char data = 0;
    unsigned char last = 0;
    unsigned int state = 0;

    fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | O_NONBLOCK);

    while(1) {
      if(!usb_interrupt_read(pDev, 0x81, &data, 1, 10)) {
        unsigned char c = 64;

        if(last != data) {
          if((data & 0xF) == 5)
            printf("button pressed\n");
            if(data & 0x2)
              printf("open button pressed\n");
            if(data == 0x68)
              printf("opening\n");
            if(data == 0x74)
              printf("closing\n");
            if(data == 0x44 && last == 0x60)
              printf("open\n");
            if(data == 0x58)
              printf("closed\n");

            last = data;
        }

        if((ec = usb_interrupt_write(pDev, 0x02, &c, 1, 10)) < 0) {
          printf("Error writing to USB device (%d): %s\n", ec, usb_strerror());
          return 2;
        }
      }

      int len = read(0, &data, 1);

      if(len > 0) {
        char cmd;

        if(state == 0) {
          state = 1;

          if(data == 'c')
            cmd = 96;
          else if(data == 'o')
            cmd = 80;

          if((ec = usb_interrupt_write(pDev, 0x02, &cmd, 1, 10)) < 0) {
            printf("Error writing to USB device (%d): %s\n", ec, usb_strerror());

            return 2;
          }
        }
        else {
          if(data == '\n')
            state = 0;
        }
      }

			usleep(100000);
		}
	}
  else {
		printf("Invalid argument. Specify open/close to open/close or nothing to listen.\n");
		return 1;
	}

	usb_close(pDev);

	return 0;
}
