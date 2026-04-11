#!/usr/bin/env python3
"""Test script using named pipes with QEMU serial pipe."""
import os
import subprocess
import time
import threading
import sys

KERNEL_DIR = "/home/wyqsxsxz/Synapse-Kernel"
ISO = f"{KERNEL_DIR}/synapse.iso"
FIFO_IN = "/tmp/qemu_serial_in"
FIFO_OUT = "/tmp/qemu_serial_out"

def main():
    # Create FIFOs
    for f in [FIFO_IN, FIFO_OUT]:
        if os.path.exists(f):
            os.remove(f)
        os.mkfifo(f)

    output_lines = []
    stop_reader = threading.Event()

    def reader_thread():
        try:
            with open(FIFO_OUT, 'rb') as f:
                buf = b""
                while not stop_reader.is_set():
                    ready = select.select([f], [], [], 0.5)
                    if ready[0]:
                        data = f.read(4096)
                        if data:
                            buf += data
                            # Decode and print
                            try:
                                text = buf.decode('utf-8', errors='replace')
                                for line in text.split('\r'):
                                    line = line.strip()
                                    if line:
                                        print(line)
                                        output_lines.append(line)
                                buf = b""
                            except:
                                pass
        except Exception as e:
            print(f"Reader error: {e}")

    import select

    # Start reader thread
    t = threading.Thread(target=reader_thread)
    t.start()

    # Start QEMU
    print(f"Starting QEMU with {ISO}...")
    proc = subprocess.Popen(
        ["qemu-system-x86_64",
         "-cdrom", ISO,
         "-m", "128M",
         "-nographic",
         "-serial", "pipe:qemu_serial",
         "-monitor", "none"],
        cwd=KERNEL_DIR,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

    # Wait for boot (FIFO_OUT might not exist yet)
    time.sleep(2)

    # Open FIFO for writing
    try:
        fifo_in = open(FIFO_IN, 'w', buffering=1)
    except Exception as e:
        print(f"Cannot open FIFO_IN: {e}")
        proc.terminate()
        stop_reader.set()
        t.join()
        return

    # Wait for kernel boot
    print("Waiting for kernel boot...")
    for i in range(15):
        time.sleep(1)
        if any("waiting" in l or "synapse>" in l for l in output_lines):
            print("Kernel boot detected!")
            break

    # Send help command
    time.sleep(1)
    print("Sending 'help'...")
    fifo_in.write("help\n")
    fifo_in.flush()
    time.sleep(3)

    # Send icm_shell command
    print("Sending '/bin/icm_shell'...")
    fifo_in.write("/bin/icm_shell\n")
    fifo_in.flush()
    time.sleep(5)

    # Send exit
    fifo_in.write("exit\n")
    fifo_in.flush()
    time.sleep(2)

    fifo_in.close()
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()

    stop_reader.set()
    t.join(timeout=3)

    print("\n=== ALL OUTPUT ===")
    for line in output_lines:
        print(line)

    # Cleanup
    for f in [FIFO_IN, FIFO_OUT]:
        if os.path.exists(f):
            os.remove(f)

if __name__ == "__main__":
    main()
