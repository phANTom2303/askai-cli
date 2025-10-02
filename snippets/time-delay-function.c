#include <errno.h>
#include <stdio.h>
#include <time.h>
void sleep_ms(long milliseconds) {
    // Ensure the input is not negative.
    if (milliseconds < 0) {
        return;
    }

    struct timespec req, rem;

    // Convert milliseconds to seconds and nanoseconds.
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000000L;

    // Loop until the sleep is complete. This handles interruptions.
    // nanosleep returns -1 on interruption and sets errno to EINTR.
    // The 'rem' struct is populated with the remaining time.
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        req = rem;
    }
}

int main() {
    for (int i = 1; i < 10; i++) {
        printf("%d\n", i);
        sleep_ms(100);
    }
}