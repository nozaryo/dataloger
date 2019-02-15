WORKNAME = dataloger
OBJFILES = main.o
DEVICE = atmega328p
CFLAGS = -mmcu=$(DEVICE) -Os -Wl,-u,vfprintf -lm -lprintf_flt
LDFLAFGS =
PORT = /dev/ttyUSB0
LFUSE = 0b10101110 #外部クロック用になってる. 外部クロックなしで動かなるため注意
HFUSE = 0b11011001

%.o: %.c
	avr-g++ $(CFLAGS) $^ -o $@

test:
	avrdude -c avrisp -P $(PORT) -b 19200 -p $(DEVICE)

write: $(OBJFILES)
	sudo avrdude -c avrisp -P $(PORT) -b 19200 -p $(DEVICE) -U flash:w:$(OBJFILES):e

fuse:
	avrdude -c avrisp -P $(PORT) -b 19200 -p $(DEVICE) -U lfuse:w:$(LFUSE):m
	avrdude -c avrisp -P $(PORT) -b 19200 -p $(DEVICE) -U hfuse:w:$(HFUSE):m

clean:
	rm -f $(OBJFILES)
