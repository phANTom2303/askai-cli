#ifndef JSONHANDLING_H
#define JSONHANDLING_H


//helper function to actually create a stringified json object that needs to be posted
char *create_gemini_json_payload(const char *prompt_text);


//function to prepare the exact prompt data to be sent to gemini api by combining prompt, history and context
const char *preparePostData(char *userPrompt, char *extraContext, char *history);

//function to extract and return the text part of the response from gemini api
char *parse_gemini_response(const char *response_json);
#endif