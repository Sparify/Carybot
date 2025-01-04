function updateImage() {
    const img = document.getElementById('dynamicimage');
    const timestamp = new Date().getTime();
    const newSrc = 'http://192.168.4.3/capture?time=' + timestamp;
    const newImg = new Image();
    newImg.src = newSrc;
    newImg.onload = function () {
        img.src = newSrc;
    }
}

setInterval(updateImage, 100);

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
    websocket = new WebSocket('ws://192.168.4.1/ws');

    websocket.onopen = () => {
        console.log('WebSocket connection established');
        document.getElementById('connectionStatus').innerText = 'Connected';
        document.getElementById('connectionStatus').className = 'status-box connected';
    };

    websocket.onmessage = (event) => {
        console.log('Received:', event.data);
    };

    websocket.onclose = () => {
        console.error('WebSocket closed. Attempting to reconnect...');
        document.getElementById('connectionStatus').innerText = 'Disconnected';
        document.getElementById('connectionStatus').className = 'status-box disconnected';
        setTimeout(connectWebSocket, 5000); // Versuche, die Verbindung wiederherzustellen.
    };

    websocket.onerror = (error) => {
        console.error('WebSocket error:', error);
        document.getElementById('connectionStatus').innerText = 'Error';
        document.getElementById('connectionStatus').className = 'status-box error';
    };
}

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