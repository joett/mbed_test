# How to switch between interrupt input and analog input on the same pin?

Was the question asked on the Mbed Forum.
https://forums.mbed.com/t/how-to-switch-between-interrupt-input-and-analog-input-on-the-same-pin/7654/3

**********
It is the "int decode_kbd();" function that I am a bit lost in.
	(The function is deleted in the uppdate, all heppen in the "kbd_int_handler();" and I'm still lost)

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

The idea is to have a command to turn off the serial line so the target can enter sleep mode, and use the keypad to enable it again if I want to.
**********

Hello Jan,

    Can I change the pin configuration / functionality “on the fly”?

Try to create the InterruptIn and AnalogIn dynamically on the heap rather than as static variables. Keep global only the associated pointers. Then you’ll be able to delete and recreate them as needed. I tested your code with such modification on an mbed LPC1768 board. Since I didn’t have your special keyboard I connected one end of a push button to pin p15 and the other end over a 4k7 resistor to the +3.3V rail:

...
#define TRACE_GROUP "main"
#define KBD_IN      p15

InterruptIn  *kbd_int;    // Global pointer to an InterruptIn
AnalogIn     *ainn;       // Global pointer to an AnalogIn

DigitalOut led1(LED1);
DigitalOut led2(LED2);

EventQueue kbd_event_q(3 * EVENTS_EVENT_SIZE);     // Let kbd get its own EventQueue
Thread kbd_tread;                                  // Let kbd get its own Thread
...
int decode_kbd()
{
    // 	Her I want to change the pin mode to Analog In
    // 	But I find no way to do it correctly
    //  AnalogIn ainn(KBD_IN);      // When I do this, I get only one interrupt.
    ainn = new AnalogIn(KBD_IN);    // Create a new 'ainn' AnalogIn object on the heap connected to the 'KBD_IN' pin
    uint16_t adc_sample;

    thread_sleep_for(10);    // 10ms

    // 	Her I want to Read the Analog value from the same pin that gave the interrupt
    //     adc_sample = ainn.read_u16();
    adc_sample = ainn->read_u16();
    delete ainn;            // Delete the 'ainn' AnalogIn object from the heap

    thread_sleep_for(3);    // 3ms (this is probably not needed)

    // 	Her I want to change the pin mode to interrupt In and continue reciveing interrupts
    // 	But I find no way to do it correctly
    //     InterruptIn kbd_int(KBD_IN, PullDown);	// This is most likely Wrong, and have no effect
    //  The 'kbd_int' object could be re-created here.
    //  But I prerere to do it in the function where it was deleted (i.e. in 'kbd_int_handler')

    return(adc_sample);
}

void kbd_int_handler()
{
    kbd_event_q.break_dispatch();                       // Forces event queue's dispatch loop to terminate.
    delete kbd_int;                                     // Delete the kbd_int InterruptIn object from the heap
    led1 = !led1;
    printf("\n **** kbd_int() **** %d\n", decode_kbd()    /*, Thread::gettid()*/);
    thread_sleep_for(300);    // debounce the KBD_INT key (wait untill it stops bouncing) - may not be needed for your hadware/keyboard
    kbd_int = new InterruptIn(KBD_IN, PullDown);        // Create a new kbd_int InterruptIn object on the heap
    kbd_event_q.dispatch_forever();                     // Dispatch events without a timeout.
    kbd_int->rise(kbd_event_q.event(kbd_int_handler));  // Attach this function to handle the kbd_int's next 'rise' event
}

int main(void)
{
    ...
    kbd_int = new InterruptIn(KBD_IN, PullDown);        // Create a new 'kbd_int' InterruptIn object on the heap
    kbd_tread.start(callback(&kbd_event_q, &EventQueue::dispatch_forever));        // Let kbd få get its own Thread
    kbd_int->rise(kbd_event_q.event(kbd_int_handler));  // The 'rise' handler 'kbd_int_handler' will execute in the context of 'kbd_tread'
    ...
}

Below is the printout on Termite 3.4 serial terminal:


[1B][2J[1B][7hARM Ltd

[1B][2K/> [1B][1D
[1B][2J[1B][7hARM Ltd

[1B][2K/> [1B][1D
 ***** mbed_kbd_test *****

 **** kbd_int() **** 65535

 **** kbd_int() **** 65535

 **** kbd_int() **** 65535

    What do I have to do with the serial line (in my example code) to enable Mbed to enter deep sleep mode?

Unfortunately, I have no answer to that question. Maybe this thread can help you a bit.

**********
My Replay to that

Thank you very much for your guidance, it helped to demystify Mbed a bit.
I tried your code, but I did not manage to read more than one value from the adc. I tried to rearrange it a bit and put in a command to toggle the reading of the adc.
I see that when the ADC reading is off, I get interrupt for every key-press I do. When I turn on ADC reading (with the “ta” command), I get one more interrupt and the adc reading is as expected (different reading for different keys). I also made a while() loop so the tread is sleeping until the key is released, this also work.
When the key is released and the interrupt should be turned on again, nothing more happen, even though the command interpreter continue running.
If this work on the LPC1768 board, I can not understand other than that must be something wrong with the nRF52 stuff in Mbed.
As you may have noticed, I’m new to Mbed, and I’m not very comfortable with C++ either, but I have some 30 years experience with C on many different systems. But I’m struggling with Mbed (feel kind of outdated).
The single one reason for using Mbed is the LoRaWan stack, and the Mbed LoRaWan example run without problems on my board.
The board is the Telenor E002 / E004 board, it has a dual-band (868 and 433 MHz) LoRa radio and the BLE radio on the nRF52832.
Exploratory Engineering
EE LoRa Starter pack bundle

Perfect for prototyping and testing out the capabilities for our LoRa module. Contains two EE-02 and one EE-04.

Price: USD 100.00

In some way I hope it is my lack of Mbed experience rather then the nRF52 Mbed implementation that is the reason for the problems.
I have updated the code I’m struggling with in the “github_repo”
github.com
joett/mbed_test/blob/master/main.cpp

#include <stdio.h>
#include <string.h>

#include "mbed.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed-client-cli/ns_cmdline.h"
#include "platform/mbed_thread.h"

#define TRACE_GROUP "main"

// #define KBD_IN      p15
                    // The KBD_IN is defined in my
                    // "mbed-os/targets/TARGET_NORDIC/TARGET_NRF5x/TARGET_NRF52/TARGET_MCU_NRF52832"
                    //    files for the ee02 board
                    //    and my modified "mbed-os/targets/targets.json" file
                    // All in my "1_my_targets" directory
                    //    (It is just a cloning of the nRF52-DK)

InterruptIn  *kbd_int;    // Global pointer to an InterruptIn
// AnalogIn     *ainn;       // Global pointer to an AnalogIn

This file has been truncated. show original


**********
