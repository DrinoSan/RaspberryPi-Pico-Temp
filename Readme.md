# RaspberryPi Pico Temp Measurement

A simple program to measure temperature/humidity/pressure.

## Used components
### Temperature/humidity/pressure 
- BMP280
  - I2C address: 0x76 (Depending on SDO; if connected to GND, the address is 0x76)

### Display
- SSD1306
  - I2C address: 0x3C

## Wiring 
The wiring is in series with the devices. Thanks to the I2C protocol, communication works as long as the I2C addresses of the devices do not collide.


## Reading logs
```
minicom -b 115200 -o -D /dev/tty.usbmodem11301
```

To move uf2 file to pico
cd /Volumes/RPI-RP2
cp u2f file