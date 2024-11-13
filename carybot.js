 /*  function updateImage() {
            const img = document.getElementById('dynamicimage');
            const timestamp = new Date().getTime();
            const newSrc = 'http://192.168.4.3/capture?time=' + timestamp;
            const newImg = new Image();
            newImg.src = newSrc;
            newImg.onload = function () {
                img.src = newSrc;
            }
        }

        setInterval(updateImage, 100);*/

        function openmenu() {
            document.getElementById("menu").style.width = "250px";
            document.getElementById("main").style.marginLeft = "250px";
            document.body.style.backgroundColor = "rgba(0,0,0,0.4)";
        }

        function closemenu() {
            document.getElementById("menu").style.width = "0";
            document.getElementById("main").style.marginLeft = "0";
            document.body.style.backgroundColor = "white";
        }

        const Directions = Object.freeze({
            UP: 'up',
            DOWN: 'down',
            RIGHT: 'right',
            LEFT: 'left',
            HALT: 'halt'
        });

        let currentDirection = Directions.HALT;

        var speed = 50;

        function speed_change(input_speed) {
            speed = input_speed;
        }

        function send(direction) {
            if (currentDirection !== direction) {
                currentDirection = direction;
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/coords?speed=" + speed + "&direction=" + direction, true);
                xhr.send();
                console.log("Direction: " + direction + " Speed: " + speed);
            }
        }

        function stop() {
            if (currentDirection !== Directions.HALT) {
                currentDirection = Directions.HALT;
                console.log("Direction: halt");
            }
        }

        function updateSliderValue(value) {
            console.log("Slider Wert: " + value);
        }