#!/usr/bin/env python3
"""Send commands to QEMU via PTY and capture output."""
import os
import pty
import select
import subprocess
import time
import sys
import re

KERNEL_DIR = "/home/wyqsxsxz/Synapse-Kernel"
ISO = f"{KERNEL_DIR}/synapse.iso"

def run_test(commands):
    # Start QEMU with serial on PTY
    master, slave = pty.openpty()
    
    proc = subprocess.Popen(
        ["qemu-system-x86_64",
         "-cdrom", ISO,
         "-m", "128M",
         "-nographic",
         "-serial", f"fd:{slave}",
         "-monitor", "none"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        cwd=KERNEL_DIR,
        pass_fds=[slave]
    )
    os.close(slave)
    
    output = b""
    boot_done = False
    
    # Wait for boot and send commands
    for i in range(20):
        time.sleep(1)
        try:
            r, _, _ = select.select([master], [], [], 0.1)
            if r:
                data = os.read(master, 4096)
                if data:
                    output += data
                    # Check if shell is ready
                    if b"synapse> " in output or b"[SH] waiting" in output:
                        boot_done = True
                        break
        except:
            break
    
    if not boot_done:
        print("Kernel did not boot fully!")
    
    # Send commands
    for cmd in commands:
        time.sleep(0.5)
        os.write(master, (cmd + "\n").encode())
        time.sleep(2)
        try:
            r, _, _ = select.select([master], [], [], 0.5)
            while r:
                data = os.read(master, 4096)
                if data:
                    output += data
                r, _, _ = select.select([master], [], [], 0.1)
        except:
            pass
    
    # Let output settle
    time.sleep(3)
    try:
        r, _, _ = select.select([master], [], [], 1)
        while r:
            data = os.read(master, 4096)
            if data:
                output += data
            r, _, _ = select.select([master], [], [], 0.5)
    except:
        pass
    
    os.close(master)
    proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()
    
    return output.decode('utf-8', errors='replace')

def main():
    commands = ["help", "/bin/icm_shell"]
    output = run_test(commands)
    
    # Strip ANSI
    output = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', output)
    output = re.sub(r'\x1b[c?7l', '', output)
    output = output.replace('\r', '')
    
    # Print
    for line in output.split('\n'):
        print(line)

if __name__ == "__main__":
    main()
