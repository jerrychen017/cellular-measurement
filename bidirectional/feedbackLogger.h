//
// Created by Jerry Chen on 3/25/20.
//

#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

/**
 * Send upload speed to Android or print to stdout if run on command line
 */
EXTERNC void sendFeedbackUpload(double d);

/**
 * Send download speed to Android or print to stdout if run on command line
 */
EXTERNC void sendFeedbackDownload(double d);

/**
 * Send latency to Android or print to stdout if run on command line
 */
EXTERNC void sendFeedbackLatency(double d);

/**
 * Sets all static FILE pointers to NULL
 */
EXTERNC void clear_file_pointers();
#undef EXTERNC

#endif //UDP_TOOLS_LOGGER_H
