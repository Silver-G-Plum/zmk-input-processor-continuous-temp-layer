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


static int continuous_temp_layer_handle_event(
    const struct device *dev,
    struct input_event *event,
    uint32_t param1,
    uint32_t param2,
    struct zmk_input_processor_state *state
) {
    struct continuous_temp_layer_data *data =
        dev->data;

    /*
     * REL X / REL Y を受け取ったかだけを見る。
     *
     * 連続時間判定などは一切しない。
     */
    if (
        event->type == INPUT_EV_REL &&
        (
            event->code == INPUT_REL_X ||
            event->code == INPUT_REL_Y
        )
    ) {
        /*
         * input-processor の引数で指定された
         * Layer 13 を記録する。
         */
        data->layer = param1;

        /*
         * 最初のREL X/Yイベントを受け取った時点で
         * Layer 13を有効化する。
         */
        if (!data->active) {
            zmk_keymap_layer_activate(
                data->layer,
                false
            );

            data->active = true;
        }
    }

    /*
     * 必ず次のinput processorへイベントを渡す。
     * これによってカーソル操作自体は止めない。
     */
    return ZMK_INPUT_PROC_CONTINUE;
}


static int continuous_temp_layer_init(
    const struct device *dev
) {
    struct continuous_temp_layer_data *data =
        dev->data;

    data->active = false;
    data->layer = 0;

    return 0;
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
