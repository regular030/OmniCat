import pygame
import RPi.GPIO as GPIO
import time

# Drive motor control pins (L298N IN1-IN4) PLEASE CHECK 
LEFT_IN1 = 17
LEFT_IN2 = 27
RIGHT_IN3 = 18
RIGHT_IN4 = 22

# Flywheel motor pins (direct control) PLEASE CHECK
FLYWHEEL1 = 23
FLYWHEEL2 = 24

GPIO.setmode(GPIO.BCM)
GPIO.setup([LEFT_IN1, LEFT_IN2, RIGHT_IN3, RIGHT_IN4, FLYWHEEL1, FLYWHEEL2], GPIO.OUT)

def drive(left_speed, right_speed):
    # Direction control via IN1–IN4
    if left_speed > 0:
        GPIO.output(LEFT_IN1, GPIO.HIGH)
        GPIO.output(LEFT_IN2, GPIO.LOW)
    elif left_speed < 0:
        GPIO.output(LEFT_IN1, GPIO.LOW)
        GPIO.output(LEFT_IN2, GPIO.HIGH)
    else:
        GPIO.output(LEFT_IN1, GPIO.LOW)
        GPIO.output(LEFT_IN2, GPIO.LOW)

    if right_speed > 0:
        GPIO.output(RIGHT_IN3, GPIO.HIGH)
        GPIO.output(RIGHT_IN4, GPIO.LOW)
    elif right_speed < 0:
        GPIO.output(RIGHT_IN3, GPIO.LOW)
        GPIO.output(RIGHT_IN4, GPIO.HIGH)
    else:
        GPIO.output(RIGHT_IN3, GPIO.LOW)
        GPIO.output(RIGHT_IN4, GPIO.LOW)

def flywheels_on():
    GPIO.output(FLYWHEEL1, GPIO.HIGH)
    GPIO.output(FLYWHEEL2, GPIO.HIGH)

def flywheels_off():
    GPIO.output(FLYWHEEL1, GPIO.LOW)
    GPIO.output(FLYWHEEL2, GPIO.LOW)

# Initialize Pygame for PS4 controller
pygame.init()
pygame.joystick.init()
joystick = pygame.joystick.Joystick(0)
joystick.init()
print(f"Connected to: {joystick.get_name()}")

try:
    while True:
        pygame.event.pump()

        x = joystick.get_axis(0)  # Left stick X
        y = -joystick.get_axis(1)  # Left stick Y (invert for forward)

        flywheel_button = joystick.get_button(5)  # R1

        if abs(y) > 0.1:
            direction = 1 if y > 0 else -1

            if abs(x) < 0.3:
                # Move straight
                drive(direction, direction)
            else:
                if x > 0.3:
                    # Turn right: stop right motors
                    drive(direction, 0)
                elif x < -0.3:
                    # Turn left: stop left motors
                    drive(0, direction)
        else:
            drive(0, 0)

        if flywheel_button:
            flywheels_on()
        else:
            flywheels_off()

        time.sleep(0.05)

except KeyboardInterrupt: #pressed CTRL + C
    print("Exiting...")

finally:
    drive(0, 0)
    flywheels_off()
    GPIO.cleanup()
