#include <cstdint>
#include <stdio.h>
#include "pico/platform.h"
#include "pico/stdlib.h"
#include <string>
#include "hardware/gpio.h"

#include "bmp280.h"
#include "oled.h"

#define MOTION_SENSOR_PIN 0


struct render_area setupFrameArea()
{
    // Initialize render area for entire frame (SSD1306_WIDTH pixels by SSD1306_NUM_PAGES pages)
    struct render_area frame_area = {
        start_col: 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
        };

    calc_render_area_buflen(&frame_area);
    return frame_area;
}

void readTemperaturePreassureAndDisplay()
{
    sleep_ms(1000);
    // retrieve fixed compensation params
    struct bmp280_calib_param params;
    bmp280_get_calib_params(&params);

    int32_t raw_temperature;
    int32_t raw_pressure;
    int32_t raw_humidity;

    sleep_ms(250); // sleep so that data polling and register update don't collide


    bmp280_read_raw(&raw_temperature, &raw_pressure, &raw_humidity);
    int32_t temperature = bmp280_convert_temp(raw_temperature, &params);
    int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);
    printf("Preassure = %.3f kPa\n", pressure / 1000.f);
    printf("Temp. = %.2f C\n", temperature / 100.f);
    sleep_ms(500);

    auto frame_area = setupFrameArea();

    calc_render_area_buflen(&frame_area);

    // zero the entire display
    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    // intro sequence: flash the screen 3 times
    for (int i = 0; i < 3; i++) {
        SSD1306_send_cmd(SSD1306_SET_ALL_ON);    // Set all pixels on
        sleep_ms(500);
        SSD1306_send_cmd(SSD1306_SET_ENTIRE_ON); // go back to following RAM for pixel state
        sleep_ms(500);
    }

    bmp280_read_raw(&raw_temperature, &raw_pressure, &raw_humidity);
    temperature = bmp280_convert_temp(raw_temperature, &params);
    pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);

    // I guess this is stupid
    float pres = pressure / 1000.f;
    float temp = temperature / 100.f;
    const char *text[] = {
        ("Temperature: " + std::to_string(temp)).c_str(),
        ("Pressure:    " + std::to_string(pres)).c_str()
    };

    printf("DEBUG Temperature: %.2f\n", temp);
    printf("DEBUG Pressure:    %.2f\n", pres);

    int y = 0;
    for (int i = 0 ;i < count_of(text); i++) {
        WriteString(buf, 5, y, text[i]);
        y+=8;
    }
    render(buf, &frame_area);

    // // Test the display invert function
    sleep_ms(3000);
    SSD1306_send_cmd(SSD1306_SET_INV_DISP);
    sleep_ms(3000);
    SSD1306_send_cmd(SSD1306_SET_NORM_DISP);
}

void displaySleeping() {
    struct render_area frame_area = setupFrameArea();
    calc_render_area_buflen(&frame_area);

    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    // Display "Sleeping..." message
    memset(buf, 0, SSD1306_BUF_LEN);
    const char *sleepingMessage = "Sleeping...";
    WriteString(buf, 5, 0, sleepingMessage);
    render(buf, &frame_area);
}


int main() {
    stdio_init_all();

    // useful information for picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("BMP280 I2C example for the Raspberry Pi Pico"));

    printf("Hello, BMP280! Reading temperaure and pressure values from sensor...\n");

    // Initialize I2C for BMP280
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Initialize BMP280
    bmp280_init();

    // Initialize I2C for SSD1306
    i2c_init(i2c_default, SSD1306_I2C_CLK * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Initialize SSD1306
    SSD1306_init();


    // Motion sensor begin
    gpio_init(MOTION_SENSOR_PIN);
    gpio_set_dir(MOTION_SENSOR_PIN, GPIO_IN);

    while(true)
    {
        if(gpio_get(MOTION_SENSOR_PIN))
        {
            printf("MOTION DETECTED\n");

            readTemperaturePreassureAndDisplay();
            printf("GOING TO PAUSE 5 seconds\n");
            while (gpio_get(MOTION_SENSOR_PIN)) {
                sleep_ms(5000);  // Adjust the delay as needed
            }
            printf("Woke up from 5 second sleep\n");

        }
        else {
            printf("NOOO motion detected\n");
            displaySleeping();
        }

        sleep_ms(500);
    }
}
