#include <stdio.h>
#include "LeapC.h"
#include "LeapMath.h"

//handle frame events
void OnFrame(const LEAP_TRACKING_EVENT *frame) {
    printf("Frame ID: %llu\n", frame->tracking_frame_id);
    printf("Hands: %d\n", frame->nHands);
    for (uint32_t i = 0; i < frame->nHands; i++) {
        LEAP_HAND hand = frame->pHands[i];
        printf("  Hand ID: %d, Palm Position: (%f, %f, %f)\n", //display hand info
               hand.id,
               hand.palm.position.x,
               hand.palm.position.y,
               hand.palm.position.z);
    }
}

//main loop
int main() {
    LEAP_CONNECTION connection; //initialize the connection
    eLeapRS result = LeapCreateConnection(NULL, &connection);
    if (result != eLeapRS_Success) {
        fprintf(stderr, "Failed to create connection.\n");
        return -1;
    }
    result = LeapOpenConnection(connection);
    if (result != eLeapRS_Success) {
        fprintf(stderr, "Failed to open connection.\n");
        return -1;
    }

    printf("Waiting for device...\n");

    //grab messages in loop
    while (1) {
        LEAP_CONNECTION_MESSAGE msg;
        result = LeapPollConnection(connection, 1000, &msg);
        if (result != eLeapRS_Success) continue;

        switch (msg.type) {
            case eLeapEventType_Tracking:
                OnFrame(msg.tracking_event);
                break;
            case eLeapEventType_Device:
                printf("Leap device connected.\n");
                break;
            default:
                break;
        }
    }
    LeapDestroyConnection(connection); //when done
    return 0;
}
