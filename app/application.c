#include <application.h>

#define TEMPERATURE_PUB_NO_CHANGE_INTEVAL (15 * 60 * 1000)
#define TEMPERATURE_PUB_VALUE_CHANGE 0.2f
#define TEMPERATURE_UPDATE_INTERVAL (1 * 1000)

// LED instance
bc_led_t led;
bool led_state = false;

// Button instance
bc_button_t button;

// Temperature instance
bc_tag_temperature_t temperature;
event_param_t temperature_event_param = { .next_pub = 0 };

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_pulse(&led, 100);
    }
}

void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;

    if (event == BC_TAG_TEMPERATURE_EVENT_UPDATE)
    {
        if (bc_tag_temperature_get_temperature_celsius(self, &value))
        {
            if ((fabs(value - param->value) >= TEMPERATURE_PUB_VALUE_CHANGE) || (param->next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_temperature(param->channel, &value);

                param->value = value;
                param->next_pub = bc_scheduler_get_spin_tick() + TEMPERATURE_PUB_NO_CHANGE_INTEVAL;
            }
        }
    }
}

void bc_radio_node_on_state_get(uint64_t *id, uint8_t state_id)
{
    (void) id;

    if (state_id == BC_RADIO_NODE_STATE_POWER_MODULE_RELAY)
    {
        bool state = bc_module_power_relay_get_state();

        bc_radio_pub_state(BC_RADIO_PUB_STATE_POWER_MODULE_RELAY, &state);
    }
    else if (state_id == BC_RADIO_NODE_STATE_LED)
    {
        bc_radio_pub_state(BC_RADIO_PUB_STATE_LED, &led_state);
    }
}

void bc_radio_node_on_state_set(uint64_t *id, uint8_t state_id, bool *state)
{
    (void) id;

    if (state_id == BC_RADIO_NODE_STATE_POWER_MODULE_RELAY)
    {
        bc_module_power_relay_set_state(*state);

        bc_radio_pub_state(BC_RADIO_PUB_STATE_POWER_MODULE_RELAY, state);
    }
    else if (state_id == BC_RADIO_NODE_STATE_LED)
    {
        led_state = *state;

        bc_led_set_mode(&led, led_state ? BC_LED_MODE_ON : BC_LED_MODE_OFF);

        bc_radio_pub_state(BC_RADIO_PUB_STATE_LED, &led_state);
    }
}

void application_init(void)
{
    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    bc_radio_init(BC_RADIO_MODE_NODE_LISTENING);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_scan_interval(&button, 20);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize temperature
    temperature_event_param.channel = BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE;
    bc_tag_temperature_init(&temperature, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);
    bc_tag_temperature_set_update_interval(&temperature, TEMPERATURE_UPDATE_INTERVAL);
    bc_tag_temperature_set_event_handler(&temperature, temperature_tag_event_handler, &temperature_event_param);

    // Initialize power module
    bc_module_power_init();

    bc_radio_pairing_request("kit-controller", VERSION);

    bc_led_pulse(&led, 2000);
}
