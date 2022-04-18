#include <homekit/homekit.h>
#include <homekit/characteristics.h>

 
homekit_characteristic_t chaCurrentTemperature 
    = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0, .min_value= (float[]){-40}, .max_value=(float[]){125});
homekit_characteristic_t chaHumidity  = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);

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
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
        .category = homekit_accessory_category_sensor,
		.accessories = accessories,
};


