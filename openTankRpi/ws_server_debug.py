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
        print('driver is closerd')
        # self.t._stop()

    def set_values(self, motor1: int, motor2: int):
        if -255 <= motor1 <= 255 and -255 <= motor2 <= 255:
            self.motors_state = True
            print(f'set motors to values <{motor1}, {motor2}>')
            self.last_update = datetime.datetime.now()
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
