#ifndef WEB_HTML_H
#define WEB_HTML_H

const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>WLED Video Streamer</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin: 20px; background-color: #222; color: #eee; }
        .card { background-color: #333; padding: 20px; border-radius: 10px; max-width: 400px; margin: 0 auto; box-shadow: 0 4px 8px rgba(0,0,0,0.5); }
        input[type=text], input[type=number], select, input[type=range] { width: 100%; padding: 10px; margin: 10px 0; box-sizing: border-box; border-radius: 5px; border: none; }
        input[type=checkbox] { transform: scale(1.5); margin: 15px; }
        button { background-color: #4CAF50; color: white; padding: 14px 20px; margin: 10px 0; border: none; border-radius: 4px; cursor: pointer; width: 100%; font-size: 16px; }
        button:hover { background-color: #45a049; }
        label { display: block; text-align: left; margin-top: 10px; font-weight: bold; }
        .inline-label { display: inline-block; text-align: left; font-weight: bold; margin-bottom: 10px;}
        .disabled-overlay { opacity: 0.5; pointer-events: none; }
    </style>
</head>
<body>
    <h2>WLED Streamer Setup</h2>
    <div class="card">
        <label for="ip">WLED Matrix IP:</label>
        <input type="text" id="ip" value="%IP%">
        
        <label for="width">Matrix Width:</label>
        <input type="number" id="width" value="%WIDTH%">
        
        <label for="height">Matrix Height:</label>
        <input type="number" id="height" value="%HEIGHT%">

        <hr style="border:1px solid #555; margin: 20px 0;">

        <label for="controlMode">Control Source:</label>
        <select id="controlMode" onchange="updateUI()">
            <option value="0" %CTRL_HW%>Hardware Pins</option>
            <option value="1" %CTRL_WEB%>Web UI</option>
        </select>
        
        <div id="controlsGroup">
            <label for="streamEnable" class="inline-label">Enable Stream:</label>
            <input type="checkbox" id="streamEnable" %STREAM_CHK%>
            
            <label for="effect">Current Effect:</label>
            <select id="effect">
                <option value="0" %S0%>Normal</option>
                <option value="1" %S1%>Spooky</option>
                <option value="2" %S2%>Grayscale</option>
                <option value="3" %S3%>Invert</option>
                <option value="4" %S4%>Sepia</option>
                <option value="5" %S5%>Psychedelic</option>
            </select>
            
            <hr style="border:1px solid #555; margin: 20px 0;">
            <h3>Camera Image Tuning</h3>

            <label for="contrast">Contrast (-2 to 2): <span id="ctVal">%CONTRAST%</span></label>
            <input type="range" id="contrast" min="-2" max="2" value="%CONTRAST%" oninput="document.getElementById('ctVal').innerText=this.value">

            <label for="saturation">Saturation (-2 to 2): <span id="stVal">%SATURATION%</span></label>
            <input type="range" id="saturation" min="-2" max="2" value="%SATURATION%" oninput="document.getElementById('stVal').innerText=this.value">

            <label for="autoExposure" class="inline-label">Auto Exposure (AEC):</label>
            <input type="checkbox" id="autoExposure" %AE_CHK% onchange="updateUI()">

            <div id="manualExpGroup">
                <label for="exposureVal">Manual Exposure Time (0-1200): <span id="evVal">%EXPOSURE%</span></label>
                <input type="range" id="exposureVal" min="0" max="1200" value="%EXPOSURE%" oninput="document.getElementById('evVal').innerText=this.value">
            </div>
        </div>

        <button onclick="saveConfig()">Save Configuration</button>
        <p id="status"></p>
    </div>

    <script>
        function updateUI() {
            var mode = document.getElementById('controlMode').value;
            var group = document.getElementById('controlsGroup');
            var aeChecked = document.getElementById('autoExposure').checked;
            var expGroup = document.getElementById('manualExpGroup');

            if (mode === "0") {
                group.classList.add('disabled-overlay');
            } else {
                group.classList.remove('disabled-overlay');
            }

            if (aeChecked) {
                expGroup.classList.add('disabled-overlay');
            } else {
                expGroup.classList.remove('disabled-overlay');
            }
        }
        
        // Call once on load
        updateUI();

        function saveConfig() {
            var ip = document.getElementById('ip').value;
            var w = document.getElementById('width').value;
            var h = document.getElementById('height').value;
            var mode = document.getElementById('controlMode').value;
            var en = document.getElementById('streamEnable').checked ? 1 : 0;
            var e = document.getElementById('effect').value;
            var ct = document.getElementById('contrast').value;
            var st = document.getElementById('saturation').value;
            var ae = document.getElementById('autoExposure').checked ? 1 : 0;
            var ev = document.getElementById('exposureVal').value;
            
            fetch('/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'ip=' + ip + '&w=' + w + '&h=' + h + '&m=' + mode + '&en=' + en + '&e=' + e + '&ct=' + ct + '&st=' + st + '&ae=' + ae + '&ev=' + ev
            })
            .then(response => {
                if(response.ok) document.getElementById('status').innerText = "Saved successfully!";
                else document.getElementById('status').innerText = "Error saving.";
            });
        }
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_HTML_H

#endif // WEB_HTML_H

