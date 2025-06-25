# VR Ball Maze Game with Tangible Interface  
**2025 HELHa BIP Project**  
*An immersive VR experience controlled by physical tilt gestures*  
---

## 🎮 Introduction  
Experience a novel VR interaction where you physically tilt a controller to navigate a 3D ball maze! This project combines:  
- **ADXL345 Accelerometer** for motion detection  
- **ESP32 Microcontroller** for wireless communication  
- **Godot Engine** for VR gameplay  
- **ROS 2** for sensor data visualization  

Tilt the real-world controller to rotate the virtual maze, guiding a ball through obstacles using physics-based movement.  

---

## ⚙️ System Architecture  
![image](https://github.com/user-attachments/assets/f9041916-5ad5-493a-9c98-5c8785668924)


### Key Components  
1. **Hardware Layer**  
   - ADXL345 sensor captures tilt data  
   - ESP32 processes and transmits data via Bluetooth  

2. **Communication Layer**  
   - BLE HID for real-time game control  
   - ROS 2 for data monitoring/analysis  

3. **VR Application**  
   - Godot engine handles physics and VR rendering  
   - Multi-level maze progression system  

---



---

## 🛠️ Hardware Requirements   
- ESP32 development board  
- ADXL345 accelerometer  
- VR Headset (Meta Quest)  
- USB-C cable & jumper wires  

---

## 🕹️ Usage  
1. Power the ESP32 controller  
2. Pair via Bluetooth as "BIP Group4" gamepad  
3. Launch VR application in Godot  
4. Tilt controller to navigate maze levels  

**Key Commands for Data Monitoring**  
```bash
# Real-time plotting
ros2 run rqt_plot rqt_plot

# Bag recording
ros2 bag record /accel_data
```

# 🔍 Technical Documentation  

## 🧩 System Architecture  
### Hardware Block Diagram  
![image](https://github.com/user-attachments/assets/ded4052e-f9e5-4bf3-ad4d-83bcafdb4620)
 
### Component Specifications  
| **Component** | **Details** |  
|----------------|-------------|  
| **ESP32-WROOM-32** | Dual-core 240MHz, 4MB Flash, BLE/Wi-Fi |  
| **ADXL345** | 3-axis ±16g digital accelerometer, I²C/SPI, 13-bit resolution |  
| **VR Headset** | OpenXR-compatible device (Tested on Meta Quest 3) |  

---

## 🕹️ Firmware Implementation (ESP32)  

### 1. Sensor Interface  
**I²C Configuration**  
```cpp
Wire.begin(SDA_PIN, SCL_PIN); // GPIO21, GPIO22 (ESP32 default)
accel.setRange(ADXL345_RANGE_16_G); // ±16g range
accel.setDataRate(ADXL345_DATARATE_100_HZ); // 100Hz sampling
```

**Noise Reduction**  
- 8-element circular buffer for each axis  
- Moving average filter:  
  ```math
  \bar{x} = \frac{1}{8}\sum_{i=0}^{7} x_i
  ```  
- Buffer update logic:  
  ```cpp
  void enqueue(CircularBuffer* buffer, float value) {
    buffer->sum -= buffer->data[buffer->ptr];  // Remove oldest value
    buffer->sum += value;                      // Add new value
    buffer->data[buffer->ptr] = value;         // Store in buffer
    buffer->ptr = (buffer->ptr + 1) % size;    // Circular index
  }
  ```

### 2. Orientation Calculation  
**Pitch/Roll Formulas**  
```math
\begin{aligned}
\text{Pitch} &= \arctan\left(\frac{-x}{\sqrt{y^2 + z^2}}\right) \times \frac{180}{\pi} \\
\text{Roll} &= \arctan\left(\frac{y}{z}\right) \times \frac{180}{\pi} 
\end{aligned}
```

**Implementation**  
```cpp
float mean_x = buffer_x.sum / size;
float roll = -atan2(mean_y, mean_z) * 180.0 / PI; 
float pitch = -atan2(-mean_x, sqrt(mean_y*mean_y + mean_z*mean_z)) * 180.0 / PI;
```

### 3. BLE HID Protocol  
**Custom Report Descriptor**  
```cpp
0x05, 0x01,  // USAGE_PAGE (Generic Desktop)
0x09, 0x05,  // USAGE (Game Pad)
...
0x09, 0x33,  // USAGE (Rx)
0x09, 0x34,  // USAGE (Ry)
```
- Maps tilt data to 4 axes (X, Y, Rx, Ry)  
- 8-bit resolution per axis (-127 to +127)  

**Data Packing**  
```cpp

typedef struct __attribute__ ((packed)) {
    int8_t x;
    int8_t y;
    int8_t rx;
    int8_t ry;
    uint8_t buttons;
    uint8_t battery;
} HIDGamepadReport;
```

---

## 📡 ROS 2 Integration  
### Node Architecture  
![ROS 2 Node Diagram](ros_node_diagram.png)  
*(Suggested: ROS 2 computation graph showing /accel_data topic)*  

### Message Structure  
**AccelStamped.msg**  
```
Header header
geometry_msgs/Accel accel
```

**Data Pipeline**  
1. ESP32 → Serial port @ 115200 baud (CSV format)  
2. Python node → Parsing → ROS 2 message conversion  
3. Published to `/accel_data` at 10Hz  

### Visualization Tools  
```bash
# Real-time plotting
ros2 run rqt_plot rqt_plot \
/accel_data/accel/linear/x \
/accel_data/accel/linear/y \
/accel_data/accel/linear/z

# Bag file analysis
ros2 bag record /accel_data
ros2 bag play <bag_name>
```

---

## 🎮 Godot VR Implementation  
The VR game logic and environment were implemented using the **Godot Engine**, taking full advantage of its physics system and OpenXR support.

### 🧠 Development Process

1. **Maze Design**  
   - Created **three unique 3D mazes**, each with distinct obstacles and increasing levels of difficulty.  
   - Designed to challenge the user’s spatial awareness and precision in tilt control.

2. **Control Prototyping**  
   - Initially enabled maze control using **keyboard input** (arrow keys or WASD).  
   - This allowed rapid gameplay prototyping and testing of physics interactions.

3. **Hardware Integration**  
   - Replaced keyboard controls with **real-time tilt data** from the physical controller (ESP32 + ADXL345).  
   - Mapped pitch and roll values to maze rotation in the virtual environment.

4. **VR Deployment**  
   - Integrated the system into a **VR experience** using a Meta Quest headset and Godot’s OpenXR plugin.  
   - The player now navigates the maze through natural hand tilting, with real-time feedback in a fully immersive VR setting.

---

## 🔄 Communication Protocol  

### BLE Characteristics  
| **Feature** | **Specification** |  
|-------------|-------------------|  
| Service UUID | `0x1812` (HID) |  
| Report UUID | `0x2A4D` |  
| MTU Size | 23 bytes |  
| Latency | ≤50ms |  

### Data Format  
**ESP32 → Godot**  
```
Byte 0: X-axis (pitch)  
Byte 1: Y-axis (roll)  
Byte 2: Rx-axis (pitch mirror)  
Byte 3: Ry-axis (roll mirror)  
```

---

## 🧪 Calibration Procedure  
1. Place controller on flat surface  
2. Run calibration script:  
```bash
ros2 run bip_package calibrate_sensor
```  
3. Collect 100 samples for zero-offset calculation:  
```math
\text{Offset}_x = \frac{1}{100}\sum_{i=1}^{100} x_i
```  

---

## ⚠️ Troubleshooting  

| **Issue** | **Solution** |  
|-----------|--------------|  
| No BLE connection | Reset ESP32 via EN button |  
| Jittery controls | Increase buffer size to 16 |  
| VR headset not detected | Verify OpenXR runtime installation |  
| Ball physics instability | Adjust Godot physics FPS to 90 |  

---

## 🔄 Data Flow  

--- 

## 🧑💻 Contributors
| **Member** | **Role** |  
|-----------|--------------|  
|Bartosz Piotrowski, Lluís Francesc Collell Erra | Hardware Integration |  
| Bartosz Piotrowski | ROS 2 Implementation |  
| Lluís Francesc Collell Erra | Godot Development |  
| Bartosz Piotrowski, Lluís Francesc Collell Erra | System Architecture |  
