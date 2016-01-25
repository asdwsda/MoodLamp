PYTHON_BIN ?= /usr/bin/python
ARDUINO_BIN ?= /usr/bin/arduino
PACKAGE ?= ~/.arduino15/packages/esp8266
SKETCH_DIR = moodlamp
BUILD_DIR = build
INO = moodlamp.ino

OTA_HOST = moodlamp.local
OTA_PORT = 8266
OTA_PASSWORD = <OTA_password>

ESPOTA = $(shell find $(PACKAGE) -type f -name espota.py)
MKSPIFFS = $(shell find $(PACKAGE) -type f -name mkspiffs)

SPIFFS_IMG = moodlamp.spiffs.bin

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

compile: $(BUILD_DIR)
	$(ARDUINO_BIN) --verify --verbose \
      --pref build.path=$(BUILD_DIR) \
      --pref sketchbook.path=$(SKETCH_DIR) \
      --pref board=nodemcuv2 \
      --pref target_platform=esp8266 \
      --pref target_package=esp8266 \
      $(SKETCH_DIR)/$(INO)

program: compile
	$(PYTHON_BIN) $(ESPOTA) \
      --ip $(OTA_HOST) \
      --port $(OTA_PORT) \
      --auth $(OTA_PASSWORD) \
      --file $(BUILD_DIR)/moodlamp.cpp.bin \
      --progress \

spiffs: $(BUILD_DIR)
	$(MKSPIFFS) \
      -c moodlamp/data \
      -p 256 \
      -b 8192 \
      -s 3125248 \
      $(BUILD_DIR)/$(SPIFFS_IMG)

data: spiffs
	$(PYTHON_BIN) $(ESPOTA) \
      --ip $(OTA_HOST) \
      --port $(OTA_PORT) \
      --auth $(OTA_PASSWORD) \
      --spiffs \
      --file $(BUILD_DIR)/$(SPIFFS_IMG) \
      --progress

.PHONY: compile program spiffs data
