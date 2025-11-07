
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
#include "tusb.h"//the library used to create a serial port over USB, according to part 5 

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048 

//Add here necessary states
enum state {WAITING=1, WRITE_TO_MEMORY, SEND_MESSAGE, upper_idle=0, upper_processing};
enum state upper_state = upper_idle;
enum state lower_state = WAITING;
char received_morse_code[256] = {0};//buffer to store 




char current_morse;

char morse_message[257];

int morse_index = 0;

static void read_sensor() {
    float ax, ay, az, gx, gy, gz, t;

    if (lower_state != WAITING)
        return;  // Do nothing if not ready to detect motion

    init_ICM42670();

    ICM42670_start_with_default_values();

    if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0)
    {
        if (az > 0.15) {
            printf("UP: %.2fg)\n", az); // delete after testing
            current_morse = ".";
        }
        else if (az < -0.15) {
            printf("DOWN: %.2fg)\n", az); // delete after testing
            current_morse = "-";
        }

        lower_state = WRITE_TO_MEMORY;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // ~10 Hz update rate
    
}


static void read_button() {

    if (lower_state != WRITE_TO_MEMORY)
        return;

    if (current_morse != '\0' && morse_index < 257) {
        morse_message[morse_index++] = current_morse;
        morse_message[morse_index] = '\0'; /// keep string terminated
        printf("Stored: %c | Buffer: %s\n", current_morse, morse_message); /// only for testing
    }

    current_morse = '\0';
    lower_state = WAITING;  /// Ready for next motion
}

int main() {
    while(1) {
        read_sensor();
        read_button();
    }
}





bool message_received = false;
void upper_state_check_message(void) {
    //ARDA IDK HOW WE CHECK THE WORKSTATIONS MESSAGE INPUTS BUT we FIGURE IT OUT
    //ARDA the message we got = received_morse_code
    

    message_received=true;}



void tud_cdc_rx_cb(uint8_t itf){   //the function name must be exactly tud_cdc_rx_cb. Do not rename it
    //THIS FUNCTION IS TAKEN FROM PART 5, PROBABLY SIMILAR TO HOW WE RECEIVE DATA?, 

    // allocate buffer for the data on the stack
    uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE + 1];

    // read the available data  
    // | IMPORTANT: do this for CDC0 as well, otherwise
    // | printing to CDC0 can stall (its RX buffer fills up)
    // | before the next time this function is called
    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));// ARDA reads data from USB into buf. You’ll then process that data as needed. 

    // check if the data was received on the second CDC interface
    if (itf == 1) {//ARDA NOTE, ADD THE DATA TO char received_morse_code

        for (int i = 0; i < count && i < sizeof(received_morse_code)-1; i++) {
            received_morse_code[i] = buf[i];
        }
        // Process the data here (e.g., store in a buffer or send to another task).
        // ...
        message_received = true;
        // Be gentle and send an OK back. 
        tud_cdc_n_write(itf, (uint8_t const *) "OK\n", 3);
        tud_cdc_n_write_flush(itf);
    }

    // Optional: if you need a C-string, you can terminate it:
    // if (count < sizeof(buf)) buf[count] = '\0';
}


static void usbTask(void *arg) {
    //ARDA THIS FUNCTION IS TAKEN FROM PART 5, it should call void tud_cdc_rx_cb whenever data is received?

    (void)arg;
    while (1) {
        tud_task();              // With FreeRTOS, wait for events
                                 // Do not add vTaskDelay.
    }
}


void morse_code_light(char* morse_code){
    //ARDA TURN MORSE CODE RECEIVED INTO LIGHT INTERVALS

    for (int i=0; morse_code[i] !='\n' && morse_code[i] !='\0';i++){
        //message ends with two spaces and new line(\n) according to the doc so it should recognize it?
        if (morse_code[i] == '  ' && morse_code[i+1] == ' ' && morse_code[i+2] == '\n') {break;}


        if (morse_code[i] == '.') {
            set_led_status(true);
            vTaskDelay(pdMS_TO_TICKS(100));//amount of ticks to indicate its a dot
            set_led_status(false);

            vTaskDelay(pdMS_TO_TICKS(100));// so they arent actuated right after each other
        } 
        else if (morse_code[i] == '-') { 
            set_led_status(true);
            vTaskDelay(pdMS_TO_TICKS(100));//amount of ticks to indicate its a dash
            set_led_status(false);

            vTaskDelay(pdMS_TO_TICKS(100));// so they arent actuated right after each other
        } 
        else if (morse_code[i] == ' ') {

            if (morse_code[i+1] == ' ') {
                vTaskDelay(pdMS_TO_TICKS(100));//amount of ticks to indicate space between two words
            }
                
            else {vTaskDelay(pdMS_TO_TICKS(100));}//amount of ticks to indicate its a space
                
                
        //NOTES; kind of confused on how to differentiate spaces and the inherent timing between dots and dashes
    }}}


static void upper_state_display_message(void *arg) {
    (void)arg;
    
    init_display();
    init_led();
    set_led_status(false);

    clear_display();
    write_text("waiting for message");

    for(;;){
        switch (upper_state){

            case upper_idle:

                if (message_received){
                    upper_state = upper_processing;}
                    break;

            
            case (upper_processing):
    
                clear_display();
                write_text("message from workstation");

                // WORKSTATIONS MESSAGE INPUT HERE, 
                // write_text(received_morse_code);????


                morse_code_light(received_morse_code);

                clear_display();
                write_text("waiting for message");
                message_received=false;
                upper_state = upper_idle;



        }
    vTaskDelay(pdMS_TO_TICKS(1000));//idk how many ticks?
    }}


static void example_task(void *arg){
    (void)arg;

    for(;;){
        tight_loop_contents(); // Modify with application code here.
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    TaskHandle_t usbtask, myExampleTask = NULL;
    

    // Create the tasks with xTaskCreate
    xTaskCreate(usbTask, "usb", 1024, NULL, 3, &usbtask);

    BaseType_t result = xTaskCreate(example_task,       // (en) Task function
                "example",              // (en) Name of the task 
                DEFAULT_STACK_SIZE, // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &myExampleTask);    // (en) A handle to control the execution of this task

    //ARDA xTaskCreate(upper_state_task, "upper_state", DEFAULT_STACK_SIZE, NULL, 2, &upperTaskHandle);

    if(result != pdPASS) {
        printf("Example Task creation failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}
