#pragma once
#include <Arduino.h>

const char WIFI_HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Mercalli Seismometer</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { font-family: Arial, sans-serif; background-color: #f0f0f0; color: #333; text-align: center; margin: 20px; }
  h1 { color: #333; margin-bottom: 30px; }
  .container { max-width: 800px; margin: 0 auto; padding: 20px; background-color: white; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-top: 20px; }
  .card { background-color: #f8f9fa; padding: 20px; border-radius: 8px; border: 1px solid #e9ecef; }
  .card h2 { margin-top: 0; color: #333; font-size: 1.2em; }
  .mercalli-peak { font-size: 4em; color: #dc3545; font-weight: bold; }
  .mercalli-now { font-size: 2em; color: #28a745; }
  .peak-values p, .current-values p { margin: 5px 0; font-size: 1.1em; }
  .peak-values span, .current-values span { font-weight: bold; color: #007bff; }
  button { background-color: #007bff; color: white; border: none; padding: 15px 30px; font-size: 1em; border-radius: 5px; cursor: pointer; margin-top: 20px; }
  button:hover { background-color: #0056b3; }
  .footer { margin-top: 20px; font-size: 0.8em; color: #666; }
  .status-indicator { display: inline-block; width: 12px; height: 12px; border-radius: 50%; margin-right: 8px; }
  .status-connected { background-color: #28a745; }
  .status-disconnected { background-color: #dc3545; }
  .loading { color: #666; font-style: italic; }
  .event-info { background-color: #e7f3ff; padding: 10px; border-radius: 5px; margin-top: 15px; font-size: 0.9em; }
  .event-info h3 { margin-top: 0; margin-bottom: 10px; color: #333; }
  .time-sync { color: #28a745; font-weight: bold; }
  .no-time-sync { color: #dc3545; font-style: italic; }
  .events-link { display: inline-block; margin-top: 10px; padding: 8px 16px; background: #17a2b8; color: white; text-decoration: none; border-radius: 5px; font-size: 0.9em; }
  .events-link:hover { background: #138496; color: white; text-decoration: none; }
</style>
<script>
  let isLoading = false;
  
  function resetPeaks() {
    if (isLoading) return;
    isLoading = true;
    
    const button = document.querySelector('button');
    const originalText = button.textContent;
    button.textContent = 'Resetting...';
    button.disabled = true;
    
    fetch('/reset', { method: 'POST' })
      .then(response => {
        if (response.ok) { 
          console.log('Peak values reset'); 
          updateData(); // Immediate update
        } else { 
          console.error('Reset failed'); 
        }
      })
      .catch(error => {
        console.error('Reset error:', error);
      })
      .finally(() => {
        setTimeout(() => {
          button.textContent = originalText;
          button.disabled = false;
          isLoading = false;
        }, 1000);
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
        
        // Update event information
        const eventInfo = document.getElementById('event-info');
        if (data.timeSync) {
          document.getElementById('time-status').innerHTML = '<span class="time-sync">Time Synchronized</span>';
          document.getElementById('event-count').innerText = data.eventCount || 0;
          
          if (data.lastEvent) {
            document.getElementById('last-event').innerHTML = 
              'Last Event: Mercalli ' + data.lastEvent.mercalli + ' at ' + data.lastEvent.timestamp;
          } else {
            document.getElementById('last-event').innerText = 'No events recorded yet';
          }
        } else {
          document.getElementById('time-status').innerHTML = '<span class="no-time-sync">Time not synchronized</span>';
          document.getElementById('event-count').innerText = 'N/A';
          document.getElementById('last-event').innerText = 'Time sync required for event logging';
        }
        
        // Remove loading states
        document.querySelectorAll('.loading').forEach(el => {
          el.classList.remove('loading');
        });
      })
      .catch(error => {
        console.error('Data fetch error:', error);
      });
  }

  // Optimized loading - no auto-refresh, manual updates only
  document.addEventListener('DOMContentLoaded', function() {
    // Initial load after a brief delay to let page render
    setTimeout(updateData, 100);
    
    // Update every 3 seconds (less frequent than before)
    setInterval(updateData, 3000);
  });
</script>
</head>
<body>
  <div class="container">
    <h1><span class="status-indicator status-connected"></span>Mercalli Seismometer</h1>
    <div class="grid">
      <div class="card mercalli">
        <h2>PEAK MERCALLI VALUE</h2>
        <p class="mercalli-peak loading" id="mercalli-peak">Loading...</p>
      </div>
      <div class="card mercalli">
        <h2>CURRENT MERCALLI VALUE</h2>
        <p class="mercalli-now loading" id="mercalli-now">Loading...</p>
      </div>
    </div>
    <div class="grid">
      <div class="card peak-values">
        <h2>Peak Deviations (m/s<sup>2</sup>)</h2>
        <p>X: <span class="loading" id="x-peak">---</span></p>
        <p>Y: <span class="loading" id="y-peak">---</span></p>
        <p>Z: <span class="loading" id="z-peak">---</span></p>
        <p>Magnitude: <span class="loading" id="dev-mag-peak">---</span></p>
      </div>
      <div class="card current-values">
        <h2>Current Deviations (m/s<sup>2</sup>)</h2>
        <p>X: <span class="loading" id="x-now">---</span></p>
        <p>Y: <span class="loading" id="y-now">---</span></p>
        <p>Z: <span class="loading" id="z-now">---</span></p>
        <p>Magnitude: <span class="loading" id="dev-mag-now">---</span></p>
      </div>
    </div>
    <button onclick="resetPeaks()">Reset Peak Values</button>
    <div class="event-info">
      <h3>Event Logging</h3>
      <div id="time-status" class="no-time-sync">⚠ Time not synchronized</div>
      <div>Events Logged: <span id="event-count">0</span></div>
      <div id="last-event">No events recorded yet</div>
      <a href="/events" target="_blank" class="events-link">View Event Log</a>
    </div>
    <div class="footer">
      <p>Device IP: %IP_ADDRESS%</p>
      <p>(c) 2025 John Schop</p>
    </div>
  </div>
</body>
</html>
)rawliteral";
