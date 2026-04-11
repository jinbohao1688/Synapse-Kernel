#!/usr/bin/env python3
"""PTY-based QEMU interaction: sends commands and captures output."""
import os
import pty
import select
import subprocess
import time
import sys
import re
import termios
import tty

KERNEL_DIR = "/home/wyqsxsxz/Synapse-Kernel"
ISO = f"{KERNEL_DIR}/synapse.iso"

def main():
    master_fd, slave_fd = pty.openpty()
    print(f"PTY: master={master_fd}, slave={slave_fd}")

    # Make slave raw
    try:
        tty.setraw(slave_fd)
    except:
        pass

    # Start QEMU with slave PTY as serial
    proc = subprocess.Popen(
        ["qemu-system-x86_64",
         "-cdrom", ISO,
         "-m", "128M",
         "-nographic",
         "-serial", f"fd:{slave_fd}",
         "-monitor", "none"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        cwd=KERNEL_DIR,
        pass_fds=[slave_fd]
    )
    os.close(slave_fd)

    output_buf = b""
    prompt_seen = False

    # Read loop
    deadline = time.time() + 60
    while time.time() < deadline:
        if proc.poll() is not None:
            print("QEMU exited!")
            break
        r, _, _ = select.select([master_fd], [], [], 0.5)
        if r:
            try:
                data = os.read(master_fd, 4096)
            except OSError:
                break
            if data:
                output_buf += data
                # Print to our stdout
                try:
                    sys.stdout.write(data.decode('utf-8', errors='replace'))
                    sys.stdout.flush()
                except:
                    pass
                # Check for shell prompt
                if b"synapse> " in output_buf or b"[SH] waiting" in output_buf:
                    if not prompt_seen:
                        print("\n=== SHELL PROMPT DETECTED ===")
                        prompt_seen = True
                        time.sleep(1)
                        # Send help
                        print("\n>>> Sending: help")
                        os.write(master_fd, b"help\n")
                        time.sleep(3)
                        # Send icm_shell
                        print("\n>>> Sending: /bin/icm_shell")
                        os.write(master_fd, b"/bin/icm_shell\n")
                        time.sleep(5)
                        # Send exit
                        os.write(master_fd, b"exit\n")
                        time.sleep(2)
        else:
            # No data, check if we should send
            if prompt_seen:
                break

    os.close(master_fd)
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
    print("\n=== DONE ===")

if __name__ == "__main__":
    main()
