###############################################################################
# Makefile for the project servo
###############################################################################

## General Flags
PROJECT = servo
MCU = at90usb1287
TARGET = stepper.elf
CC = avr-gcc

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2          -DF_CLOCK=8000000UL -DBOARD=BOARD_USBKEY -DUSE_NONSTANDARD_DESCRIPTOR_NAMES -DNO_STREAM_CALLBACKS -DUSB_HOST_ONLY -DUSE_STATIC_OPTIONS="(USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"  -DF_CPU=8000000UL -O2 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -std=gnu99
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -nostartfiles


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES = -I"H:\roomba460\RTOS\..\LUFA" 

## Objects that must be built in order to link
OBJECTS = crt0.o os.o main.o myConfigDescriptor.o joy2mov.o radio.o spi.o ConfigDescriptor.o HIDParser.o Events.o StdDescriptors.o USBInterrupt.o USBTask.o DevChapter9.o Endpoint.o Host.o HostChapter9.o LowLevel.o Pipe.o Serial.o SerialStream.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) stepper.hex stepper.eep size

## Compile
crt0.o: ../OS/crt0.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

os.o: ../OS/os.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

main.o: ../main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

myConfigDescriptor.o: ../myConfigDescriptor.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

joy2mov.o: ../joy2mov.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

radio.o: ../radio.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

spi.o: ../spi.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ConfigDescriptor.o: ../../LUFA/LUFA/Drivers/USB/Class/ConfigDescriptor.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

HIDParser.o: ../../LUFA/LUFA/Drivers/USB/Class/HIDParser.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

Events.o: ../../LUFA/LUFA/Drivers/USB/HighLevel/Events.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

StdDescriptors.o: ../../LUFA/LUFA/Drivers/USB/HighLevel/StdDescriptors.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

USBInterrupt.o: ../../LUFA/LUFA/Drivers/USB/HighLevel/USBInterrupt.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

USBTask.o: ../../LUFA/LUFA/Drivers/USB/HighLevel/USBTask.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

DevChapter9.o: ../../LUFA/LUFA/Drivers/USB/LowLevel/DevChapter9.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

Endpoint.o: ../../LUFA/LUFA/Drivers/USB/LowLevel/Endpoint.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

Host.o: ../../LUFA/LUFA/Drivers/USB/LowLevel/Host.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

HostChapter9.o: ../../LUFA/LUFA/Drivers/USB/LowLevel/HostChapter9.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

LowLevel.o: ../../LUFA/LUFA/Drivers/USB/LowLevel/LowLevel.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

Pipe.o: ../../LUFA/LUFA/Drivers/USB/LowLevel/Pipe.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

Serial.o: ../../LUFA/LUFA/Drivers/Peripheral/Serial.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

SerialStream.o: ../../LUFA/LUFA/Drivers/Peripheral/SerialStream.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) stepper.elf dep/* stepper.hex stepper.eep

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

