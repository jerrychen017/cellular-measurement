#include <stdio.h>
#include "feedbackLogger.h"

void sendFeedbackUpload(double d) {
    printf("Upload speed %f Mbps\n", d);
}

void sendFeedbackDownload(double d) {
    printf("Download speed %f Mbps\n", d);
}

void javaStartBandwidth() {

}
