#!/bin/bash
cd /home/wyqsxsxz/Synapse-Kernel

lo=2
hi=256

while [ $lo -lt $hi ]; do
    mid=$(( (lo + hi) / 2 ))
    echo "=== Testing $mid PDEs ==="
    
    # Replace only the first for loop (with "256" literal) in init_paging
    sed "s/i < 256; i++)/i < $mid; i++)/" kernel/mm/paging.c > kernel/mm/paging_test.c
    cp kernel/mm/paging.c kernel/mm/paging_backup.c
    cp kernel/mm/paging_test.c kernel/mm/paging.c
    
    make kernel 2>/dev/null | grep -q "Kernel built" || { echo "BUILD FAILED"; cp kernel/mm/paging_backup.c kernel/mm/paging.c; exit 1; }
    
    output=$(timeout 8 qemu-system-i386 -nographic -kernel synapse.bin -m 128M 2>&1)
    if echo "$output" | grep -q "paging ON"; then
        echo "PASS: $mid PDEs works"
        lo=$(( mid + 1 ))
    else
        echo "FAIL: $mid PDEs crashes"
        hi=$mid
    fi
    
    cp kernel/mm/paging_backup.c kernel/mm/paging.c
    make kernel 2>/dev/null | grep -q "Kernel built"
done

echo "=== EXACT THRESHOLD: $lo PDEs ==="
