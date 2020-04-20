#include <stdio.h>
#include "feedbackLogger.h"

void sendFeedbackMessage(char* str)
{
    printf("%s\n", str);
}

void sendFeedbackBandwidth(double d)
{
    printf("%f\n", d);
}