from Firmware.flask import Flask, Response
from picamera2 import Picamera2
import cv2
import time
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
  app.run(host='0.0.0.0', port=5000)