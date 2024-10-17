#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "Carybot";
const char* password = "123456789";

// Create AsyncWebServer object on port 80
WebServer server(80);

const char* html_page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Joystick with Camera Stream</title>
    <meta name="viewport" content="user-scalable=no">
    <style>
        body {
            display: flex;
            height: 100vh;
            margin: 0;
        }

        #left {
            width: 50%;
            background-color: #ececec;
        }

        #right {
            width: 50%;
            position: relative;
            background-color: white;
        }

        #canvas {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
        }

        iframe {
            width: 100%;
            height: 100%;
            border: none;
        }
    </style>
</head>
<body>
    <div id="left">
        <!-- Embedding the camera stream from the ESP32-CAM -->
        <iframe src="http://192.168.4.1"></iframe>
    </div>

    <div id="right">
        <canvas id="canvas" name="game"></canvas>
    </div>


    <script>
 
        var canvas, ctx;

        window.addEventListener('load', () => {
            canvas = document.getElementById('canvas');
            ctx = canvas.getContext('2d');
            resize();

            document.addEventListener('mousedown', startDrawing);
            document.addEventListener('mouseup', stopDrawing);
            document.addEventListener('mousemove', Draw);

            document.addEventListener('touchstart', startDrawing);
            document.addEventListener('touchend', stopDrawing);
            document.addEventListener('touchcancel', stopDrawing);
            document.addEventListener('touchmove', Draw);
            window.addEventListener('resize', resize);

           // window.addEventListener('resize', checkorientation);
           // window.addEventListener("orientationchange", checkorientation);

           /* window.addEventListener('scroll', function (e) {
                if(window.scrollY > 0) {
                    e.preventDefault();
                }
            });

            window.addEventListener('touchstart', function (e) {
                this.startY = e.touches[0].clientY;
            });

            window.addEventListener('touchmove', function (e) {
                let moveY = e.touches[0].clientY - this.startY;
                if(moveY > 0){
                    e.preventDefault();
                }
            });*/

            //window.onload = checkorientation;
        });

        const Directions = Object.freeze({
            UP: 'up',
            DOWN: 'down',
            RIGHT: 'right',
            LEFT: 'left',
            UP_LEFT: 'up_left',
            UP_RIGHT: 'up_right',
            DOWN_LEFT: 'down_left',
            DOWN_RIGHT: 'down_right',
            HALT: 'halt'
        });

        /*function checkorientation() {
            const message = document.getElementById('rotatemessage');
            if (window.innerHeight > window.innerWidth) {
                message.style.display = 'flex';
            } else {
                message.style.display = 'none';
            }
        }*/

        function send(speed, direction) {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/coords?speed=" + speed + "&direction=" + direction, true);
            xhr.send();
        }

        function get_direction(angle_in_degrees) {
            if (angle_in_degrees <= 22.5 && angle_in_degrees >= 0 || angle_in_degrees <= 365 && angle_in_degrees > 337.5) {
                return Directions.RIGHT;
            } else if (angle_in_degrees > 22.5 && angle_in_degrees <= 67.5) {
                return Directions.UP_RIGHT;
            } else if (angle_in_degrees > 67.5 && angle_in_degrees <= 112.5) {
                return Directions.UP;
            } else if (angle_in_degrees > 112.5 && angle_in_degrees <= 157.5) {
                return Directions.UP_LEFT;
            } else if (angle_in_degrees > 157.5 && angle_in_degrees <= 202.5) {
                return Directions.LEFT;
            } else if (angle_in_degrees > 202.5 && angle_in_degrees <= 247.5) {
                return Directions.DOWN_LEFT;
            } else if (angle_in_degrees > 247.5 && angle_in_degrees <= 292.5) {
                return Directions.DOWN;
            } else if (angle_in_degrees > 292.5 && angle_in_degrees <= 337.5) {
                return Directions.DOWN_RIGHT;
            } else {
                return Directions.HALT;
            }
        }

        var width, height, radius, x_orig, y_orig;
        function resize() {
            width = window.innerWidth;
            height = window.innerHeight;
            radius = Math.min(width, height) / 5;
            ctx.canvas.width = width;
            ctx.canvas.height = height;
            background();
            joystick(x_orig, y_orig);
        }

        function background() {
            x_orig = width - radius - 20;
            y_orig = height / 2;

            ctx.beginPath();
            ctx.arc(x_orig, y_orig, radius + 20, 0, Math.PI * 2, true);
            ctx.fillStyle = '#ECE5E5';
            ctx.fill();
        }

        function joystick(width, height) {
            ctx.beginPath();
            ctx.arc(width, height, radius, 0, Math.PI * 2, true);
            ctx.fillStyle = '#808080';      // grey
            ctx.fill();
            ctx.strokeStyle = '#a0a0a0';    // light grey
            ctx.lineWidth = 8;
            ctx.stroke();
        }

        let coord = { x: 0, y: 0 };
        let paint = false;

        function getPosition(event) {
            var mouse_x = event.clientX || (event.touches && event.touches[0].clientX);
            var mouse_y = event.clientY || (event.touches && event.touches[0].clientY);
            coord.x = mouse_x - canvas.offsetLeft;
            coord.y = mouse_y - canvas.offsetTop;
        }

        function is_it_in_the_circle() {
            var current_radius = Math.sqrt(Math.pow(coord.x - x_orig, 2) + Math.pow(coord.y - y_orig, 2));
            return radius >= current_radius;
        }

        function startDrawing(event) {
            paint = true;
            getPosition(event);
            if (is_it_in_the_circle()) {
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                background();
                joystick(coord.x, coord.y);
                Draw(event);  // Pass event to Draw
            }
        }

        function stopDrawing() {
            paint = false;
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            background();
            joystick(width - radius - 20, height / 2);
            send(0, Directions.HALT);
        }

        function Draw(event) {
            if (paint) {
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                background();
                var angle_in_degrees, x, y, speed;
                var angle = Math.atan2((coord.y - y_orig), (coord.x - x_orig));

                angle_in_degrees = Math.round((Math.sign(angle) === -1) ? -angle * 180 / Math.PI : 360 - angle * 180 / Math.PI);

                if (is_it_in_the_circle()) {
                    joystick(coord.x, coord.y);
                    x = coord.x;
                    y = coord.y;
                } else {
                    x = radius * Math.cos(angle) + x_orig;
                    y = radius * Math.sin(angle) + y_orig;
                    joystick(x, y);
                }

                getPosition(event);

                speed = Math.round(100 * Math.sqrt(Math.pow(x - x_orig, 2) + Math.pow(y - y_orig, 2)) / radius);
                var direction = get_direction(angle_in_degrees);

                var prev_speed = -1, prev_dir = Directions.HALT;
                if (speed != prev_speed && direction != prev_dir) {
                    send(speed, direction);
                    prev_speed = speed;
                    prev_dir = direction;
                }

                console.log("speed: " + speed + " direction: " + direction);
            }
        }


    </script>
</body>

</html>

)rawliteral";

void handleRoot() {
  server.send(200, "text/html", html_page);
}

// Handle joystick coordinates
void handleCoords() {
  String speed = server.arg("speed");
  String dir = server.arg("direction");

  Serial.print("Speed: ");
  Serial.println(speed);
  Serial.print("Direction: ");
  Serial.println(dir);

  server.send(200, "text/plain", "Coords received");
}

// Handle image upload from ESP32-CAM
void handleImageUpload() {
  // Check if the request has the image data
  if (server.hasArg("plain") == false) {
    server.send(400, "text/plain", "No image data received");
    return;
  }

  // Read the image data
  String imageData = server.arg("plain");  // You may need to adjust this if you expect binary data
  
  // For testing purposes, print the size of the received data
  Serial.print("Received image of size: ");
  Serial.println(imageData.length());

  // Respond to the ESP32-CAM
  server.send(200, "text/plain", "Image received successfully");
}

void setup() {
  // Connect to Wi-Fi
  Serial.begin(115200);
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  Serial.println("Hotspot started");

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Register endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/coords", HTTP_GET, handleCoords);
  server.on("/upload", HTTP_POST, handleImageUpload);  // New route for image upload

  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
  server.handleClient();
}