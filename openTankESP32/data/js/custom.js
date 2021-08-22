var targetUrl = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);

var setr = [0, 0];
var joy;

var d = new Date();
var time = d.getMilliseconds();

function onLoad() {
  var joyStickParameters = {
    internalFillColor: "#000000",
    internalLineWidth: "#000000",
    internalStrokeColor: "#AAAAAA",
    externalStrokeColor: "#000000",
  };

  joy = new JoyStick('joyDiv', joyStickParameters);

  initializeSocket();
}

function initializeSocket() {
  console.log('Opening WebSocket connection to ESP32...');
  websocket = new WebSocket(targetUrl);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Starting connection to server..');
  setInterval(loop, 100);
}

function onClose(event) {
  console.log('Closing connection to server..');
  setTimeout(initializeSocket, 2000);
}

function onMessage(event) {
  console.log("WebSocket message received:", event)
  var received_msg = evt.data;
  console.log("received: " + received_msg);

  d = new Date();
  var ping = d.getMilliseconds() - time;

  document.getElementById('stat').innerHTML = 'ping: ' + ping + 'ms';
}

function loop() {
  setr[0] = joy.GetX();
  setr[1] = joy.GetY();

  d = new Date();
  time = d.getMilliseconds();
  websocket.send(setr);
}
