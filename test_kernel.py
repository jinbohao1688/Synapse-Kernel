#!/usr/bin/env python3
"""Test script: sends commands to QEMU via serial and captures output."""
import subprocess
import time
import sys
import os

KERNEL_DIR = "/home/wyqsxsxz/Synapse-Kernel"
ISO = f"{KERNEL_DIR}/synapse.iso"
LOG = f"{KERNEL_DIR}/qemu_serial.log"

def run_test(commands):
    """Run QEMU, send commands, return serial log."""
    # Clean log
    if os.path.exists(LOG):
        os.remove(LOG)

    # Start QEMU in background
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

    # Wait for kernel boot
    time.sleep(8)

    # Check if QEMU is still running
    if proc.poll() is not None:
        print("QEMU died early!")
        return

    # Send commands
    for cmd in commands:
        time.sleep(1)
        if proc.poll() is not None:
            print(f"QEMU died before sending '{cmd}'")
            break
        # Send character by character
        proc.stdin.write(cmd.encode())
        proc.stdin.flush()

    time.sleep(3)
    proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()

def main():
    commands = ["help\n", "/bin/icm_shell\n"]
    run_test(commands)

    # Read and print log
    if not os.path.exists(LOG):
        print("No serial log found!")
        return

    with open(LOG) as f:
        data = f.read()

    # Strip ANSI
    import re
    data = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', data)
    data = re.sub(r'\x1b[c?7l', '', data)
    data = re.sub(r'\x02J', '\n', data)
    data = data.replace('\r', '')

    lines = [l for l in data.split('\n') if l.strip()]
    for line in lines:
        print(line)

if __name__ == "__main__":
    main()
