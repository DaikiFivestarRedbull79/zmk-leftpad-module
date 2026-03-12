/*
 * First-pass analog WASD reader for leftpad
 * Reads two ADC channels and logs / tracks direction state.
 *
 * This is a starter implementation and may need one round of adjustment
 * depending on the exact XIAO ADC node naming in the active ZMK/Zephyr version.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(leftpad_analog, LOG_LEVEL_INF);

#if IS_ENABLED(CONFIG_LEFTPAD_ANALOG_WASD)

#define STACK_SIZE 1024
#define THREAD_PRIORITY 7

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

static void leftpad_analog_thread(void *, void *, void *) {
    LOG_INF("LEFTPAD ANALOG THREAD STARTED");
    int err;

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

        err = adc_read_dt(&x_spec, &x_seq);
        if (err == 0) {
            x_mv = x_buf;
            (void)adc_raw_to_millivolts_dt(&x_spec, &x_mv);
        }

        err = adc_read_dt(&y_spec, &y_seq);
        if (err == 0) {
            y_mv = y_buf;
            (void)adc_raw_to_millivolts_dt(&y_spec, &y_mv);
        }

        LOG_INF("RAW X=%d RAW Y=%d | X=%d mV Y=%d mV", x_buf, y_buf, x_mv, y_mv);

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
