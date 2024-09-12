#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_random.h"
#include "bootloader_random.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

#include "bitbang.h"

hub75 *panel = NULL;

static void prepare_pins()
{
    gpio_reset_pin((gpio_num_t)R0);
    gpio_set_direction((gpio_num_t)R0, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)G0);
    gpio_set_direction((gpio_num_t)G0, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)B0);
    gpio_set_direction((gpio_num_t)B0, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)R1);
    gpio_set_direction((gpio_num_t)R1, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)G1);
    gpio_set_direction((gpio_num_t)G1, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)B1);
    gpio_set_direction((gpio_num_t)B1, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)LAT);
    gpio_set_direction((gpio_num_t)LAT, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)CLK);
    gpio_set_direction((gpio_num_t)CLK, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)OE);
    gpio_set_direction((gpio_num_t)OE, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)A);
    gpio_set_direction((gpio_num_t)A, GPIO_MODE_OUTPUT);
    gpio_pullup_en((gpio_num_t)A);

    gpio_reset_pin((gpio_num_t)B);
    gpio_set_direction((gpio_num_t)B, GPIO_MODE_OUTPUT);
    gpio_pullup_en((gpio_num_t)B);

    gpio_reset_pin((gpio_num_t)C);
    gpio_set_direction((gpio_num_t)C, GPIO_MODE_OUTPUT);
    gpio_pullup_en((gpio_num_t)C);

    gpio_reset_pin((gpio_num_t)D);
    gpio_set_direction((gpio_num_t)D, GPIO_MODE_OUTPUT);
    gpio_pullup_en((gpio_num_t)D);

}

static void hub75_set_pixel1(pixel_color pixel, uint8_t bit)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level((gpio_num_t)R0, (pixel.r >> bit) & 0x1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level((gpio_num_t)G0, (pixel.g >> bit) & 0x1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level((gpio_num_t)B0, (pixel.b >> bit) & 0x1));
}

static void hub75_set_pixel2(pixel_color pixel2, uint8_t bit)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level((gpio_num_t)R1, (pixel2.r >> bit) & 0x1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level((gpio_num_t)G1, (pixel2.g >> bit) & 0x1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level((gpio_num_t)B1, (pixel2.b >> bit) & 0x1));
}

static uint8_t hub75_next_row(uint8_t row)
{
    gpio_set_level((gpio_num_t)A, (row >> 0) & 0x1);
    gpio_set_level((gpio_num_t)B, (row >> 1) & 0x1);
    gpio_set_level((gpio_num_t)C, (row >> 2) & 0x1);
    gpio_set_level((gpio_num_t)D, (row >> 3) & 0x1);

    // if(row == COL_SIZE/2) return 0;
    return row++;
}

static void hub75_gate_clk()
{
    gpio_set_level((gpio_num_t)CLK, 1);
    gpio_set_level((gpio_num_t)CLK, 0);
}

static void hub75_latch_row(uint8_t lvl) // LAT pin
{
    gpio_set_level((gpio_num_t)LAT, lvl);
}

static void hub75_swap_pixels(uint8_t lvl) // OE pin
{
    gpio_set_level((gpio_num_t)OE, lvl);
}

void rotate_matrix_flip();
void rotate_matrix_180();

IRAM_ATTR static void copy_image()
{
#if 0 // 64x64px
    for (size_t j = 0; j < 64;)
    {
        for (size_t i = 0; i < 64;)
        {
            panel->cols[j].pixels[i + 32].g = ELE013_map2[j * 256 + i * 4 + 1]; //
            panel->cols[j].pixels[i + 32].b = ELE013_map2[j * 256 + i * 4 + 2]; //
            panel->cols[j].pixels[i + 32].r = ELE013_map2[j * 256 + i * 4 + 3]; //
            i += 1;
        }
        j++;
    }
#else // 32x32px
    for (size_t j = 0; j < 32;)
    {
        for (size_t i = 0; i < 32;)
        {
            panel->cols[j + 6].pixels[i + 18].g = ELE013_map[j * 32 * 3 + i * 3 + 0]; //
            panel->cols[j + 6].pixels[i + 18].b = ELE013_map[j * 32 * 3 + i * 3 + 1]; //
            panel->cols[j + 6].pixels[i + 18].r = ELE013_map[j * 32 * 3 + i * 3 + 2]; //
            i += 1;
        }
        j++;
    }
#endif
}

void app_main(void)
{
    prepare_pins();
    bootloader_random_enable();
    hub75_next_row(0);
    hub75_gate_clk();
    uint8_t row = 0;

    // for performance comparison based on where pixels buffer is placed
    // panel = (hub75 *)heap_caps_calloc(ROW_SIZE * COL_SIZE * PANELS_H * 4, 4, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
    // panel = (hub75 *)heap_caps_calloc(ROW_SIZE * COL_SIZE * PANELS_H * 4, 4, MALLOC_CAP_SPIRAM);
    // panel = (hub75 *)heap_caps_calloc(ROW_SIZE * COL_SIZE * PANELS_H * 4, 4, MALLOC_CAP_DMA);
    // panel = (hub75 *)heap_caps_calloc(ROW_SIZE * COL_SIZE * PANELS_H * 4, 4, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    panel = (hub75 *)heap_caps_calloc(ROW_SIZE * COL_SIZE * PANELS_H * 4, 4, MALLOC_CAP_INTERNAL);
    assert(panel);
    copy_image();

    while (1)
    {

        // copy_image();
#if PANELS_H > 1
        rotate_matrix_180();
#endif
        int min = 0 * COL_SIZE / 2;
        int max = COL_SIZE / 2 + min;
        for (size_t i = min; i < max; i++)
        {
            uint8_t n = 7;
            // for (n = 0; n < 8; n++)
            {
                row = hub75_next_row(i);
                hub75_latch_row(0);

                for (size_t z = 0; z < PANELS_H; z++)
                {
                    int dir = z % 2;
                    if (dir)
                    {
                        // rotate matrix
                        rotate_matrix_flip();
                        for (int j = ROW_SIZE * PANELS_W - 1; j >= 0; j--)
                        {
                            pixel_color pixel1 = panel->cols[0 * 32 + i].pixels[j];
                            pixel_color pixel2 = panel->cols[0 * 32 + i + 16].pixels[j];
                            hub75_set_pixel1(pixel1, n);
                            hub75_set_pixel2(pixel2, n);
                            hub75_gate_clk();
                        }
                        rotate_matrix_flip();
                    }
                    else
                    {
                        for (int j = 0; j < ROW_SIZE * PANELS_W; j++)
                        {
                            pixel_color pixel1 = panel->cols[z * 32 + i].pixels[j];
                            pixel_color pixel2 = panel->cols[z * 32 + i + 16].pixels[j];
                            hub75_set_pixel1(pixel1, n);
                            hub75_set_pixel2(pixel2, n);
                            hub75_gate_clk();
                        }
                    }
                }
                hub75_latch_row(1);
                hub75_swap_pixels(0);
                ets_delay_us(2); // <------------ brightness
                hub75_swap_pixels(1);
            }
        }
        vTaskDelay(1);
        static uint16_t ticks = 0;
        static uint64_t start = 0;
        ticks++;
        uint64_t end = esp_timer_get_time();

        if (end - start >= 1000 * 1000)
        {
            start = esp_timer_get_time();
            ESP_LOGI("", "fps: %d", ticks);
            ticks = 0;
        }
    }
}

void rotate_matrix_flip()
{
    for (size_t i = 0; i < ROW_SIZE * PANELS_W; i++)
    {
        for (size_t j = 0; j < COL_SIZE / 2 * PANELS_H; j++)
        {
            pixel_color temp = panel->cols[j].pixels[i];
            panel->cols[j].pixels[i] = panel->cols[COL_SIZE * PANELS_H - 1 - j].pixels[i];
            panel->cols[COL_SIZE * PANELS_H - 1 - j].pixels[i] = temp;
        }
    }
}

void rotate_matrix_180()
{
    for (size_t i = 0; i < COL_SIZE * PANELS_H; i++)
    {
        for (size_t j = 0; j < ROW_SIZE * PANELS_W / 2; j++)
        {
            pixel_color temp = panel->cols[i].pixels[j];
            panel->cols[i].pixels[j] = panel->cols[i].pixels[ROW_SIZE * PANELS_W - 1 - j];
            panel->cols[i].pixels[ROW_SIZE * PANELS_W - 1 - j] = temp;
        }
    }

    for (size_t i = 0; i < ROW_SIZE * PANELS_W; i++)
    {
        for (size_t j = 0; j < COL_SIZE / 2 * PANELS_H; j++)
        {
            pixel_color temp = panel->cols[j].pixels[i];
            panel->cols[j].pixels[i] = panel->cols[COL_SIZE * PANELS_H - 1 - j].pixels[i];
            panel->cols[COL_SIZE * PANELS_H - 1 - j].pixels[i] = temp;
        }
    }
}
