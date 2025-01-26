
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

let websocket;

function connectWebSocket() {
    websocket = new WebSocket('ws://' + window.location.hostname + ':81');  // WebSocket-Verbindung zum ESP32

    websocket.onmessage = function(event) {
        const blob = new Blob([event.data], { type: 'image/jpeg' });
        const url = URL.createObjectURL(blob);
        const img = document.getElementById('dynamicimage');
        img.src = url;  // Setzen des neuen Bildes im <img> Tag
    };

    websocket.onopen = function() {
        console.log('WebSocket verbunden');
        document.getElementById('connectionStatus').innerText = 'Connected';
        document.getElementById('connectionStatus').className = 'status-box connected';
    };

    websocket.onclose = function() {
        console.log('WebSocket getrennt. Erneuter Versuch in 5 Sekunden...');
        setTimeout(connectWebSocket, 5000);  // Versuchen, die Verbindung erneut herzustellen
    };
}

connectWebSocket();

function send(direction) {
    if (websocket.readyState === WebSocket.OPEN) {
        if (currentdir_robot !== direction) {
            currentdir_robot = direction;

            const message = JSON.stringify({
                robot_direction: currentdir_robot,
                speed: speed
            });
            websocket.send(message);
            console.log("Direction: " + direction + " Speed: " + speed);
        };
    } else {
        console.error('Cannot send message. WebSocket is not open.');
    }
}

function stop() {
    if (websocket.readyState === WebSocket.OPEN) {
        if (currentdir_robot !== Directions.HALT) {
            currentdir_robot = Directions.HALT;
            websocket.send(JSON.stringify({
                robot_direction: Directions.HALT,
                speed: speed
            }));
            console.log('Message sent: halt');
        }
    } else {
        console.error('Cannot send message. WebSocket is not open.');
    }
}

connectWebSocket();

function updateSliderValue(value) {
    console.log("Slider Wert: " + value);
}