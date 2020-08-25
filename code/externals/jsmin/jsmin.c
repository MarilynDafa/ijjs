/* jsmin.c
   2019-10-30

Copyright (C) 2002 Douglas Crockford  (www.crockford.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
static FILE* fp = NULL;
static FILE* outfp = NULL;
static int the_a;
static int the_b;
static int look_ahead = EOF;
static int the_x = EOF;
static int the_y = EOF;


static void error(char* string) {
    fputs("JSMIN Error: ", stderr);
    fputs(string, stderr);
    fputc('\n', stderr);
    exit(1);
}

/* is_alphanum -- return true if the character is a letter, digit, underscore,
        dollar sign, or non-ASCII character.
*/

static int is_alphanum(int codeunit) {
    return (
        (codeunit >= 'a' && codeunit <= 'z')
        || (codeunit >= '0' && codeunit <= '9')
        || (codeunit >= 'A' && codeunit <= 'Z')
        || codeunit == '_'
        || codeunit == '$'
        || codeunit == '\\'
        || codeunit > 126
    );
}


/* get -- return the next character from stdin. Watch out for lookahead. If
        the character is a control character, translate it to a space or
        linefeed.
*/

static int get() {
    int codeunit = look_ahead;
    look_ahead = EOF;
    if (codeunit == EOF) {
        codeunit = getc(fp);
    }
    if (codeunit >= ' ' || codeunit == '\n' || codeunit == EOF) {
        return codeunit;
    }
    if (codeunit == '\r') {
        return '\n';
    }
    return ' ';
}


/* peek -- get the next character without advancing.
*/

static int peek() {
    look_ahead = get();
    return look_ahead;
}


/* next -- get the next character, excluding comments. peek() is used to see
        if a '/' is followed by a '/' or '*'.
*/

static int next() {
    int codeunit = get();
    if  (codeunit == '/') {
        switch (peek()) {
        case '/':
            for (;;) {
                codeunit = get();
                if (codeunit <= '\n') {
                    break;
                }
            }
            break;
        case '*':
            get();
            while (codeunit != ' ') {
                switch (get()) {
                case '*':
                    if (peek() == '/') {
                        get();
                        codeunit = ' ';
                    }
                    break;
                case EOF:
                    error("Unterminated comment.");
                }
            }
            break;
        }
    }
    the_y = the_x;
    the_x = codeunit;
    return codeunit;
}


/* action -- do something! What you do is determined by the argument:
        1   Output A. Copy B to A. Get the next B.
        2   Copy B to A. Get the next B. (Delete A).
        3   Get the next B. (Delete B).
   action treats a string as a single character.
   action recognizes a regular expression if it is preceded by the likes of
   '(' or ',' or '='.
*/

static void action(int determined) {
    switch (determined) {
    case 1:
        putc(the_a, outfp);
        if (
            (the_y == '\n' || the_y == ' ')
            && (the_a == '+' || the_a == '-' || the_a == '*' || the_a == '/')
            && (the_b == '+' || the_b == '-' || the_b == '*' || the_b == '/')
        ) {
            putc(the_y, outfp);
        }
    case 2:
        the_a = the_b;
        if (the_a == '\'' || the_a == '"' || the_a == '`') {
            for (;;) {
                putc(the_a, outfp);
                the_a = get();
                if (the_a == the_b) {
                    break;
                }
                if (the_a == '\\') {
                    putc(the_a, outfp);
                    the_a = get();
                }
                if (the_a == EOF) {
                    error("Unterminated string literal.");
                }
            }
        }
    case 3:
        the_b = next();
        if (the_b == '/' && (
            the_a == '(' || the_a == ',' || the_a == '=' || the_a == ':'
            || the_a == '[' || the_a == '!' || the_a == '&' || the_a == '|'
            || the_a == '?' || the_a == '+' || the_a == '-' || the_a == '~'
            || the_a == '*' || the_a == '/' || the_a == '{' || the_a == '}'
            || the_a == ';'
        )) {
            putc(the_a, outfp);
            if (the_a == '/' || the_a == '*') {
                putc(' ', outfp);
            }
            putc(the_b, outfp);
            for (;;) {
                the_a = get();
                if (the_a == '[') {
                    for (;;) {
                        putc(the_a, outfp);
                        the_a = get();
                        if (the_a == ']') {
                            break;
                        }
                        if (the_a == '\\') {
                            putc(the_a, outfp);
                            the_a = get();
                        }
                        if (the_a == EOF) {
                            error(
                                "Unterminated set in Regular Expression literal."
                            );
                        }
                    }
                } else if (the_a == '/') {
                    switch (peek()) {
                    case '/':
                    case '*':
                        error(
                            "Unterminated set in Regular Expression literal."
                        );
                    }
                    break;
                } else if (the_a =='\\') {
                    putc(the_a, outfp);
                    the_a = get();
                }
                if (the_a == EOF) {
                    error("Unterminated Regular Expression literal.");
                }
                putc(the_a, outfp);
            }
            the_b = next();
        }
    }
}


/* jsmin -- Copy the input to the output, deleting the characters which are
        insignificant to JavaScript. Comments will be removed. Tabs will be
        replaced with spaces. Carriage returns will be replaced with linefeeds.
        Most spaces and linefeeds will be removed.
*/

static void jsmin() {
    if (peek() == 0xEF) {
        get();
        get();
        get();
    }
    the_a = '\n';
    action(3);
    while (the_a != EOF) {
        switch (the_a) {
        case ' ':
            action(
                is_alphanum(the_b)
                ? 1
                : 2
            );
            break;
        case '\n':
            switch (the_b) {
            case '{':
            case '[':
            case '(':
            case '+':
            case '-':
            case '!':
            case '~':
                action(1);
                break;
            case ' ':
                action(3);
                break;
            default:
                action(
                    is_alphanum(the_b)
                    ? 1
                    : 2
                );
            }
            break;
        default:
            switch (the_b) {
            case ' ':
                action(
                    is_alphanum(the_a)
                    ? 1
                    : 3
                );
                break;
            case '\n':
                switch (the_a) {
                case '}':
                case ']':
                case ')':
                case '+':
                case '-':
                case '"':
                case '\'':
                case '`':
                    action(1);
                    break;
                default:
                    action(
                        is_alphanum(the_a)
                        ? 1
                        : 3
                    );
                }
                break;
            default:
                action(1);
                break;
            }
        }
    }
}

int compressjs(const char* file) {
    fp = fopen(file, "rb");
    outfp = fopen("temp", "wb");
    jsmin();
    fclose(fp);
    fclose(outfp);
    outfp = fopen("temp", "rb");
    fseek(outfp, 0, SEEK_END);
    size_t sz = ftell(outfp);
    fseek(outfp, 0, SEEK_SET);
    unsigned char* data = (unsigned char*)malloc(sz);
    fread(data, 1, sz, outfp);
    fclose(outfp);
    fp = fopen(file, "wb");
    fwrite(data, 1, sz, fp);
    free(data);
    fclose(fp);
    unlink("temp");
    return 0;
}
void listFiles(const char* dir)
{
    char dirNew[260];
    strcpy(dirNew, dir);
    strcat(dirNew, "/*.*");
    intptr_t handle;
    struct _finddata_t findData;

    handle = _findfirst(dirNew, &findData);
    if (handle == -1)  
        return;

    do
    {
        if (findData.attrib & _A_SUBDIR)
        {
            if (strcmp(findData.name, ".") == 0 || strcmp(findData.name, "..") == 0)
                continue;
            strcpy(dirNew, dir);
            strcat(dirNew, "/");
            strcat(dirNew, findData.name);
            listFiles(dirNew);
        }
        else
        {
            char rpath[260] = { 0 };
            strncpy(rpath, dirNew, strlen(dirNew) - 3);
            strcat(rpath, findData.name);
            size_t sz = strlen(rpath);
            if (rpath[sz-3] == '.' && rpath[sz-2] == 'j' && rpath[sz-1] == 's')
                compressjs(rpath);
        }
    } while (_findnext(handle, &findData) == 0);
    _findclose(handle); 
}


void compress(const char* dir) {
    listFiles(dir);
    printf("js files minify complete");
}

/* main -- Output any command line arguments as comments
        and then minify the input.

extern int main(int argc, char* argv[]) {
    int i;
    for (i = 1; i < argc; i += 1) {
        fprintf(fp, "// %s\n", argv[i]);
    }
    jsmin();
    return 0;
}

*/