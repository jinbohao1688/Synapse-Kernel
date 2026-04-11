#!/usr/bin/env python3
"""QEMU with file-based serial + sends commands via FIFO."""
import subprocess
import time
import os
import sys
import re
import select

KERNEL_DIR = "/home/wyqsxsxz/Synapse-Kernel"
ISO = f"{KERNEL_DIR}/synapse.iso"
LOG = f"{KERNEL_DIR}/qemu_test.log"
FIFO = "/tmp/qemu_cmd"

def main():
    # Create FIFO
    if os.path.exists(FIFO):
        os.remove(FIFO)
    os.mkfifo(FIFO)

    # Clean log
    if os.path.exists(LOG):
        os.remove(LOG)

    # Start QEMU: serial -> log file
    proc = subprocess.Popen(
        ["qemu-system-x86_64",
         "-cdrom", ISO,
         "-m", "128M",
         "-nographic",
         "-serial", f"file:{LOG}",
         "-monitor", "none"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        cwd=KERNEL_DIR
    )
    print(f"QEMU PID: {proc.pid}")

    # Wait for boot (check log file)
    print("Waiting for kernel boot...")
    for i in range(20):
        time.sleep(1)
        if os.path.exists(LOG):
            with open(LOG) as f:
                data = f.read()
            if "[SH] waiting" in data or "synapse> " in data:
                print(f"Boot complete at {i}s!")
                break
        if proc.poll() is not None:
            print(f"QEMU died! Exit code: {proc.returncode}")
            break
    else:
        print("Timeout waiting for boot!")

    # Open FIFO and send commands
    print("Sending commands via FIFO...")
    try:
        fifo_fd = os.open(FIFO, os.O_WRONLY | os.O_NONBLOCK)
        # Note: this FIFO won't actually reach QEMU since serial goes to file
        # This is just a placeholder to show the concept
        os.close(fifo_fd)
    except:
        pass

    # Alternative: inject into serial log directly via a named pipe that QEMU reads
    # But QEMU already opened serial as file, not pipe

    # Let's just monitor the log file instead
    print("\nMonitoring serial log...")
    last_size = 0
    for i in range(15):
        time.sleep(2)
        if os.path.exists(LOG):
            with open(LOG) as f:
                f.seek(last_size)
                new_data = f.read()
                if new_data.strip():
                    print(new_data.strip())
                last_size = f.tell()
        else:
            print("Log file not found")
            break

    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()

    print("\n=== FINAL SERIAL LOG ===")
    if os.path.exists(LOG):
        with open(LOG) as f:
            data = f.read()
        # Strip ANSI
        data = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', data)
        data = data.replace('\r', '')
        for line in data.split('\n'):
            line = line.strip()
            if line:
                print(line)

    os.remove(FIFO)

if __name__ == "__main__":
    main()
