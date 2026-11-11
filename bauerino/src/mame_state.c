// Reuse the device-tested Retro state machine; the full frontend owns touch.
#define RETRO_UI_EXTERNAL_TOUCH 1
#define retro_ui_init retro_state_init
#define retro_ui_poll retro_state_poll
#define retro_ui_tap retro_state_tap
#define retro_ui_driver retro_state_driver
#define retro_ui_command retro_state_command
#define retro_ui_open retro_state_open
#define retro_ui_closed retro_state_closed
#define retro_ui_reply retro_state_reply
#define retro_ui_observe retro_state_observe
#define retro_ui_diagnostic retro_state_diagnostic
#define retro_ui_scanline retro_state_scanline
#define retro_ui_rate retro_state_rate
#include "../../esp32s3/src/retro_test_ui.c"
