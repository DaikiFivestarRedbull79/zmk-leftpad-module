#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/keys.h>

LOG_MODULE_REGISTER(leftpad_analog, LOG_LEVEL_INF);

#if IS_ENABLED(CONFIG_LEFTPAD_ANALOG_WASD)

#define STACK_SIZE 1024
#define THREAD_PRIORITY 7

#define LEFTPAD_LOW_THR   1000
#define LEFTPAD_HIGH_THR  3000

static const struct adc_dt_spec x_spec = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const struct adc_dt_spec y_spec = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);

static int16_t x_buf;
static int16_t y_buf;

static struct adc_sequence x_seq = {
    .buffer = &x_buf,
    .buffer_size = sizeof(x_buf),
    .resolution = 12,
};

static struct adc_sequence y_seq = {
    .buffer = &y_buf,
    .buffer_size = sizeof(y_buf),
    .resolution = 12,
};

static bool left_pressed = false;
static bool right_pressed = false;
static bool up_pressed = false;
static bool down_pressed = false;

static void send_key_state(uint32_t keycode, bool pressed) {
    int err;

    if (pressed) {
        err = zmk_hid_press(keycode);
        if (err < 0) {
            LOG_ERR("zmk_hid_press failed: %d", err);
            return;
        }
    } else {
        err = zmk_hid_release(keycode);
        if (err < 0) {
            LOG_ERR("zmk_hid_release failed: %d", err);
            return;
        }
    }

    LOG_INF("KEY %s %s", pressed ? "DOWN" : "UP",
            keycode == D ? "D" :
            keycode == A ? "A" :
            keycode == W ? "W" : "S");
}

static void leftpad_analog_thread(void *, void *, void *) {
    int err;

    LOG_INF("LEFTPAD ANALOG THREAD STARTED");

    if (!adc_is_ready_dt(&x_spec) || !adc_is_ready_dt(&y_spec)) {
        LOG_ERR("ADC device not ready");
        return;
    }

    LOG_INF("ADC ready: X=%s Y=%s",
            adc_is_ready_dt(&x_spec) ? "ok" : "ng",
            adc_is_ready_dt(&y_spec) ? "ok" : "ng");

    err = adc_channel_setup_dt(&x_spec);
    if (err < 0) {
        LOG_ERR("Failed to setup X ADC channel: %d", err);
        return;
    }

    err = adc_channel_setup_dt(&y_spec);
    if (err < 0) {
        LOG_ERR("Failed to setup Y ADC channel: %d", err);
        return;
    }

    LOG_INF("ADC channels initialized");

    while (1) {
        int x_mv = 0;
        int y_mv = 0;

        adc_sequence_init_dt(&x_spec, &x_seq);
        adc_sequence_init_dt(&y_spec, &y_seq);

        x_seq.resolution = 12;
        y_seq.resolution = 12;

        err = adc_read_dt(&x_spec, &x_seq);
        if (err == 0) {
            x_mv = x_buf;
            (void)adc_raw_to_millivolts_dt(&x_spec, &x_mv);
        } else {
            LOG_ERR("adc_read_dt X failed: %d", err);
        }

        err = adc_read_dt(&y_spec, &y_seq);
        if (err == 0) {
            y_mv = y_buf;
            (void)adc_raw_to_millivolts_dt(&y_spec, &y_mv);
        } else {
            LOG_ERR("adc_read_dt Y failed: %d", err);
        }

        LOG_INF("RAW X=%d RAW Y=%d | X=%d mV Y=%d mV", x_buf, y_buf, x_mv, y_mv);

        bool new_left  = x_buf < LEFTPAD_LOW_THR;
        bool new_right = x_buf > LEFTPAD_HIGH_THR;
        bool new_down  = y_buf < LEFTPAD_LOW_THR;
        bool new_up    = y_buf > LEFTPAD_HIGH_THR;

        if (new_left != left_pressed) {
            left_pressed = new_left;
            LOG_INF("LEFT %s", left_pressed ? "ON" : "OFF");
        }

        if (new_right != right_pressed) {
            right_pressed = new_right;
            LOG_INF("RIGHT %s", right_pressed ? "ON" : "OFF");
            send_key_state(D, right_pressed);
        }

        if (new_up != up_pressed) {
            up_pressed = new_up;
            LOG_INF("UP %s", up_pressed ? "ON" : "OFF");
        }

        if (new_down != down_pressed) {
            down_pressed = new_down;
            LOG_INF("DOWN %s", down_pressed ? "ON" : "OFF");
        }

        k_msleep(CONFIG_LEFTPAD_ANALOG_POLL_MS);
    }
}

K_THREAD_DEFINE(leftpad_analog_tid,
                STACK_SIZE,
                leftpad_analog_thread,
                NULL, NULL, NULL,
                THREAD_PRIORITY,
                0,
                0);

#endif
