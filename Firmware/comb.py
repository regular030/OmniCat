from multiprocessing import Process

# ----- setup the controller connection and pins -----
import pygame
import RPi.GPIO as GPIO
import time
import board
import neopixel

#driver control pins
LEFT_IN1 = 17
LEFT_IN2 = 27
RIGHT_IN3 = 18
RIGHT_IN4 = 22

#flywheel control pins
FLYWHEEL_AA = 23
FLYWHEEL_AB = 24

#neopixel pins
NEOPIXEL_PIN = board.D21
NUM_PIXELS = 2
BRIGHTNESS = 0.5

pixels = neopixel.NeoPixel(NEOPIXEL_PIN, NUM_PIXELS, brightness=BRIGHTNESS, auto_write=True)

#confirm setup
GPIO.setmode(GPIO.BCM)
GPIO.setup([LEFT_IN1, LEFT_IN2, RIGHT_IN3, RIGHT_IN4, FLYWHEEL_AA, FLYWHEEL_AB], GPIO.OUT)

#helpers :D
def drive(left_speed, right_speed): #four wheel drive adjust
    GPIO.output(LEFT_IN1, GPIO.HIGH if left_speed > 0 else GPIO.LOW)
    GPIO.output(LEFT_IN2, GPIO.HIGH if left_speed < 0 else GPIO.LOW)
    GPIO.output(RIGHT_IN3, GPIO.HIGH if right_speed > 0 else GPIO.LOW)
    GPIO.output(RIGHT_IN4, GPIO.HIGH if right_speed < 0 else GPIO.LOW)

def flywheels_on(): #launch/don't launch
    GPIO.output(FLYWHEEL_AA, GPIO.HIGH)
    GPIO.output(FLYWHEEL_AB, GPIO.LOW)
    pixels.fill((255, 0, 0))  # Red

def flywheels_off():
    GPIO.output(FLYWHEEL_AA, GPIO.LOW)
    GPIO.output(FLYWHEEL_AB, GPIO.LOW)
    pixels.fill((255, 255, 0))  # Yellow

#setup controller
pygame.init()
pygame.joystick.init()

def controller_function():
    try:
        joystick = pygame.joystick.Joystick(0)
        joystick.init()
        print(f"Connected to: {joystick.get_name()}")
    except pygame.error:
        print("No joystick detected. Please connect a PS4 controller.")
        exit()

# ----- setup the controller connection and pins -----

from Firmware.flask import Flask, Response
from picamera2 import Picamera2
import cv2
import time

#use flask as web server to recieve data
app = Flask(__name__)
picam2 = Picamera2()
picam2.configure(picam2.create_video_configuration(main={"size": (640, 480)}))
picam2.start()

#helpers 2.0
def generate_frames():
  while True:
    frame = picam2.capture_array()
    frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
    ret, buffer = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
    if not ret:
      continue
    frame_bytes = buffer.tobytes()
    yield (b'--frame\r\n'
        b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
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
object-fit: cover; /* maintain aspect ratio, crop if needed */
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

if __name__ == '__main__':
  thread = Process(target=controller_function)
  thread.start()
  app.run(host='0.0.0.0', port=5000)