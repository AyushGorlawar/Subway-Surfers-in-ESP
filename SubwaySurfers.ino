#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

const char* AP_SSID = "SubwayRunner";
const char* AP_PASS = "12345678";

ESP8266WebServer server(80);
DNSServer dnsServer;

const char GAME_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Subway Runner</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
        body {
            margin: 0;
            overflow: hidden;
            background: #000;
        }
        #gameCanvas {
            display: block;
            width: 100vw;
            height: 100vh;
            touch-action: none;
        }
        #stats {
            position: fixed;
            top: 20px;
            left: 20px;
            color: white;
            font-family: Arial;
            font-size: 24px;
            text-shadow: 2px 2px 2px #000;
        }
        .stat {
            margin: 5px 0;
        }
        #gameOver {
            position: fixed;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: rgba(0,0,0,0.8);
            color: white;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            display: none;
            font-family: Arial;
        }
        button {
            padding: 10px 20px;
            margin-top: 10px;
            cursor: pointer;
            background: #4CAF50;
            border: none;
            color: white;
            border-radius: 5px;
        }
    </style>
</head>
<body>
    <canvas id="gameCanvas"></canvas>
    <div id="stats">
        <div class="stat" id="score">Score: 0</div>
        <div class="stat" id="coins">Coins: 0</div>
    </div>
    <div id="gameOver">
        <h2>Game Over!</h2>
        <p>Final Score: <span id="finalScore">0</span></p>
        <p>Coins Collected: <span id="finalCoins">0</span></p>
        <button onclick="restartGame()">Play Again</button>
    </div>

    <script>
        // Sound synthesis setup
        const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        
        function createCoinSound() {
            const oscillator = audioCtx.createOscillator();
            const gainNode = audioCtx.createGain();
            
            oscillator.connect(gainNode);
            gainNode.connect(audioCtx.destination);
            
            oscillator.type = 'sine';
            oscillator.frequency.setValueAtTime(800, audioCtx.currentTime);
            oscillator.frequency.linearRampToValueAtTime(1200, audioCtx.currentTime + 0.1);
            
            gainNode.gain.setValueAtTime(0.3, audioCtx.currentTime);
            gainNode.gain.linearRampToValueAtTime(0, audioCtx.currentTime + 0.1);
            
            oscillator.start();
            oscillator.stop(audioCtx.currentTime + 0.1);
        }

        function createJumpSound() {
            const oscillator = audioCtx.createOscillator();
            const gainNode = audioCtx.createGain();
            
            oscillator.connect(gainNode);
            gainNode.connect(audioCtx.destination);
            
            oscillator.type = 'square';
            oscillator.frequency.setValueAtTime(200, audioCtx.currentTime);
            oscillator.frequency.linearRampToValueAtTime(150, audioCtx.currentTime + 0.1);
            
            gainNode.gain.setValueAtTime(0.2, audioCtx.currentTime);
            gainNode.gain.linearRampToValueAtTime(0, audioCtx.currentTime + 0.1);
            
            oscillator.start();
            oscillator.stop(audioCtx.currentTime + 0.1);
        }

        function createCrashSound() {
            const oscillator = audioCtx.createOscillator();
            const gainNode = audioCtx.createGain();
            
            oscillator.connect(gainNode);
            gainNode.connect(audioCtx.destination);
            
            oscillator.type = 'sawtooth';
            oscillator.frequency.setValueAtTime(150, audioCtx.currentTime);
            oscillator.frequency.linearRampToValueAtTime(50, audioCtx.currentTime + 0.3);
            
            gainNode.gain.setValueAtTime(0.3, audioCtx.currentTime);
            gainNode.gain.linearRampToValueAtTime(0, audioCtx.currentTime + 0.3);
            
            oscillator.start();
            oscillator.stop(audioCtx.currentTime + 0.3);
        }

        const canvas = document.getElementById('gameCanvas');
        const ctx = canvas.getContext('2d');
        const scoreElement = document.getElementById('score');
        const coinsElement = document.getElementById('coins');
        const gameOverElement = document.getElementById('gameOver');
        const finalScoreElement = document.getElementById('finalScore');
        const finalCoinsElement = document.getElementById('finalCoins');

        function resizeCanvas() {
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;
        }
        resizeCanvas();
        window.addEventListener('resize', resizeCanvas);

        let score = 0;
        let coins = 0;
        let gameSpeed = 5;
        let isGameOver = false;
        const LANE_WIDTH = 100;
        const PLAYER_HEIGHT = 60;
        const GRAVITY = 0.8;
        const JUMP_FORCE = -15;

        const player = {
            x: canvas.width / 2,
            y: canvas.height - 100,
            width: 40,
            height: PLAYER_HEIGHT,
            velocityY: 0,
            lane: 1,
            isJumping: false
        };

        class Coin {
            constructor() {
                this.lane = Math.floor(Math.random() * 3);
                this.size = 20;
                this.x = canvas.width/2 + (this.lane - 1) * LANE_WIDTH;
                this.y = -this.size;
                this.speed = gameSpeed;
                this.rotation = 0;
            }

            update() {
                this.y += this.speed;
                this.rotation += 0.1;
                return this.y > canvas.height;
            }

            draw() {
                ctx.save();
                ctx.translate(this.x, this.y + this.size/2);
                ctx.rotate(this.rotation);
                
                // Draw coin
                ctx.fillStyle = '#FFD700';
                ctx.beginPath();
                ctx.arc(0, 0, this.size/2, 0, Math.PI * 2);
                ctx.fill();
                
                // Draw $ symbol
                ctx.fillStyle = '#DAA520';
                ctx.font = '15px Arial';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText('$', 0, 0);
                
                ctx.restore();
            }

            checkCollision() {
                const distance = Math.sqrt(
                    Math.pow(this.x - player.x, 2) +
                    Math.pow(this.y + this.size/2 - (player.y + player.height/2), 2)
                );
                return distance < (this.size/2 + player.width/2);
            }
        }

        class Obstacle {
            constructor() {
                this.lane = Math.floor(Math.random() * 3);
                this.width = 40;
                this.height = 40;
                this.x = canvas.width/2 + (this.lane - 1) * LANE_WIDTH;
                this.y = -this.height;
                this.speed = gameSpeed;
            }

            update() {
                this.y += this.speed;
                return this.y > canvas.height;
            }

            draw() {
                ctx.fillStyle = 'red';
                ctx.fillRect(this.x - this.width/2, this.y, this.width, this.height);
            }

            checkCollision() {
                return (
                    player.x - player.width/2 < this.x + this.width/2 &&
                    player.x + player.width/2 > this.x - this.width/2 &&
                    player.y < this.y + this.height &&
                    player.y + player.height > this.y
                );
            }
        }

        let obstacles = [];
        let coinsList = [];
        let lastObstacleTime = 0;
        let lastCoinTime = 0;

        document.addEventListener('keydown', (e) => {
            if (isGameOver) return;
            switch(e.key) {
                case 'ArrowLeft':
                    if (player.lane > 0) player.lane--;
                    break;
                case 'ArrowRight':
                    if (player.lane < 2) player.lane++;
                    break;
                case 'ArrowUp':
                case ' ':
                    if (!player.isJumping) {
                        player.velocityY = JUMP_FORCE;
                        player.isJumping = true;
                        createJumpSound();
                    }
                    break;
            }
        });

        let touchStartX = 0;
        canvas.addEventListener('touchstart', (e) => {
            touchStartX = e.touches[0].clientX;
            if (!player.isJumping) {
                player.velocityY = JUMP_FORCE;
                player.isJumping = true;
                createJumpSound();
            }
        });

        canvas.addEventListener('touchmove', (e) => {
            if (isGameOver) return;
            const touchX = e.touches[0].clientX;
            const diff = touchX - touchStartX;
            
            if (diff > 50 && player.lane < 2) {
                player.lane++;
                touchStartX = touchX;
            } else if (diff < -50 && player.lane > 0) {
                player.lane--;
                touchStartX = touchX;
            }
        });

        function drawTrack() {
            ctx.fillStyle = '#333';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            ctx.strokeStyle = '#fff';
            ctx.lineWidth = 2;
            for (let i = -1; i <= 1; i++) {
                const x = canvas.width/2 + i * LANE_WIDTH;
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, canvas.height);
                ctx.stroke();
            }
        }

        function drawPlayer() {
            const targetX = canvas.width/2 + (player.lane - 1) * LANE_WIDTH;
            player.x += (targetX - player.x) * 0.2;

            player.velocityY += GRAVITY;
            player.y += player.velocityY;

            if (player.y > canvas.height - PLAYER_HEIGHT) {
                player.y = canvas.height - PLAYER_HEIGHT;
                player.velocityY = 0;
                player.isJumping = false;
            }

            ctx.fillStyle = '#00ff00';
            ctx.fillRect(player.x - player.width/2, player.y, player.width, player.height);
        }

        function updateObstacles() {
            const now = Date.now();
            if (now - lastObstacleTime > 1500) {
                obstacles.push(new Obstacle());
                lastObstacleTime = now;
            }

            obstacles = obstacles.filter(obstacle => {
                if (obstacle.checkCollision()) {
                    createCrashSound();
                    gameOver();
                    return false;
                }
                return !obstacle.update();
            });
        }

        function updateCoins() {
            const now = Date.now();
            if (now - lastCoinTime > 1000) {
                coinsList.push(new Coin());
                lastCoinTime = now;
            }

            coinsList = coinsList.filter(coin => {
                if (coin.checkCollision()) {
                    coins++;
                    coinsElement.textContent = `Coins: ${coins}`;
                    createCoinSound();
                    return false;
                }
                return !coin.update();
            });
        }

        function gameOver() {
            isGameOver = true;
            gameOverElement.style.display = 'block';
            finalScoreElement.textContent = score;
            finalCoinsElement.textContent = coins;
        }

        function restartGame() {
            score = 0;
            coins = 0;
            gameSpeed = 5;
            isGameOver = false;
            player.lane = 1;
            player.y = canvas.height - 100;
            player.velocityY = 0;
            obstacles = [];
            coinsList = [];
            lastObstacleTime = 0;
            lastCoinTime = 0;
            gameOverElement.style.display = 'none';
            scoreElement.textContent = 'Score: 0';
            coinsElement.textContent = 'Coins: 0';
        }

        function gameLoop() {
            if (!isGameOver) {
                score++;
                scoreElement.textContent = `Score: ${score}`;
                if (score % 500 === 0) gameSpeed += 0.5;
            }

            ctx.clearRect(0, 0, canvas.width, canvas.height);
            
            drawTrack();
            drawPlayer();
            
            obstacles.forEach(obstacle => obstacle.draw());
            coinsList.forEach(coin => coin.draw());
            
            updateObstacles();
            updateCoins();

            requestAnimationFrame(gameLoop);
        }

        // Start the game
        gameLoop();
    </script>
</body>
</html>
)=====";

void setup() {
    Serial.begin(115200);
    WiFi.softAP(AP_SSID, AP_PASS);
    
    Serial.println("\nSubway Runner Game Server Starting...");
    Serial.print("Connect to WiFi Network: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASS);
    Serial.print("Then visit: http://");
    Serial.println(WiFi.softAPIP());

    dnsServer.start(53, "*", WiFi.softAPIP());

    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", GAME_HTML);
    });
    
    server.begin();
    Serial.println("Game server started!");
}

void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
    delay(1);
}
