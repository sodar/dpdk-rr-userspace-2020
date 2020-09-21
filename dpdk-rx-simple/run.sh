#!/bin/bash

set -exuo pipefail

function main()
{
    local args=(
        --no-shconf
        -w pci:0000:01:00.1,rx_vec_en=0
        -l 0-1
        --master-lcore 0
        --log-level lib.eal:debug
        --log-level pmd.net.mlx5:debug
    )

    ./build/dpdk-rx-simple "${args[@]}"
}

main "$@"
