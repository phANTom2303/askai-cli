#include "cJSON.h"
#include "jsonHandling.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *create_gemini_json_payload(const char *prompt_text) {
    cJSON *root = cJSON_CreateObject();
    cJSON *contents_array = cJSON_AddArrayToObject(root, "contents");
    cJSON *content_item = cJSON_CreateObject();
    cJSON_AddItemToArray(contents_array, content_item);
    cJSON *parts_array = cJSON_AddArrayToObject(content_item, "parts");
    cJSON *part_item = cJSON_CreateObject();
    cJSON_AddItemToArray(parts_array, part_item);
    cJSON_AddStringToObject(part_item, "text", prompt_text);
    // printJSON(root);
    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_string;
}

const char *preparePostData(char *userPrompt, char *extraContext,
                            char *history) {
    int fullPromptLength =
        strlen(userPrompt) + strlen(extraContext) + strlen(history) + 1;
    char *fullPrompt = malloc(fullPromptLength);

    strcpy(fullPrompt, userPrompt);
    strcat(fullPrompt, extraContext);
    strcat(fullPrompt, history);
    const char *post_data = create_gemini_json_payload(fullPrompt);
    free(fullPrompt);
    return post_data;
}

char *parse_gemini_response(const char *response_json) {
    char *extracted_text = NULL;
    cJSON *root = cJSON_Parse(response_json);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return NULL;
    }

    // Navigate to: candidates -> [0] -> content -> parts -> [0] -> text
    cJSON *candidates = cJSON_GetObjectItemCaseSensitive(root, "candidates");
    cJSON *candidate = cJSON_GetArrayItem(candidates, 0);

    cJSON *content = cJSON_GetObjectItemCaseSensitive(candidate, "content");

    cJSON *parts = cJSON_GetObjectItemCaseSensitive(content, "parts");

    cJSON *part = cJSON_GetArrayItem(parts, 0);

    cJSON *text_obj = cJSON_GetObjectItemCaseSensitive(part, "text");

    extracted_text = strdup(text_obj->valuestring);

    cJSON_Delete(root);  // Free the parsed JSON object
    return extracted_text;
}