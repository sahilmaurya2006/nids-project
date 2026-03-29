# NIDS Examples & Usage Scenarios

This file provides practical examples and usage scenarios for the Network Intrusion Detection System.

## Basic Examples

### Example 1: Interactive Mode (Easiest)

**The most straightforward way to run NIDS:**

```bash
# Step 1: Open Command Prompt as Administrator
# (Right-click Command Prompt → Run as Administrator)

# Step 2: Navigate to NIDS directory
cd C:\Users\YourUsername\OneDrive\Desktop\nids using c

# Step 3: Run the program
nids.exe

# Step 4: Select your network interface when prompted
# Available network interfaces:
# ===================================
# [0] \Device\NPF_{...guid...}
#     Description: Ethernet (Intel)
# [1] \Device\NPF_{...guid...}
#     Description: Wi-Fi (Qualcomm)
# ===================================
# 
# Enter interface number (0-1): 0

# NIDS is now running and monitoring traffic!
# You will see alerts in real-time:
# [2024-03-29 14:23:45] ALERT: PORT_SCAN | Source: 192.168.1.100 | ...
```

### Example 2: Command-Line Mode with Custom Interface

**If you already know your interface:**

```bash
# Get your interface GUID from first run, then use it directly
nids.exe --interface "\Device\NPF_{12345678-1234-1234-1234-123456789012}"

# With custom thresholds
nids.exe --interface "\Device\NPF_{...}" --port-threshold 8 --syn-threshold 40
```

### Example 3: Testing Mode with Low Thresholds

**For testing and generating alerts quickly:**

```bash
# Use very low thresholds to trigger alerts easily
nids.exe --port-threshold 3 --syn-threshold 10 --icmp-threshold 20

# Send a test ping from another computer - should trigger alert quickly
```

## Attack Scenarios & Detection

### Scenario 1: Port Scan Attack

**Description**: Attacker scans multiple ports on target system

**How to simulate** (from another computer on same network):

**Using Nmap (if available):**
```bash
# Find NIDS computer IP first
ipconfig

# Then from attacker machine
nmap -p 1-100 192.168.1.100  # Replace with NIDS host IP
```

**Using PowerShell (Windows):**
```powershell
# Scan ports 1-50 on target
1..50 | ForEach-Object {
    Test-NetConnection -ComputerName 192.168.1.100 -Port $_ -WarningAction SilentlyContinue | 
    Select-Object @{Name="Port";Expression={$_}}, TcpTestSucceeded
}
```

**Expected NIDS output:**
```
[2024-03-29 14:23:45] ALERT: PORT_SCAN | Source: 192.168.1.50 | Destination: 192.168.1.100 | Port scan detected: 12 unique ports scanned
```

**NIDS behavior:**
- Tracks unique destination ports per source IP
- When count exceeds threshold (default 10), generates alert
- Resets counter after alert

---

### Scenario 2: SYN Flood Attack

**Description**: Attacker floods target with TCP SYN packets without completing handshake

**How to simulate** (requires specialized tools):

**Using Hping3:**
```bash
# Install hping3 or download Windows binary
# Then run:
hping3 -S --flood -p 80 192.168.1.100
```

**Expected NIDS output:**
```
[2024-03-29 14:24:12] ALERT: SYN_FLOOD | Source: 192.168.1.50 | Destination: 192.168.1.100 | SYN flood detected: 67 SYN packets without ACK
```

**NIDS behavior:**
- Tracks SYN packets without ACK responses
- Counts unfinished TCP handshakes
- Triggers alert when SYN count exceeds threshold (default 50)

---

### Scenario 3: ICMP Flood (Ping Flood)

**Description**: Attacker floods target with ICMP echo requests (ping)

**How to simulate** (simple and safe):

**From another computer:**
```bash
# Continuous ping (use Ctrl+C to stop)
ping -t 192.168.1.100

# Or send specific number of packets
ping -n 200 192.168.1.100
```

**Expected NIDS output:**
```
[2024-03-29 14:25:34] ALERT: ICMP_FLOOD | Source: 192.168.1.50 | Destination: 192.168.1.100 | ICMP flood detected: 128 ICMP packets
```

**NIDS behavior:**
- Counts ICMP packets per source IP
- Triggers alert when ICMP count exceeds threshold (default 100)

---

## Performance Testing

### Example: High-Traffic Network

**For networks with high baseline traffic:**

```bash
# Use higher thresholds to reduce false positives
nids.exe --port-threshold 50 --syn-threshold 200 --icmp-threshold 500

# Monitor baseline and adjust accordingly
# After 24 hours, analyze alerts.log and tune further
```

### Example: Network Baseline Analysis

```bash
# Run NIDS for 24 hours with default settings
# Log file: alerts.log

# Then analyze:
echo Analyzing alerts...

# Count total alerts
findstr /c:"ALERT:" alerts.log | find /c ":"

# Count by type
echo Port scans detected:
findstr /c:"PORT_SCAN" alerts.log | find /c ":"

echo SYN floods detected:
findstr /c:"SYN_FLOOD" alerts.log | find /c ":"

echo ICMP floods detected:
findstr /c:"ICMP_FLOOD" alerts.log | find /c ":"

# List top attacking IPs
echo Top attacking IPs:
findstr /c:"Source:" alerts.log | more
```

## Integration Examples

### Example: Log to Text File with Timestamp

**Capture output to file:**

```bash
# Run NIDS and save console output
nids.exe > nids_output_%date:~10,4%%date:~4,2%%date:~7,2%.log 2>&1
```

### Example: Continuous Monitoring Script

**Batch file for continuous operation:**

Create `monitor_nids.bat`:

```batch
@echo off
setlocal enabledelayedexpansion

:RESTART
echo [%date% %time%] Starting NIDS...
nids.exe --port-threshold 10 --syn-threshold 50 --icmp-threshold 100

echo.
echo NIDS crashed or stopped. Restarting in 10 seconds...
timeout /t 10

goto RESTART
```

Run with:
```bash
monitor_nids.bat
```

### Example: Alert Notification Script

**PowerShell script to send email on high-severity alerts:**

Create `monitor_alerts.ps1`:

```powershell
# Check for new alerts and send email
$logFile = "alerts.log"
$lastCheck = Get-Content "lastcheck.txt" -ErrorAction SilentlyContinue
if (-not $lastCheck) { $lastCheck = (Get-Date).AddHours(-1) }

# Search for recent alerts
$alerts = Select-String -Path $logFile -Pattern "ALERT:" | 
          Where-Object { [datetime]::Parse($_.Line.Substring(1, 19)) -gt $lastCheck }

if ($alerts) {
    # Count by type
    $synFloods = $alerts | Where-Object { $_ -match "SYN_FLOOD" } | Measure-Object | ForEach-Object Count
    $portScans = $alerts | Where-Object { $_ -match "PORT_SCAN" } | Measure-Object | ForEach-Object Count
    $icmpFloods = $alerts | Where-Object { $_ -match "ICMP_FLOOD" } | Measure-Object | ForEach-Object Count
    
    Write-Host "Found $($alerts.Count) new alerts:"
    Write-Host "  SYN Floods: $synFloods"
    Write-Host "  Port Scans: $portScans"
    Write-Host "  ICMP Floods: $icmpFloods"
    
    # TODO: Send email notification here
    # Send-MailMessage -To admin@example.com -From nids@example.com ...
}

# Update last check time
(Get-Date) | Out-File "lastcheck.txt"
```

Run periodically with Task Scheduler:
1. Open Task Scheduler
2. Create Basic Task
3. Set trigger to "Every 5 minutes"
4. Action: "Start a program" → `powershell.exe`
5. Arguments: `-NoProfile -ExecutionPolicy Bypass -File monitor_alerts.ps1`

---

## Advanced Usage Scenarios

### Scenario: Monitor Multiple Interfaces

**Run multiple NIDS instances for different network segments:**

```batch
# Run in separate windows
start cmd /k "nids.exe --interface \Device\NPF_{GUID1}"
start cmd /k "nids.exe --interface \Device\NPF_{GUID2}"

# Each creates its own alerts.log (or modify code to use different log files)
```

### Scenario: Fine-Tuned Detection for Specific Network

```bash
# Production network baseline (low false positives):
nids.exe --port-threshold 30 --syn-threshold 150 --icmp-threshold 300

# Development network (catch more issues):
nids.exe --port-threshold 8 --syn-threshold 40 --icmp-threshold 80

# High-security network (aggressive):
nids.exe --port-threshold 3 --syn-threshold 20 --icmp-threshold 50
```

### Scenario: Correlate with Windows Event Viewer

**NIDS alerts + Windows firewall logs:**

```powershell
# Export Windows Firewall logs
Get-WinEvent -LogName "Security" -FilterXPath "*[System[EventID=5156]]" | 
    Export-Csv firewall_events.csv

# Cross-reference with NIDS alerts.log for correlation
```

---

## Troubleshooting Examples

### Issue: No Alerts Being Generated

**Diagnostic steps:**

```bash
# 1. Verify NIDS is capturing packets
# Run with very low thresholds
nids.exe --port-threshold 1 --syn-threshold 1 --icmp-threshold 1

# 2. Send a simple test ping
# From another computer:
ping 192.168.1.100

# If still no alerts, check:
# - Is NIDS and target connected to same network?
# - Is Npcap capturing on correct interface?
# - Check alerts.log file is being created
```

### Issue: Alert Generation Too Frequent (False Positives)

**Decrease sensitivity:**

```bash
# Increase thresholds
nids.exe --port-threshold 25 --syn-threshold 100 --icmp-threshold 250

# Monitor for 1 hour
# If still too many alerts, increase further
nids.exe --port-threshold 40 --syn-threshold 150 --icmp-threshold 400
```

### Issue: Missing Attacks (Too High Thresholds)

**Increase sensitivity:**

```bash
# Decrease thresholds
nids.exe --port-threshold 5 --syn-threshold 30 --icmp-threshold 50

# Test with deliberate attack
# Note: May create false positives, but won't miss attacks
```

---

## Performance Monitoring Example

**Monitor NIDS resource usage:**

```powershell
# PowerShell script to monitor nids.exe
$proc = Get-Process nids -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "NIDS Process Statistics:"
    Write-Host "  PID: $($proc.Id)"
    Write-Host "  Memory: $([math]::Round($proc.WorkingSet / 1MB, 2)) MB"
    Write-Host "  CPU Threads: $($proc.Threads.Count)"
    Write-Host "  Start Time: $($proc.StartTime)"
}
```

---

## Real-World Example: Enterprise Network Setup

**Monitor company network for intrusions:**

```batch
@echo off
REM Enterprise NIDS Setup Script

REM Run NIDS with production thresholds
REM Conservative to avoid false positives in busy network
nids.exe ^
    --interface "\Device\NPF_{company-network-guid}" ^
    --port-threshold 25 ^
    --syn-threshold 100 ^
    --icmp-threshold 200

REM Create timestamped backup of alerts
setlocal enabledelayedexpansion
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%c%%a%%b)
for /f "tokens=1-2 delims=/:" %%a in ('time /t') do (set mytime=%%a%%b)

if exist alerts.log (
    copy alerts.log alerts_!mydate!_!mytime!.log
)
```

---

## Summary Reference Table

| Attack Type | Threshold | Trigger | Source | Target |
|-------------|-----------|---------|--------|--------|
| Port Scan | 10+ ports | Unique destination ports | External/Internal | Your system |
| SYN Flood | 50+ packets | TCP SYN without ACK | External/Internal | Your system |
| ICMP Flood | 100+ packets | ICMP echo requests | External/Internal | Your system |

---

**For more information, see README.md and SETUP.md**
