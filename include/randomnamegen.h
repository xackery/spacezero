#ifndef _RANDOMNAMEGEN_
#define _RANDOMNAMEGEN_

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

char* createName(int length);
void loadFrequencyTable(char* filename);
char* getRandomName();
char* getRandomName(int length);
char* getRandomFullName();
char* getRandomOrkishName();

/*
Since there can be multiple version of swearwords (such as shithead and
fuckface, etc) the rountine will filter words if the bad word appears at the
start of the name generated. We've also added in "Bob" since we don't want
anyone else to be called that!
*/
/* const char badWord[][25] = { */

/* 	"fuck", */
/* 	"shit", */
/* 	"cunt", */
/* 	"rape", */
/* 	"wank", */
/* 	"arse", */
/* 	"nigger", */
/* 	"lesbian", */
/* 	"queer", */
/* 	"gay", */
/* 	"homosexual", */
/* 	"bob" */
/* }; */

#endif
