"""Select the attached Waveshare S3 16 MiB flash / 8 MiB OPI PSRAM profile."""
from pathlib import Path
import re

project = Path(__file__).resolve().parents[1] / "esp32s3"
settings = {
    "CONFIG_SPIRAM_MODE_QUAD": "n",
    "CONFIG_SPIRAM_MODE_OCT": "y",
    "CONFIG_SPIRAM_SPEED_40M": "n",
    "CONFIG_SPIRAM_SPEED_80M": "y",
    "CONFIG_SPIRAM_SPEED": "80",
    "CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL": "4096",
    "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160": "n",
    "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240": "y",
    "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ": "240",
    "CONFIG_ESP32S3_DEFAULT_CPU_FREQ_160": "n",
    "CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240": "y",
    "CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ": "240",
    "CONFIG_ESP_CONSOLE_UART_DEFAULT": "n",
    "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG": "y",
    "CONFIG_ESP_CONSOLE_SECONDARY_NONE": "y",
    "CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG": "n",
    "CONFIG_CONSOLE_UART_DEFAULT": "n",
    "CONFIG_FATFS_LFN_NONE": "n",
    "CONFIG_FATFS_LFN_HEAP": "y",
}

for filename in ("sdkconfig.esp32s3-skeleton", "sdkconfig.defaults"):
    path = project / filename
    content = path.read_text(encoding="utf-8")
    for key, value in settings.items():
        pattern = rf"^(?:{key}=.*|# {key} is not set)$"
        line = f"# {key} is not set" if value == "n" else f"{key}={value}"
        if re.search(pattern, content, re.M):
            content = re.sub(pattern, line, content, flags=re.M)
        else:
            content += "\n" + line + "\n"
    path.write_text(content, encoding="utf-8")
    print(f"Updated {path.name}: octal PSRAM 80 MHz, CPU 240 MHz, USB console, FAT LFN")

main = project / "src/main.c"
content = main.read_text(encoding="utf-8")
start = content.index("// Rough workload:")
end = content.index("void app_main(void)", start)
content = content[:start] + content[end:]
header_end = content.index("#include <stdio.h>")
content = "// MAME 0.220 ROM bring-up on the Waveshare ESP32-S3.\n// This image tests the real core and ROM; the Bauerino UI is a later step.\n\n" + content[header_end:]
main.write_text(content, encoding="utf-8")
