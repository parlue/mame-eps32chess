idf_component_register(
    SRCS "main.c" "bauerino_retro_lcd.c" "retro_test_ui.c" "retro_touch.c" "retro_base.S" "../../integration/chessengine/src/s3_rom_fs.c"
    INCLUDE_DIRS "."
    REQUIRES mame0220 fatfs wear_levelling esp_driver_spi esp_driver_gpio esp_driver_i2c
)
set_source_files_properties("retro_base.S" PROPERTIES COMPILE_OPTIONS "-Wa,-I${CMAKE_CURRENT_LIST_DIR}")
