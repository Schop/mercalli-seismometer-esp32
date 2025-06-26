#pragma once
#include <Arduino.h>

const char WIFI_HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Mercalli Seismometer</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="refresh" content="5">
<style>
  body { font-family: Arial, sans-serif; background-color: #121212; color: #e0e0e0; text-align: center; }
  h1 { color: #e0e0e0; }
  .container { max-width: 800px; margin: auto; padding: 20px; background-color: #1e1e1e; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.5); }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-top: 20px; }
  .card { background-color: #2c2c2c; padding: 20px; border-radius: 8px; }
  .card h2 { margin-top: 0; color: #e0e0e0; }
  .mercalli-peak { font-size: 4em; color: #cf6679; font-weight: bold; }
  .mercalli-now { font-size: 2em; color: #e0e0e0; }
  .peak-values p, .current-values p { margin: 5px 0; font-size: 1.1em; }
  .peak-values span, .current-values span { font-weight: bold; color: #f2f2f2; }
  button { background-color: #cf6679; color: #ffffff; border: none; padding: 15px 30px; font-size: 1em; border-radius: 5px; cursor: pointer; margin-top: 20px; }
  button:hover { background-color:rgb(221, 21, 57); color: #ffffff; }
  .footer { margin-top: 20px; font-size: 0.8em; color: #888; }
</style>
<script>
  function resetPeaks() {
    fetch('/reset', { method: 'POST' })
      .then(response => {
        if (response.ok) { 
          console.log('Peak values reset'); 
          setTimeout(() => location.reload(), 500); // Refresh after a short delay
        } else { 
          console.error('Reset failed'); 
        }
      });
  }
  
  function updateData() {
      fetch('/data')
          .then(response => response.json())
          .then(data => {
              document.getElementById('mercalli-peak').innerText = data.mercalli_peak;
              document.getElementById('mercalli-now').innerText = data.mercalli_now;
              document.getElementById('x-peak').innerText = data.x_peak.toFixed(3);
              document.getElementById('y-peak').innerText = data.y_peak.toFixed(3);
              document.getElementById('z-peak').innerText = data.z_peak.toFixed(3);
              document.getElementById('dev-mag-peak').innerText = data.dev_mag_peak.toFixed(3);
              document.getElementById('x-now').innerText = data.x_now.toFixed(3);
              document.getElementById('y-now').innerText = data.y_now.toFixed(3);
              document.getElementById('z-now').innerText = data.z_now.toFixed(3);
              document.getElementById('dev-mag-now').innerText = data.dev_mag_now.toFixed(3);
          });
  }

  // Initial call and periodic updates
  document.addEventListener('DOMContentLoaded', function() {
      updateData(); // Initial load
      setInterval(updateData, 2000); // Update every 2 seconds
  });
</script>
</head>
<body>
  <div class="container">
    <h1>Mercalli Seismometer</h1>
    <div class="grid">
      <div class="card mercalli">
        <h2>PEAK MERCALLI VALUE</h2>
        <p class="mercalli-peak" id="mercalli-peak">0</p>
      </div>
      <div class="card mercalli">
        <h2>CURRENT MERCALLI VALUE</h2>
        <p class="mercalli-now" id="mercalli-now">0</p>
      </div>
    </div>
    <div class="grid">
      <div class="card peak-values">
        <h2>Peak Deviations (m/s<sup>2</sup>)</h2>
        <p>X: <span id="x-peak">0.000</span></p>
        <p>Y: <span id="y-peak">0.000</span></p>
        <p>Z: <span id="z-peak">0.000</span></p>
        <p>Magnitude: <span id="dev-mag-peak">0.000</span></p>
      </div>
      <div class="card current-values">
        <h2>Current Deviations (m/s<sup>2</sup>)</h2>
        <p>X: <span id="x-now">0.000</span></p>
        <p>Y: <span id="y-now">0.000</span></p>
        <p>Z: <span id="z-now">0.000</span></p>
        <p>Magnitude: <span id="dev-mag-now">0.000</span></p>
      </div>
    </div>
    <button onclick="resetPeaks()">Reset Peak Values</button>
    <div class="footer">
      <p>IP Address: %IP_ADDRESS%</p>
      <p>(c) 2025 John Schop</p>
    </div>
  </div>
</body>
</html>
)rawliteral";
