import asyncio
import websockets
import tornado.httpserver
import tornado.websocket
import tornado.ioloop
import tornado.web
import socket
import asyncio
import websockets
import tornado.httpserver
import tornado.websocket
import tornado.ioloop
import tornado.web
import socket
import datetime
import time
import threading

import RPi.GPIO as GPIO

GPIO.setwarnings(False)

#set GPIO numbering mode and define output pins
GPIO.setmode(GPIO.BOARD)

motor1a = 5
motor1b = 7

motor2a = 8
motor2b = 10

GPIO.setup(motor1a,GPIO.OUT)
GPIO.setup(motor1b,GPIO.OUT)
GPIO.setup(motor2a,GPIO.OUT)
GPIO.setup(motor2b,GPIO.OUT)

def parse_values(values):
    try:
        vals = values.split(',')
        m1 = int(vals[0])
        m2 = int(vals[1])

        return [m1, m2]
    except:
        return [0,0]

class motor_controller:

    def __init__(self, pwm_freq = 10, driver_pins = [1,2,3,4], update_timeout_ms = 2000):
        self.pwm_freq = pwm_freq
        self.motors = driver_pins #pins for l293d [motor1_a, motor1_b, motor2_a, motor2_b]
        self.last_update = datetime.datetime.now()
        self.update_timeout_ms = update_timeout_ms #time to update motors

        self.motors_state = False

    def _check_timeout(self):
        while True:
            time.sleep(float(self.update_timeout_ms/1000))
            # print('timeout checker tick')
            # print((datetime.datetime.now() - self.last_update))
            if (datetime.datetime.now() - self.last_update).microseconds/1000 > self.update_timeout_ms and self.motors_state is True:
                self.motors_state = False
                print('signal timeout!!!')
                self.set_values(0, 0)

    def init_driver(self):
        print('starting loop timeouter')
        self.t = threading.Thread(target=self._check_timeout)
        self.t.start()

    def close_driver(self):
        print('driver is closed')
        # self.t._stop()

    def set_values(self, motor1: int, motor2: int):
        if -255 <= motor1 <= 255 and -255 <= motor2 <= 255:
            self.last_update = datetime.datetime.now()
            self.motors_state = True
            # print(f'set motors to values <{motor1}, {motor2}>')

            if motor1 == 1:
                motor1 = 0
            if motor2 == 1:
                motor2 = 0

            if motor1 > 1:
                GPIO.output(motor1a,GPIO.HIGH)
                GPIO.output(motor1b,GPIO.LOW)
            elif motor1 < 0:
                GPIO.output(motor1a,GPIO.LOW)
                GPIO.output(motor1b,GPIO.HIGH)
            elif motor1 == 0:
                GPIO.output(motor1a,GPIO.LOW)
                GPIO.output(motor1b,GPIO.LOW)

            if motor2 > 1:
                GPIO.output(motor2a,GPIO.HIGH)
                GPIO.output(motor2b,GPIO.LOW)
            elif motor2 < 0:
                GPIO.output(motor2a,GPIO.LOW)
                GPIO.output(motor2b,GPIO.HIGH)
            elif motor2 == 0:
                GPIO.output(motor2a,GPIO.LOW)
                GPIO.output(motor2b,GPIO.LOW)

            time.sleep(0.05)

        elif not self.motors_state:
            print(f'wrong values {motor1} {motor2}')

class WSHandler(tornado.websocket.WebSocketHandler):
    def open(self):
        print('connected')
        l293.init_driver()

    def on_message(self, message):
        # print(f'message: {message}')

        motors = parse_values(message)

        # send motors controller values
        l293.set_values(motors[0], motors[1])

        #send response 'ok'
        self.write_message('ok')

    def on_close(self):
        print('closed')
        l293.close_driver()

    def check_origin(self, origin):
        return True

application = tornado.web.Application([
    (r'/ws', WSHandler),
])

l293 = motor_controller()

if __name__ == "__main__":
    http_server = tornado.httpserver.HTTPServer(application)
    http_server.listen(8765)
    myIP = socket.gethostbyname(socket.gethostname())
    print(f'Websocket Server Started at {myIP}')
    tornado.ioloop.IOLoop.instance().start()
