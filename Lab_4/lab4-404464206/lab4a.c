#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <mraa/aio.h>

sig_atomic_t volatile run_flag = 1;

void do_when_interrupted(int sig) {
    if (sig == SIGINT)
        run_flag = 0;
}

int main() {
    uint16_t value;
    int B = 4275;
    
    // declare rotary as an analog I/O context
    mraa_aio_context temperature;
    temperature = mraa_aio_init(0);
    signal(SIGINT, do_when_interrupted);
    
    while (run_flag) {
        // read the rotary sensor value
        value = mraa_aio_read(temperature);
        double R = 1023.0/value - 1.0;
        R = 100000.0*R;
        double tmpInCels = 1.0/(log(R/100000.0)/B + 1/298.15) - 273.15;
        double tmpInFahr = (tmpInCels * 1.80) + 32.0;
        
        // timing code
        char formatted_time[20];
        time_t epoch_time;
        time(&epoch_time);
        struct tm *local_time = localtime(&epoch_time);
        strftime(formatted_time, 80, "%H:%M:%S", local_time);
        
        fprintf(stdout, "%s %04.1f\n", formatted_time, tmpInFahr);
        fflush(stdout);
        sleep(1);
    }
    
    mraa_aio_close(temperature);
    
    return 0;
}

