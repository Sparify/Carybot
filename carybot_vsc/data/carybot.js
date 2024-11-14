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

        let currentdir_robot = Directions.HALT;
        let currentdir_cam = Directions.HALT;

        var speed = 50;

        function speed_change(input_speed) {
            speed = input_speed;
        }

        function send(direction) {
            if (currentdir_robot !== direction) {
                currentdir_robot = direction;
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/move_robot?speed=" + speed + "&direction=" + direction, true);
                xhr.send();
                console.log("Direction: " + direction + " Speed: " + speed);
            }
        }

        function move_camera(direction) {
            if(currentdir_cam !== direction) {
                currentdir_cam = direction;
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/move_cam?direction=" + direction, true);
                xhr.send();
                console.log("Direction cam: " + direction);
            }
        }

        function stop() {
            if (currentdir_robot !== Directions.HALT) {
                currentdir_robot = Directions.HALT;
                console.log("Direction: halt");
            }

            if(currentdir_cam !== Directions.HALT){
                currentdir_cam = Directions.HALT;
                console.log("Direction: halt");
            }
        }

        function updateSliderValue(value) {
            console.log("Slider Wert: " + value);
        }