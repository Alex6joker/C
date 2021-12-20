#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "Test.h"

int main(void)
{
    char *noInfo = "None";

    // Get linux hostname from C linrary function
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    printf("hostname: %s\n", hostname);

    // Get motherboard info
    fputs("motherboard model: ", stdout);

    FILE *motherboardInfo = fopen("/sys/devices/virtual/dmi/id/board_vendor", "rb");
    char *linePtr = 0;
    size_t size = 0;
    while (getdelim(&linePtr, &size, 0, motherboardInfo) != -1)
    {
        if (!stringEqual(linePtr, noInfo))
        {
            linePtr[strlen(linePtr) - 1] = ' ';
            fputs(linePtr, stdout);
        }
    }
    fclose(motherboardInfo);

    motherboardInfo = fopen("/sys/devices/virtual/dmi/id/board_name", "rb");
    linePtr = 0;
    size = 0;
    while (getdelim(&linePtr, &size, 0, motherboardInfo) != -1)
    {
        if (!stringEqual(linePtr, noInfo))
        {
            linePtr[strlen(linePtr) - 1] = ' ';
            fputs(linePtr, stdout);
        }
    }
    fclose(motherboardInfo);

    motherboardInfo = fopen("/sys/devices/virtual/dmi/id/board_version", "rb");
    linePtr = 0;
    size = 0;
    while (getdelim(&linePtr, &size, 0, motherboardInfo) != -1)
    {
        if (!stringEqual(linePtr, noInfo))
        {
            linePtr[strlen(linePtr) - 1] = ' ';
            fputs(linePtr, stdout);
        }
    }
    fclose(motherboardInfo);

    puts(""); // Only fputs is used, must add end of line

    // Get CPU info
    FILE *cpuInfo = fopen("/proc/cpuinfo", "rb");
    linePtr = 0;
    size = 0;
    fputs("cpu model: ", stdout);
    char *targetString1 = "model name\t:";
    char *targetString2 = "cpu MHz\t\t:";
    int targetString1Lenght = strlen(targetString1);
    int targetString2Lenght = strlen(targetString2);

    while (getline(&linePtr, &size, cpuInfo) != -1)
    {
        char *subStr = strstr(linePtr, targetString1);
        if (subStr != NULL)
        {
            char *end = strstr(subStr, "\n");
            end[0] = ' ';
            fputs(subStr + targetString1Lenght, stdout);
            fputs(" @ ", stdout);
        }
        subStr = strstr(linePtr, targetString2);
        if (subStr != NULL)
        {
            float cpuGHZ = (float)atoi(subStr + targetString2Lenght) / 1024;
            printf("%.2f", cpuGHZ);
            puts(" GHz");
            break;
        }
    }
    fclose(cpuInfo);

    // Get traffic data (I/O)

    FILE *networkInfo = fopen("/proc/net/dev", "rb");
    linePtr = 0;
    size = 0;
    int interval = 5;
    int interation;
    int totalIterationsCount = 2;
    int receiveDataPos = 0;
    int transmittedDataPos = 8;
    float* receivedDataSum = malloc(sizeof(float) * totalIterationsCount);
    float* transmittedDataSum = malloc(sizeof(float) * totalIterationsCount);

    for(interation = 0; interation < totalIterationsCount; interation++)
    {
        fseek(networkInfo, 0, SEEK_SET);
        while (getline(&linePtr, &size, networkInfo) != -1)
        {
            // Only no errors/network interfaces lines have ':'
            char *subStr = strstr(linePtr, ":");
            if (subStr != NULL)
            {
                char *data = strtok(subStr, " ");
                for (int dataPosition = 0; data != NULL; dataPosition++)
                {
                    data = strtok(NULL, " ");
                    if (dataPosition == receiveDataPos)
                    {
                        receivedDataSum[interation] += atof(data);
                    }
                    else if (dataPosition == transmittedDataPos)
                    {
                        transmittedDataSum[interation] += atof(data);
                        break;
                    }
                }
            }
        }
        if(interation != totalIterationsCount - 1)
            sleep(interval);
    }

    printf("traffic data: in: %.2f Kb/s, out: %.2f Kb/s \n",
        (receivedDataSum[1] - receivedDataSum[0]) / (interval * 1024),
        (transmittedDataSum[1] - transmittedDataSum[0]) / (interval * 1024));

    free(receivedDataSum);
    free(transmittedDataSum);

    fclose(networkInfo);

    // Get memory info
    FILE *memoryInfo = fopen("/proc/meminfo", "rb");
    linePtr = 0;
    size = 0;
    targetString1 = "MemTotal";
    targetString2 = "MemAvailable";
    float memoryTotal = 0.0f;
    float memoryFree = 0.0f;

    while (getline(&linePtr, &size, memoryInfo) != -1)
    {
        char *subStr = strstr(linePtr, targetString1);
        if (subStr != NULL)
        {
            char *memoryTotalStr = strtok(linePtr, ": \n");
            memoryTotalStr = strtok(NULL, " ");                 // String of total memory in Kb
            memoryTotal = atof(memoryTotalStr) / (1024 * 1024); // Digital total memory in Gb
        }
        subStr = strstr(linePtr, targetString2);
        if (subStr != NULL)
        {
            char *memoryFreeStr = strtok(linePtr, ": \n");
            memoryFreeStr = strtok(NULL, " ");                      // String of total memory in Kb
            memoryFree = atof(memoryFreeStr) / (1024 * 1024); // Digital total memory in Gb
        }
    }
    float memoryLoad = memoryTotal - memoryFree; // Digital total memory in Gb

    printf("memory: %.2fG / %.2fG \n", memoryLoad, memoryTotal);
    fclose(memoryInfo);

    // Get linux distribution version
    FILE *osInfo = fopen("/etc/os-release", "rb");
    linePtr = 0;
    size = 0;
    targetString1 = "PRETTY_NAME";

    fputs("release: ", stdout);
    while (getline(&linePtr, &size, osInfo) != -1)
    {
        char *subStr = strstr(linePtr, targetString1);
        if (subStr != NULL)
        {
            char *osName = strtok(subStr, "\"\n");
            osName = strtok(NULL, "\"\n"); // 2-nd split is os name
            puts(osName);
        }
    }

    fclose(osInfo);
}

// This funcion compare two char* strings
// by means of pointers
// Fuction returns: 0 if not equal, 1 - if equal
int stringEqual(char *firstString, char *secondString)
{
    int result = 1;

    // At least one string must be readed to the end
    while (*firstString != '\0' && *secondString != '\0')
    {
        if (*firstString != *secondString)
        {
            result = 0;
        }

        // Move char* pointers
        firstString++;
        secondString++;
    }

    return result;
}