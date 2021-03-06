#!/bin/bash

set -x

function main()
{
    local app_args=(
        --no-shconf
        -w pci:0000:01:00.1,rx_vec_en=0
        -l 0-1
        --master-lcore 0
        --log-level lib.eal:debug
        --log-level pmd.net.mlx5:debug
    )

    rr record ./build/dpdk-eal "${app_args[@]}"
}

main "$@"
