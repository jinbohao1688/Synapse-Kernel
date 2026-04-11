#!/bin/bash
# Test kernel with serial input
cd /home/wyqsxsxz/Synapse-Kernel

# Build kernel and ISO if needed
if [ ! -f synapse.bin ]; then
    echo "Building kernel..."
    make 2>&1 | tail -3
fi

# Create named pipes for serial
rm -f serial_in serial_out
mkfifo serial_in serial_out

# Build ISO
cp synapse.bin iso/boot/synapse.bin
grub-mkrescue -o synapse.iso iso/ 2>/dev/null

# Start QEMU with serial on named pipes
# QEMU reads from serial_in (our commands) and writes to serial_out
echo "Starting QEMU..."
(qemu-system-x86_64 \
    -cdrom synapse.iso \
    -m 128M \
    -nographic \
    -serial pipe:serial \
    -monitor none 2>/dev/null) &
QEMU_PID=$!

sleep 8

# Send commands through the pipe
echo "Sending commands..."
echo "help" > serial_in
sleep 2
echo "/bin/icm_shell" > serial_in
sleep 3
echo "exit" > serial_in
sleep 2

# Close input
exec 3>&-
sleep 2

# Kill QEMU
kill $QEMU_PID 2>/dev/null

# Read output
echo "=== QEMU OUTPUT ==="
cat serial_out

# Cleanup
rm -f serial_in serial_out
