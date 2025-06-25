#include <Arduino.h>
#include <BleCustomGamepad.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Initialize the ADXL345 sensor
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Initialize the BLE Gamepad
BleCustomGamepad bleGamepad;

// Circular buffer for smoothing sensor data
const int size = 8;

typedef struct {
  int data[size] = {0};  // Initialize array to zeros
  int ptr = 0;
  float sum = 0.0;
} CircularBuffer;

CircularBuffer buffer_x;
CircularBuffer buffer_y;
CircularBuffer buffer_z;

void initializeBuffer(CircularBuffer* buffer) {
  buffer->ptr = 0;
  buffer->sum = 0.0;
  for (int i = 0; i < size; i++) {
    buffer->data[i] = 0;
  }
}

void enqueue(CircularBuffer* buffer, int value) {
  buffer->sum -= buffer->data[buffer->ptr];
  buffer->sum += value;
  buffer->data[buffer->ptr] = value;
  buffer->ptr = (buffer->ptr + 1) % size;
}

// Updated HID Report Descriptor to include Rx and Ry axes
uint8_t defaultHIDReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa1, 0x00,                    //   COLLECTION (Physical)
         // ReportID - 8 bits
    0x85, 0x03,                    //     REPORT_ID (3)

    // X & Y - 2x8 = 16 bits
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x80,                    //     LOGICAL_MINIMUM (-128)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)

    // Rx & Ry - 2x8 = 16 bits
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x33,                    //     USAGE (Rx)
    0x09, 0x34,                    //     USAGE (Ry)
    0x15, 0x80,                    //     LOGICAL_MINIMUM (-128)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)

    // Buttons - 8 bits
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x08,                    //     USAGE_MAXIMUM (Button 8)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x08,                    //     REPORT_COUNT (8)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0,                           // END_COLLECTION

    // Battery Level
    0x05, 0x06,                    //   Usage Page (Generic Device Controls)
    0x09, 0x20,                    //   Usage (Battery Strength)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x26, 0x64, 0x00,              // Logical Maximum (100)
    0x75, 0x08,                    //   Report Size (8)
    0x95, 0x01,                    //   Report Count (1)
    0x81, 0x02,                    //   Input (Data, Variable, Absolute)
    0xc0                           // END_COLLECTION
};

// Updated HID Gamepad Report Structure to include Rx and Ry
typedef struct __attribute__ ((packed)) {
    int8_t x;
    int8_t y;
    int8_t rx;
    int8_t ry;
    uint8_t buttons;
    uint8_t battery;
} HIDGamepadReport;

HIDGamepadReport hidReport = {
    0, // x
    0, // y
    0, // rx
    0, // ry
    0, // buttons
    100 // battery
};

void setup() {
    Serial.begin(115200);

    // Initialize the ADXL345 sensor
    if (!accel.begin()) {
        Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
        while (1);
    }
    accel.setRange(ADXL345_RANGE_16_G);

    // Initialize circular buffers
    initializeBuffer(&buffer_x);
    initializeBuffer(&buffer_y);
    initializeBuffer(&buffer_z);

    // Initialize the BLE Gamepad
    Serial.println("Starting BLE work!");
    bleGamepad = BleCustomGamepad("ESP32 BLE AR", "Espressif", 100, false);
    bleGamepad.setReportDescriptor(defaultHIDReportDescriptor, sizeof(defaultHIDReportDescriptor));
    bleGamepad.setReport(&hidReport, sizeof(HIDGamepadReport));
    bleGamepad.begin();
}

void loop() {
    if (bleGamepad.isConnected()) {
        // Read sensor data
        sensors_event_t event;
        accel.getEvent(&event);

        // Smooth sensor data using circular buffers
        enqueue(&buffer_x, event.acceleration.x);
        enqueue(&buffer_y, event.acceleration.y);
        enqueue(&buffer_z, event.acceleration.z);

        float mean_x = buffer_x.sum / size;
        float mean_y = buffer_y.sum / size;
        float mean_z = buffer_z.sum / size;

        // Calculate pitch and roll
        float roll = -atan2(mean_y, mean_z) * 180.0 / PI;
        float pitch = -atan2(-mean_x, sqrt(mean_y * mean_y + mean_z * mean_z)) * 180.0 / PI;

        // Map pitch and roll to gamepad axes
        hidReport.x = map(pitch, -90, 90, -127, 127);  // Map pitch to X axis
        hidReport.y = map(roll, -90, 90, -127, 127);   // Map roll to Y axis
        hidReport.rx = map(pitch, -90, 90, -127, 127); // Map pitch to Rx axis
        hidReport.ry = map(roll, -90, 90, -127, 127);  // Map roll to Ry axis

        // Send the updated HID report
        bleGamepad.sendReport();

        // Debug output
        Serial.print("Pitch: ");
        Serial.print(pitch);
        Serial.print(" Roll: ");
        Serial.println(roll);
    }

    delay(20); // Small delay to avoid flooding the BLE connection
}
