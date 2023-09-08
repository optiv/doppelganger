![Doppelgänger Logo](https://github.com/optiv/doppelganger/blob/main/git_images/dop_logo.png)

# 
Doppelgänger is firmware that runs on ESP32 devices that can be embedded within commercially available RFID readers with the intent of capturing access control card data while performing physical security assessments. Doppelgänger keeps the operator's ease of access, maintenance, and operational communications in mind.

## Background

This project stemmed from the Raspberry Pi chip shortage, which drove up the cost of RPi Nano W boards making the cost to repair the team's long-range cloners not feasible. In addition, there were some limitations with existing tooling that I aimed to mitigate.

## Intent
This project intended to accomplish the following:

1) Use modern/actively supported CoTS equipment that can easily be replaced. 
2) Hot-swappable components for easy servicing.
3) The operator can't go into a comms blackhole while connected to the device.
4) Egress for notifications, reducing the need to check for card reads while in the middle of an operation.
5) Simplified WebGUI that only displays Bit Length, Facility Code, and Card Number. Option to download the complete data set(e.g., BL, FC, CC, HEX, BIN).
6) Error handling, so the device doesn't log bad reads, EMI, etc.
7) Easy configuration and reset functionality for team use.

## Device Specifications

The Doppelgänger firmware is designed to work with either the IoT Redboard or the Thing Plus - ESP32 WROOM (USB-C), which employs the Espressif ESP32-WROOM-32E chip. These boards were chosen due to their availability and onboard peripherals, primarily the SDCard slot and power options. *Note, other ESP32 boards may be used but modifications to the firmware will be required.*

**ESP32-WROOM-32E**

* Operating Voltage: 2.3 to 3.6 V
* Xtensa® Single-Core 32-bit LX6 Microprocessor 
* 448KB of ROM and 520KB SRAM
* 16MB of Embedded SPI flash storage
* Integrated 802.11b/g/n WiFi 2.4GHz transceiver
* Integrated dual-mode Bluetooth® (classic and BLE®)
* Hardware accelerated encryption (AES, SHA2, ECC, RSA-4096)
* IoT Redboard Footprint: 2.7" x 2.3"
* Thing Plus C Footprint: 2.35" x 0.9"

## Features

The following features are built into the firmware:

* Wireless Configuration Manager ***(Default SSID: doppelgänger, Password: UndertheRadar)***
* mDNS Server: **http://rfid.local/**
* Optional E-mail / Text Notifications
* Web interface for viewing, sorting and downloading card data
* Web application-based reset functionality

## Getting Started with Doppelgänger

For setup purposes, I recommend using a computer to configure the device initially. Once the Doppelgänger has been configured to connect to a mobile device, there should be no need to use a computer again.

1) Apply power to the reader and ESP-32.
2) If the blue LED is illuminated, the device is in configuration mode. Connect to the **doppelgänger** network using the default password **UndertheRadar**.
3) The Captive Portal should automatically launch. If it does not, navigate to http://192.168.4.1/. 

**Captive Portal Menu Options:**
   * **Configure WiFi** - Performs a scan of wireless networks.
   * **Configure WiFi (No scan)** - Same as above, but without scanning for wireless networks.
   *  **Info** - Provides general information about the device, including temp, memory usage, build date etc. Note that the "ERASE WiFi Button" only clears stored wireless network credentials. It does not clear email configuration data. The only way to clear e-mail configuration data is through the web application.
   *  **Update** - OTA firmware update. To choose this option, use your web browser and open the Captive Portal (http://192.168.4.1/).
**Restart** - Self-explanatory 
**Exit** - Self-explanatory

![WiFi Manager Menu.png](https://github.com/optiv/doppelganger/blob/main/git_images/wifimanager.jpg)

## Connecting Doppelgänger to a Mobile Device
Before selecting **Configure WiFi** in the captive portal, turn on your mobile device's **Personal Hotspot**.

* **iPhone:**
  * Ensure that you enable ***Maximize Compatibility*** in the ***Personal Hotspot*** menu.
  * You must leave the Personal Hotspot menu open while scanning and connecting Doppelgänger to your device.
  * If no wireless networks appear in the **Configure WiFi** menu, clicking **Refresh** at the bottom of the page will initiate a network scan. Doppelgänger is preconfigured to filter out weak wireless networks which should help reduce clutter in WiFi-dense areas.
  * Connect to your Mobile Device's hotspot. 
  * Upon clicking the **Save** button, Doppelgänger will save the wireless configuration and reboot. Upon reboot, Doppelgänger will automatically attempt to connect to the assigned wireless network.
  * Once connected: Open your web browser to http://rfid.local/ to access the Doppelgänger web application.
* **Android:**
  * Not tested: The process will be similar to the iPhone instructions. 

## Setting Up E-mail Notifications 
* If you want to enable e-mail notifications, navigate to the Notifications page on the Doppelgänger web application. Enter your SMTP credentials and recipient information. This data is stored on the internal flash storage and is not accessible from the SD Card/web application. The notification configuration can be wiped by using Doppelgänger's reset functionality from within the web application. 

* To send a text notification follow this schema.
  ```
  Verizon: 5551234567@vtext.com
  AT&T: 5551234567@txt.att.net
  Rogers: 5551234567@pcs.rogers.com
  T-Mobile: 5551234567@tmomail.net
  Google-Fi: 5551234567@msg.fi.google.com
  Sprint: 5551234567@messaging.sprintpcs.com
  Virgin Mobile: 5551234567@vmobl.com
  ```

## Reconnecting to Doppelgänger

Ensure that you have the Personal Hotspot menu opened on your iPhone before powering Doppelgänger back up (I am uncertain if Andriod devices operate in the same manner). Otherwise, Doppelgänger may not see your hotspot and will enter configuration mode. If this happens, you can either wait out the 120-second reset timer, in which Doppelgänger will reboot and look for the stored wireless network again or enter into the Captive Portal and make changes/restart the device manually.


## Web Application & Notifications

Here's a quick look at the web application for reference:

![WebApp.png](https://github.com/optiv/doppelganger/blob/main/git_images/webapp_view.jpg)

## Debugging

Should you want to dive deeper into what Doppelgänger is doing under the hood, You can connect to the device using the terminal, [PlatformIO/vscode](https://docs.platformio.org/en/latest/), or [Arduino IDE](https://www.arduino.cc/en/software). In any case, connect a USB-C cable from your computer to Doppelgänger.

Set the serial speed to **115200**.

Connecting to the device from the terminal:
```
ls /dev/tty.usb*
screen /dev/tty.usbserial-13440 115200
```

![Boot Process](https://github.com/optiv/doppelganger/blob/main/git_images/boot_process.jpg)
![Card Handling](https://github.com/optiv/doppelganger/blob/main/git_images/card_handling.jpg)

## Flashing the device

Doppelgänger can be flashed in two ways:

**Over the Air (OTA) using the *Update* function.**
* Open a web browser (Captive Portal is not supported for updates) to http:192.168.4.1/update
* Follow the instructions on the screen

**Serial connection**
* This option is required for setting up new devices.
* Install the PlatformIO plugin in VSCode and open this repository.

**Configure the Project**
* Depending on which SparkFun board you're using, modify the **platformio.ini** file with the correct information:
```
; Uncomment if you're using a SparkFun Thing Plus C
; [env:sparkfun_esp32s2_thing_plus_c]
; board = sparkfun_esp32s2_thing_plus_c

; Uncomment if you're using a SparkFun IoT Redboard
[env:sparkfun_esp32_iot_redboard]
board = sparkfun_esp32_iot_redboard
```
* You will also need to update the GPIO information within **main.cpp** based on the SparkFun board that you're using:
```
#define DATA0 26               // Pin Header for DATA0 Connetion (IoT Redboard = 26 | Thing Plus C = 25)
#define DATA1 27               // Pin Header for DATA1 Connetion (IoT Redboard = 27 | Thing Plus C = 26)
const int C_PIN_LED = 18;      // LED indicator for config mode (IOT Redboard = 18 | Thing Plus C = 13)
```

**Build and Upload the Filesystem:**
The **config.json** file within the **data** directory will be written to flash memory and will be used as a placeholder for the email notification configuration.
* Attach the SparkFun board to your computer and click on the alien (PlatformIO) icon inside of VSCode. If you're using the IoT Redboard, you will not need to change the boot mode. If you're using the Thing Plus C, you will need to press the **BOOT** button before writing firmware / performing filesystem operations.
* Under **PROJECT TASKS -> Platform**, click **Build Filesystem Image**. This will create the LittleFS filesystem.
* Under **PROJECT TASKS -> Platform**, click **Upload Filesystem Image**. This will upload the filesystem to the IoT RedBoard. *Note, if you receive an error, it's likely that you have the serial monitor running. Close the serial monitor and try again.*

**Build and upload the Firmware:**
* Click the **Trashcan** on the lower ribbon of VSCode. This will **clean** the project. 
* Click the **Checkmark** on the lower ribbon of VSCode. This will build the firmware.
* Click the **RightArrow** on the lower ribbon of VSCode. This will upload the firmware to the IoT RedBoard.

**Copy WebApp and blank logs to the SDCard:**
* Attach the MicroSD Card to the computer and erase/format the card to FAT.
* Drag and drop the contents of the **SDCard directory** to the root of the SD Card. *Note, the prepopulated cards.json file consists of randomly generated test data. Use the Reset function within the web application to clear the data before use.*

## Wiring Doppelgänger to a RFID Reader

Only attempt to build this device if you know what you're doing. The developer is not responsible for damaged equipment/property, personal injury, or death that may result in exposure to high voltage. Doppelgänger has been tested on the following devices, but it should work on any reader that outputs a Weigand signal:
* HID MaxiProx 5375AGN00
* HID ICLASS SE R90 940NTTEK00000 (this model has legacy support enabled)
* HID Indala ASR-620++

The following additional items are recommended to build a complete system:

| Item                                                        | Qty |
| ----------------------------------------------------------- | --- |
| SparkFun IoT Redboard or Thing Plus C                       | 1   |
| Powerbank (Omni Mobile 25600mah recommended)                | 1   |
| Adhesive Standoffs 0.180" Height                            | 4   |
| 1.5' USB-C to USB-A Cable                                   | 1   |
| 8GB Micro SD Card (Use a Trusted Brand)                     | 1   |
| 5.5x2.1mm Power Cable (Male)                                | 1   |
| SparkFun Logic Level Converter                              | 1   |
| 22 awg Solid Core Wire (Six Color Pack)                     | 1   |
| 1" Outdoor Zip Tie Mounts and Zip Ties                      | 2   |
| 2.54mm 0.1" Pitch PCB Mount Screw Terminal Block Connectors | 1   |


Below are the wiring diagrams for each reader. As the RFID readers below output a 5V signal, the use of a logic level converter is required to adapt the signal to 3.3V. 

*Note, you can use either the IoT Redboard or the Thing Plus C, the diagrams are merely an example. However, Indala readers are tight on space, so I recommend the Thing Plus C.*

![MaxiProx Wiring](https://github.com/optiv/doppelganger/blob/main/git_images/maxiprox_wiring.jpg)

![ICLASS SE Wiring](https://github.com/optiv/doppelganger/blob/main/git_images/iclass_se_wiring.jpg)

![Indala Wiring](https://github.com/optiv/doppelganger/blob/main/git_images/indala_wiring.jpg)

![SE Installation](https://github.com/optiv/doppelganger/blob/main/git_images/se_install.jpg)

![Indala Wiring](https://github.com/optiv/doppelganger/blob/main/git_images/indala_install.jpg)

### Credit
Special thanks to the following folks for doing all of the heavy lifting with the original research,
development and Wiegand code refreshing. With this project, I merely created a method of delivery that suits my team's needs.

Francis Brown - To this day, "RFID Hacking Live Free or RFID Hard" remains my favorite
conference talk. Watching that talk blew my mind as I left the military many years ago and started penetration testing. Truly inspirational!
studebacon - Without stumbling across your Tastic refresh, I may not have invested the time building this out.
Daniel Smith - For laying so much of the groundwork that has made all of these awesome tools possible.

Checkout the following links; they're worth your time:
* https://bishopfox.com/blog/tastic-rfid-thief-silent-deadly
* https://github.com/studebacon/TasticCodeUpdate (Wiegand processing code in Doppelgänger originates from this repo)
* https://www.pagemac.com/projects/rfid/hid_data_formats
