#include "apiKeyManager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // For mkdir() and chmod()
#include <sys/types.h>  // For mkdir()

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