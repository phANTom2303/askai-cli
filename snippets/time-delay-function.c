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

void displayStringWithDelay(char *str) {
    char ch;
    int len = strlen(str);
    int lineCount = 0;
    for (int i = 0; i < len; i++) {
        ch = str[i];
        printf("%c", ch);
        if (i % 100 == 0)
            sleep_ms(50);
        else if (i % 501 == 0)
            sleep_ms(150);
    }
}

int main() {
    for (int i = 1; i < 10; i++) {
        printf("%d\n", i);
        sleep_ms(100);
    }
}