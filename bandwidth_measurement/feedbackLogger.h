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
EXTERNC void sendFeedbackMessage(char* str);
#undef EXTERNC


#endif //UDP_TOOLS_LOGGER_H
