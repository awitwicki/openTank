var targetUrl = "ws://" + window.location.host + ":82";

var websocket;
window.addEventListener('load', onLoad);

const view = document.getElementById('stream');

var setr = [0, 0];
var joy;

var d = new Date();
var time = d.getMilliseconds();

function onLoad() {
  var joyStickParameters = {
    internalFillColor: "#000000",
    internalLineWidth: "#000000",
    internalStrokeColor: "#999999",
    externalStrokeColor: "#000000",
  };

  joy = new JoyStick('joyDiv', joyStickParameters);

  initializeSockets();
}

function initializeSockets() {
  console.log('Opening WebSocket connection to OpenTank ESP32...');

  websocket = new WebSocket(targetUrl);

  websocket.onopen = event => {
    console.log('Starting connection to server..');
    setInterval(loop, 100);
  };

  websocket.onmessage = message => {
    if (message.data instanceof Blob) {
      console.log("WebSocket image received")
      var urlObject = URL.createObjectURL(message.data);
      view.src = urlObject;
    }

    d = new Date();
    var ping = d.getMilliseconds() - time;
  
    document.getElementById('stat').innerHTML = 'ping: ' + ping + 'ms';
  };

  websocket.onclose = message => {
    console.log('camWebSocket Closing connection to server..');
    setTimeout(initializeSockets, 2000);
  };
}

function loop() {
  //get data from canvas joy stick
  setr[0] = joy.GetX();
  setr[1] = joy.GetY();

  d = new Date();
  time = d.getMilliseconds();
  websocket.send(setr);
}
