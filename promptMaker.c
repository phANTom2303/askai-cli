#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

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

void printJSON(cJSON *json) { printf("%s\n", (cJSON_PrintUnformatted(json))); }

char *create_gemini_json_payload(const char *prompt_text) {
    cJSON *root = cJSON_CreateObject();
    printJSON(root);
    cJSON *contents_array = cJSON_AddArrayToObject(root, "contents");
    printJSON(root);
    cJSON *content_item = cJSON_CreateObject();
    printJSON(root);
    cJSON_AddItemToArray(contents_array, content_item);
    printJSON(root);
    cJSON *parts_array = cJSON_AddArrayToObject(content_item, "parts");
    printJSON(root);
    cJSON *part_item = cJSON_CreateObject();
    printJSON(root);
    cJSON_AddItemToArray(parts_array, part_item);
    printJSON(root);
    cJSON_AddStringToObject(part_item, "text", prompt_text);
    printJSON(root);
    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_string;
}

int main() {
    char *promptString = create_gemini_json_payload(readString());
    printf("%s\n", promptString);
    return 0;
}