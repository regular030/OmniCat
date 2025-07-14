import cv2
from leapc_cffi import ffi, libleapc
import numpy as np
import time

# init connection
connection_ptr = ffi.new("LEAP_CONNECTION*")
result = libleapc.LeapCreateConnection(ffi.NULL, connection_ptr)
if result != 0:
    print("Failed to create connection")
    exit(1)
connection = connection_ptr[0]

result = libleapc.LeapOpenConnection(connection)
if result != 0:
    print("Failed to open connection")
    exit(1)

print("Leap Motion connection opened!")

# print hand info
def print_hand_info(frame):
    nHands = frame.nHands
    print(f"Frame ID: {frame.tracking_frame_id}, Hands detected: {nHands}", flush=True)
    for i in range(nHands):
        hand = frame.pHands[i]
        print(f"Hand {hand.id}:")
        for j in range(5):
            finger = hand.digits[j]
            tip = finger.distal.position  # distal bone = fingertip
            print(f"Finger {j} position: ({tip.x:.2f}, {tip.y:.2f}, {tip.z:.2f})", flush=True)

start_time = time.time()
timeout = 30
loop_counter = 0

#main loop
try:
    while True:
        if time.time() - start_time > timeout:
            print("Timeout reached, exiting...")
            break

        msg = ffi.new("LEAP_CONNECTION_MESSAGE*")
        result = libleapc.LeapPollConnection(connection, 20, msg)
        if result != 0:
            continue

        msg_type = msg.type
        if msg_type == libleapc.eLeapEventType_Tracking:
            frame = msg.tracking_event

            # only print every 10 loops
            if loop_counter % 10 == 0:
                print_hand_info(frame)

            # simple visual feedback
            img = np.zeros((480, 640, 3), dtype=np.uint8)
            cv2.putText(img, f"Hands detected: {frame.nHands}", (50, 50),
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.imshow('Leap Motion Feed', img)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

        elif msg_type == libleapc.eLeapEventType_Device:
            print("Leap device connected.")
        loop_counter += 1
        time.sleep(0.02)

except KeyboardInterrupt:
    print("Exiting...")

cv2.destroyAllWindows()
libleapc.LeapDestroyConnection(connection)
