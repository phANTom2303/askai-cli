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

// Parses the Gemini JSON response and extracts the generated text.
// Returns a newly allocated string which must be freed by the caller.
char *parse_gemini_response_with_validation(const char *response_json) {
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
    if (cJSON_IsArray(candidates)) {
        cJSON *candidate = cJSON_GetArrayItem(candidates, 0);
        if (cJSON_IsObject(candidate)) {
            cJSON *content =
                cJSON_GetObjectItemCaseSensitive(candidate, "content");
            if (cJSON_IsObject(content)) {
                cJSON *parts =
                    cJSON_GetObjectItemCaseSensitive(content, "parts");
                if (cJSON_IsArray(parts)) {
                    cJSON *part = cJSON_GetArrayItem(parts, 0);
                    if (cJSON_IsObject(part)) {
                        cJSON *text_obj =
                            cJSON_GetObjectItemCaseSensitive(part, "text");
                        if (cJSON_IsString(text_obj) &&
                            (text_obj->valuestring != NULL)) {
                            // Duplicate the string, as it will be freed with
                            // the root object
                            extracted_text = strdup(text_obj->valuestring);
                        }
                    }
                }
            }
        }
    }

    cJSON_Delete(root);  // Free the parsed JSON object
    return extracted_text;
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

int main(void) {
    CURL *curl_handle;
    CURLcode res;

    // Initialize the struct that will hold our response
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  // will be grown as needed by the callback
    chunk.size = 0;            // no data at this point

    // Get API Key from environment variable for security
    const char *api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        fprintf(stderr,
                "Error: GEMINI_API_KEY environment variable not set.\n");
        return 1;
    }

    // Construct the full URL with the API key
    char url[256];
    snprintf(url, sizeof(url),
             "https://generativelanguage.googleapis.com/v1beta/models/"
             "gemini-2.5-flash-lite:generateContent?key=%s",
             api_key);

    // The JSON data to send in the POST request
    const char *post_data =
        "{\"contents\":[{\"parts\":[{\"text\":\"Explain how AI works in a few "
        "words\"}]}]}";

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
            char *responseText = parse_gemini_response(chunk.memory);
            printf("Gemini's textual response:\n%s\n", responseText);
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