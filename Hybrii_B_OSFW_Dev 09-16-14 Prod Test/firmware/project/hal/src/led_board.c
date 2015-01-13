#include <stdio.h>
#include <REG51.H>
#include <string.h>
#include "hal_common.h"
#include "utils.h"
#include "led_board.h"
static led_char_t led_num_tbl [] = {
    { 0x5e, 0x4a, 0xe9, 0x01 },   // 0
    { 0x88, 0x21, 0xc4, 0x01 },   // 1
    { 0x1e, 0x7a, 0xe1, 0x01 },   // 2
    { 0x1e, 0x7a, 0xe8, 0x01 },   // 3
    { 0x52, 0x7a, 0x08, 0x01 },   // 4
    { 0x5e, 0x78, 0xe8, 0x01 },   // 5
    { 0x5e, 0x78, 0xe9, 0x01 },   // 6
    { 0x1e, 0x42, 0x08, 0x01 },   // 7
    { 0x5e, 0x7a, 0xe9, 0x01 },   // 8
    { 0x5e, 0x7a, 0xe8, 0x01 },   // 9
};

static led_char_t led_gvc_tbl [] = {
    { 0x3f, 0xf4, 0xf8, 0x01 },   // G
    { 0x31, 0x46, 0x45, 0x00 },   // V
    { 0x2e, 0x86, 0xe8, 0x00 },   // C
};

static led_char_t led_letters_tbl [] = {
    { 0x44, 0xFD, 0x18, 0x01 },  // A
    { 0x2F, 0xBE, 0xF8, 0x00 },  // B
    { 0x2E, 0x86, 0xE8, 0x00 },  // C
    { 0x2F, 0xC6, 0xF8, 0x00 },  // D
    { 0x3F, 0xBC, 0xF0, 0x01 },  // E
    { 0x3F, 0xBC, 0x10, 0x00 },  // F
    { 0x3F, 0xF4, 0xF8, 0x01 },  // G
    { 0x31, 0xFE, 0x18, 0x01 },  // H
    { 0x8E, 0x10, 0xE2, 0x00 },  // I
    { 0x8E, 0x90, 0x62, 0x00 },  // J
    { 0xA9, 0x8C, 0x92, 0x00 },  // K
    { 0x21, 0x84, 0xF0, 0x00 },  // L
    { 0x71, 0xD7, 0x18, 0x01 },  // M
    { 0x71, 0xD6, 0x1C, 0x01 },  // N
    { 0x26, 0xA5, 0x64, 0x00 },  // O
    { 0x2F, 0xBD, 0x10, 0x00 },  // P
    { 0x26, 0xA5, 0xE4, 0x01 },  // Q
    { 0x3F, 0xFE, 0x14, 0x01 },  // R
    { 0x3E, 0x38, 0xF8, 0x00 },  // S
    { 0x9F, 0x10, 0x42, 0x00 },  // T
    { 0x31, 0xC6, 0xE8, 0x00 },  // U
    { 0x31, 0x46, 0x45, 0x00 },  // V
    { 0x31, 0xD6, 0x1D, 0x01 },  // W
    { 0x51, 0x11, 0x15, 0x01 },  // X
    { 0x51, 0x11, 0x42, 0x00 },  // Y
    { 0x1F, 0x11, 0xF1, 0x01 },  // Z
};

static u8 dim_value [] = {
    0x1f, 0x1c, 0x19, 0x16, 0x13, 0x0f, 0x0b, 0x07, 0x03, 0x00
};

static void led_cs (u8 dev)
{
#ifdef OLD_LED_BOARD
    WriteU8Reg(0x401, dev);
    WriteU8Reg(0x401, dev);
#else
    dev = dev; // Make compiler happy
#endif
}

static void init_led_bar ()
{
    mac_utils_spi_led_write(0, 0, 0x6f);
}

static void led_dim (u8 led_bar_id, u8 usr_value)
{
    u8 adj_value = 0;

    if (led_bar_id == 2) {
        adj_value = 0x20;
    } else if (led_bar_id == 3) {
        adj_value = 0x40;
    }
    
    if (usr_value > 9) {
        usr_value = 9;
    }
    mac_utils_spi_led_write(0, 0, dim_value[usr_value]+ adj_value);
}

void init_led_board ()
{
#ifdef MATRIX_LED_BOARD
    u8 i, j, k;
    u8 spi_led_dev;

    WriteU8Reg(0x402, 0x18);  // Tel PHY to send 24 bits
    
    for (spi_led_dev = 0; spi_led_dev < 6; spi_led_dev++) {
        switch (spi_led_dev) {
        case 0:
            led_cs(RED1_DEV);
            break;
        case 1:
            led_cs(RED2_DEV);
            break;
        case 2:
            led_cs(GREEN1_DEV);
            break;
        case 3:
            led_cs(GREEN2_DEV);
            break;
        case 4:
            led_cs(BLUE1_DEV);
            break;
        case 5:
            led_cs(BLUE2_DEV);
            break;
        }

        for (j = 0; j < 6; j++) {
            for (k = 0; k < 2; k++) {
                for (i = 0; i < 0x16; i++) {
                    mac_utils_spi_led_write(RED1_SPI_ADDR, i, 0);
                    mac_utils_spi_led_write(RED2_SPI_ADDR, i, 0);
                    mac_utils_spi_led_write(GREEN1_SPI_ADDR, i, 0);
                    mac_utils_spi_led_write(GREEN2_SPI_ADDR, i, 0);
                    mac_utils_spi_led_write(BLUE1_SPI_ADDR, i, 0);
                    mac_utils_spi_led_write(BLUE2_SPI_ADDR, i, 0);
                }
            }
            mac_utils_spi_led_write(RED1_SPI_ADDR, 0xa, 0x38);
            mac_utils_spi_led_write(RED2_SPI_ADDR, 0xa, 0x38);
            mac_utils_spi_led_write(GREEN1_SPI_ADDR, 0xa, 0x38);
            mac_utils_spi_led_write(GREEN2_SPI_ADDR, 0xa, 0x38);
            mac_utils_spi_led_write(BLUE1_SPI_ADDR, 0xa, 0x38);
            mac_utils_spi_led_write(BLUE2_SPI_ADDR, 0xa, 0x38);
        }
    }
    WriteU8Reg(0x402, 0x10);  // Return to default (send 16)
#endif
    init_led_bar();
}

static void led_select_color (bool on, u8 color)
{
    u8  value;
    u8  led_id_1;
    u8  led_id_2;
    u16 led_addr_1;
    u16 led_addr_2;

    if (TRUE == on) {
        value = 0xff;
    } else {
        value = 0x00;
    }

    switch (color) {
    case RED:
        led_id_1   = RED1_DEV;
        led_id_2   = RED2_DEV;
        led_addr_1 = RED1_SPI_ADDR;
        led_addr_2 = RED2_SPI_ADDR;
        break;
    case GREEN:
        led_id_1   = GREEN1_DEV;
        led_id_2   = GREEN2_DEV;
        led_addr_1 = GREEN1_SPI_ADDR;
        led_addr_2 = GREEN2_SPI_ADDR;
        break;
    case BLUE:
    default:
        led_id_1   = BLUE1_DEV;
        led_id_2   = BLUE2_DEV;
        led_addr_1 = BLUE1_SPI_ADDR;
        led_addr_2 = BLUE2_SPI_ADDR;
        break;
    }
    led_cs(led_id_1);
    mac_utils_spi_led_write(led_addr_1, 0x12, value);
    mac_utils_spi_led_write(led_addr_1, 0x13, value);
    led_cs(led_id_2);
    mac_utils_spi_led_write(led_addr_2, 0x12, value);
    mac_utils_spi_led_write(led_addr_2, 0x13, value);
}

static void led_state (bool on, u8 color)
{
    switch (color) {
    case RED:
        led_select_color(on, RED);
        break;
    case GREEN:
        led_select_color(on, GREEN);
        break;
    case BLUE_LITE:
        led_select_color(on, BLUE);
        led_select_color(on, GREEN);
        break;
    case PINK:
        led_select_color(on, RED);
        led_select_color(on, BLUE);
        break;
    case YELLOW:
        led_select_color(on, RED);
        led_select_color(on, GREEN);
        break;
    case WHITE:
        led_select_color(on, RED);
        led_select_color(on, GREEN);
        led_select_color(on, BLUE);
        break;
    case BLUE:
    default:
        led_select_color(on, BLUE);
        break;
    }
}

static u8 led_dev_to_spi_addr (u8 dev)
{
    u8 dev_spi_addr;

    switch (dev) {
    case RED1_DEV:
        dev_spi_addr = RED1_SPI_ADDR;
        break;
    case RED2_DEV:
        dev_spi_addr = RED2_SPI_ADDR;
        break;
    case GREEN1_DEV:
        dev_spi_addr = GREEN1_SPI_ADDR;
        break;
    case GREEN2_DEV:
        dev_spi_addr = GREEN2_SPI_ADDR;
        break;
    case BLUE1_DEV:
        dev_spi_addr = BLUE1_SPI_ADDR;
        break;
    case BLUE2_DEV:
        dev_spi_addr = BLUE2_SPI_ADDR;
        break;
    }

    return (dev_spi_addr);
}

static void led_char_display (led_char_t* led_char_tbl, u8 tbl_index,
                              u8 dev1_addr, u8 dev2_addr)
{
    u8 dev1_spi_addr = led_dev_to_spi_addr(dev1_addr);
    u8 dev2_spi_addr = led_dev_to_spi_addr(dev2_addr);

    led_cs(dev1_addr);
    mac_utils_spi_led_write(dev1_spi_addr, 0x12, led_char_tbl[tbl_index].a1);
    mac_utils_spi_led_write(dev1_spi_addr, 0x13, led_char_tbl[tbl_index].b1);
    led_cs(dev2_addr);
    mac_utils_spi_led_write(dev2_spi_addr, 0x12, led_char_tbl[tbl_index].a2);
    mac_utils_spi_led_write(dev2_spi_addr, 0x13, led_char_tbl[tbl_index].b2);
}

void led_char_display_color (led_char_t* led_char_tbl, u8 tbl_index, u8 color)
{
    switch (color) {
    case RED:
        led_char_display(led_char_tbl, tbl_index, RED1_DEV, RED2_DEV);
        mac_utils_delay_ms(3000);
        led_state(FALSE, color);
        break;
    case GREEN:
        led_char_display(led_char_tbl, tbl_index, GREEN1_DEV, GREEN2_DEV);
        mac_utils_delay_ms(3000);
        led_state(FALSE, color);
        break;    
    case BLUE_LITE:
        led_char_display(led_char_tbl, tbl_index, BLUE1_DEV, BLUE2_DEV);
        led_char_display(led_char_tbl, tbl_index, GREEN1_DEV, GREEN2_DEV);
        mac_utils_delay_ms(3000);
        led_state(FALSE, BLUE);
        led_state(FALSE, GREEN);
        break;
    case PINK:
        led_char_display(led_char_tbl, tbl_index, RED1_DEV, RED2_DEV);
        led_char_display(led_char_tbl, tbl_index, BLUE1_DEV, BLUE2_DEV);
        mac_utils_delay_ms(3000);
        led_state(FALSE, RED);
        led_state(FALSE, BLUE);
        break;
    case YELLOW:
        led_char_display(led_char_tbl, tbl_index, RED1_DEV, RED2_DEV);
        led_char_display(led_char_tbl, tbl_index, GREEN1_DEV, GREEN2_DEV);
        mac_utils_delay_ms(3000);
        led_state(FALSE, RED);
        led_state(FALSE, GREEN);
        break;
    case WHITE:
        led_char_display(led_char_tbl, tbl_index, RED1_DEV, RED2_DEV);
        led_char_display(led_char_tbl, tbl_index, GREEN1_DEV, GREEN2_DEV);
        led_char_display(led_char_tbl, tbl_index, BLUE1_DEV, BLUE2_DEV);
        mac_utils_delay_ms(3000);
        led_state(FALSE, RED);
        led_state(FALSE, GREEN);
        led_state(FALSE, BLUE);
        break;
    case BLUE:
    default:
        led_char_display(led_char_tbl, tbl_index, BLUE1_DEV, BLUE2_DEV);
        mac_utils_delay_ms(3000);
        led_state(FALSE, BLUE);
        break;
    }
}


static void led_gvc_display (u8 dev1_addr, u8 dev2_addr, u8 color)
{
    u8 i;
    
    for (i = 0; i < 3; i++) {
        led_char_display(led_gvc_tbl, i, dev1_addr, dev2_addr);
        led_state(FALSE, color);
    }
}

static void led_letter (u8 color)
{
    switch (color) {
    case RED:
        led_gvc_display(RED1_DEV, RED2_DEV, color);
        break;
    case GREEN:
        led_gvc_display(GREEN1_DEV, GREEN2_DEV, color);
        break;
    case BLUE:
    default:
        led_gvc_display(BLUE1_DEV, BLUE2_DEV, color);
        break;
    }
}

static void led_number (u8 color, u8 num)
{
    switch (color) {
    case RED:
        led_char_display(led_num_tbl, num, RED1_DEV, RED2_DEV);
        break;
    case GREEN:
        led_char_display(led_num_tbl, num, GREEN1_DEV, GREEN2_DEV);
        break;
    case BLUE:
    default:
        led_char_display(led_num_tbl, num, BLUE1_DEV, BLUE2_DEV);
        break;
    }
    led_state(FALSE, color);
}

void led_control (bool state, u8 color, bool letter, u8 num)
{
    WriteU8Reg(0x402, 0x18);  // Tel PHY to send 24 bits
    if (FALSE == state) {
        led_state(state, RED);
        led_state(state, GREEN);
        led_state(state, BLUE);
        return;
    }
    if (TRUE == letter) {
        led_letter(color);
    } else {
        if (num < 10) {
            led_number(color, num);
        } else {
            led_state(state, color);
        }
    }
    WriteU8Reg(0x402, 0x10);  // Return to default (send 16)
}

static u8* color_support_str[COLOR_SUPPORT_MAX] = {
    "red", "green", "blue-lite", "blue", "pink", "yellow", "white"
};

u8 led_get_color (u8 *led_msg)
{
    u8 *cmd_p;
    u8 *payload_p;
    u8 color;
    u8 idx;

    payload_p = led_msg;
    color = BLUE;
    for (idx = 0; idx < COLOR_SUPPORT_MAX; idx++) {
        cmd_p = strstr(payload_p, color_support_str[idx]);
        if (cmd_p) {
            color = idx;
            break;
        }
    }

    return (color);
}

void led_msg_decode (u8 *led_msg)
{
    u8   *payload_p;
    u8   *cmd_p;
    u8   color;
    u8   *string_p;
    u8   c;
    u8   display_str_size;
    u8   led_bar_id;
    led_char_t *led_char_tbl;
    bool led_state;
    bool led_ctrl;
    bool invalid_char;

    color = led_get_color(led_msg);
    payload_p = led_msg;
    cmd_p = strstr(payload_p, "display");

    if (cmd_p) {
        string_p = cmd_p + strlen("display ");
        cmd_p = strstr(string_p, "dim");
        if (cmd_p) {
            string_p = cmd_p + strlen("dim");
            c = *string_p;
            if (c >= '1' && c <= '3') {
                led_bar_id = c - '0';
            } else {
                led_bar_id = 1;
            }
            string_p = cmd_p + strlen("dimx ");
            c = *string_p;
            if (c >= '0' && c <= '9') {
                led_char_tbl = led_num_tbl;
                c = c - '0';
            } else {
                c = 9;
            }
            printf("dim%bu = %bu", led_bar_id, c);
            led_dim(led_bar_id, c);
            return;
        } 
        cmd_p = strstr(string_p, "color");
        if (cmd_p) {
            display_str_size = cmd_p - string_p;
        } else {
            display_str_size = strlen(string_p);        
        }
        printf("\ncolor = %bu\n", color);
        while (display_str_size--) {
            c = *string_p;
            invalid_char = FALSE;
            printf("%c", c);
            if (c >= '0' && c <= '9') {
                led_char_tbl = led_num_tbl;
                c = c - '0';
            } else {
                if (c >= 'a' && c <= 'z') {
                    c = c + 'A' - 'a';
                }
                if (c >= 'A' && c <= 'Z') {
                    c = c - 'A';
                    led_char_tbl = led_letters_tbl;
                } else {
                    invalid_char = TRUE;
                }
            } 
            if (FALSE == invalid_char) {
                led_char_display_color(led_char_tbl, c, color);
            }
            string_p++;
        }
    } else {
        cmd_p = strstr(payload_p, "led on");
        if (cmd_p) {
            printf("led on color = %bu", color);
            led_state = TRUE;
            led_ctrl = TRUE;
        } else {
            cmd_p = strstr(payload_p, "led off");
            if (cmd_p) {
                printf("led off");
                led_state = FALSE;
                led_ctrl = TRUE;
            }
        }
        if (TRUE == led_ctrl) {
            printf("\n");
            led_control(led_state, color, FALSE, 10);
        }
    }
}
