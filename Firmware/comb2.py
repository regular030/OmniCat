import os
import threading
import pygame
import RPi.GPIO as GPIO
import time
import board
import neopixel
import cv2
from flask import Flask, Response
from picamera2 import Picamera2

# Fix pygame segmentation fault when headless
os.environ["SDL_VIDEODRIVER"] = "dummy"
os.environ["SDL_AUDIODRIVER"] = "dummy"

# ----- Setup GPIO and Neopixels -----
LEFT_IN1 = 17
LEFT_IN2 = 27
RIGHT_IN3 = 18
RIGHT_IN4 = 22
FLYWHEEL_AA = 23
FLYWHEEL_AB = 24

NEOPIXEL_PIN = board.D21
NUM_PIXELS = 2
BRIGHTNESS = 0.5

pixels = neopixel.NeoPixel(NEOPIXEL_PIN, NUM_PIXELS, brightness=BRIGHTNESS, auto_write=False)
neopixel_lock = threading.Lock()

GPIO.setmode(GPIO.BCM)
GPIO.setup([LEFT_IN1, LEFT_IN2, RIGHT_IN3, RIGHT_IN4, FLYWHEEL_AA, FLYWHEEL_AB], GPIO.OUT)

# ----- Motor control functions -----
def drive(left_speed, right_speed):
    GPIO.output(LEFT_IN1, GPIO.HIGH if left_speed > 0 else GPIO.LOW)
    GPIO.output(LEFT_IN2, GPIO.HIGH if left_speed < 0 else GPIO.LOW)
    GPIO.output(RIGHT_IN3, GPIO.HIGH if right_speed > 0 else GPIO.LOW)
    GPIO.output(RIGHT_IN4, GPIO.HIGH if right_speed < 0 else GPIO.LOW)

def flywheels_on():
    GPIO.output(FLYWHEEL_AA, GPIO.HIGH)
    GPIO.output(FLYWHEEL_AB, GPIO.LOW)

def flywheels_off():
    GPIO.output(FLYWHEEL_AA, GPIO.LOW)
    GPIO.output(FLYWHEEL_AB, GPIO.LOW)

# ----- Shared state for flywheel control -----
flywheel_state = {"on": False}
flywheel_lock = threading.Lock()

# ----- Controller logic -----
def controller_function():
    pygame.init()
    pygame.joystick.init()

    try:
        joystick = pygame.joystick.Joystick(0)
        joystick.init()
        print(f"Connected to: {joystick.get_name()}")
    except pygame.error:
        print("No joystick detected.")
        return

    last_x, last_y = 0, 0

    try:
        while True:
            pygame.event.pump()
            x = joystick.get_axis(0)
            y = -joystick.get_axis(1)

            if abs(x - last_x) > 0.1 or abs(y - last_y) > 0.1:
                print(f"Joystick moved: X={x:.2f}, Y={y:.2f}")
                last_x, last_y = x, y

            if abs(y) > 0.1 or abs(x) > 0.1:
                left = y + x
                right = y - x
                drive(left, right)
            else:
                drive(0, 0)

            if joystick.get_button(0):
                with flywheel_lock:
                    if not flywheel_state["on"]:
                        print("X button pressed — Flywheels ON")
                    flywheel_state["on"] = True
            elif joystick.get_button(1):
                with flywheel_lock:
                    if flywheel_state["on"]:
                        print("O button pressed — Flywheels OFF")
                    flywheel_state["on"] = False

            time.sleep(0.05)

    except KeyboardInterrupt:
        print("Shutting down controller thread...")
    finally:
        drive(0, 0)
        with flywheel_lock:
            flywheel_state["on"] = False

# ----- Hardware update loop -----
def hardware_update_loop():
    last_state = None
    try:
        while True:
            with flywheel_lock:
                current = flywheel_state["on"]

            if current != last_state:
                if current:
                    flywheels_on()
                    with neopixel_lock:
                        pixels.fill((255, 0, 0))
                        pixels.show()
                else:
                    flywheels_off()
                    with neopixel_lock:
                        pixels.fill((255, 255, 0))
                        pixels.show()
                last_state = current

            time.sleep(0.05)
    except KeyboardInterrupt:
        pass
    finally:
        drive(0, 0)
        flywheels_off()
        with neopixel_lock:
            pixels.fill((0, 0, 0))
            pixels.show()
            pixels.deinit()
        GPIO.cleanup()

# ----- Flask Camera Stream -----
app = Flask(__name__)
picam2 = Picamera2()
picam2.configure(picam2.create_video_configuration(main={"size": (640, 480)}))
picam2.start()

def generate_frames():
    while True:
        try:
            frame = picam2.capture_array()
            frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
            ret, buffer = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
            if not ret:
                continue
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
        except Exception as e:
            print(f"Camera error: {e}")
            time.sleep(1)

@app.route('/')
def index():
    return '''
    <html>
    <head>
     <title>Pi Camera Stream</title>
     <style>
      body, html {
       margin: 0; padding: 0; height: 100%; background: #000;
       display: flex; justify-content: center; align-items: center;
      }
      img {
        width: 100vw;
        height: 100vh;
        object-fit: cover;
      }
     </style>
    </head>
    <body>
     <img src="/video_feed" alt="Pi Camera Stream" />
    </body>
    </html>
    '''

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

# ----- Main -----
if __name__ == '__main__':
    # Start controller thread
    controller_thread = threading.Thread(target=controller_function, daemon=True)
    controller_thread.start()
    print("Controller thread started.")

    # Start Flask server in a thread
    flask_thread = threading.Thread(target=lambda: app.run(host='0.0.0.0', port=5000), daemon=True)
    flask_thread.start()
    print("Flask server started on http://<pi-ip>:5000")

    # Run the hardware update loop
    try:
        hardware_update_loop()
    except KeyboardInterrupt:
        print("Exiting main program...")
