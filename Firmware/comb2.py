from multiprocessing import Process
import pygame
import RPi.GPIO as GPIO
import time
import board
import neopixel
import cv2
from flask import Flask, Response
from picamera2 import Picamera2

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

pixels = neopixel.NeoPixel(NEOPIXEL_PIN, NUM_PIXELS, brightness=BRIGHTNESS, auto_write=True)

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
    pixels.fill((255, 0, 0))  # Red

def flywheels_off():
    GPIO.output(FLYWHEEL_AA, GPIO.LOW)
    GPIO.output(FLYWHEEL_AB, GPIO.LOW)
    pixels.fill((255, 255, 0))  # Yellow

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

    try:
        while True:
            pygame.event.pump()
            x = joystick.get_axis(0)
            y = -joystick.get_axis(1)

            flywheel_button = joystick.get_button(5)
            if abs(y) > 0.1:
                direction = 1 if y > 0 else -1
                if abs(x) < 0.3:
                    drive(direction, direction)
                elif x > 0.3:
                    drive(direction, 0)
                elif x < -0.3:
                    drive(0, direction)
            else:
                drive(0, 0)

            if flywheel_button:
                flywheels_on()
            else:
                flywheels_off()

            time.sleep(0.05)
    except KeyboardInterrupt:
        print("Shutting down controller thread...")
    finally:
        drive(0, 0)
        flywheels_off()
        GPIO.cleanup()

# ----- Flask Camera Stream -----
app = Flask(__name__)
picam2 = Picamera2()
picam2.configure(picam2.create_video_configuration(main={"size": (640, 480)}))
picam2.start()

def generate_frames():
    while True:
        frame = picam2.capture_array()
        frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        ret, buffer = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
        if not ret:
            continue
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
        time.sleep(0.01)

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
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

# ----- Main -----
if __name__ == '__main__':
    controller_process = Process(target=controller_function)
    controller_process.start()
    print("Controller process started.")
    print("Starting Flask server...")
    app.run(host='0.0.0.0', port=5000)

