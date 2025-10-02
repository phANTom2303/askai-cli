#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

// A struct to hold the response from the server
struct MemoryStruct {
    char *memory;
    size_t size;
};

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
    cJSON *contents_array = cJSON_AddArrayToObject(root, "contents");
    cJSON *content_item = cJSON_CreateObject();
    cJSON_AddItemToArray(contents_array, content_item);
    cJSON *parts_array = cJSON_AddArrayToObject(content_item, "parts");
    cJSON *part_item = cJSON_CreateObject();
    cJSON_AddItemToArray(parts_array, part_item);
    cJSON_AddStringToObject(part_item, "text", prompt_text);
    printJSON(root);
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

const char *preparePostData() {
    char *userPrompt = readString();
    char *extraContext =
        "Streamline your output text as if it were to be directly displayed in "
        "a terminal. Remove any and all markdown formatting, just use "
        "newlines, tabs spaces and ASCII characters.";

    int fullPromptLength = strlen(userPrompt) + strlen(extraContext) + 1;
    char *fullPrompt = malloc(fullPromptLength);

    strcpy(fullPrompt, userPrompt);
    strcat(fullPrompt, extraContext);
    // The JSON data to send in the POST request
    const char *post_data = create_gemini_json_payload(fullPrompt);
}

const char *extractAPIkey() {
    // Get API Key from environment variable for security
    const char *api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        fprintf(stderr,
                "Error: GEMINI_API_KEY environment variable not set.\n");
        return NULL;
    }
    return api_key;
}

int main() {
    CURL *curl_handle;
    CURLcode res;

    // Initialize the struct that will hold our response
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  // will be grown as needed by the callback
    chunk.size = 0;            // no data at this point

    const char *api_key = extractAPIkey();
    if (!api_key)
        return 0;

    // Construct the full URL with the API key
    char url[256];
    snprintf(url, sizeof(url),
             "https://generativelanguage.googleapis.com/v1beta/models/"
             "gemini-2.5-flash-lite:generateContent?key=%s",
             api_key);

    const char *post_data = preparePostData();

    // Initialize libcurl globally
    curl_global_init(CURL_GLOBAL_ALL);

    // Initialize a curl easy handle
    curl_handle = curl_easy_init();
    if (curl_handle) {
        // Set the required headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Set the curl options
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);
        // Set the callback function to handle the response
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                         WriteMemoryCallback);
        // Pass our 'chunk' struct to the callback function
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        // Set a user-agent
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl_handle);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        } else {
            // The request was successful, print the response
            printf("Response from server:\n%s\n", chunk.memory);
        }

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl_handle);
        free(chunk.memory);
    }

    // Global cleanup
    curl_global_cleanup();
    return 0;
}