# How to switch between interrupt input and analog input on the same pin?



I want to connect a "one pin" 12key keypad to my Mbed board. The keypad sends a High pulse when a key is pressed, after some mS, the signal falls down to a level that is determent by the key pressed. It falls to zero when the key is released.

I have some code that get an interrupt each time a key is pressed, when this interrupt come, I want to keep the pull down resistor and read the analog value on the same pin that gave the interrupt.
And thereafter I want to set the pin back to detect a new interrupt (with the pull down resistor).

How can I do this with Mbed?

The HW and SW is tested, and works very well on another OS, but I want to use Mbed, and I want it to be usable on boards with different hardware (STM and nRF52832 MCU. The chosen pin, of course, have to support both interrupt and analog in, plus pull down resistor).

Is this possible with Mbed?

In other words, can I change the pin configuration / functionality "on the fly"?

I think the code could run on any board with a key input and analog input on the same pin (but the analog value wil not mean anything without my keypad).


One more question:
	What do I have to do with the serial line (in my examle code) to enable Mbed to enter deep sleep mode?

