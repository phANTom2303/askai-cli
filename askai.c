#include <curl/curl.h>
#include <errno.h>  // Required for checking errno against EINTR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // For mkdir() and chmod()
#include <sys/types.h>  // For mkdir()
#include <time.h>       // Required for nanosleep and struct timespec

#include "dependencies/cJSON.h"

char *terminalFormattingContext =
    "\n\n"
    "====================================\n"
    "OUTPUT FORMAT: TERMINAL PLAIN TEXT\n"
    "====================================\n"
    "Rules:\n"
    "- ASCII only (no Unicode/emoji)\n"
    "- No Markdown (**, __, ##, ```)\n"
    "\n"
    "Visual Elements:\n"
    "  Headers:     === TITLE ===\n"
    "  Subheaders:  --- Title ---\n"
    "  Dividers:    ----------------\n"
    "  Emphasis:    _text_\n"
    "  Lists:       * item or 1. item\n"
    "  Code:        (indent 4 spaces)\n"
    "  Links:       Name (url)\n"
    "\n"
    "Add blank lines between sections.\n"
    "====================================\n";

// A struct to hold the response from the server
struct MemoryStruct {
    char *memory;
    size_t size;
};

char *appendToHistory(char *history, char *role, char *contents) {
    int historyLen = history ? strlen(history) : 0;
    int newLen =
        historyLen + strlen(role) + strlen(contents) + 3;  // +4 for "\n : \0"

    char *newHistory = realloc(history, newLen);
    if (!newHistory)
        return history;

    if (historyLen == 0) {
        newHistory[0] = '\0';  // Initialize empty string
    }

    strcat(newHistory, "\n");
    strcat(newHistory, role);
    strcat(newHistory, ":");
    strcat(newHistory, contents);
    return newHistory;
}

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

void printJSON(cJSON *json) { printf("%s\n", (cJSON_PrintUnformatted(json))); }

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

// This callback function gets called by libcurl as soon as there is data
// received
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // Reallocate memory for the response buffer
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
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

char *getApiKey() {
    char config_dir[1024];
    char config_path[1024];
    const char *home_dir = getenv("HOME");

    // 1. CONSTRUCT THE PATHS with bounds checking
    int dir_len =
        snprintf(config_dir, sizeof(config_dir), "%s/.askai-cli", home_dir);
    if (dir_len >= sizeof(config_dir)) {
        fprintf(stderr, "Error: Config directory path too long.\n");
        return NULL;
    }

    int path_len =
        snprintf(config_path, sizeof(config_path), "%s/config", config_dir);
    if (path_len >= sizeof(config_path)) {
        fprintf(stderr, "Error: Config file path too long.\n");
        return NULL;
    }

    // 2. TRY TO READ THE KEY FROM THE CONFIG FILE
    FILE *file = fopen(config_path, "r");
    if (file) {
        char key_buffer[256] = {0};  // Initialize buffer to zero

        // Read the first line, which we assume is the entire key.
        if (fgets(key_buffer, sizeof(key_buffer), file)) {
            key_buffer[strcspn(key_buffer, "\n")] =
                0;  // Remove trailing newline

            // Ensure the key is not just an empty line
            if (strlen(key_buffer) > 0) {
                fclose(file);
                return strdup(key_buffer);  // Return a copy of the key
            }
        }
        fclose(file);  // Close even if read fails
    }

    // 3. IF READING FAILED, PROMPT THE USER (FIRST-TIME SETUP)
    printf("--- AskAI CLI Setup ---\n");
    printf("Please enter your Gemini API Key: ");

    char user_key_input[256];
    if (!fgets(user_key_input, sizeof(user_key_input), stdin)) {
        fprintf(stderr, "Error reading API Key from input.\n");
        return NULL;
    }
    user_key_input[strcspn(user_key_input, "\n")] =
        0;  // Remove trailing newline

    mkdir(config_dir, 0700);  // Create directory, ignore error if it exists

    FILE *outfile = fopen(config_path, "w");

    // Write only the key to the file, no "API_KEY=" prefix.
    fprintf(outfile, "%s", user_key_input);
    fclose(outfile);

    chmod(config_path, 0600);  // Set secure permissions

    printf("API Key saved to %s for future use.\n", config_path);

    return strdup(user_key_input);
}

int main() {
    CURL *curl_handle;
    CURLcode res;

    const char *api_key = getApiKey();
    if (!api_key)
        return 0;

    // Construct the full URL with the API key
    char url[256];
    snprintf(url, sizeof(url),
             "https://generativelanguage.googleapis.com/v1beta/models/"
             "gemini-2.5-flash-lite:generateContent?key=%s",
             api_key);

    char *history = malloc(1024);  // Increased buffer size
    strcpy(history,
           "CONVERSATION HISTORY:\n"
           "Below is the complete conversation between user and assistant. "
           "Maintain context from all previous exchanges. "
           "user: indicates messages from the user. "
           "model: indicates your previous responses.\n"
           "---\n");

    printf(
        "Welcome to AskAI Chat. Get answers to your question. (type \"stop\" "
        "and press enter to exit )");
    while (1) {
        // Initialize the struct that will hold our response
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);  // will be grown as needed by the callback
        chunk.size = 0;            // no data at this point

        printf("\n");
        char *userPrompt = readString();
        // Check if user wants to stop
        if (strcmp(userPrompt, "stop") == 0) {
            printf("Exiting AskAI CLI. Goodbye!\n");
            free(userPrompt);
            break;
        }

        history = appendToHistory(history, "user", userPrompt);
        const char *post_data =
            preparePostData(userPrompt, terminalFormattingContext, history);
        // Initialize libcurl globally
        curl_global_init(CURL_GLOBAL_ALL);

        // Initialize a curl easy handle
        curl_handle = curl_easy_init();
        if (curl_handle) {
            // Set the required headers
            struct curl_slist *headers = NULL;
            headers =
                curl_slist_append(headers, "Content-Type: application/json");

            // Set the curl options
            curl_easy_setopt(curl_handle, CURLOPT_URL, url);
            curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);
            // Set the callback function to handle the response
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                             WriteMemoryCallback);
            // Pass our 'chunk' struct to the callback function
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

            // Perform the request, res will get the return code
            res = curl_easy_perform(curl_handle);

            // Check for errors
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
            } else {
                // The request was successful, print the response
                char *responseText = parse_gemini_response(chunk.memory);
                printf("\n");
                displayStringWithDelay(responseText);
                history = appendToHistory(history, "model", responseText);
            }

            // Cleanup
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl_handle);
            free(chunk.memory);
            free((void *)post_data);
            free(userPrompt);
        } else {
            printf("Failed due to some network related error");
            break;
        }
    }

    // Global cleanup
    curl_global_cleanup();
    return 0;
}