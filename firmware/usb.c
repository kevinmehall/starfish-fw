#include "usb.h"
#include "firmware.h"
#include "class/cdc/cdc_standard.h"

USB_ENDPOINTS(5);

__attribute__((__aligned__(4))) const USB_DeviceDescriptor device_descriptor = {
	.bLength = sizeof(USB_DeviceDescriptor),
	.bDescriptorType = USB_DTYPE_Device,

	.bcdUSB                 = 0x0200,
	.bDeviceClass           = 0,
	.bDeviceSubClass        = USB_CSCP_NoDeviceSubclass,
	.bDeviceProtocol        = USB_CSCP_NoDeviceProtocol,

	.bMaxPacketSize0        = 64,
	.idVendor               = 0x59e3,
	.idProduct              = 0x5555,
	.bcdDevice              = 0x0110,

	.iManufacturer          = 0x01,
	.iProduct               = 0x02,
	.iSerialNumber          = 0x03,

	.bNumConfigurations     = 1
};

uint16_t altsetting = 0;

typedef struct ConfigDesc {
	USB_ConfigurationDescriptor Config;
	USB_InterfaceDescriptor OffInterface;

	USB_InterfaceDescriptor PortInterface;
	USB_EndpointDescriptor PortInEndpoint;
	USB_EndpointDescriptor PortOutEndpoint;

	USB_InterfaceDescriptor DAPInterface;
	USB_EndpointDescriptor DAPInEndpoint;
	USB_EndpointDescriptor DAPOutEndpoint;

}  __attribute__((packed)) ConfigDesc;

__attribute__((__aligned__(4))) const ConfigDesc configuration_descriptor = {
	.Config = {
		.bLength = sizeof(USB_ConfigurationDescriptor),
		.bDescriptorType = USB_DTYPE_Configuration,
		.wTotalLength  = sizeof(ConfigDesc),
		.bNumInterfaces = 3,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIG_ATTR_BUSPOWERED,
		.bMaxPower = USB_CONFIG_POWER_MA(500)
	},
	.OffInterface = {
		.bLength = sizeof(USB_InterfaceDescriptor),
		.bDescriptorType = USB_DTYPE_Interface,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = USB_CSCP_VendorSpecificClass,
		.bInterfaceSubClass = 0x00,
		.bInterfaceProtocol = 0x00,
		.iInterface = 0,
	},
	.PortInterface = {
		.bLength = sizeof(USB_InterfaceDescriptor),
		.bDescriptorType = USB_DTYPE_Interface,
		.bInterfaceNumber = 0,
		.bAlternateSetting = ALTSETTING_PORT,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_CSCP_VendorSpecificClass,
		.bInterfaceSubClass = 0x00,
		.bInterfaceProtocol = 0x00,
		.iInterface = 0x10,
	},
	.PortInEndpoint = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = USB_EP_PORT_IN,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 64,
		.bInterval = 0x00
	},
	.PortOutEndpoint = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = USB_EP_PORT_OUT,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 64,
		.bInterval = 0x00
	},
	.DAPInterface = {
		.bLength = sizeof(USB_InterfaceDescriptor),
		.bDescriptorType = USB_DTYPE_Interface,
		.bInterfaceNumber = 0,
		.bAlternateSetting = ALTSETTING_DAP,
		.bNumEndpoints = 2,
		.bInterfaceClass = 3, // HID (but not really)
		.bInterfaceSubClass = 0x00,
		.bInterfaceProtocol = 0x00,
		.iInterface = 0x11,
	},
	.DAPInEndpoint = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = USB_EP_DAP_HID_IN,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 64,
		.bInterval = 0x00
	},
	.DAPOutEndpoint = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = USB_EP_DAP_HID_OUT,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 64,
		.bInterval = 0x00
	},
};

__attribute__((__aligned__(4))) const USB_StringDescriptor language_string = {
	.bLength = USB_STRING_LEN(1),
	.bDescriptorType = USB_DTYPE_String,
	.bString = {USB_LANGUAGE_EN_US},
};

#define MSFT_ID 0xEE
#define MSFT_ID_STR u"\xEE"

__attribute__((__aligned__(4))) const USB_StringDescriptor msft_os = {
	.bLength = 18,
	.bDescriptorType = USB_DTYPE_String,
	.bString = u"MSFT100" MSFT_ID_STR
};

__attribute__((__aligned__(4))) uint8_t ep0_buffer[146];

// TODO: this doesn't need to be in RAM if it is copied into usb_ep0_out one packet at a time
const USB_MicrosoftCompatibleDescriptor msft_compatible = {
	.dwLength = sizeof(USB_MicrosoftCompatibleDescriptor) + (3 * sizeof(USB_MicrosoftCompatibleDescriptor_Interface)),
	.bcdVersion = 0x0100,
	.wIndex = 0x0004,
	.bCount = 3,
	.reserved = {0, 0, 0, 0, 0, 0, 0},
	.interfaces = {
		{
			.bFirstInterfaceNumber = 0,
			.reserved1 = 0x01,
			.compatibleID = "WINUSB\0\0",
			.subCompatibleID = {0, 0, 0, 0, 0, 0, 0, 0},
			.reserved2 = {0, 0, 0, 0, 0, 0},
		},
		{
			.bFirstInterfaceNumber = 1,
			.reserved1 = 0x01,
			.compatibleID = "WINUSB\0\0",
			.subCompatibleID = {0, 0, 0, 0, 0, 0, 0, 0},
			.reserved2 = {0, 0, 0, 0, 0, 0},
		},
		{
			.bFirstInterfaceNumber = 2,
			.reserved1 = 0x01,
			.compatibleID = "WINUSB\0\0",
			.subCompatibleID = {0, 0, 0, 0, 0, 0, 0, 0},
			.reserved2 = {0, 0, 0, 0, 0, 0},
		},
	}
};

typedef struct {
	uint32_t dwLength;
	uint16_t bcdVersion;
	uint16_t wIndex;
	uint16_t wCount;
	uint32_t dwPropLength;
	uint32_t dwType;
	uint16_t wNameLength;
	uint16_t name[21];
	uint32_t dwDataLength;
	uint16_t data[40];
	uint8_t _padding[2];
} __attribute__((packed)) USB_MicrosoftExtendedPropertiesDescriptor;

const USB_MicrosoftExtendedPropertiesDescriptor msft_extended = {
	.dwLength = 146,
	.bcdVersion = 0x0100,
	.wIndex = 0x05,
	.wCount = 0x01,
	.dwPropLength = 136,
	.dwType = 7,
	.wNameLength = 42,
	.name = u"DeviceInterfaceGUIDs\0",
	.dwDataLength = 80,
	.data = u"{3c33bbfd-71f9-4815-8b8f-7cd1ef928b3d}\0\0",
};

uint16_t usb_cb_get_descriptor(uint8_t type, uint8_t index, const uint8_t** ptr) {
	const void* address = NULL;
	uint16_t size    = 0;

	switch (type) {
		case USB_DTYPE_Device:
			address = &device_descriptor;
			size    = sizeof(USB_DeviceDescriptor);
			break;
		case USB_DTYPE_Configuration:
			address = &configuration_descriptor;
			size    = sizeof(ConfigDesc);
			break;
		case USB_DTYPE_String:
			switch (index) {
				case 0x00:
					address = &language_string;
					break;
				case 0x01:
					address = usb_string_to_descriptor("Signalspec");
					break;
				case 0x02:
					address = usb_string_to_descriptor("Starfish");
					break;
				case 0x03:
					address = samd_serial_number_string_descriptor();
					break;
				case 0x10:
					address = usb_string_to_descriptor("TMPort");
					break;
				case 0x11:
					address = usb_string_to_descriptor("CMSIS-DAP");
					break;
				case 0xee:
					address = &msft_os;
					break;
			}
			size = (((USB_StringDescriptor*)address))->bLength;
			break;
	}

	*ptr = address;
	return size;
}

void usb_cb_reset(void) {
}

bool usb_cb_set_configuration(uint8_t config) {
	if (config <= 1) {
		return true;
	}
	return false;
}

#define REQ_PWR 0x10
#define REQ_PWR_EN_A 0x10
#define REQ_PWR_EN_B 0x11
#define REQ_PWR_EN_REG 0x12
#define REQ_PWR_LED_R 0x20
#define REQ_PWR_LED_G 0x21
#define REQ_PWR_LED_B 0x22
#define REQ_PWR_DAC 0xd0

#define REQ_INFO 0x30
#define REQ_PWR_PORT_A_IO 0x40
#define REQ_PWR_PORT_B_IO 0x50
#define REQ_INFO_GIT_HASH 0x0
#define REQ_BOOT 0xBB
#define REQ_OPENWRT_BOOT_STATUS 0xBC

void req_gpio(uint16_t wIndex, uint16_t wValue) {
	switch (wIndex) {
		case REQ_PWR_EN_A:
			pin_set(PIN_EN_A, wValue);
			break;
		case REQ_PWR_EN_B:
			pin_set(PIN_EN_B, wValue);
			break;
		case REQ_PWR_EN_REG:
			pin_set(PIN_EN_REG, wValue);
			break;
		case REQ_PWR_LED_R:
			pin_set(PIN_LED[0], wValue);
			break;
		case REQ_PWR_LED_G:
			pin_set(PIN_LED[1], wValue);
			break;
		case REQ_PWR_LED_B:
			pin_set(PIN_LED[2], wValue);
			break;
		case REQ_PWR_DAC:
			DAC->DATA.reg = wValue;
			break;
		default:
			return usb_ep0_stall();
	}

	usb_ep0_out();
	return usb_ep0_in(0);
}

void req_info(uint16_t wIndex) {
    const char* str = 0;
    switch (wIndex) {
        case REQ_INFO_GIT_HASH:
            str = git_version;
            break;
        default:
            return usb_ep0_stall();
    }
    uint16_t len = strlen(str);
    if (len > USB_EP0_SIZE) len = USB_EP0_SIZE;
    memcpy(ep0_buf_in, str, len);
    usb_ep0_out();
    return usb_ep0_in(len);
}

void req_boot() {
    wdt_reset(GCLK_32K);
    usb_ep0_out();
    return usb_ep0_in(0);
}

// TODO: Use the version in the USB library after making it handle descriptors larger than 64 bytes
static inline void handle_msft_compatible(const USB_MicrosoftCompatibleDescriptor* msft_compatible, const USB_MicrosoftExtendedPropertiesDescriptor* msft_extended) {
	uint16_t len;
	if (usb_setup.wIndex == 0x0005) {
		len = msft_extended->dwLength;
		memcpy(ep0_buffer, msft_extended, len);
	} else if (usb_setup.wIndex == 0x0004) {
		len = msft_compatible->dwLength;
		memcpy(ep0_buffer, msft_compatible, len);
	} else {
		return usb_ep0_stall();
	}
	if (len > usb_setup.wLength) {
		len = usb_setup.wLength;
	}
	usb_ep_start_in(0x80, ep0_buffer, len, true);
	usb_ep0_out();
}

void usb_cb_control_setup(void) {
	uint8_t recipient = usb_setup.bmRequestType & USB_REQTYPE_RECIPIENT_MASK;
	if (recipient == USB_RECIPIENT_DEVICE) {
		switch(usb_setup.bRequest) {
			case MSFT_ID: return handle_msft_compatible(&msft_compatible, &msft_extended);
			case REQ_PWR: return req_gpio(usb_setup.wIndex, usb_setup.wValue);
			case REQ_INFO: return req_info(usb_setup.wIndex);
			case REQ_BOOT: return req_boot();
		}
	} else if (recipient == USB_RECIPIENT_INTERFACE) {
		switch(usb_setup.bRequest) {
			case MSFT_ID: return handle_msft_compatible(&msft_compatible, &msft_extended);
		}
	}
	return usb_ep0_stall();
}

void usb_cb_control_in_completion(void) {
}

void usb_cb_control_out_completion(void) {
}

void usb_cb_completion(void) {
	if (altsetting == ALTSETTING_PORT) {
        if (usb_ep_pending(USB_EP_PORT_OUT)) {
			usb_ep_handled(USB_EP_PORT_OUT);
			port_bridge_out_completion(&port_a, usb_ep_out_length(USB_EP_PORT_OUT));
        }

        if (usb_ep_pending(USB_EP_PORT_IN)) {
			usb_ep_handled(USB_EP_PORT_IN);
			port_bridge_in_completion(&port_a);
        }
    } else if (altsetting == ALTSETTING_DAP) {
		if (usb_ep_pending(USB_EP_DAP_HID_OUT)) {
			usb_ep_handled(USB_EP_DAP_HID_OUT);
			dap_handle_usb_out_completion();
		}

		if (usb_ep_pending(USB_EP_DAP_HID_IN)) {
			usb_ep_handled(USB_EP_DAP_HID_IN);
			dap_handle_usb_in_completion();
		}
	}
}

bool usb_cb_set_interface(uint16_t interface, uint16_t new_altsetting) {
	if (interface == 0) {
		if (new_altsetting > 2) {
			return false;
		}

		if (altsetting == ALTSETTING_PORT){
			port_disable(&port_a);
		} else if (altsetting == ALTSETTING_DAP) {
			dap_disable();
		}

		if (new_altsetting == ALTSETTING_PORT) {
			port_enable(&port_a);
		} else if (new_altsetting == ALTSETTING_DAP) {
			dap_enable();
		}

		altsetting = new_altsetting;
		return true;
	}
	return false;
}
