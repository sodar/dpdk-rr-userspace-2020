#!/bin/bash

set -exuo pipefail

function main()
{
    local do_hexdump=1

    while getopts "n" opt; do
        case $opt in
            n) do_hexdump=0 ;;
            *) ;;
        esac
    done

    local app_args=(
        --no-shconf
        -w pci:0000:01:00.1,rx_vec_en=0,tx_vec_en=0
        -l 0-1
        --master-lcore 0
        --log-level lib.eal:info
        --log-level pmd.net.mlx5:info
    )

    (
        if [ "${do_hexdump}" == "0" ]; then
            export NO_HEXDUMP="1"
        fi

        rr record ./build/dpdk-rx-intr "${app_args[@]}"
    )
}

main "$@"
