#ifndef APIKEYMANAGER_H
#define APIKEYMANAGER_H
// function to extract api key from the config file, if not present, get it from
// the user and store in the config file
char *getApiKey();
#endif