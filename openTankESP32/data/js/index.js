// Use this for tests on local machine without websocket server
const test = false;

const targetUrl = `ws://${window.location.hostname}:82`;

// Keyboard
let pressedKeys = {};
window.onkeyup = function (e) { pressedKeys[e.keyCode] = false; }
window.onkeydown = function (e) { pressedKeys[e.keyCode] = true; }

let websocket;
window.addEventListener('load', onLoad);

const view = document.getElementById('stream');

let setr = [0, 0];
let joystick;

// Time calculations
let d = new Date();
let time = d.getMilliseconds();
let lastFrameTime = d;
let fps = 0;
let ping = 0;

// Send ws commands
let commandsSent = [];

// View in fullscreen
function openFullscreen() {
  let elem = document.documentElement;

  if (elem.requestFullscreen) {
    elem.requestFullscreen();
  } else if (elem.webkitRequestFullscreen) { // Safari
    elem.webkitRequestFullscreen();
  } else if (elem.msRequestFullscreen) { // IE11
    elem.msRequestFullscreen();
  }
}

// Close fullscreen
function closeFullscreen() {
  if (document.exitFullscreen) {
    document.exitFullscreen();
  } else if (document.webkitExitFullscreen) { // Safari
    document.webkitExitFullscreen();
  } else if (document.msExitFullscreen) {  // IE11
    document.msExitFullscreen();
  }
}

// Get background click object
let object = document.getElementById('fullscreenBtn');
let fullScreen = false;

object.onclick = function(){
  if (!fullScreen) {
    openFullscreen();
  } else {
    closeFullscreen()
  }

  fullScreen = !fullScreen;
};

function onLoad() {
  // Get joystick area width
  let joystickWidth = document.getElementById("joyDiv").clientWidth;
  joystickWidth /= 2;

  joystick = new JoystickController("stick", joystickWidth, 8);
  initializeSockets();
}

function drawUI(ping, fps) {
  document.getElementById('stat').innerHTML = `ping: ${ping}ms, ${fps}FPS`;
}

function initializeSockets() {
  console.log('Opening WebSocket connection to OpenTank ESP32...');

  if (!test) {
    websocket = new WebSocket(targetUrl);

    websocket.onopen = event => {
      console.log('Starting connection to server..');
    };

    websocket.onmessage = message => {
      const dateNow = new Date();

      if (message.data instanceof Blob) { // WebSocket image received
        // Calculate fps
        fps = (dateNow - lastFrameTime);
        fps = 1000 / fps;
        fps = Math.round(fps * 100) / 100;
        lastFrameTime = dateNow;

        // Parse image and show it on page
        view.src = URL.createObjectURL(message.data);
      }

      // Calculate ping
      if (commandsSent.length > 0) {
        let oldestCommandSent = commandsSent[0];
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
  if (pressedKeys[87]) { // W
    setr[1] = 100;
  }
  else if (pressedKeys[83]) { // S
    setr[1] = -100;
  }

  if (pressedKeys[65]) { // A
    setr[0] = -100;
  }
  else if (pressedKeys[68]) { // D
    setr[0] = 100;
  }

  d = new Date();
  time = d.getMilliseconds();

  // Send data to tank
  if (!test) {
    // Check if websocket connected
    if (websocket.readyState == websocket.OPEN) {
      websocket.send(setr);
    }
  }
}
