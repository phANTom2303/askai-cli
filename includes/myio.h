#ifndef MYIO_H
#define MTIO_H

//helper function to pause the printing for some time
void sleep_ms(long milliseconds);

//function to display output with delays in between to simulate chatbot like response
void displayStringWithDelay(char *str);

//function to read a variable length string from user and return it.
char *readString();
#endif