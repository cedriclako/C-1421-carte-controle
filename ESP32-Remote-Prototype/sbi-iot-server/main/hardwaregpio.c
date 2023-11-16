#include "hardwaregpio.h"
#include "fwconfig.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"

#define TAG "hardwaregpio"

static void InitStatusLEDsGPIO();
static void InitModelBitsGPIO();
static void InitBuzzerGPIO();
static void InitSDCardGPIO();
static void InitSTM32GPIO();

static void InitSDCardDriver();

static bool m_bIsSDCardAvailable = false;

static bool m_bIsUARTReady = false;

void HARDWAREGPIO_Init()
{
    // Init GPIOs
    InitStatusLEDsGPIO();
    InitModelBitsGPIO();
    InitBuzzerGPIO();
    InitSDCardGPIO();
    InitSTM32GPIO();

    InitSDCardDriver();
}

static void InitStatusLEDsGPIO()
{
    // Zero-initialize the config structure.
    gpio_config_t io_conf = {0};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ((1ULL<<HWGPIO_STATUSLED0_PIN) | (1ULL<<HWGPIO_STATUSLED1_PIN) | (1ULL<<HWGPIO_SANITYLED_PIN));
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    // Set level
    gpio_set_level(HWGPIO_STATUSLED0_PIN, false);
    gpio_set_level(HWGPIO_STATUSLED1_PIN, false);
    gpio_set_level(HWGPIO_SANITYLED_PIN, false);
}

static void InitModelBitsGPIO()
{
    // Zero-initialize the config structure.
    gpio_config_t io_conf = {0};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ((1ULL<<HWGPIO_MODEL_BIT0_PIN) | (1ULL<<HWGPIO_MODEL_BIT1_PIN) | (1ULL<<HWGPIO_MODEL_BIT2_PIN));
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

static void InitBuzzerGPIO()
{
    // Zero-initialize the config structure.
    gpio_config_t io_conf = {0};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL<<HWGPIO_BUZZER_ON_PIN);
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    
    gpio_set_level(HWGPIO_BUZZER_ON_PIN, false);
}

static void InitSDCardGPIO()
{
    // Zero-initialize the config structure.
    gpio_config_t io_conf = {0};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ((1ULL<<HWGPIO_SDCARD_CS_PIN) | (1ULL<<HWGPIO_SDCARD_MOSI_PIN) | (1ULL<<HWGPIO_SDCARD_MISO_PIN) | (1ULL<<HWGPIO_SDCARD_SCLK_PIN) | (1ULL<<HWGPIO_SDCARD_DETECT_PIN));
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

static void InitSTM32GPIO()
{
    // Zero-initialize the config structure.
    gpio_config_t io_conf = {0};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_DISABLE;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ((1ULL<<HWGPIO_STM32_BOOT1_PIN) | (1ULL<<HWGPIO_STM32_RESET_PIN) | (1ULL<<HWGPIO_STM32_BOOT0_PIN));
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

static void InitSDCardDriver()
{    
    #if FWCONFIG_SDCARD_ISACTIVE != 0
    // Mount SD Card
    const spi_bus_config_t buscfgSPI2={
        .miso_io_num=HWGPIO_SDCARD_MISO_PIN,
        .mosi_io_num=HWGPIO_SDCARD_MOSI_PIN,
        .sclk_io_num=HWGPIO_SDCARD_SCLK_PIN,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=0
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    //host.max_freq_khz = SDMMC_FREQ_PROBING;
    host.slot = HWGPIO_SDCARD_SPI_HOST;

    const esp_err_t ret2=spi_bus_initialize(host.slot, &buscfgSPI2, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret2);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = HWGPIO_SDCARD_CS_PIN;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    const char mount_point[] = FWCONFIG_SDCARD_ROOTPATH;
    const esp_err_t ret3 = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret3 != ESP_OK)
    {
        if (ret3 == ESP_FAIL) 
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret3));
        }
        return;
    }

    ESP_LOGI(TAG, "Filesystem mounted");
    sdmmc_card_print_info(stdout, card);
    m_bIsSDCardAvailable = true;
    #endif
}

bool HARDWAREGPIO_InitUARTDriver()
{
    if (uart_is_driver_installed(HWGPIO_BRIDGEUART_PORT_NUM)) {
        ESP_LOGE(TAG, "The driver UART driver is already running");
        return false;
    }

     /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    const uart_config_t uart_config = {
        .baud_rate = HWGPIO_BRIDGEUART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Init UARTs
    int intr_alloc_flags = 0;
     
    // Configure a temporary buffer for the incoming data
    ESP_ERROR_CHECK(uart_param_config(HWGPIO_BRIDGEUART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(HWGPIO_BRIDGEUART_PORT_NUM, HWGPIO_BRIDGEUART_TXD_PIN, HWGPIO_BRIDGEUART_RXD_PIN, HWGPIO_BRIDGEUART_RTS_PIN, HWGPIO_BRIDGEUART_CTS_PIN));
    ESP_ERROR_CHECK(uart_driver_install(HWGPIO_BRIDGEUART_PORT_NUM, HWGPIO_BRIDGEUART_BUFFSIZE * 2, 0, 0, NULL, intr_alloc_flags));
    m_bIsUARTReady = true;
    return true;
}

bool HARDWAREGPIO_DeinitUARTDriver()
{
    if (!uart_is_driver_installed(HWGPIO_BRIDGEUART_PORT_NUM)) {
        ESP_LOGW(TAG, "The driver UART driver is not installed");
        return false;
    }
    ESP_ERROR_CHECK(uart_driver_delete(HWGPIO_BRIDGEUART_PORT_NUM));
    return true;
}

void HARDWAREGPIO_SetSanity(bool bIsActive)
{
    gpio_set_level(HWGPIO_SANITYLED_PIN, !bIsActive);
}

void HARDWAREGPIO_SetStatusLED(HARDWAREGPIO_ESTATUSLED eStatusLED, bool bIsActive)
{
    if (eStatusLED == HARDWAREGPIO_ESTATUSLED_0)
        gpio_set_level(HWGPIO_STATUSLED0_PIN, !bIsActive);
    else if (eStatusLED == HARDWAREGPIO_ESTATUSLED_1)
        gpio_set_level(HWGPIO_STATUSLED1_PIN, !bIsActive);
    else if (eStatusLED == HARDWAREGPIO_ESTATUSLED_2)
        gpio_set_level(HWGPIO_STATUSLED2_PIN, !bIsActive);
}

bool HARDWAREGPIO_GetISSDCardAvailable() { return m_bIsSDCardAvailable; }

uint32_t HARDWAREGPIO_GetModel()
{
    return ((uint32_t)gpio_get_level(HWGPIO_MODEL_BIT2_PIN) << 2) | ((uint32_t)gpio_get_level(HWGPIO_MODEL_BIT1_PIN) << 1) | (uint32_t)gpio_get_level(HWGPIO_MODEL_BIT0_PIN);
}
