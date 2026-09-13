#ifndef WEB_HTML_H
#define WEB_HTML_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WLED Video Streamer</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #222; color: #fff; max-width: 600px; margin: 0 auto; padding: 20px; display: flex; flex-direction: column; }
        .card { background-color: #333; padding: 20px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); margin-bottom: 20px; display: flex; flex-direction: column; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="text"], input[type="number"], select { align-self: stretch; padding: 8px; margin-bottom: 15px; border-radius: 4px; border: 1px solid #555; background: #444; color: white; box-sizing: border-box; }
        .toggle-switch { display: flex; align-items: center; margin-bottom: 15px; }
        .toggle-switch input { margin-right: 10px; transform: scale(1.5); }
        button { align-self: stretch; background-color: #007bff; color: white; border: none; padding: 10px 20px; font-size: 16px; border-radius: 4px; cursor: pointer; }
        button:hover { background-color: #0056b3; }
        #status { text-align: center; margin-top: 15px; font-weight: bold; color: #4CAF50; }
        #vidCanvas { align-self: stretch; border-radius: 8px; margin-bottom: 20px; background-color: #000; image-rendering: pixelated; box-shadow: 0 4px 6px rgba(0,0,0,0.5); width: 288px; height: 160px; margin-left: auto; margin-right: auto; display: block; }
        input[type="range"] { align-self: stretch; margin-bottom: 15px; }
    </style>
</head>
<body>

    <h2 style="text-align:center;">WLED Video Streamer</h2>

    <canvas id="vidCanvas" width="%WIDTH%" height="%HEIGHT%"></canvas>

    <div class="card">
        <h3>Stream Control</h3>
        <div class="toggle-switch">
            <input type="checkbox" id="streamEn" %STREAM_CHK%>
            <label for="streamEn" style="margin-bottom:0;">Enable Video Stream</label>
        </div>
        
        <label for="ctrlMode">Control Source</label>
        <select id="ctrlMode">
            <option value="0" %CTRL_HW%>Hardware (Buttons/Knobs)</option>
            <option value="1" %CTRL_WEB%>Web Interface</option>
        </select>
        
        <label for="fps">Target Framerate (FPS)</label>
        <select id="fps">
            <option value="5" %F5%>5 FPS (Chunky/Stop-Motion)</option>
            <option value="10" %F10%>10 FPS (Stable/Recommended)</option>
            <option value="12" %F12%>12 FPS</option>
            <option value="15" %F15%>15 FPS</option>
            <option value="20" %F20%>20 FPS (Max)</option>
        </select>
        
        <label for="webBrt">Brightness (Web Control) (0-255)</label>
        <input type="range" id="webBrt" min="0" max="255" value="%BRIGHTNESS%">
        <span id="brtValue" style="display:block; text-align:center; margin-bottom: 10px;">%BRIGHTNESS%</span>
    </div>

    <div class="card">
        <h3>Camera Hardware Settings</h3>
        <div style="display: flex; gap: 20px;">
            <div class="toggle-switch">
                <input type="checkbox" id="vflip" %VFLIP_CHK%>
                <label for="vflip" style="margin-bottom:0;">Flip Vertical</label>
            </div>
            <div class="toggle-switch">
                <input type="checkbox" id="hmirror" %HMIRROR_CHK%>
                <label for="hmirror" style="margin-bottom:0;">Mirror Horizontal</label>
            </div>
        </div>
        
        <label for="contrast">Hardware Contrast (-2 to 2)</label>
        <input type="range" id="contrast" min="-2" max="2" value="%CONTRAST%">
        <span id="conValue" style="display:block; text-align:center; margin-bottom: 10px;">%CONTRAST%</span>
        
        <label for="saturation">Hardware Saturation (-2 to 2)</label>
        <input type="range" id="saturation" min="-2" max="2" value="%SATURATION%">
        <span id="satValue" style="display:block; text-align:center; margin-bottom: 10px;">%SATURATION%</span>
    </div>

    <div class="card">
        <h3>Target Setup</h3>
        <label for="ip">WLED IP Address</label>
        <input type="text" id="ip" value="%IP%" placeholder="192.168.1.100">

        <div style="display: flex; gap: 10px;">
            <div style="flex: 1; display: flex; flex-direction: column;">
                <label for="width">Matrix Width</label>
                <input type="number" id="width" value="%WIDTH%">
            </div>
            <div style="flex: 1; display: flex; flex-direction: column;">
                <label for="height">Matrix Height</label>
                <input type="number" id="height" value="%HEIGHT%">
            </div>
        </div>

        <label for="ddp">DDP Network Color Order</label>
        <select id="ddp">
            <option value="0" %C0%>RGB (Standard)</option>
            <option value="1" %C1%>GRB (WS2812B / Neopixel)</option>
            <option value="2" %C2%>BGR</option>
            <option value="3" %C3%>RBG</option>
            <option value="4" %C4%>GBR</option>
            <option value="5" %C5%>BRG</option>
        </select>
        <small style="display:block;margin-bottom:15px;color:#aaa;">If red and green are swapped on your LED panel, change this.</small>

        <label for="effect">Video Effect</label>
        <select id="effect">
            <option value="0" %S0%>Normal</option>
            <option value="1" %S1%>Psychedelic</option>
            <option value="2" %S2%>Retro 8-Bit (Palette)</option>
            <option value="3" %S3%>Cyberpunk (Palette)</option>
            <option value="4" %S4%>Thermal Camera</option>
            <option value="5" %S5%>Digital Matrix</option>
            <option value="6" %S6%>Neon Edge Glow (Shadows)</option>
        </select>
    </div>

    <button onclick="saveConfig()">Save Settings</button>
    <div id="status"></div>
    <div id="wifiStatus" style="text-align:center; margin-top: 10px; color: #888;"></div>
    
    <button onclick="rebootDevice()" style="background-color: #dc3545; margin-top: 20px;">Reboot ESP32</button>

    <script>
        const brtSlider = document.getElementById('webBrt');
        const brtValue = document.getElementById('brtValue');
        brtSlider.oninput = function() { brtValue.innerHTML = this.value; }
        
        const conSlider = document.getElementById('contrast');
        const conValue = document.getElementById('conValue');
        conSlider.oninput = function() { conValue.innerHTML = this.value; }
        
        const satSlider = document.getElementById('saturation');
        const satValue = document.getElementById('satValue');
        satSlider.oninput = function() { satValue.innerHTML = this.value; }

        function saveConfig() {
            const btn = document.querySelector('button');
            btn.innerText = "Saving...";
            
            const formData = new URLSearchParams();
            formData.append('ip', document.getElementById('ip').value);
            formData.append('w', document.getElementById('width').value);
            formData.append('h', document.getElementById('height').value);
            formData.append('m', document.getElementById('ctrlMode').value);
            formData.append('en', document.getElementById('streamEn').checked ? '1' : '0');
            formData.append('e', document.getElementById('effect').value);
            formData.append('b', document.getElementById('webBrt').value);
            formData.append('ddp', document.getElementById('ddp').value);
            formData.append('vf', document.getElementById('vflip').checked ? '1' : '0');
            formData.append('hm', document.getElementById('hmirror').checked ? '1' : '0');
            formData.append('con', document.getElementById('contrast').value);
            formData.append('sat', document.getElementById('saturation').value);
            formData.append('fps', document.getElementById('fps').value);

            fetch('/save', {
                method: 'POST',
                body: formData
            }).then(r => {
                btn.innerText = "Save Settings";
                document.getElementById('status').innerText = "Settings Saved!";
                setTimeout(() => document.getElementById('status').innerText = "", 3000);
            }).catch(e => {
                btn.innerText = "Save Settings";
                document.getElementById('status').style.color = "red";
                document.getElementById('status').innerText = "Error Saving!";
            });
        }
        
        function rebootDevice() {
            if(confirm("Are you sure you want to reboot the ESP32?")) {
                fetch('/reboot', { method: 'POST' }).then(() => {
                    document.getElementById('status').innerText = "Rebooting... please wait.";
                    setTimeout(() => location.reload(), 5000);
                });
            }
        }
        
        function fetchStatus() {
            fetch('/status').then(r => r.json()).then(data => {
                document.getElementById('wifiStatus').innerText = "Wi-Fi Signal (RSSI): " + data.rssi + " dBm";
            }).catch(e => {});
        }
        setInterval(fetchStatus, 5000);
        setTimeout(fetchStatus, 500);

        let ws;
        function connectWS() {
            ws = new WebSocket("ws://" + window.location.hostname + "/ws");
            ws.binaryType = "arraybuffer";
            
            let canvas = document.getElementById('vidCanvas');
            let ctx = canvas.getContext('2d');
            let w = parseInt(document.getElementById('width').value);
            let h = parseInt(document.getElementById('height').value);
            let imgData = ctx.createImageData(w, h);
            
            ws.onmessage = function(e) {
                if(e.data instanceof ArrayBuffer) {
                    let bytes = new Uint8Array(e.data);
                    let j = 0;
                    for(let i = 0; i < bytes.length; i += 3) {
                        imgData.data[j++] = bytes[i];
                        imgData.data[j++] = bytes[i+1];
                        imgData.data[j++] = bytes[i+2];
                        imgData.data[j++] = 255;
                    }
                    ctx.putImageData(imgData, 0, 0);
                }
            };
            ws.onclose = function() {
                setTimeout(connectWS, 2000);
            };
        }
        window.onload = connectWS;
    </script>
</body>
</html>
)rawliteral";

#endif
