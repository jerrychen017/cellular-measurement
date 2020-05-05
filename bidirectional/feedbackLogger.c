#include <stdio.h>
#include "feedbackLogger.h"
static FILE *upload_fd = NULL;
static FILE *download_fd = NULL;
static FILE *latency_fd = NULL;

void sendFeedbackUpload(double d)
{
    printf("Upload speed %f Mbps\n", d);
    if (upload_fd == NULL)
    {
        upload_fd = fopen("upload.txt", "w");
    }
    else
    {
        fprintf(upload_fd, "%.3f\n", d);
        fflush(upload_fd);
    }
}

void sendFeedbackLatency(double d)
{
    printf("Latency %.3f ms\n", d);
    if (latency_fd == NULL)
    {
        latency_fd = fopen("latency.txt", "w");
    }
    else
    {
        fprintf(latency_fd, "%.3f\n", d);
        fflush(latency_fd);
    }
}

void sendFeedbackDownload(double d)
{
    printf("Download speed %f Mbps\n", d);
    if (download_fd == NULL)
    {
        download_fd = fopen("download.txt", "w");
    }
    else
    {
        fprintf(download_fd, "%.3f\n", d);
        fflush(download_fd);
    }
}

void javaStartBandwidth()
{
}
