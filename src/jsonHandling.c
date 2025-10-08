#include "jsonHandling.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

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

// --- Function 2: The new function you requested ---
// Parses the structured JSON string and displays its content line by line.
void parseAndDisplayLines(const char *structured_json_string) {
    if (structured_json_string == NULL) {
        fprintf(stderr, "Error: Input string for parsing is NULL.\n");
        return;
    }

    cJSON *structured_data = cJSON_Parse(structured_json_string);
    if (structured_data == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error parsing structured JSON: %s\n", error_ptr);
        }
        return;
    }

    if (!cJSON_IsArray(structured_data)) {
        fprintf(stderr, "Error: The provided JSON is not an array.\n");
        cJSON_Delete(structured_data);
        return;
    }

    // Iterate through the array of line objects
    cJSON *line_object = NULL;
    cJSON_ArrayForEach(line_object, structured_data) {
        cJSON *line_type =
            cJSON_GetObjectItemCaseSensitive(line_object, "lineType");
        cJSON *line_content =
            cJSON_GetObjectItemCaseSensitive(line_object, "lineContent");

        if (cJSON_IsString(line_type) && cJSON_IsString(line_content)) {
            const char *type_str = line_type->valuestring;
            const char *content_str = line_content->valuestring;

            // Apply different formatting based on the line type for a TUI feel
            if (strcmp(type_str, "heading") == 0) {
                printf("\n--- %s ---\n", content_str);
            } else if (strcmp(type_str, "subheading") == 0) {
                printf("\n## %s\n", content_str);
            } else if (strcmp(type_str, "code") == 0) {
                printf("\n```\n%s\n```\n", content_str);
            } else if (strcmp(type_str, "quote") == 0) {
                printf("> %s\n", content_str);
            } else {  // "text"
                printf("%s\n", content_str);
            }
        }
    }

    cJSON_Delete(structured_data);  // Clean up
}