/*
 * Copyright (c) 2018 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>

#include "mbed.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed-client-cli/ns_cmdline.h"

#define TRACE_GROUP   "main"

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
 * *****************************************************************************
 * *****************************************************************************
 */

static int cmd_sys_info(int argc, char **argv)
{
#if defined(MBED_SYS_STATS_ENABLED)
    mbed_stats_sys_t    sys_stats;

    mbed_stats_sys_get(&sys_stats);
    printf("Mbed OS Version: %" PRId32 "\n", sys_stats.os_version);
    printf("CPU ID: 0x%" PRIx32 "\n", sys_stats.cpu_id);
    printf("Compiler ID: %d \n", sys_stats.compiler_id);
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
    mbed_stats_heap_t    heap_stats;
                // Collect and print heap stats
    mbed_stats_heap_get(&heap_stats);
    printf("Current heap:  %lu\t", heap_stats.current_size);
    printf("Max heap size: %lu\n", heap_stats.max_size);
#else
    printf("!MBED_HEAP_STATS_ENABLED\n");
#endif

#if defined(MBED_THREAD_STATS_ENABLED)
#define MAX_TR            10
    mbed_stats_thread_t    thread_stats[MAX_TR];
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
#define SAMPLE_TIME_MS    2000
    static uint64_t prev_idle_time;
    mbed_stats_cpu_t    cpu_stats;
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
 * *****************************************************************************
 */

const char spinner_char[]    = "-\\|/";
char char_spinn(){ static int i; return(spinner_char[++i & 3]); }
unsigned short nop_cnt;

static int cmd_noop(int argc, char **argv)
{
    char str_n[256], sp;
    int i;

    strcpy(str_n, ": - This Command does almost nothing - ");
    sp = char_spinn(); i = strlen(str_n); str_n[i-37] = str_n[i-2] = sp;
    printf("%s%d", str_n, nop_cnt++);

    if(argc >1){
        for(i = 0; i < argc; i++){
            printf("\n\targv[%d] = %s", i, argv[i]);
        }
    }
    printf("\n");
    return CMDLINE_RETCODE_SUCCESS;
}

/*
 * *****************************************************************************
 * *****************************************************************************
 * *****************************************************************************
 */

#define ADC_KBD_SAMPLES 10

int decode_kbd()
{
//     AnalogIn ainn(KBD_IN);	// When I do this, I get only one interrupt.

    uint16_t adc_sample;

    thread_sleep_for(10);    // 10ms

    adc_sample = 1234;	// ainn.read_u16();

    thread_sleep_for(3);    // 3ms

//    InterruptIn kbd_int(KBD_IN, PullDown);	// This is possibly Wrong, and have no effect

    return(adc_sample);
}

InterruptIn kbd_int(KBD_IN, PullDown);

DigitalOut led1(LED1);
DigitalOut led2(LED2);

EventQueue    kbd_event_q(3 * EVENTS_EVENT_SIZE);    // Let kbd get its own EventQueue
Thread kbd_tread;                                    // Let kbd get its own Thread

void kbd_int_handler()
{
    led1 = !led1;
    printf("\n **** kbd_int() **** %d\n", decode_kbd()    /*, Thread::gettid()*/);
}

int main(void)
{
    mbed_trace_init();                                                  // Initialize trace library
    mbed_trace_mutex_wait_function_set( serial_out_mutex_wait );        // Register callback used to lock serial out mutex
    mbed_trace_mutex_release_function_set( serial_out_mutex_release );  // Register callback used to release serial out mutex

    cmd_init( 0 );                                           // Initialize cmd library
    cmd_mutex_wait_func( serial_out_mutex_wait );            // Register callback used to lock serial out mutex
    cmd_mutex_release_func( serial_out_mutex_release );      // Register callback used to release serial out mutex

    cmd_add("noop",      cmd_noop,        "noop command",        "This is noop command, which does not do anything except\nprint the args");                 // add one dummy command
    cmd_add("si",        cmd_sys_info,    "cmd_sys_info",        0);

    cmd_init_screen();

    printf("\n ***** mbed_kbd_test *****\n");

    kbd_tread.start(callback(&kbd_event_q, &EventQueue::dispatch_forever));        // Let kbd get its own Thread
    kbd_int.rise(kbd_event_q.event(kbd_int_handler));    // The 'rise' handler will execute in the context of 'kbd_tread'

    while(true){
        int c = getchar();
        if (c != EOF) {
            cmd_char_input(c);
        }
        led2 = !led2;
    }
}
