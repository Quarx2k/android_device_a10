#include <linux/input.h>

#include "recovery_ui.h"
#include "common.h"
#include "extendedcommands.h"


int device_toggle_display(volatile char* key_pressed, int key_code) {
    return 0;
}

int device_handle_key(int key_code, int visible) {
    if (visible) {
        switch (key_code) {
            case KEY_3:
            case KEY_ENTER:
            case KEY_SEARCH:
                return HIGHLIGHT_DOWN;

            case KEY_4:
            case KEY_HOME:
            case KEY_MENU:
                return HIGHLIGHT_UP;

            case KEY_2:
            case KEY_POWER:
                return SELECT_ITEM;
            
            case KEY_1:
            case KEY_ESC:
                    return GO_BACK;
        }
    }

    return NO_ACTION;
}
