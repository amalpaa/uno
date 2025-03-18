#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "antixss.h"

char** htmlTagList = NULL;
int numLines = 0;

int count_lines(FILE* file)
{
    int l = 0;
    char ch;
    while ((ch=fgetc(file)) != EOF)
    {
        if(ch == '\n')
        {
            l++;
        }
    }

    rewind(file);
    return l;
}

void getTags()
{
    //reads all problematic html tags into an array
    FILE* file = fopen("html_tags.txt", "r");

    numLines = count_lines(file);

    char** tagList = malloc(numLines * sizeof(char*));
    
    char buff[20];
    for(int i = 0; i < numLines; i++)
    {
        fgets(buff, 18, file);
        size_t len = strlen(buff);

        if(buff[len-1] == '\n')
        {
            buff[len-1] = '\0';
        }

        tagList[i] = malloc(len+1);
        strcpy(tagList[i], buff);
    }

    fclose(file);
    htmlTagList = tagList;
}

char isSafe(char* input)
{
    if(htmlTagList == 0)
    {
        getTags();
    }

    if(strstr(input,"<") == 0)
    {
        return 1;
    }

    for(int i = 0; i < numLines; i++)
    {
        if(strstr(input,htmlTagList[i]) != 0)
        {
            printf("Potential XSS detected in user input: %s\n", input);
            memset(input, 0, strlen(input)); //bc uncleared buffer could block incoming messages
            return 0;
        }
    }

    return 1;
}



