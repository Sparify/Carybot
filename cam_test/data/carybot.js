
function togglemenu() {
    const menu = document.getElementById("menu");
    const main = document.getElementById("main");

    if (menu.style.width === "300px") {
        menu.style.width = "0";
        main.style.marginLeft = "0";
        document.body.style.backgroundColor = "white";
    } else {
        menu.style.width = "300px";
        main.style.marginLeft = "300px";
        document.body.style.backgroundColor = "rgba(0,0,0,0,4)";
    }
}

document.addEventListener('dragstart', event => {
    event.preventDefault();
});

document.addEventListener('contextmenu', event => {
    event.preventDefault();
});

document.addEventListener('mousedown', (event) => {
    const target = event.target.closest('.button');
    if (target) {
        const direction = target.dataset.direction;
        if (direction) {
            send(target.dataset.direction);
        }
    }
});

document.addEventListener('mouseup', (event) => {
    const target = event.target.closest('.button');
    if (target) {
        stop();
    }
});

document.addEventListener('touchstart', (event) => {
    const target = event.target.closest('.button');
    if (target) {
        const direction = target.dataset.direction;
        if (direction) {
            send(direction);
        }
    }
});

document.addEventListener('touchend', (event) => {
    const target = event.target.closest('.button');
    if (target) {
        stop();
    }
});

const Directions = Object.freeze({
    UP: 'up',
    DOWN: 'down',
    RIGHT: 'right',
    LEFT: 'left',
    HALT: 'halt'
});

let currentdir_robot = Directions.HALT;
let currentdir_cam = Directions.HALT;

var speed = 50;

function speed_change(input_speed) {
    speed = input_speed;
}

let websocket_cam;

function connectWebSocket_cam() {
    websocket_cam = new WebSocket('ws://' + window.location.hostname + ':81');  // WebSocket-Verbindung zum ESP32

    websocket_cam.onmessage = function(event) {
        const blob = new Blob([event.data], { type: 'image/jpeg' });
        const url = URL.createObjectURL(blob);
        const img = document.getElementById('dynamicimage');
        img.src = url;  // Setzen des neuen Bildes im <img> Tag
    };

    websocket_cam.onopen = function() {
        console.log('WebSocket cam verbunden');
    };

    websocket_cam.onclose = function() {
        console.log('WebSocket Cam getrennt. Erneuter Versuch in 5 Sekunden...');
        setTimeout(connectWebSocket_cam, 5000);  // Versuchen, die Verbindung erneut herzustellen
    };
}

connectWebSocket_cam();

let websocket_carybot;

function connectWebSocket_carybot() {
    websocket_carybot = new WebSocket('ws://192.168.136.42:8080');

    websocket_carybot.onopen = function() {
        console.log("WebSocket Carybot verbunden");
        document.getElementById('connectionStatus').innerText = 'Connected';
        document.getElementById('connectionStatus').className = 'status-box connected';
    }

    websocket_carybot.onclose = function() {
        document.getElementById('connectionStatus').innerText = 'Disonnected';
        document.getElementById('connectionStatus').className = 'status-box disconnected';

        console.log('WebSocket Carybot getrennt. Erneuter Versuch in 5 Sekunden...');
        setTimeout(connectWebSocket_carybot, 5000);  // Versuchen, die Verbindung erneut herzustellen
    }
}

connectWebSocket_carybot();

function send(direction) {
    if(currentdir_robot !== direction){
        currentdir_robot = direction;

        const message = JSON.stringify({
            robot_direction: currentdir_robot,
            speed: speed
        });

        console.log("Direction: " + direction + " Speed: " + speed);

        if(websocket_cam.readyState === WebSocket.OPEN) {
            websocket_cam.send(message);
            console.log("An CAM gesendet");
        } else {
            console.error("Senden fehlgeschlagen. WebSocket Cam is not open");
        }

        if(websocket_carybot.readyState === WebSocket.OPEN) {
            websocket_carybot.send(message);
            console.log("An Carybot gesendet");
        } else {
            console.error("Senden fehlgeschlagen. WebSocket Carybot is not open");
        }
    }
}

function stop() {
    if(currentdir_robot !== Directions.HALT) {
        currentdir_robot = Directions.HALT;
        
        const message = JSON.stringify({
            robot_direction: Directions.HALT,
            speed: speed
        });

        console.log('Message sent: halt');

        if(websocket_cam.readyState === WebSocket.OPEN) {
            websocket_cam.send(message);
        } else {
            console.error("Senden fehlgeschlagen. WebSocket Cam is not open");
        }

        if(websocket_carybot.readyState === WebSocket.OPEN) {
            websocket_carybot.send(message);
        } else {
            console.error("Senden fehlgeschlagen. WebSocket Carybot is not open");
        }
    }
}

function updateSliderValue(value) {
    console.log("Slider Wert: " + value);
}