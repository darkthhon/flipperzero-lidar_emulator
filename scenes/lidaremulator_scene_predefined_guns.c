#include "../lidar_emulator_app_i.h"
#include <furi.h>
#include <gui/gui.h>
#include <gui/gui_i.h>
#include <gui/modules/submenu.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <gui/view.h>
#include <gui/viewport.h>
#include "../view_hijacker.h"

#define ARRAY_SIZE(x) ((int)(sizeof(x) / sizeof(*(x))))

enum SubmenuIndex {
    SubmenuIndexPredefinedGUNs,
};

typedef uint32_t GunTiming;

typedef struct {
    char name[40];
    GunTiming timing;
} GunDef;

const GunDef guns[] = {
    { " 67.6 - Zurad Rapid Laser", 14800 },
    { " 68.1 - Laser Atlanta Stealth", 4194 },
    // ... (le reste du tableau identique)
    { "4673.0 - Riegl FG21-P", 214 },
};

static void lidar_emulator_scene_predefined_guns_submenu_callback(void* context, uint32_t index) {
    LidarEmulatorApp* lidar_emulator = context;
    view_dispatcher_send_custom_event(lidar_emulator->view_dispatcher, index);
}

bool lidar_emulator_scene_predefined_guns_view_on_event(InputEvent* event, void* context) {
    bool consumed = false;
    ViewHijacker* view_hijacker = context;
    furi_check(context != NULL);

    LidarEmulatorApp* lidar_emulator = view_hijacker->new_input_callback_context;
    furi_check(lidar_emulator != NULL);

    Submenu* submenu = lidar_emulator->submenu;
    furi_check(submenu != NULL);

    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        consumed = true;
    }

    if(event->type == InputTypePress && event->key == InputKeyOk) {
        static uint32_t timing;
        uint32_t idx = submenu_get_selected_item(submenu);
        furi_check(idx < (uint32_t)ARRAY_SIZE(guns));

        furi_hal_light_set(LightRed, 0);
        furi_hal_light_set(LightGreen, 0);
        furi_hal_light_set(LightBlue, 255);

        // MODIFIÉ : Utilise GPIO A7 (pin 2) au lieu de gpio_infrared_tx
        const GpioPin* const pin_led = &gpio_ext_pa7;  // <-- CHANGEMENT ICI
        const GpioPin* const pin_ok = &gpio_button_ok;

        timing = guns[idx].timing - 1;
        furi_hal_gpio_init(pin_led, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);

        if(idx == 1) {  // Laser Atlanta Stealth mode
            uint32_t timing2 = (timing + 1) * 6 - 1;
            do {
                furi_hal_gpio_write(pin_led, true);
                furi_delay_us(1);
                furi_hal_gpio_write(pin_led, false);
                furi_delay_us(timing);

                furi_hal_gpio_write(pin_led, true);
                furi_delay_us(1);
                furi_hal_gpio_write(pin_led, false);
                furi_delay_us(timing2);
            } while(furi_hal_gpio_read(pin_ok));
        } else {  // Common gun pattern
            do {
                furi_hal_gpio_write(pin_led, true);
                furi_delay_us(1);
                furi_hal_gpio_write(pin_led, false);
                furi_delay_us(timing);
            } while(furi_hal_gpio_read(pin_ok));
        }

        furi_hal_gpio_init_simple(pin_led, GpioModeAnalog);
        consumed = true;
    }

    if(event->type == InputTypeRelease && event->key == InputKeyOk) {
        furi_hal_light_set(LightRed, 0);
        furi_hal_light_set(LightGreen, 0);
        furi_hal_light_set(LightBlue, 0);
        consumed = true;
    }

    if(!consumed && (view_hijacker->orig_input_callback)) {
        consumed = view_hijacker->orig_input_callback(event, view_hijacker->orig_context);
    }
    return consumed;
}

// Le reste du code (on_enter, on_event, on_exit) reste IDENTIQUE
// ...
