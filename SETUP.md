# NIDS Setup & Quick Start Guide

This guide will help you get the Network Intrusion Detection System up and running on Windows in minutes.

## 5-Minute Setup

### Step 1: Install Npcap (Required)

1. Download Npcap installer from: https://npcap.com
2. Run the installer
3. Check "Npcap SDK" during installation
4. Complete the installation

**Verify Installation:**
- Check if `C:\Npcap` folder exists
- Should contain `include/` and `lib/` subdirectories

### Step 2: Install a Compiler

**Option A: MinGW (Recommended for beginners)**
- Download MSYS2 from: https://www.msys2.org
- Run installer
- In MSYS2 terminal, run:
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

**Option B: Use Visual Studio Community (Free)**
- Download from: https://visualstudio.microsoft.com/vs/community/
- Run installer
- Select "Desktop development with C++"
- Complete installation

### Step 3: Build NIDS

**Using MinGW:**
1. Open Command Prompt
2. Navigate to NIDS directory:
```bash
cd "C:\Users\YourUsername\OneDrive\Desktop\nids using c"
```
3. Build:
```bash
build.bat mingw
```

**Using MSVC:**
1. Open "Developer Command Prompt for Visual Studio" 
   (Search in Start Menu)
2. Navigate to NIDS directory
3. Build:
```bash
build.bat msvc
```

### Step 4: Run NIDS

1. Right-click Command Prompt → "Run as Administrator"
2. Navigate to NIDS directory
3. Run:
```bash
nids.exe
```
4. Select a network interface from the list
5. Wait for alerts (see Generating Test Traffic below)

## Compilation Commands (If Build Script Fails)

### MinGW - Manual Build

```bash
cd C:\path\to\nids
gcc -Wall -Wextra -O2 -std=c99 ^
    -IC:/Npcap/include ^
    -LC:/Npcap/lib ^
    -o nids.exe main.c capture.c parser.c detection.c logger.c ^
    -lwpcap -lws2_32
```

### MSVC - Manual Build

Run from Developer Command Prompt:

```cmd
pushd "C:\path\to\nids"

cl /W3 /O2 /D_CRT_SECURE_NO_WARNINGS ^
   /I"C:\Npcap\include" ^
   /Fe:nids.exe main.c capture.c parser.c detection.c logger.c ^
   /link /LIBPATH:"C:\Npcap\lib" wpcap.lib ws2_32.lib iphlpapi.lib

popd
```

## Common Issues & Fixes

### 1. "No network devices found"

**Problem**: NIDS can't detect your network interfaces

**Solution**:
```bash
# Check if Npcap service is running
net start npcap

# If not installed, reinstall Npcap with admin rights
# Right-click Command Prompt → Run as administrator
# Then run Npcap installer
```

### 2. "make: command not found"

**Problem**: `make` is not installed or not in PATH

**Solution**:
- **MinGW**: Install MSYS2 and gcc/make as described above
- **MSVC**: Run from Developer Command Prompt, use manual compilation commands
- **Alternative**: Use `build.bat` instead of `make` command

### 3. "fatal error: pcap.h: No such file or directory"

**Problem**: Npcap SDK is not installed or path is wrong

**Solution**:
1. Download and install Npcap SDK from https://npcap.com
2. Verify `C:\Npcap` exists
3. Edit Makefile and update paths:
```makefile
NPCAP_INCLUDE = C:/Npcap/include    # Adjust if installed elsewhere
NPCAP_LIB = C:/Npcap/lib
```

### 4. "Access denied" error when running

**Problem**: Need administrator privileges for packet capture

**Solution**:
1. Right-click Command Prompt → "Run as Administrator"
2. OR right-click `nids.exe` → Properties → Advanced → Check "Run as administrator"

### 5. Build fails with MSVC "cl.exe: not found"

**Problem**: Visual Studio toolchain not in PATH

**Solution**:
- Don't run from regular Command Prompt
- Use "Developer Command Prompt for Visual Studio" instead
- Search for it in Start Menu

## Testing & Generating Alerts

### Test 1: Port Scan Detection

Run a port scan against your computer from another machine:

```bash
# On another computer on the same network:
# Using Nmap (if available)
nmap -p 1-100 192.168.1.100  # Replace with NIDS host IP
```

Or use built-in Windows tools:

```bash
# Scan ports 1-100 on a target
for /L %i in (1,1,100) do (
    powershell -Command "Test-NetConnection -ComputerName 192.168.1.100 -Port %i -WarningAction SilentlyContinue | Select -Property TcpTestSucceeded"
)
```

**Expected Alert:**
```
[2024-03-29 14:23:45] ALERT: PORT_SCAN | Source: 192.168.1.50 | Destination: 192.168.1.100 | Port scan detected: 10 unique ports scanned
```

### Test 2: SYN Flood Detection

Using Hping3 (if available):

```bash
# On attacker machine with hping3
hping3 -S --flood 192.168.1.100 -p 80
```

Or use TCP SYN craft tool:

```powershell
# PowerShell script to send many SYN packets
# Note: Requires admin and raw socket support
# This is complex in Windows - use hping3 if possible
```

**Expected Alert:**
```
[2024-03-29 14:24:12] ALERT: SYN_FLOOD | Source: 192.168.1.50 | Destination: 192.168.1.100 | SYN flood detected: 50 SYN packets without ACK
```

### Test 3: ICMP Flood Detection

```bash
# From another computer, send continuous ping
ping -t 192.168.1.100

# Or use hping3
hping3 -1 --flood 192.168.1.100
```

**Expected Alert:**
```
[2024-03-29 14:25:34] ALERT: ICMP_FLOOD | Source: 192.168.1.50 | Destination: 192.168.1.100 | ICMP flood detected: 100 ICMP packets
```

### Test with Lower Thresholds (Safe Testing)

For safe testing without actual attacks:

```bash
# Use much lower thresholds for easier detection
nids.exe --port-threshold 2 --syn-threshold 5 --icmp-threshold 10
```

Then:
```bash
# Send 3 pings from another computer - should trigger alert
ping 192.168.1.100
```

## Running NIDS in Production

### Recommended Setup

1. **Run with lowered thresholds initially:**
```bash
nids.exe --port-threshold 15 --syn-threshold 80 --icmp-threshold 150
```

2. **Monitor for false positives for 24 hours**

3. **Tune thresholds based on your network baseline**

4. **Run as a background service** (optional):

Create a batch file `nids_service.bat`:
```batch
@echo off
:LOOP
nids.exe
timeout /t 5
goto LOOP
```

Then use Windows Task Scheduler:
- Create a new task
- Set it to run `nids_service.bat`
- Set trigger to "At startup"
- Set "Run with highest privileges"

### Monitor Logs

Real-time log monitoring:

```bash
# Watch alerts as they appear
powershell -Command "Get-Content alerts.log -Wait"

# Count alerts per type
findstr "ALERT" alerts.log | findstr "PORT_SCAN" | find /c "PORT_SCAN"
findstr "ALERT" alerts.log | findstr "SYN_FLOOD" | find /c "SYN_FLOOD"
findstr "ALERT" alerts.log | findstr "ICMP_FLOOD" | find /c "ICMP_FLOOD"
```

## Understanding the Output

### Console Output Example

```
====================================
Network Intrusion Detection System
Windows (Npcap/WinPcap)
====================================

Available network interfaces:
===================================
[0] \Device\NPF_{12345678-1234-1234-1234-123456789012}
    Description: Ethernet (Intel 82579LM)
[1] \Device\NPF_{87654321-4321-4321-4321-210987654321}
    Description: Wi-Fi (Qualcomm QCA6174)
===================================

Enter interface number (0-1): 0

[INFO] Selected interface: \Device\NPF_{12345678-1234-1234-1234-123456789012}
[INFO] Thresholds - Port: 10, SYN: 50, ICMP: 100
[INFO] Interface opened successfully
[INFO] Starting capture and processing threads
[INFO] NIDS is running. Press Ctrl+C to stop.
[INFO] Process thread started
[INFO] Capture thread started

[2024-03-29 14:23:45] ALERT: PORT_SCAN | Source: 192.168.1.50 | Destination: 192.168.1.100 | Port scan detected: 12 unique ports scanned

Ctrl+C pressed

[INFO] Stopping NIDS...
[INFO] Capture thread stopped
[INFO] Process thread stopped
[INFO] NIDS stopped successfully
[INFO] Alerts logged to: alerts.log
```

### Alert Format

```
[TIMESTAMP] ALERT: ATTACK_TYPE | Source: SOURCE_IP | Destination: DEST_IP | Details
```

Fields:
- **TIMESTAMP**: Date and time of alert (YYYY-MM-DD HH:MM:SS)
- **ATTACK_TYPE**: PORT_SCAN, SYN_FLOOD, or ICMP_FLOOD
- **SOURCE_IP**: IP address of attacker
- **DEST_IP**: Target IP address
- **Details**: Specific information about the attack

## Advanced Configuration

### Custom Thresholds

Start with defaults, then tune based on your network:

```bash
# Aggressive detection (more alerts, higher CPU)
nids.exe --port-threshold 5 --syn-threshold 30 --icmp-threshold 50

# Conservative detection (fewer alerts, less CPU)
nids.exe --port-threshold 20 --syn-threshold 100 --icmp-threshold 200

# Balanced (recommended starting point)
nids.exe --port-threshold 10 --syn-threshold 50 --icmp-threshold 100
```

### Specific Interface Monitoring

```bash
# Get your interface name from the first run
nids.exe

# Then use it directly
nids.exe --interface "\Device\NPF_{your-guid-here}"
```

## Next Steps

1. **Test thoroughly** with your network configuration
2. **Monitor logs** for false positives
3. **Adjust thresholds** to match your network baseline
4. **Integrate with SIEM** (if available)
5. **Set up alerts** to email/Slack when attacks detected

## Support & Troubleshooting

If you encounter issues:

1. **Check the README.md** for detailed documentation
2. **Verify Npcap is installed**: Run network sniffer tools to test
3. **Check logs**: Look in `alerts.log` for error details
4. **Run with verbose output**: Check console messages during startup
5. **Test with admin privileges**: Ensure running as administrator

## Quick Reference

| Command | Purpose |
|---------|---------|
| `nids.exe` | Run interactively (select interface) |
| `nids.exe --help` | Show all options |
| `build.bat mingw` | Build with MinGW |
| `build.bat msvc` | Build with MSVC |
| `make clean` | Remove build artifacts |

---

**Need help?** Check the README.md for comprehensive documentation.

**Ready to run?** Use `build.bat mingw` (or `msvc`) to build, then run `nids.exe` as administrator!
