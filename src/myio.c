#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../includes/myio.h"

void sleep_ms(long milliseconds) {
    if (milliseconds < 0)
        return;
    struct timespec req;
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&req, NULL);
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

char *readString() {
    char *prompt = NULL;
    char ch;
    int size = 0;
    int len = 0;

    // Get username from environment
    const char *username = getenv("USER");  // On Linux/Mac
    if (!username) {
        username = "You";  // Final fallback
    }
    printf("\n%s : ", username);
    while ((ch = fgetc(stdin)) != EOF && ch != '\n') {
        if (len + 1 >= size) {
            size = size == 0 ? 128 : size * 2;
            char *new_prompt = realloc(prompt, size);
            if (new_prompt == NULL) {
                free(prompt);
                return NULL;
            }
            prompt = new_prompt;
        }
        prompt[len++] = ch;
    }

    if (prompt != NULL) {
        prompt[len] = '\0';
    }
    return prompt;
}