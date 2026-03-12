#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(leftpad_analog, LOG_LEVEL_INF);

#define DEAD_LOW 1000
#define DEAD_HIGH 3000

static const struct device *adc_dev;

static int16_t x_buf;
static int16_t y_buf;

static struct adc_channel_cfg x_channel_cfg = {
    .gain = ADC_GAIN_1_4,
    .reference = ADC_REF_VDD_1_4,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = 0,
    .input_positive = NRF_SAADC_AIN0,
};

static struct adc_channel_cfg y_channel_cfg = {
    .gain = ADC_GAIN_1_4,
    .reference = ADC_REF_VDD_1_4,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = 1,
    .input_positive = NRF_SAADC_AIN1,
};

static struct adc_sequence x_seq = {
    .channels = BIT(0),
    .buffer = &x_buf,
    .buffer_size = sizeof(x_buf),
    .resolution = 12,
};

static struct adc_sequence y_seq = {
    .channels = BIT(1),
    .buffer = &y_buf,
    .buffer_size = sizeof(y_buf),
    .resolution = 12,
};

static bool left_pressed = false;
static bool right_pressed = false;
static bool up_pressed = false;
static bool down_pressed = false;

void analog_thread(void)
{
    adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC not ready");
        return;
    }

    adc_channel_setup(adc_dev, &x_channel_cfg);
    adc_channel_setup(adc_dev, &y_channel_cfg);

    LOG_INF("LEFTPAD ANALOG THREAD STARTED");

    while (1) {

        adc_read(adc_dev, &x_seq);
        adc_read(adc_dev, &y_seq);

        int x = x_buf;
        int y = y_buf;

        LOG_INF("RAW X=%d Y=%d", x, y);

        bool new_left  = x < DEAD_LOW;
        bool new_right = x > DEAD_HIGH;
        bool new_down  = y < DEAD_LOW;
        bool new_up    = y > DEAD_HIGH;

        if (new_left != left_pressed) {
            left_pressed = new_left;
            LOG_INF("LEFT %s", left_pressed ? "ON" : "OFF");
        }

        if (new_right != right_pressed) {
            right_pressed = new_right;
            LOG_INF("RIGHT %s", right_pressed ? "ON" : "OFF");
        }

        if (new_up != up_pressed) {
            up_pressed = new_up;
            LOG_INF("UP %s", up_pressed ? "ON" : "OFF");
        }

        if (new_down != down_pressed) {
            down_pressed = new_down;
            LOG_INF("DOWN %s", down_pressed ? "ON" : "OFF");
        }

        k_msleep(20);
    }
}
