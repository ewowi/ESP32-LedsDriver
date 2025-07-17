# ESP32-LedsDriver

## Code

### main.cpp

Run Physical or Virtual driver for all defined boards:

Environment         Status    Duration
------------------  --------  ------------
esp32dev            SUCCESS   00:00:03.637
esp-wrover-kit      SUCCESS   00:00:03.098
esp32-s3-devkitc-1  SUCCESS   00:00:02.658
esp32-p4            SUCCESS   00:00:01.988

### Class structure:

| Board    | General | Physical /  | Virtual |
|----------|---------|----------|---------|
| **general** | ESP32LedsDriver* | ESP32PhysicalDriver* | ESP32VirtualDriver* |
| **dev** | LedsDriverESP32dev* | PhysicalDriverESP32dev | VirtualDriverESP32dev |
| **S3** | LedsDriverESP32S3* | PhysicalDriverESP32S3 | VirtualDriverESP32S3 |
| **P4** | LedsDriverESP32P4* | PhysicalDriverESP32P4 | VirtualDriverESP32P4 |

*: Abstract classes

### Class usage

* setPins:

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **general** | - | - | latchPin, clockPin, clockSpeed, dmaBuffer |
| **dev** | deviceBaseIndex, setPinsM | - | deviceClockIndex |
| **S3** | - | - | signalsID |
| **P4** | - | - | - |