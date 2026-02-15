#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/oled_display.h"
#include "application.h"
#include "button.h"
#include "led/single_led.h"
// #include "iot/thing_manager.h"
#include "settings.h"
#include "config.h"
#include "power_save_timer.h"
// #include "font_awesome_symbols.h"
#include "mcp_server.h"
#include "press_to_talk_mcp_tool.h"


#include <wifi_station.h>
#include <esp_log.h>
#include <esp_efuse_table.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>

#include "display/lcd_display.h"
#include "assets/lang_config.h"
#include "system_reset.h"
#include <esp_lcd_panel_io.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <esp_io_expander_tca9554.h>
#include <driver/spi_common.h>
#include "i2c_device.h"
#include <esp_timer.h>
#include <esp_lcd_gc9a01.h>
#include "esp_lcd_gc9d01n.h"

#define TAG "XminiC3Board"




class CustomLcdDisplay : public SpiLcdDisplay {
public:
    CustomLcdDisplay(esp_lcd_panel_io_handle_t io_handle,
                    esp_lcd_panel_handle_t panel_handle,
                    int width,
                    int height,
                    int offset_x,
                    int offset_y,
                    bool mirror_x,
                    bool mirror_y,
                    bool swap_xy)
        : SpiLcdDisplay(io_handle, panel_handle, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy){

        DisplayLockGuard lock(this);
        // 由于屏幕是圆的，所以状态栏需要增加左右内边距
        lv_obj_set_style_pad_left(status_bar_, LV_HOR_RES * 0.33, 0);
        lv_obj_set_style_pad_right(status_bar_, LV_HOR_RES * 0.33, 0);
    }
};


class Gc9D01LcdDisplay : public SpiLcdDisplay {
public:
    Gc9D01LcdDisplay(esp_lcd_panel_io_handle_t io_handle,
                    esp_lcd_panel_handle_t panel_handle,
                    int width,
                    int height,
                    int offset_x,
                    int offset_y,
                    bool mirror_x,
                    bool mirror_y,
                    bool swap_xy)
        : SpiLcdDisplay(io_handle, panel_handle, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy) {

        DisplayLockGuard lock(this);
        // 由于屏幕是圆的，所以状态栏需要增加左右内边距
        lv_obj_set_style_pad_left(status_bar_, LV_HOR_RES * 0.33, 0);
        lv_obj_set_style_pad_right(status_bar_, LV_HOR_RES * 0.33, 0);
    }
};




static const gc9a01_lcd_init_cmd_t gc9107_lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0xfe, (uint8_t[]){0x00}, 0, 0},
    {0xef, (uint8_t[]){0x00}, 0, 0},
    {0xb0, (uint8_t[]){0xc0}, 1, 0},
    {0xb1, (uint8_t[]){0x80}, 1, 0},
    {0xb2, (uint8_t[]){0x27}, 1, 0},
    {0xb3, (uint8_t[]){0x13}, 1, 0},
    {0xb6, (uint8_t[]){0x19}, 1, 0},
    {0xb7, (uint8_t[]){0x05}, 1, 0},
    {0xac, (uint8_t[]){0xc8}, 1, 0},
    {0xab, (uint8_t[]){0x0f}, 1, 0},
    {0x3a, (uint8_t[]){0x05}, 1, 0},
    {0xb4, (uint8_t[]){0x04}, 1, 0},
    {0xa8, (uint8_t[]){0x08}, 1, 0},
    {0xb8, (uint8_t[]){0x08}, 1, 0},
    {0xea, (uint8_t[]){0x02}, 1, 0},
    {0xe8, (uint8_t[]){0x2A}, 1, 0},
    {0xe9, (uint8_t[]){0x47}, 1, 0},
    {0xe7, (uint8_t[]){0x5f}, 1, 0},
    {0xc6, (uint8_t[]){0x21}, 1, 0},
    {0xc7, (uint8_t[]){0x15}, 1, 0},
    {0xf0,
    (uint8_t[]){0x1D, 0x38, 0x09, 0x4D, 0x92, 0x2F, 0x35, 0x52, 0x1E, 0x0C,
                0x04, 0x12, 0x14, 0x1f},
    14, 0},
    {0xf1,
    (uint8_t[]){0x16, 0x40, 0x1C, 0x54, 0xA9, 0x2D, 0x2E, 0x56, 0x10, 0x0D,
                0x0C, 0x1A, 0x14, 0x1E},
    14, 0},
    {0xf4, (uint8_t[]){0x00, 0x00, 0xFF}, 3, 0},
    {0xba, (uint8_t[]){0xFF, 0xFF}, 2, 0},
};

class XminiC3Board : public WifiBoard
{
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display *display_ = nullptr;
    Button boot_button_;
    bool press_to_talk_enabled_ = false;
    PowerSaveTimer *power_save_timer_;
    PressToTalkMcpTool* press_to_talk_tool_ = nullptr;
    // GC9107Display* display_;

    // esp_lcd_panel_io_handle_t panel_io = nullptr;
    // esp_lcd_panel_handle_t panel = nullptr;

    void InitializePowerSaveTimer()
    {
        power_save_timer_ = new PowerSaveTimer(160, 60);
        power_save_timer_->OnEnterSleepMode([this]()
                                            {
            ESP_LOGI(TAG, "Enabling sleep mode");
            auto display = GetDisplay();
            display->SetChatMessage("system", "");
            display->SetEmotion("sleepy");
            
            auto codec = GetAudioCodec();
            codec->EnableInput(false); });
        power_save_timer_->OnExitSleepMode([this]()
                                           {
            auto codec = GetAudioCodec();
            codec->EnableInput(true);
            
            auto display = GetDisplay();
            display->SetChatMessage("system", "");
            display->SetEmotion("neutral"); });
        power_save_timer_->SetEnabled(true);
    }

    void InitializeCodecI2c()
    { // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
    }


   

    // //0.85
    // void InitializeSpi() {
    //     spi_bus_config_t buscfg = {};
    //     buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
    //     buscfg.miso_io_num = GPIO_NUM_NC;
    //     buscfg.sclk_io_num = DISPLAY_SPI_SCLK_PIN;
    //     buscfg.quadwp_io_num = GPIO_NUM_NC;
    //     buscfg.quadhd_io_num = GPIO_NUM_NC;
    //     buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
    //     ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // }

    // void InitializeGc9107Display(){
    //     esp_lcd_panel_io_handle_t panel_io = nullptr;
    //     esp_lcd_panel_handle_t panel = nullptr;
    //     // 液晶屏控制IO初始化
    //     ESP_LOGD(TAG, "Install panel IO");
    //     esp_lcd_panel_io_spi_config_t io_config = {};
    //     io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
    //     io_config.dc_gpio_num = DISPLAY_SPI_DC_PIN;
    //     io_config.spi_mode = 0;
    //     io_config.pclk_hz = 40 * 1000 * 1000;
    //     io_config.trans_queue_depth = 10;
    //     io_config.lcd_cmd_bits = 8;
    //     io_config.lcd_param_bits = 8;
    //     ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

    //     // 初始化液晶屏驱动芯片GC9107
    //     ESP_LOGD(TAG, "Install LCD driver");        
    //     gc9a01_vendor_config_t gc9107_vendor_config = {
    //         .init_cmds = gc9107_lcd_init_cmds,
    //         .init_cmds_size = sizeof(gc9107_lcd_init_cmds) / sizeof(gc9a01_lcd_init_cmd_t),
    //     };
    //     esp_lcd_panel_dev_config_t panel_config = {};
    //     panel_config.reset_gpio_num = DISPLAY_SPI_RESET_PIN;
    //     panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    //     panel_config.bits_per_pixel = 16;
    //     panel_config.vendor_config = &gc9107_vendor_config;

    //     esp_lcd_new_panel_gc9a01(panel_io, &panel_config, &panel);

    //     esp_lcd_panel_reset(panel);

    //     esp_lcd_panel_init(panel);
    //     esp_lcd_panel_invert_color(panel, false);
    //     esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
    //     esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
    //     ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    //     display_ = new GC9107Display(panel_io, panel,
    //                                 DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    // }

    // 0.91,0.96
    // void InitializeSsd1306Display() {
    //     // SSD1306 config
    //     esp_lcd_panel_io_i2c_config_t io_config = {
    //         .dev_addr = 0x3C,
    //         .on_color_trans_done = nullptr,
    //         .user_ctx = nullptr,
    //         .control_phase_bytes = 1,
    //         .dc_bit_offset = 6,
    //         .lcd_cmd_bits = 8,
    //         .lcd_param_bits = 8,
    //         .flags = {
    //             .dc_low_on_data = 0,
    //             .disable_control_phase = 0,
    //         },
    //         .scl_speed_hz = 400 * 1000,
    //     };
    //     ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c_v2(codec_i2c_bus_, &io_config, &panel_io_));
    //     ESP_LOGI(TAG, "Install SSD1306 driver");
    //     esp_lcd_panel_dev_config_t panel_config = {};
    //     panel_config.reset_gpio_num = -1;
    //     panel_config.bits_per_pixel = 1;
    //     esp_lcd_panel_ssd1306_config_t ssd1306_config = {
    //         .height = static_cast<uint8_t>(DISPLAY_HEIGHT),
    //     };
    //     panel_config.vendor_config = &ssd1306_config;
    //     ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(panel_io_, &panel_config, &panel_));
    //     ESP_LOGI(TAG, "SSD1306 driver installed");
    //     // Reset the display
    //     ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
    //     if (esp_lcd_panel_init(panel_) != ESP_OK) {
    //         ESP_LOGE(TAG, "Failed to initialize display");
    //         display_ = new NoDisplay();
    //         return;
    //     }
    //     // Set the display to on
    //     ESP_LOGI(TAG, "Turning display on");
    //     ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));
    //     display_ = new OledDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
    //         {&font_puhui_14_1, &font_awesome_14_1});
    // }

    // // 1.54
     void InitializeSpi()
    {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // ST7789初始化
    void InitializeSt7789Display()
    {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;
        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_SPI_DC_PIN;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));
        // 初始化液晶屏驱动芯片ST7789
        ESP_LOGD(TAG, "Install LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_SPI_RESET_PIN;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        // EnableLcdCs();
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
        display_ = new SpiLcdDisplay(panel_io, panel,
                                     DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    // // 1.28
    // // SPI初始化
    // void InitializeSpi()
    // {
    //     ESP_LOGI(TAG, "Initialize SPI bus");
    //     spi_bus_config_t buscfg = GC9A01_PANEL_BUS_SPI_CONFIG(DISPLAY_SPI_SCLK_PIN, DISPLAY_SPI_MOSI_PIN,
    //                                                           DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));
    //     ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // }

    // // // GC9A01初始化
    // void InitializeGc9a01Display()
    // {
    //     ESP_LOGI(TAG, "Init GC9A01 display");

    //     ESP_LOGI(TAG, "Install panel IO");
    //     esp_lcd_panel_io_handle_t io_handle = NULL;
    //     esp_lcd_panel_io_spi_config_t io_config = GC9A01_PANEL_IO_SPI_CONFIG(DISPLAY_SPI_CS_PIN, DISPLAY_SPI_DC_PIN, NULL, NULL);
    //     io_config.pclk_hz = DISPLAY_SPI_SCLK_HZ;
    //     ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));

    //     ESP_LOGI(TAG, "Install GC9A01 panel driver");
    //     esp_lcd_panel_handle_t panel_handle = NULL;
    //     esp_lcd_panel_dev_config_t panel_config = {};
    //     panel_config.reset_gpio_num = DISPLAY_SPI_RESET_PIN; // Set to -1 if not use
    //     panel_config.rgb_endian = LCD_RGB_ENDIAN_BGR;        // LCD_RGB_ENDIAN_RGB;
    //     panel_config.bits_per_pixel = 16;                    // Implemented by LCD command `3Ah` (16/18)

    //     ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
    //     ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    //     ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    //     ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    //     ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    //     ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    //     uint8_t data_0x62[] = { 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70 };
    //     esp_lcd_panel_io_tx_param(io_handle, 0x62, data_0x62, sizeof(data_0x62));

    //     uint8_t data_0x63[] = { 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70 };
    //     esp_lcd_panel_io_tx_param(io_handle, 0x63, data_0x63, sizeof(data_0x63));

    //    display_ = new CustomLcdDisplay(io_handle, panel_handle,
    //                                 DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    // }

    // 0.71
    // void InitializeSpi(){
    //     spi_bus_config_t buscfg = {};
    //     buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
    //     buscfg.miso_io_num = GPIO_NUM_NC;
    //     buscfg.sclk_io_num = DISPLAY_SPI_SCLK_PIN;
    //     buscfg.quadwp_io_num = GPIO_NUM_NC;
    //     buscfg.quadhd_io_num = GPIO_NUM_NC;
    //     buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
    //     ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // }

    // void InitGc9d01nDisplay()
    // {
    //     ESP_LOGI(TAG, "Init GC9D01N");
    //     esp_lcd_panel_io_handle_t panel_io = nullptr;
    //     esp_lcd_panel_handle_t panel = nullptr;
    //     ESP_LOGD(TAG, "Install panel IO");
    //     esp_lcd_panel_io_spi_config_t io_config = {};
    //     io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
    //     io_config.dc_gpio_num = DISPLAY_SPI_DC_PIN;
    //     io_config.spi_mode = 0;
    //     io_config.pclk_hz = 40 * 1000 * 1000;
    //     io_config.trans_queue_depth = 10;
    //     io_config.lcd_cmd_bits = 8;
    //     io_config.lcd_param_bits = 8;
    //     ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));
    //     ESP_LOGD(TAG, "Install LCD driver");
    //     esp_lcd_panel_dev_config_t panel_config = {};
    //     panel_config.reset_gpio_num = DISPLAY_SPI_RESET_PIN;
    //     panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    //     panel_config.bits_per_pixel = 16;
    //     ESP_ERROR_CHECK(esp_lcd_new_panel_gc9d01n(panel_io, &panel_config, &panel));
    //     esp_lcd_panel_reset(panel);
    //     esp_lcd_panel_init(panel);
    //     esp_lcd_panel_invert_color(panel, false);
    //     esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
    //     esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

    //     display_ = new Gc9D01LcdDisplay(panel_io, panel,
    //                                 DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);

    // }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            if (!press_to_talk_tool_ || !press_to_talk_tool_->IsPressToTalkEnabled()) {
                app.ToggleChatState();
            }
        });
        boot_button_.OnPressDown([this]() {
            if (power_save_timer_) {
                power_save_timer_->WakeUp();
            }
            if (press_to_talk_tool_ && press_to_talk_tool_->IsPressToTalkEnabled()) {
                Application::GetInstance().StartListening();
            }
        });
        boot_button_.OnPressUp([this]() {
            if (press_to_talk_tool_ && press_to_talk_tool_->IsPressToTalkEnabled()) {
                Application::GetInstance().StopListening();
            }
        });
    }

    // 物联网初始化，添加对 AI 可见设备
    void InitializeTools() {
        press_to_talk_tool_ = new PressToTalkMcpTool();
        press_to_talk_tool_->Initialize();
    }

public:
    XminiC3Board() : boot_button_(BOOT_BUTTON_GPIO)
    {
        

        InitializeCodecI2c();
        // InitializeSsd1306Display();

        InitializeSpi();
        InitializeSt7789Display();
        // InitializeGc9a01Display();
        // InitGc9d01nDisplay();
        // InitializeGc9107Display();
        InitializeButtons();
        InitializePowerSaveTimer();
        InitializeTools();
        // 把 ESP32C3 的 VDD SPI 引脚作为普通 GPIO 口使用
        esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);
        // InitializeIot();
    }

    virtual Led *GetLed() override
    {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual Display *GetDisplay() override
    {
        return display_;
    }

    virtual AudioCodec *GetAudioCodec() override
    {
        static Es8311AudioCodec audio_codec(codec_i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
                                            AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual void SetPowerSaveMode(bool enabled) override {
        if (!enabled) {
            power_save_timer_->WakeUp();
        }
        WifiBoard::SetPowerSaveMode(enabled);
    }
};

DECLARE_BOARD(XminiC3Board);


