#!/bin/bash
cd /home/wyqsxsxz/Synapse-Kernel

# Test with 131 PDEs (fails)
sed "s/i < 256; i++)/i < 131; i++)/" kernel/mm/paging.c > kernel/mm/paging_test.c
cp kernel/mm/paging_backup.c kernel/mm/paging.c 2>/dev/null || true
sed "s/i < 256; i++)/i < 131; i++)/" kernel/mm/paging.c > kernel/mm/paging_test.c
cp kernel/mm/paging_test.c kernel/mm/paging.c
make kernel 2>/dev/null | grep -q "Kernel built"

# Start QEMU with GDB server, capture serial to file
rm -f /tmp/qemu_gdb.log
timeout 8 qemu-system-i386 -kernel synapse.bin -m 128M -s -S -monitor none -serial file:/tmp/qemu_gdb.log 2>/dev/null &
QEMU_PID=$!
sleep 1

# Connect GDB and set breakpoint at crash location
timeout 6 gdb -q -batch \
  -ex "target remote localhost:1234" \
  -ex "break *0x100000" \
  -ex "continue" \
  -ex "bt" \
  -ex "info registers" \
  -ex "x/8i \$eip" \
  2>/dev/null

kill $QEMU_PID 2>/dev/null
echo "=== Serial log (last 20 lines) ==="
tail -20 /tmp/qemu_gdb.log 2>/dev/null
