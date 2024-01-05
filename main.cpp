#include <cstdint>
#include <stdio.h>
#include "pico/stdlib.h"
#include <string>

#include "bmp280.h"
#include "oled.h"


// int main() {
//     stdio_init_all();
//     std::string x = "Sandrino";
//     while (true) {
//         printf("Hello, world! %s\n", x.c_str());
//         sleep_ms(1000);
//     }
// }

int main() {
    stdio_init_all();

    // useful information for picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("BMP280 I2C example for the Raspberry Pi Pico"));

    printf("Hello, BMP280! Reading temperaure and pressure values from sensor...\n");

    // I2C is "open drain", pull ups to keep signal high when no data is being sent
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    sleep_ms(1000);
    for (int addr = 0x08; addr <= 0x77; addr++) {
        uint8_t data = 0;  // You can change this to any data
        int result = i2c_write_blocking(i2c_default, addr, &data, 1, true);
        if (result >= 0) {
            printf("Found device at address 0x%02X\n", addr);
        }
        else
        {
            printf("11 DID not found device at address0x%02X\n", addr);
        }
    }

    // configure BMP280
    bmp280_init();

    // retrieve fixed compensation params
    struct bmp280_calib_param params;
    bmp280_get_calib_params(&params);

    int32_t raw_temperature;
    int32_t raw_pressure;
    int32_t raw_humidity;

    sleep_ms(250); // sleep so that data polling and register update don't collide

    // I2C scanner loop (outside the main while loop)
    printf("Scanning I2C bus...\n");

    for (int addr = 0x08; addr <= 0x77; addr++) {
        uint8_t data = 0;  // You can change this to any data
        int result = i2c_write_blocking(i2c_default, addr, &data, 1, true);
        if (result >= 0) {
            printf("Found device at address 0x%02X\n", addr);
        }
        else
        {
            printf("22 DID not found device at address0x%02X\n", addr);
        }
    }
    // while (1) {
        bmp280_read_raw(&raw_temperature, &raw_pressure, &raw_humidity);
        int32_t temperature = bmp280_convert_temp(raw_temperature, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);
        printf("Preassure = %.3f kPa\n", pressure / 1000.f);
        printf("Temp. = %.2f C\n", temperature / 100.f);
        // poll every 500ms
        sleep_ms(500);
    // }

    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);

    // run through the complete initialization process
    SSD1306_init();

    // Initialize render area for entire frame (SSD1306_WIDTH pixels by SSD1306_NUM_PAGES pages)
    struct render_area frame_area = {
        start_col: 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
        };

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

    // render 3 cute little raspberries
    struct render_area area = {
        start_page : 0,
        end_page : (IMG_HEIGHT / SSD1306_PAGE_HEIGHT)  - 1
    };

restart:

    area.start_col = 0;
    area.end_col = IMG_WIDTH - 1;

    calc_render_area_buflen(&area);
    
    uint8_t offset = 5 + IMG_WIDTH; // 5px padding

    for (int i = 0; i < 3; i++) {
        render(raspberry26x32, &area);
        area.start_col += offset;
        area.end_col += offset;
    }

    SSD1306_scroll(true);
    sleep_ms(5000);
    SSD1306_scroll(false);

    bmp280_read_raw(&raw_temperature, &raw_pressure, &raw_humidity);
    temperature = bmp280_convert_temp(raw_temperature, &params);
    pressure = bmp280_convert_pressure(raw_pressure, raw_temperature, &params);

    // char *text[] = {
    //     "Temperature: ",
    //     "Preassure:   ",
    // };

    std::string text = "Temperatur: " + std::to_string(static_cast<int32_t>(temperature)) + "\nPreassure: " + std::to_string(static_cast<int32_t>(pressure)) + "\n";

    int y = 0;
    for (int i = 0 ;i < count_of(text.c_str()); i++) {
        WriteString(buf, 5, y, text[i]);
        y+=8;
    }
    render(buf, &frame_area);

    // Test the display invert function
    sleep_ms(3000);
    SSD1306_send_cmd(SSD1306_SET_INV_DISP);
    sleep_ms(3000);
    SSD1306_send_cmd(SSD1306_SET_NORM_DISP);

    bool pix = true;
    for (int i = 0; i < 2;i++) {
        for (int x = 0;x < SSD1306_WIDTH;x++) {
            DrawLine(buf, x, 0,  SSD1306_WIDTH - 1 - x, SSD1306_HEIGHT - 1, pix);
            render(buf, &frame_area);
        }

        for (int y = SSD1306_HEIGHT-1; y >= 0 ;y--) {
            DrawLine(buf, 0, y, SSD1306_WIDTH - 1, SSD1306_HEIGHT - 1 - y, pix);
            render(buf, &frame_area);
        }
        pix = false;
    }

    goto restart;

    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////

}
