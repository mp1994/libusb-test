/*
 * Simple libusb-1.0 test programm
 * It openes an USB device, expects two Bulk endpoints,
 *   EP1 should be IN
 *   EP2 should be OUT
 * It alternates between reading and writing a packet to the Device.
 * It uses Synchronous device I/O
 *
 * Compile:
 *   gcc -lusb-1.0 -o test test.c
 * Run:
 *   ./test
 * Thanks to BertOS for the example:
 *   http://www.bertos.org/use/tutorial-front-page/drivers-usb-device
 *
 * For Documentation on libusb see:
 *   http://libusb.sourceforge.net/api-1.0/modules.html
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include <assert.h>

// Raspberry Pi Pico
#define VID	 0x2e8a
#define PID	 0x000a

#define USB_ENDPOINT_IN	    (LIBUSB_ENDPOINT_IN  | 1)  
#define USB_ENDPOINT_OUT	(LIBUSB_ENDPOINT_OUT | 2) 
#define USB_TIMEOUT	        3000

// Device details found with: 
// lsusb -d <VID>: -v 
#define ENDPOINT_BULK_READ 0x82 // Bulk IN endpoint
#define USB_BULK_INTERFACE 0x01 // Interface for bulk transfer

libusb_context* ctx = NULL;
libusb_device_handle* handle;

static uint8_t receiveBuf[50];
uint8_t transferBuf[50];

uint16_t counter=0;

/*
 * Read a packet
 */
int usb_read() {

	int nread = 0, ret = 0;
	ret = libusb_bulk_transfer(handle, ENDPOINT_BULK_READ, receiveBuf, sizeof(receiveBuf), &nread, USB_TIMEOUT);
	if (ret){
		printf("ERROR in bulk read: %d\n", ret);
		return -1;
    }
	else{
		printf("%d receive %d bytes from device: %s\n", ++counter, nread, receiveBuf);
		return 0;
    }

}


/*
 * write a few bytes to the device
 *
 */
uint16_t count=0;
int usb_write() {

	int n, ret;
    //count up
    n = sprintf(transferBuf, "%d\0",count++);
    //write transfer
    //probably unsafe to use n twice...
	ret = libusb_bulk_transfer(handle, USB_ENDPOINT_OUT, transferBuf, n, &n, USB_TIMEOUT);
    //Error handling
    switch(ret){
        case 0:
            printf("send %d bytes to device\n", n);
            return 0;
        case LIBUSB_ERROR_TIMEOUT:
            printf("ERROR in bulk write: %d Timeout\n", ret);
            break;
        case LIBUSB_ERROR_PIPE:
            printf("ERROR in bulk write: %d Pipe\n", ret);
            break;
        case LIBUSB_ERROR_OVERFLOW:
            printf("ERROR in bulk write: %d Overflow\n", ret);
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            printf("ERROR in bulk write: %d No Device\n", ret);
            break;
        default:
            printf("ERROR in bulk write: %d\n", ret);
            break;

    }
    return -1;

}

static void sighandler(int signum) {

    printf( "\nInterrupt signal received\n" );
	if (handle){
        libusb_release_interface(handle, USB_BULK_INTERFACE);
        printf( "\nInterrupt signal received1\n" );
        libusb_close(handle);
        printf( "\nInterrupt signal received2\n" );
	}
	printf( "\nInterrupt signal received3\n" );
	libusb_exit(NULL);
	printf( "\nInterrupt signal received4\n" );

	exit(0);

}

int main(int argc, char **argv) {

    //Pass Interrupt Signal to our handler
	signal(SIGINT, sighandler);

	int rc = libusb_init(&ctx);
    assert(rc == 0);
    printf("libusb_init()\n");
	libusb_set_debug(ctx, 3);

    //Open Device with VendorID and ProductID
	handle = libusb_open_device_with_vid_pid(ctx, VID, PID);
	if (!handle) {
		perror("device not found");
		return 1;
	}

    // Set auto detach/re-attach of kernel driver
    libusb_set_auto_detach_kernel_driver(handle, 1);
    assert(rc == 0);
    printf("Auto detach kernel driver: enabled\n");

	//Claim Interface 0 from the device
    rc = libusb_claim_interface(handle, USB_BULK_INTERFACE); // interface 1: bulk
	if (rc < 0) {
		fprintf(stderr, "usb_claim_interface error %d\n", r);
		return 2;
	}
	printf("Interface %d claimed\n", USB_BULK_INTERFACE);

    // r = libusb_set_interface_alt_setting(handle, 1, 1);
    // assert(r == 0);
    // printf("Set alternate setting: ok\n");
    
    libusb_clear_halt(handle, ENDPOINT_BULK_READ);

    // Skip this to test only claim-release
	while(0) {

		usb_read();
    
    }

    rc = libusb_release_interface(handle, USB_BULK_INTERFACE);
    assert(rc == 0);
    printf("Interface released.\n");
	libusb_close(handle);
	libusb_exit(NULL);

	return 0;

}
