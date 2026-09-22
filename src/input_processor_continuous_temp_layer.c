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
    int64_t first_event_time;
    int64_t last_event_time;

    bool active;
    uint8_t layer;

    struct k_work_delayable disable_work;
};

static void disable_layer_work(struct k_work *work) {
    struct k_work_delayable *dwork =
        k_work_delayable_from_work(work);

    struct continuous_temp_layer_data *data =
        CONTAINER_OF(
            dwork,
            struct continuous_temp_layer_data,
            disable_work
        );

    if (data->active) {
        zmk_keymap_layer_deactivate(
            data->layer,
            false
        );

        data->active = false;

        data->first_event_time = 0;
        data->last_event_time = 0;
    }
}

static int continuous_temp_layer_handle_event(
    const struct device *dev,
    struct input_event *event,
    uint32_t param1,
    uint32_t param2,
    struct zmk_input_processor_state *state
) {
    const struct continuous_temp_layer_config *config =
        dev->config;

    struct continuous_temp_layer_data *data =
        dev->data;

    /*
     * Only watch relative X/Y movement.
     *
     * Other events are simply passed through.
     */
    if (
        event->type != INPUT_EV_REL ||
        (
            event->code != INPUT_REL_X &&
            event->code != INPUT_REL_Y
        )
    ) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    int64_t now = k_uptime_get();

    /*
     * Remember the target layer and timeout.
     */
    data->layer = param1;

    /*
     * If AML is already active, every movement refreshes
     * the normal temporary-layer timeout.
     */
    if (data->active) {

        if (param2 > 0) {
            k_work_reschedule(
                &data->disable_work,
                K_MSEC(param2)
            );
        }

        data->last_event_time = now;

        /*
         * IMPORTANT:
         * Always allow the actual trackball movement
         * to continue to the next input processor.
         */
        return ZMK_INPUT_PROC_CONTINUE;
    }

    /*
     * First movement after being idle.
     */
    if (data->last_event_time == 0) {
        data->first_event_time = now;
        data->last_event_time = now;

        /*
         * Do NOT activate AML yet.
         *
         * But DO allow the cursor movement through.
         */
        return ZMK_INPUT_PROC_CONTINUE;
    }

    /*
     * How long since the previous movement event?
     */
    int64_t gap =
        now - data->last_event_time;

    /*
     * If the gap is too large, the previous movement sequence
     * has ended. Start counting again.
     */
    if (gap > config->max_gap_ms) {

        data->first_event_time = now;
        data->last_event_time = now;

        return ZMK_INPUT_PROC_CONTINUE;
    }

    /*
     * Input is continuous.
     */
    data->last_event_time = now;

    /*
     * Has continuous movement lasted long enough?
     */
    if (
        now - data->first_event_time
        >= config->continuous_ms
    ) {

        /*
         * Activate the AML layer.
         */
        zmk_keymap_layer_activate(
            data->layer,
            false
        );

        data->active = true;

        /*
         * Start the normal temporary-layer timeout.
         */
        if (param2 > 0) {
            k_work_reschedule(
                &data->disable_work,
                K_MSEC(param2)
            );
        }
    }

    /*
     * Always pass the movement through.
     *
     * This is the important difference from a normal
     * filtering processor: the cursor never stops.
     */
    return ZMK_INPUT_PROC_CONTINUE;
}

static int continuous_temp_layer_init(
    const struct device *dev
) {
    struct continuous_temp_layer_data *data =
        dev->data;

    data->first_event_time = 0;
    data->last_event_time = 0;
    data->active = false;
    data->layer = 0;

    k_work_init_delayable(
        &data->disable_work,
        disable_layer_work
    );

    return 0;
}

static const struct zmk_input_processor_driver_api
continuous_temp_layer_api = {
    .handle_event =
        continuous_temp_layer_handle_event,
};

#define CONTINUOUS_TEMP_LAYER_INIT(n)                         \
                                                               \
    static const struct continuous_temp_layer_config          \
        continuous_temp_layer_config_##n = {                  \
            .continuous_ms =                                   \
                DT_INST_PROP(n, continuous_ms),               \
            .max_gap_ms =                                      \
                DT_INST_PROP_OR(n, max_gap_ms, 20),            \
        };                                                      \
                                                               \
    static struct continuous_temp_layer_data                   \
        continuous_temp_layer_data_##n;                       \
                                                               \
    DEVICE_DT_INST_DEFINE(                                     \
        n,                                                     \
        continuous_temp_layer_init,                            \
        NULL,                                                  \
        &continuous_temp_layer_data_##n,                       \
        &continuous_temp_layer_config_##n,                     \
        POST_KERNEL,                                           \
        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
        &continuous_temp_layer_api                             \
    );

DT_INST_FOREACH_STATUS_OKAY(
    CONTINUOUS_TEMP_LAYER_INIT
)
