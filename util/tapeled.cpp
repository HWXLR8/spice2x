#include "tapeled.h"

namespace tapeledutils {

    led_tape_color_pick_algorithm TAPE_LED_ALGORITHM = TAPE_LED_USE_MIDDLE;

    bool is_enabled() {
        return (TAPE_LED_ALGORITHM != TAPE_LED_USE_NONE);
    }

    rgb_float3_t pick_color_from_led_tape(uint8_t *data, size_t data_size) {
        rgb_float3_t result = {0.f, 0.f, 0.f};
        if (TAPE_LED_ALGORITHM == TAPE_LED_USE_AVERAGE) {

            // calculate average color
            size_t avg_ri = 0;
            size_t avg_gi = 0;
            size_t avg_bi = 0;
            for (size_t i = 0; i < data_size; i++) {
                const auto color = &data[i * 3];
                avg_ri += color[0];
                avg_gi += color[1];
                avg_bi += color[2];
            }

            // normalize
            const float avg_mult = 1.f / (data_size * 255);
            result.r = avg_ri * avg_mult;
            result.g = avg_gi * avg_mult;
            result.b = avg_bi * avg_mult;

        } else if (TAPE_LED_ALGORITHM == TAPE_LED_USE_FIRST ||
            TAPE_LED_ALGORITHM == TAPE_LED_USE_MIDDLE ||
            TAPE_LED_ALGORITHM == TAPE_LED_USE_LAST ) {

            // pick one LED
            const uint8_t *color;
            switch (TAPE_LED_ALGORITHM) {
                case TAPE_LED_USE_FIRST:
                    color = &data[0];
                    break;
                case TAPE_LED_USE_LAST:
                    color = &data[(data_size - 1) * 3];
                    break;
                case TAPE_LED_USE_MIDDLE:
                default:
                    color = &data[(data_size / 2) * 3];
                    break;
            }

            // normalize
            const float single_mult = 1.f / 255;
            result.r = color[0] * single_mult;
            result.g = color[1] * single_mult;
            result.b = color[2] * single_mult;
        }
        return result;
    }
}
