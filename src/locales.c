#include <glib.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "general.h"
#include "locales.h"

char localetype[L_NUM][MAXTEXTLEN];

int GetLocales(char* lan) {
  FILE* fp;
  int i, j;
  int n;
  int status;
  char* datadir;
  char flocales[128];

  gchar* text;      /*[MAXTEXTLEN];*/
  gchar* utf8_text; /*[MAXTEXTLEN];*/
  gsize length;
  GError* error = NULL;

  datadir = DATADIR;
  snprintf(flocales, 128, "%s/locales/%s.txt", datadir, lan);
  printf("locales file: %s\n", flocales);
  if ((fp = fopen(flocales, "rt")) == NULL) {
    fprintf(stdout, "I can't open the file: %s\n", flocales);

    datadir = INSTALL_DATA_DIR;
    snprintf(flocales, 128, "%s/locales/%s.txt", datadir, lan);
    printf("checking for locales file: %s\n", flocales);

    if ((fp = fopen(flocales, "rt")) == NULL) {
      fprintf(stdout, "Can't open the file: %s\n", flocales);
      exit(-1);
    }
  }

  text = malloc(MAXTEXTLEN * sizeof(gchar));
  if (text == NULL) {
    fprintf(stderr, "ERROR in malloc");
    exit(-1);
  }

  utf8_text = malloc(MAXTEXTLEN * sizeof(gchar));
  if (utf8_text == NULL) {
    fprintf(stderr, "ERROR in malloc");
    exit(-1);
  }

  j = 0;
  for (i = 0; i < L_NUM; i++) {
    j = 0;

    status = fscanf(fp, "%d", &n);

    fscanf(fp, " %c", &text[j]);
    while (text[j] != '\n') {
      j++;
      fscanf(fp, "%c", &text[j]);
    }
    text[j] = '\0';

    /* gchar*      g_locale_from_utf8              (const gchar *utf8string, */
    /* 						 gssize len, */
    /* 						 gsize *bytes_read, */
    /* 						 gsize *bytes_written, */
    /* 						 GError **error); */
    length = strlen(text);
    if (status != 0) {
      utf8_text = g_locale_from_utf8(text, length, NULL, NULL, &error);
      /* utf8_text=text; */

      if (error != NULL) {
        fprintf(stderr, "Couldn't convert string %s to UTF-8\n", text);
        printf("\t%s\n", error->message);
        g_error_free(error);
        error = NULL;
        snprintf(localetype[i], MAXTEXTLEN, "%s", text);
      } else {
        snprintf(localetype[i], MAXTEXTLEN, "%s", utf8_text);
      }
    } else {
      i--;
    }
  }
  free(text);
  free(utf8_text);
  return (0);
}

int GetLocaleNum(void) {
  return L_NUM;
}

char* GetLocale(enum LocaleText n) {
  return (localetype[n]);
}
