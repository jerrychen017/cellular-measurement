#include <stdio.h>
#include "feedbackLogger.h"
static FILE *upload_fd = NULL;
static FILE *download_fd = NULL;
static FILE *latency_fd = NULL;

/**
 * Send upload speed to Android or print to stdout if run on command line
 */
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

/**
 * Send latency to Android or print to stdout if run on command line
 */
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

/**
 * Send download speed to Android or print to stdout if run on command line
 */
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

/**
 * Sets all static FILE pointers to NULL
 */
void clear_file_pointers() {
    upload_fd = NULL;
    download_fd = NULL;
    latency_fd = NULL;
}
