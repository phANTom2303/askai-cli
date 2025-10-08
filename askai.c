#include <curl/curl.h>
#include <errno.h>  // Required for checking errno against EINTR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apiKeyManager.h"
#include "cJSON.h"
#include "jsonHandling.h"
#include "myio.h"

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