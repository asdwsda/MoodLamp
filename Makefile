ARDUINO_BIN ?= /usr/bin/arduino
PACKAGE ?= ~/.arduino15/packages/esp8266
ESPOTA = $(shell find $(PACKAGE) -type f -name espota.py)
#MKSPIFFS = $(shell $(ARDUINO_BIN) --get-pref runtime.tools.mkspiffs.path)/mkspiffs
MKSPIFFS = $(shell find $(PACKAGE) -type f -name mkspiffs)
SKETCH_DIR = moodlamp
INO = moodlamp.ino
BUILD_DIR = build
OTA_HOST = moodlamp.local
OTA_PORT = 8266
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
	@env python2 $(ESPOTA) \
      --ip $(OTA_HOST) \
      --port $(OTA_PORT) \
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
	@env python2 $(ESPOTA) \
      --ip $(OTA_HOST) \
      --port $(OTA_PORT) \
      --spiffs \
      --file $(BUILD_DIR)/$(SPIFFS_IMG) \
      --progress

.PHONY: compile program spiffs data
