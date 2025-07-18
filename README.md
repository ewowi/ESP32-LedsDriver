# ESP32-LedsDriver

Physical and Virtual LedsDriver for ESP32-dev, ESP32-wrover, ESP32-S3, ESP32-P4

See [2025-07-16-MoonLightv057](https://moonmodules.org/2025-07-16-MoonLightv057)

This project is 🚧:

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | 20%  | 20% | 20% |
| **dev / wrover** | 10% | 10% | 3% |
| **S3** | 3% | 3% | 3% |
| **P4** | 1% | 0% | 0% |

## Code

### 🚧

* ESP32LedsDriver (abstract)
    * initLeds()
        * virtual setPins()
        * virtual i2sInit();
        * virtual initDMABuffers();
        * virtual allocateDMABuffer()
        * virtual putdefaultones()
    * show()
* CONFIG_IDF_TARGET_ESP32
    * LedsDriverESP32dev
        * PhysicalDriverESP32dev
            * setPins() 60%
            * i2sInit() 0%
            * initDMABuffers() 1%
            * allocateDMABuffer() 60%
            * putdefaultones() 60%
        * VirtualDriverESP32dev
            * setPins() 60%
            * i2sInit() 0%
            * initDMABuffers() 1%
            * allocateDMABuffer() 60%
            * putdefaultones() 60%
* CONFIG_IDF_TARGET_ESP32S3
    * LedsDriverESP32S3
        * PhysicalDriverESP32S3
            * setPins() 1%
            * i2sInit() 0%
            * initDMABuffers() 0%
            * allocateDMABuffer() 60%
            * putdefaultones() 60%
        * VirtualDriverESP32S3
            * setPins() 60%
            * i2sInit() 0%
            * initDMABuffers() 1%
            * allocateDMABuffer() 60%
            * putdefaultones() 60%
* CONFIG_IDF_TARGET_ESP32P4
    * LedsDriverESP32P4
        * PhysicalDriverESP32P4
            * setPins() 1%
            * i2sInit() 0%
            * initDMABuffers() 0%
            * allocateDMABuffer() 0%
            * putdefaultones() 0%
        * VirtualDriverESP32P4
            * setPins() 1%
            * i2sInit() 0%
            * initDMABuffers() 0%
            * allocateDMABuffer() 0%
            * putdefaultones() 0%

### main.cpp

Run Physical or Virtual driver for all defined boards:

| Environment         | Status    | Duration |
| ------------------  | --------  | ------------ |
| esp32dev            | SUCCESS   | 00:00:03.637 |
| esp-wrover-kit      | SUCCESS   | 00:00:03.098 |
| esp32-s3-devkitc-1  | SUCCESS   | 00:00:02.658 |
| esp32-p4            | SUCCESS   | 00:00:01.988 |

### Class structure:

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | ESP32LedsDriver* | ESP32PhysicalDriver* | ESP32VirtualDriver* |
| **dev / wrover** | LedsDriverESP32dev* | PhysicalDriverESP32dev | VirtualDriverESP32dev |
| **S3** | LedsDriverESP32S3* | PhysicalDriverESP32S3 | VirtualDriverESP32S3 |
| **P4** | LedsDriverESP32P4* | PhysicalDriverESP32P4 | VirtualDriverESP32P4 |

*: Abstract classes

### Class usage

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | initLeds(), setPins(), setBrightness(), setColorCorrection(), show(), setPixel() | - | setLatchAndClockPin(), setClockSpeed(), dmaBuffer |
| **dev / wrover** | deviceBaseIndex, setPinsDev() | - | deviceClockIndex |
| **S3** | - | - | signalsID |
| **P4** | - | - | - |
