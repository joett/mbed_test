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
     // Moved inside the "kbd_int_handler();"

DigitalOut led1(LED1);
DigitalOut led2(LED2);

EventQueue kbd_event_q(3 * EVENTS_EVENT_SIZE);    // Let kbd get its own EventQueue
Thread kbd_tread;                                // Let kbd get its own Thread
void on_kbd_int_rise();                         // function prototype

/*
 * *****************************************************************************
 */

static Mutex SerialOutMutex;
void serial_out_mutex_wait()
{
    SerialOutMutex.lock();
}
void serial_out_mutex_release()
{
    osStatus s = SerialOutMutex.unlock();
    MBED_ASSERT(s == osOK);
}

/*
 * *****************************************************************************
 */

/*
 * *****************************************************************************
 * *****************************************************************************
 * ****** cmd_sys_info() *******************************************************
 */

static int cmd_sys_info(int argc, char **argv)
{
#if defined(MBED_SYS_STATS_ENABLED)
    mbed_stats_sys_t	sys_stats;
    mbed_stats_sys_get(&sys_stats);
    printf("Mbed OS Version: %" PRId32 "\n", sys_stats.os_version);
				/* CPUID Register information
				[31:24]Implementer      0x41 = ARM
				[23:20]Variant          Major revision 0x0  =  Revision 0
				[19:16]Architecture     0xC  = Baseline Architecture
										0xF  = Constant (Mainline Architecture)
				[15:4]PartNO            0xC20 =  Cortex-M0
										0xC60 = Cortex-M0+
										0xC23 = Cortex-M3
										0xC24 = Cortex-M4
										0xC27 = Cortex-M7
										0xD20 = Cortex-M23
										0xD21 = Cortex-M33
				[3:0]Revision           Minor revision: 0x1 = Patch 1
				*/
    printf("CPU ID: 0x%" PRIx32 "\n", sys_stats.cpu_id);
				/* Compiler IDs
					ARM     = 1
					GCC_ARM = 2
					IAR     = 3
				*/
    printf("Compiler ID: %d \n", sys_stats.compiler_id);
				/* Compiler versions:
				ARM: PVVbbbb (P = Major; VV = Minor; bbbb = build number)
				GCC: VVRRPP  (VV = Version; RR = Revision; PP = Patch)
				IAR: VRRRPPP (V = Version; RRR = Revision; PPP = Patch)
				*/
    printf("Compiler Version: %" PRId32 "\n", sys_stats.compiler_version);
				/* RAM / ROM memory start and size information */
    for (int i = 0; i < MBED_MAX_MEM_REGIONS; i++) {
        if (sys_stats.ram_size[i] != 0) {
            printf("RAM%d: Start 0x%" PRIx32 " Size: 0x%" PRIx32 "\n", i, sys_stats.ram_start[i], sys_stats.ram_size[i]);
        }
    }
    for (int i = 0; i < MBED_MAX_MEM_REGIONS; i++) {
        if (sys_stats.rom_size[i] != 0) {
            printf("ROM%d: Start 0x%" PRIx32 " Size: 0x%" PRIx32 "\n", i, sys_stats.rom_start[i], sys_stats.rom_size[i]);
        }
	}
#else
    printf("!MBED_SYS_STATS_ENABLED\n");	
#endif

	
#if defined(MBED_HEAP_STATS_ENABLED)
	mbed_stats_heap_t	heap_stats;
				// Collect and print heap stats
	mbed_stats_heap_get(&heap_stats);
	printf("Current heap:  %lu\t", heap_stats.current_size);
	printf("Max heap size: %lu\n", heap_stats.max_size);
#else
    printf("!MBED_HEAP_STATS_ENABLED\n");	
#endif


#if defined(MBED_THREAD_STATS_ENABLED)
#define MAX_TR			10
	mbed_stats_thread_t	thread_stats[MAX_TR];
	size_t treds;
	treds = mbed_stats_thread_get_each(thread_stats, MAX_TR);
    for (size_t i = 0; i < treds; i++) {
		printf("%2d %s \t\tid:%0x\tst:%d\tpr:%d\tss:%d\tsp:%d\n",
				(int)i, thread_stats[i].name, (int)thread_stats[i].id, (int)thread_stats[i].state,
				(int)thread_stats[i].priority, (int)thread_stats[i].stack_size, (int)thread_stats[i].stack_space);
	}
#else
    printf("!MBED_THREAD_STATS_ENABLED\n");	
#endif

#if defined(MBED_CPU_STATS_ENABLED)
#define SAMPLE_TIME_MS	2000
	static uint64_t prev_idle_time;
    mbed_stats_cpu_t	cpu_stats;
				// Calculate the percentage of CPU usage
    mbed_stats_cpu_get(&cpu_stats);
    uint64_t diff_usec = (cpu_stats.idle_time - prev_idle_time);
    uint8_t idle = (diff_usec * 100) / (SAMPLE_TIME_MS*1000);
    uint8_t usage = 100 - ((diff_usec * 100) / (SAMPLE_TIME_MS*1000));
    prev_idle_time = cpu_stats.idle_time;
	
    printf("Time(us): Up: %08lld", cpu_stats.uptime);
    printf(" Idle: %08lld", cpu_stats.idle_time);
    printf(" Sleep: %08lld", cpu_stats.sleep_time);
    printf(" DeepSleep: %08lld", cpu_stats.deep_sleep_time);
    printf(" Idle: %08d%% Usage: %d%%\r\n", idle, usage);
#else
    printf("!MBED_CPU_STATS_ENABLED\n");	
#endif

	return CMDLINE_RETCODE_SUCCESS;
}

/*
 * ************ cmd_toggle_adc_flagg *******************************************
 */
int adc_flagg;
static int cmd_toggle_adc_flagg(int argc, char **argv)
{
	if(adc_flagg)
		adc_flagg = 0;
	else
		adc_flagg = 1;
	printf("adc_flagg = %d\n", adc_flagg);
	
	return CMDLINE_RETCODE_SUCCESS;
}

/*
 * *****************************************************************************
 * *****************************************************************************
 * *****************************************************************************
 */

void kbd_int_handler()
{
	AnalogIn *ainn;
    uint16_t adc_sample = 0;
	
    printf("\n **** Enter kbd_int_handler(); ****\n");
	
// 	kbd_event_q.break_dispatch();     // Forces event queue's dispatch loop to terminate.
    delete kbd_int;                   // Delete the kbd_int InterruptIn object from the heap
    led1 = !led1;
	
	if(adc_flagg){
		printf("\tStart adc\n");
		ainn = new AnalogIn(KBD_IN);    // Create a new 'ainn' AnalogIn object on the heap connected to the 'KBD_IN' pin

		thread_sleep_for(10);    // 10ms for the kbd signal to stabilize
		
		printf("\tADC = %d\n", ainn->read_u16());		// Read one sample
		
		while((adc_sample = ainn->read_u16()) > 150 )
			thread_sleep_for(5);		// wait for signal to go low (key to be released)

		thread_sleep_for(500);
		printf("\tKey release %x, %d : %d\n", ainn, (int)adc_sample, (int)ainn->read_u16());
		
		delete ainn;            // Delete the 'ainn' AnalogIn object from the heap
	}
	
//  thread_sleep_for(300);    // debounce the KBD_INT key (wait untill it stops bouncing) - may not be needed for your hadware/keyboard
								// The kbd signal is quit slow, so debouncing shoud not be nesseserry

    kbd_int = new InterruptIn(KBD_IN, PullDown);        // Create a new kbd_int InterruptIn object on the heap
//     kbd_int->rise(kbd_event_q.event(kbd_int_handler));  // Attach this function to handle the kbd_int's next 'rise' event
//     kbd_event_q.dispatch_forever();                     // Dispatch events without a timeout.
    kbd_int->rise(on_kbd_int_rise);                     // Re-attache interrupt handler

	printf("**** Leaving kbd_int_handler(); ****\n");
}

/*
 * *****************************************************************************
 */

void on_kbd_int_rise()
{
    kbd_int->rise(NULL);                // Detach interrupt handler
    kbd_event_q.call(kbd_int_handler);  // Handle by event queue
}

int main(void)
{
	mbed_trace_init();													// Initialize trace library
	mbed_trace_mutex_wait_function_set( serial_out_mutex_wait );		// Register callback used to lock serial out mutex
	mbed_trace_mutex_release_function_set( serial_out_mutex_release );	// Register callback used to release serial out mutex

	cmd_init( 0 );											// Initialize cmd library
	cmd_mutex_wait_func( serial_out_mutex_wait );			// Register callback used to lock serial out mutex
	cmd_mutex_release_func( serial_out_mutex_release );		// Register callback used to release serial out mutex

/*
 * *****************************************************************************
 */
	cmd_add("si",		cmd_sys_info,			"cmd_sys_info",			0);
	cmd_add("ta",		cmd_toggle_adc_flagg,	"cmd_toggle_adc_flagg",	0);
/*
 * *****************************************************************************
 */

	cmd_init_screen();

	printf("\n ***** mbed_kbd_test *****\n");
	
    kbd_tread.start(callback(&kbd_event_q, &EventQueue::dispatch_forever)); // Let kbd get its own Thread
    kbd_int = new InterruptIn(KBD_IN, PullDown);                            // Create a new 'kbd_int' InterruptIn object on the heap
//     kbd_int->rise(kbd_event_q.event(kbd_int_handler));                   // The 'rise' handler 'kbd_int_handler' will execute in the context of 'kbd_tread'
    kbd_int->rise(on_kbd_int_rise);                                         // Attach interrupt handler

	while(true){
		int c = getchar();
		if (c != EOF) {
			cmd_char_input(c);
		}
		led2 = !led2;
	}
}
