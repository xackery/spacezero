#include "randomnamegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char letterFrequency[26][300];
int tableLoaded;

void RandomNameGenerator() {
  int i;
  for (i = 0; i < 26; i++) {
    strcpy(letterFrequency[i], "");
  }

  tableLoaded = FALSE;
}

void loadFrequencyTable(char* filename) {
  int size = 0;
  char* dataBuffer;
  FILE* fp = fopen(filename, "rb");
  char* token;
  int i;

  if (!fp) {
    printf(
        "ERROR: RandomNameGenerator::loadFrequencyTable - Couldn't load "
        "Frequency Table '%s'\n",
        filename);
    return;
  }

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  rewind(fp);

  dataBuffer = malloc(size * sizeof(char));
  if (dataBuffer == NULL) {
    fprintf(stderr, "ERROR in malloc\n");
    exit(-1);
  }

  if (fread(dataBuffer, 1, size, fp) != size) {
    fprintf(stderr, "Error. fread(): Error reading file: %s\n", filename);
    exit(-1);
  }
  fclose(fp);

  token = strtok(dataBuffer, "\n");

  for (i = 0; i < 26; i++) {
    sscanf(token, "%s", letterFrequency[i]);
    token = strtok(NULL, "\n");
  }

  tableLoaded = TRUE;
  free(dataBuffer);
}

char* createName(int length) {
  static char name[10];

  int letter = (int)(rand() % 26);
  int len = 0;
  char c;
  char i = 0;
  char cons = 0;
  char vowels = 0;

  if (!tableLoaded) {
    printf(
        "ERROR: RandomNameGenerator::createName - Frequency Table Not "
        "Loaded\n");
    return "";
  }

  if (length == 0) {
    length = 4 + rand() % 4;
  }

  strcpy(name, "");

  while (TRUE) {
    len = strlen(letterFrequency[letter]);
    c = (char)letterFrequency[letter][rand() % len];

    switch (c) {
      case 'A':
      case 'a':
      case 'E':
      case 'e':
      case 'I':
      case 'i':
      case 'O':
      case 'o':
      case 'U':
      case 'u':
        vowels++;
        cons = 0;
        break;
      default:
        vowels = 0;
        cons++;
        break;
    }

    if ((vowels > 2) || (cons > 2)) {
      continue;
    }

    if (i > 0) {
      c += 32;
    }

    letter = (char)c;
    letter -= 65;

    if (i > 0) {
      letter -= 32;
    }

    sprintf(name, "%s%c", name, c);
    i++;

    if (i == length) {
      break;
    }
  }

  return name;
}

char* getRandomName(int length) {
  return createName(length);
}

char* getRandomFullName() {
  static char rtn[256];

  int length1 = 3 + rand() % 4;
  int length2 = 5 + rand() % 3;

  sprintf(rtn, "%s", createName(length1));
  sprintf(rtn, "%s %s", rtn, createName(length2));

  return rtn;
}

char* getRandomOrkishName() {
  static char rtn[256];

  int length1 = 1 + rand() % 2;
  int length2 = 3 + rand() % 4;
  int length3 = 5 + rand() % 3;

  sprintf(rtn, "%s", createName(length1));
  sprintf(rtn, "%s'%s", rtn, createName(length2));
  sprintf(rtn, "%s %s", rtn, createName(length3));

  return rtn;
}
