#define DT_DRV_COMPAT zmk_input_processor_continuous_temp_layer

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>

#include <drivers/input_processor.h>

#include <zmk/keymap.h>


struct continuous_temp_layer_config {
    uint32_t continuous_ms;
    uint32_t max_gap_ms;
};


struct continuous_temp_layer_data {
    bool active;
    uint8_t layer;
};


/*
 * ---------------------------------------------------------
 * 初期化テスト
 *
 * processorが実際に生成・初期化されたことを確認するため、
 * 起動時にLayer 13を直接有効化する。
 * ---------------------------------------------------------
 */
static void activate_test_layer(struct k_work *work)
{
    zmk_keymap_layer_activate(13, false);
}


static struct k_work_delayable test_work;


static int continuous_temp_layer_init(
    const struct device *dev
) {
    struct continuous_temp_layer_data *data =
        dev->data;

    data->active = false;
    data->layer = 13;

    /*
     * キーボード起動処理がある程度終わってから
     * Layer 13を有効化する。
     */
    k_work_init_delayable(
        &test_work,
        activate_test_layer
    );

    k_work_reschedule(
        &test_work,
        K_MSEC(1000)
    );

    return 0;
}


static int continuous_temp_layer_handle_event(
    const struct device *dev,
    struct input_event *event,
    uint32_t param1,
    uint32_t param2,
    struct zmk_input_processor_state *state
) {
    /*
     * 今回は入力イベントを一切処理しない。
     *
     * 初期化テストだけを行う。
     */
    return ZMK_INPUT_PROC_CONTINUE;
}


static const struct zmk_input_processor_driver_api
continuous_temp_layer_api = {
    .handle_event =
        continuous_temp_layer_handle_event,
};


#define CONTINUOUS_TEMP_LAYER_INIT(n) \
    static const struct continuous_temp_layer_config \
        continuous_temp_layer_config_##n = { \
            .continuous_ms = DT_INST_PROP(n, continuous_ms), \
            .max_gap_ms = DT_INST_PROP_OR(n, max_gap_ms, 20), \
        }; \
    static struct continuous_temp_layer_data \
        continuous_temp_layer_data_##n; \
    DEVICE_DT_INST_DEFINE( \
        n, \
        continuous_temp_layer_init, \
        NULL, \
        &continuous_temp_layer_data_##n, \
        &continuous_temp_layer_config_##n, \
        POST_KERNEL, \
        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
        &continuous_temp_layer_api \
    );


DT_INST_FOREACH_STATUS_OKAY(
    CONTINUOUS_TEMP_LAYER_INIT
)
