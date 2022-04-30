#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#define HOMEKIT_CUSTOM_UUID(value) (value "-03e9-4157-b099-54f4a4944163")

#define HOMEKIT_SERVICE_CUSTOM_WIFI HOMEKIT_CUSTOM_UUID("F0000000")
#define HOMEKIT_SERVICE_CUSTOM_SETUP HOMEKIT_CUSTOM_UUID("F0000001")

#define HOMEKIT_CHARACTERISTIC_CUSTOM_IP_ADDR HOMEKIT_CUSTOM_UUID("00000001")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_IP_ADDR(_value, ...)               \
    .type = HOMEKIT_CHARACTERISTIC_CUSTOM_IP_ADDR,                               \
    .description = "Wifi IP Addr",                                               \
    .format = homekit_format_string,                                             \
    .permissions = homekit_permissions_paired_read | homekit_permissions_notify, \
    .value = HOMEKIT_STRING_(_value),                                            \
    ##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_WIFI_RSSI HOMEKIT_CUSTOM_UUID("00000002")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_WIFI_RSSI(_value, ...)             \
    .type = HOMEKIT_CHARACTERISTIC_CUSTOM_WIFI_RSSI,                             \
    .description = "Wifi RSSI",                                                  \
    .format = homekit_format_int,                                                \
    .permissions = homekit_permissions_paired_read | homekit_permissions_notify, \
    .min_value = (float[]){-100},                                                \
    .max_value = (float[]){0},                                                   \
    .min_step = (float[]){1},                                                    \
    .value = HOMEKIT_INT_(_value),                                               \
    ##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_SENSOR_REFRESH_INTERVAL HOMEKIT_CUSTOM_UUID("00000003")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_SENSOR_REFRESH_INTERVAL(_value, ...)                                  \
    .type = HOMEKIT_CHARACTERISTIC_CUSTOM_SENSOR_REFRESH_INTERVAL,                                                  \
    .description = "Sensor Refresh Interval",                                                                       \
    .format = homekit_format_uint64,                                                                                \
    .unit = homekit_unit_seconds,                                                                                   \
    .permissions = homekit_permissions_paired_read | homekit_permissions_paired_write | homekit_permissions_notify, \
    .min_value = (float[]){0},                                                                                      \
    .max_value = (float[]){604800},                                                                                 \
    .min_step = (float[]){1},                                                                                       \
    .value = HOMEKIT_UINT64_(_value),                                                                               \
    ##__VA_ARGS__

homekit_characteristic_t chaCurrentTemperature 
    = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0, .min_value= (float[]){-40}, .max_value=(float[]){125});
homekit_characteristic_t chaHumidity  = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
homekit_characteristic_t chaWifiIPAddress  = HOMEKIT_CHARACTERISTIC_(CUSTOM_IP_ADDR, "");
homekit_characteristic_t chaWifiRssi  = HOMEKIT_CHARACTERISTIC_(CUSTOM_WIFI_RSSI, 0);
homekit_characteristic_t chaSensorRefershInterval  = HOMEKIT_CHARACTERISTIC_(CUSTOM_SENSOR_REFRESH_INTERVAL, 0);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_sensor, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Sensor"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Wemo Temperature Humidity Sensor"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, NULL),
            NULL
        }),
        HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            &chaCurrentTemperature,
            NULL
        }),
		
        HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            &chaHumidity,
            NULL
        }),

        HOMEKIT_SERVICE(CUSTOM_WIFI, .characteristics=(homekit_characteristic_t*[]) {
            &chaWifiRssi,
            &chaWifiIPAddress,
            NULL
        }),
        HOMEKIT_SERVICE(CUSTOM_SETUP, .characteristics=(homekit_characteristic_t*[]) {
            &chaSensorRefershInterval,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
        .category = homekit_accessory_category_sensor,
		.accessories = accessories,
};