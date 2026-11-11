# ESP-IDF-Projekt: MAME 0.220 auf ESP32-S3
#
# Ziel: eine aus den Originalquellen (Tag mame0220) gebaute Fassung, die auf dem
# ESP32-S3 laeuft und den Treiber mm4 (Mephisto MM IV, 65C02) ausfuehrt.
#
# Die MAME-Quellen liegen NICHT in diesem Projekt, sondern unter ref/mame0220-src
# (per git clone, Tag mame0220). Das ist Absicht: unveraenderte Originalquellen.
# ESP-IDF-Projekt: MAME 0.220 auf ESP32-S3
#
# Ziel: aus den Originalquellen (Tag mame0220) gebaute Fassung, die auf dem
# ESP32-S3 laeuft und den Treiber mm4 (Mephisto MM IV, 65C02) ausfuehrt.
#
# Die MAME-Quellen liegen NICHT hier, sondern unter ../ref/mame0220-src.
#
# WICHTIG: EXTRA_COMPONENT_DIRS wird von PlatformIO per Kommandozeile
# uebergeben. Hier darf es NICHT gesetzt werden (sonst bricht der Builder ab).
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mame_s3)
