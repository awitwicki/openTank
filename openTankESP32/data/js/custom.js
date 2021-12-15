// Use this for tests on local machine without websocket server
var test = false;

var targetUrl = "ws://" + window.location.host + ":82";

// Keyboard
var pressedKeys = {};
window.onkeyup = function (e) { pressedKeys[e.keyCode] = false; }
window.onkeydown = function (e) { pressedKeys[e.keyCode] = true; }

var websocket;
window.addEventListener('load', onLoad);

const view = document.getElementById('stream');

var setr = [0, 0];
let joystick;

// Time calculations
var d = new Date();
var time = d.getMilliseconds();
var lastFrameTime = d;
var fps = 0;
var ping = 0;

// Send ws commands
var commandsSent = [];

function onLoad() {
  joystick = new JoystickController("stick", 64, 8);
  initializeSockets();
}

function drawUI(ping, fps) {
  document.getElementById('stat').innerHTML = 'ping: ' + ping + 'ms, ' + fps +'FPS';
}

function initializeSockets() {
  console.log('Opening WebSocket connection to OpenTank ESP32...');

  if (!test) {
    websocket = new WebSocket(targetUrl);

    websocket.onopen = event => {
      console.log('Starting connection to server..');
    };

    websocket.onmessage = message => {
      dateNow = new Date();

      if (message.data instanceof Blob) { // WebSocket image received
        // Calculate fps
        fps = (dateNow - lastFrameTime);
        fps = 1000 / fps;
        fps = Math.round(fps * 100) / 100;
        lastFrameTime = dateNow;

        // Parse image and show it on page
        var urlObject = URL.createObjectURL(message.data);
        view.src = urlObject;
      }

      // Calculate ping
      if (commandsSent.length > 0) {
        var oldestCommandSent = commandsSent[0];
        ping = (dateNow - oldestCommandSent);

        // Delete first element
        commandsSent.shift();
      }

      // Append new timestamp of command sent
      commandsSent.push(dateNow);

      // Update data
      drawUI(ping, fps);
    };

    websocket.onclose = message => {
      console.log('camWebSocket Closing connection to server..');
      setTimeout(initializeSockets, 2000);
    };
  }

  setInterval(loop, 100);
}

function loop() {
  // Get data from joystick
  setr[0] = Math.round(joystick.value.x * 100);
  setr[1] = Math.round(-joystick.value.y * 100);

  // Get data from keyboard ONE KEY PER TIME
  // Keyboard control override joystick
  // WASD = 87, 65, 83, 68
  if (pressedKeys[87] == true) { // W
    setr[1] = 100;
  }
  else if (pressedKeys[83] == true) { // S
    setr[1] = -100;
  }

  if (pressedKeys[65] == true) { // A
    setr[0] = -100;
  }
  else if (pressedKeys[68] == true) { // D
    setr[0] = 100;
  }

  d = new Date();
  time = d.getMilliseconds();

  // Send data to tank
  if (!test) {
    // check if websocket connected
    if (websocket.readyState == websocket.OPEN) {
      websocket.send(setr);
    }
  }

  console.log(setr);
}
