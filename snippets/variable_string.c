#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *readString() {
    char *prompt = NULL;
    char ch;
    int size = 0;
    int len = 0;

    printf("Enter your AI prompt (can be of any length) : ");
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


int main() {
    char *prompt = readString();
    if (!prompt)
        return 0;

    int promptLen = strlen(prompt);
    printf("prompt length = %d\n", promptLen);
    printf("%s\n", prompt);
    free(prompt);
    return 0;
}
