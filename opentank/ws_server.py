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

motor1a = 11
motor1b = 13

motor2a = 8
motor2b = 10

GPIO.setup(motor1a,GPIO.OUT)
GPIO.setup(motor1b,GPIO.OUT)
GPIO.setup(motor2a,GPIO.OUT)
GPIO.setup(motor2b,GPIO.OUT)

m1_a = GPIO.PWM(motor1a, 100)
m1_b = GPIO.PWM(motor1b, 100)
m2_a = GPIO.PWM(motor2a, 100)
m2_b = GPIO.PWM(motor2b, 100)

m1_last_value = 0
m2_last_value = 0

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
        m1_a.start(0)
        m2_a.start(0)

        print('starting loop timeouter')
        self.t = threading.Thread(target=self._check_timeout)
        self.t.start()

    def close_driver(self):
        print('driver is closed')
        m1_a.stop()
        m1_b.stop()
        m2_a.stop()
        m2_b.stop()

    def set_values(self, motor1: int, motor2: int):
        global m1_last_value
        global m2_last_value

        if -100 <= motor1 <= 100 and -100 <= motor2 <= 100:
            self.last_update = datetime.datetime.now()
            self.motors_state = True
            print(f'set motors to values <{motor1}, {motor2}>')

            if motor1 > 1:
                m1_b.stop()
                if m1_last_value > 0:
                    m1_a.ChangeDutyCycle(motor1)
                else:
                    m1_a.start(motor1)
            elif motor1 < 0:
                m1_a.stop()
                if m1_last_value < 0:
                    m1_b.ChangeDutyCycle(-motor1)
                else:
                    m1_b.start(-motor1)
                print(-motor1)
            elif motor1 == 0:
                m1_a.stop()
                m1_b.stop()

            m1_last_value = motor1

            if motor2 > 1:
                m2_b.stop()
                if m2_last_value > 0:
                    m2_a.ChangeDutyCycle(motor2)
                else:
                    m2_a.start(motor2)
            elif motor2 < 0:
                m2_a.stop()
                if m2_last_value < 0:
                    m2_b.ChangeDutyCycle(-motor2)
                else:
                    m2_b.start(-motor2)
                print(-motor2)
            elif motor2 == 0:
                m2_a.stop()
                m2_b.stop()

            m2_last_value = motor2

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
    try:
        http_server = tornado.httpserver.HTTPServer(application)
        http_server.listen(8765)
        myIP = socket.gethostbyname(socket.gethostname())
        print(f'Websocket Server Started at {myIP}')
        tornado.ioloop.IOLoop.instance().start()
    except KeyboardInterrupt:
        print('Koniec')
        tornado.ioloop.IOLoop.instance().stop()
    finally:
        m1_a.stop()
        m1_b.stop()
        m2_a.stop()
        m2_b.stop()
        GPIO.cleanup()
