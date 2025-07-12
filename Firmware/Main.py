import pygame
import RPi.GPIO as GPIO
import time
import board
import neopixel

# === CONFIGURATION ===

# Drive motor control pins
LEFT_IN1 = 17
LEFT_IN2 = 27
RIGHT_IN3 = 18
RIGHT_IN4 = 22

# Flywheel H-bridge control pins
FLYWHEEL_AA = 23
FLYWHEEL_AB = 24

# NeoPixel setup
NEOPIXEL_PIN = board.D21   # Physical pin 40 (GPIO 21) — adjust if different
NUM_PIXELS = 2
BRIGHTNESS = 0.5

pixels = neopixel.NeoPixel(NEOPIXEL_PIN, NUM_PIXELS, brightness=BRIGHTNESS, auto_write=True)

# === SETUP ===
GPIO.setmode(GPIO.BCM)
GPIO.setup([LEFT_IN1, LEFT_IN2, RIGHT_IN3, RIGHT_IN4, FLYWHEEL_AA, FLYWHEEL_AB], GPIO.OUT)

# === HELPER FUNCTIONS ===

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

# === MAIN ===

pygame.init()
pygame.joystick.init()

try:
    joystick = pygame.joystick.Joystick(0)
    joystick.init()
    print(f"Connected to: {joystick.get_name()}")
except pygame.error:
    print("No joystick detected. Please connect a PS4 controller.")
    exit()

try:
    while True:
        pygame.event.pump()

        x = joystick.get_axis(0)   # Left stick X
        y = -joystick.get_axis(1)  # Left stick Y (invert Y)

        flywheel_button = joystick.get_button(5)  # R1

        # Movement
        if abs(y) > 0.1:
            direction = 1 if y > 0 else -1
            if abs(x) < 0.3:
                drive(direction, direction)
            elif x > 0.3:
                drive(direction, 0)  # Turn right
            elif x < -0.3:
                drive(0, direction)  # Turn left
        else:
            drive(0, 0)

        # Flywheel control
        if flywheel_button:
            flywheels_on()
        else:
            flywheels_off()

        time.sleep(0.05)

except KeyboardInterrupt:
    print("Shutting down...")

finally:
    drive(0, 0)
    flywheels_off()
    GPIO.cleanup()
