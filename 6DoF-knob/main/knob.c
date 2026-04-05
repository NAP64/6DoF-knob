#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

static const char *TAG = "";

//The following is taken from somewhere else, for the USB setup

    #define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

    const uint8_t hid_report_descriptor[] = {
        0x05, 0x01,          // Usage Page (Generic Desktop)
        0x09, 0x08,          // Usage (Multi-Axis)
        0xA1, 0x01,          // Collection (Application)
                             // Report 1: Translation
        0xA1, 0x00,          //   Collection (Physical)
        0x85, 0x01,          //     Report ID (1)
        0x16, 0x00, 0x80,     //logical minimum (-500)
        0x26, 0xff, 0x7f,     //logical maximum (500)
        0x36, 0x00, 0x80,     //Physical Minimum (-32768)
        0x46, 0xff, 0x7f,     //Physical Maximum (32767)
        0x55, 0x0C,          //     Unit Exponent (-4)
        0x65, 0x11,          //     Unit (System: SI Linear, Length: Centimeter)
        0x09, 0x30,          //     Usage (X)
        0x09, 0x31,          //     Usage (Y)
        0x09, 0x32,          //     Usage (Z)
        0x09, 0x33,          //     Usage (Rx)
        0x09, 0x34,          //     Usage (Ry)
        0x09, 0x35,          //     Usage (Rz)
        0x75, 0x10,          //     Report Size (16)
        0x95, 0x06,          //     Report Count (6)
    #ifdef ADV_HID_REL    // see Advanced HID settings in config_sample.h
        0x81, 0x06,       //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    #else
        0x81, 0x02, // Input (variable,absolute)
    #endif
        0xC0,                //   End Collection
                             // Report 3: Keys  
        0xa1, 0x00,          // Collection (Physical)
        0x85, 0x03,          //  Report ID (3)
        0x15, 0x00,          //   Logical Minimum (0)
        0x25, 0x01,          //    Logical Maximum (1)
        0x75, 0x01,          //    Report Size (1)
        0x95, 0x20,          //    Report Count (32)
        0x05, 0x09,          //    Usage Page (Button)
        0x19, 0x01,          //    Usage Minimum (Button #1)
        0x29, 0x20,          //    Usage Maximum (Button #24, needs 32 bits)
        0x81, 0x02,          //    Input (variable,absolute)
        0xC0,                // End Collection
                             // Report 4: LEDs
        0xA1, 0x02,          //   Collection (Logical)
        0x85, 0x04,          //     Report ID (4)
        0x05, 0x08,          //     Usage Page (LEDs)
        0x09, 0x4B,          //     Usage (Generic Indicator)
        0x15, 0x00,          //     Logical Minimum (0)
        0x25, 0x01,          //     Logical Maximum (1)
        0x95, 0x01,          //     Report Count (1)
        0x75, 0x01,          //     Report Size (1)
        0x91, 0x02,          //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x95, 0x01,          //     Report Count (1)
        0x75, 0x07,          //     Report Size (7)
        0x91, 0x03,          //     Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0xC0,                //   End Collection
        0xC0                 // END_COLLECTION
    };

    static tusb_desc_device_t descriptor_config = {
        .bLength = sizeof(descriptor_config),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = 0x256f, 
        .idProduct = 0xc631,
        .bcdDevice = 0x100,
        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,
        .bNumConfigurations = 0x01
    };

    const char* hid_string_descriptor[5] = {
        // array of pointer to string descriptors
        (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
        "TinyUSB",             // 1: Manufacturer
        "TinyUSB Device",      // 2: Product
        "123456",              // 3: Serials, should use chip ID
        "Knob test",  // 4: HID
    };

// The following is taken from some tinyUSB example

    /**
     * @brief Configuration descriptor
     *
     * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
     */
    static const uint8_t hid_configuration_descriptor[] = {
        // Configuration number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

        // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 64, 10),
    };

    /********* TinyUSB HID callbacks ***************/

    // Invoked when received GET HID REPORT DESCRIPTOR request
    // Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
    uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
    {
        // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
        return hid_report_descriptor;
    }

    // Invoked when received GET_REPORT control request
    // Application must fill buffer report's content and return its length.
    // Return zero will cause the stack to STALL request
    uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
    {
        (void) instance;
        (void) report_id;
        (void) report_type;
        (void) buffer;
        (void) reqlen;

        return 0;
    }

    // Invoked when received SET_REPORT control request or
    // received data on OUT endpoint ( Report ID = 0, Type = 0 )
    void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
    {
    }

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12

// see wiring
int ADC1_chas[] = {
    ADC_CHANNEL_7, //xy -
    ADC_CHANNEL_2, //xz -
    ADC_CHANNEL_4, //yx -
    ADC_CHANNEL_3, //yz -
    ADC_CHANNEL_5, //zx -
    ADC_CHANNEL_6  //zy -
};

static int adc_raw[6];
static int knob_center[6];
static int mr[6];
void app_main(void) {
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &descriptor_config,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif // TUD_OPT_HIGH_SPEED
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB MSC initialization DONE");

    // Initialize ADC
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    for (int i = 0; i < 6; i++)
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_chas[i], &config));
    for (int i = 0; i < 6; i++)
        knob_center[i] = 0;
    // sample and take average
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 6; i++)
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_chas[i], &adc_raw[i]));
        for (int i = 0; i < 6; i++)
            knob_center[i] += adc_raw[i] - 2000;
        vTaskDelay(pdMS_TO_TICKS(400));
    }
    for (int i = 0; i < 6; i++)
        knob_center[i] = knob_center[i] / 4 + 2000;
    ESP_LOGI(TAG, "Raw %d %d %d %d %d %d", knob_center[0], knob_center[1], knob_center[2], knob_center[3], knob_center[4], knob_center[5]); 
    
    vTaskDelay(pdMS_TO_TICKS(5000)); // some delay, to allow USB to catch up.
    int sdz = 22; // sensor deadzone
    int dmz = 65; // displacement deadzone
    int rdz = 110; // rotation deadzone

    // Main loop - read ADC values and report
    while (1) {
        for (int i = 0; i < 6; i++)
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_chas[i], &adc_raw[i]));
        for (int i = 0; i < 6; i++)
            adc_raw[i] = knob_center[i] - adc_raw[i];
        for (int i = 0; i < 6; i++)
            if (adc_raw[i] > 0)
                adc_raw[i] = adc_raw[i] > sdz ? adc_raw[i] - sdz : 0;
            else
                adc_raw[i] = adc_raw[i] < -sdz ? adc_raw[i] + sdz : 0;

        //Displacement
        mr[0] = ( adc_raw[0] + adc_raw[1] + adc_raw[2] - adc_raw[3] + adc_raw[4] - adc_raw[5])/2;
        mr[1] = ( adc_raw[0] - adc_raw[1] + adc_raw[2] + adc_raw[3] - adc_raw[4] + adc_raw[5])/2;
        mr[2] = ( adc_raw[0] - adc_raw[1] + adc_raw[2] - adc_raw[3] - adc_raw[4] - adc_raw[5])/2;
        //Rotation
        mr[3] = ( adc_raw[0] - adc_raw[1] + adc_raw[2] + adc_raw[3] - adc_raw[4] - adc_raw[5])/2;
        mr[4] = (-adc_raw[0] - adc_raw[1] - adc_raw[2] + adc_raw[3] + adc_raw[4] + adc_raw[5])/2;
        mr[5] = (-adc_raw[0] - adc_raw[1] + adc_raw[2] + adc_raw[3] - adc_raw[4] + adc_raw[5])/2;
        
        //Suppress comparativly low strenth channels (decrease unintended movements)
        int temp, sum = 0;
        for (int j = 0; j < 6; j++)
            sum += abs(mr[j]);

        for (int i = 0; i < 3; i++) {
            temp = (sum - abs(mr[i])) / 7 + dmz;
            if (mr[i] > 0)
                mr[i] = mr[i] > temp ? mr[i] - temp : 0;
            else
                mr[i] = mr[i] < -temp ? mr[i] + temp : 0;
        }
        for (int i = 3; i < 6; i++) {
            temp = (sum - abs(mr[i])) / 7 + rdz;
            if (mr[i] > 0)
                mr[i] = mr[i] > temp ? mr[i] - temp : 0;
            else
                mr[i] = mr[i] < -temp ? mr[i] + temp : 0;
        }

        //ESP_LOGI(TAG, "Mx%d, y%d.z%d;  Rx%d,y%d,z%d", mr[0], mr[1], mr[2], mr[3], mr[4], mr[5]); 
        //ESP_LOGI(TAG, "Raw %d %d %d %d %d %d", adc_raw[0], adc_raw[1], adc_raw[2], adc_raw[3], adc_raw[4], adc_raw[5]);
        
        //Only report when active
        temp = mr[0] | mr[1] | mr[2] | mr[3] | mr[4] | mr[5];
        if (temp)
        {
            uint8_t mb[12] = { mr[0] & 0xFF, (mr[0] >> 8) & 0xFF, mr[1] & 0xFF, (mr[1] >> 8) & 0xFF, mr[2] & 0xFF, (mr[2] >> 8) & 0xFF, mr[3] & 0xFF, (mr[3] >> 8) & 0xFF, mr[4] & 0xFF, (mr[4] >> 8) & 0xFF, mr[5] & 0xFF, (mr[5] >> 8) & 0xFF };
            tud_hid_report(1, mb, 12);
        }

        vTaskDelay(pdMS_TO_TICKS(30));  // Delay
    }
}
