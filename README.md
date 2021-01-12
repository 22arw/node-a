# Node-A Cruiser Maintenance Management for Wichita
## About this project
This project will improve cruiser maintenance tracking for the Wichita Police department. To better maintain our Wichita Police cruisers, an OBD-II compatible device connects to and monitors each cruiser. The device utilizes a LoRaWAN network within Wichita to send realtime notifications and data through gateways to a central server which can then be displayed on dashboards for the department. In order to utilize the LoRaWAN network, we’ll make use of the Protocol Buffers method of serializing data into small packets. We may divide the data into multiple packets sent periodically to meet the LoRaWAN duty cycle standard. Once the Server group has set up a network server, we’ll work to create a windows application that will display statistics from all cruisers and notify the maintenance department when maintenance is due. 
## Pinout
The pinout for the MKRCAN sheild, Adafruit Feather 32u4, ODB2 connector, and LED is listed below.


| DESC           | MKRCAN | Feather   | ODB2 | LED |
|----------------|--------|-----------|------|-----|
| CAN CS         | 3      | 6         |      |     |
| INT            | 7      | SDA(1)    |      |     |
| MOSI           | 8      | MO        |      |     |
| SCK            | 9      | SCK       |      |     |
| MISO           | 10     | MI        |      |     |
| CPU POWER      | vcc    | 3v        |      |     |
| POWER TO F     | vin    | usb       |      |     |
| POWER FROM F   | 5v     | usb       |      |     |
| GND            | GND    | GND       |      |     |
| LED GND        |        | GND       |      | GND |
| Red            |        | 9         |      | R   |
| Green          |        | 10        |      | G   |
| Blue           |        | 11        |      | B   |
| POWER FROM CAR |        | 16        | VIN  |     |
| CAR CAN        |        | 6 (CANH)  | CANL |     |
| CAR CAN        |        | 14 (CANL) | CANH |     |
| GND            |        | 5         | GND  |     |

*The CAN library uses default CS and INT pins. To set different pins, use CAN.setPins(CS, INT).*
