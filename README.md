# DPDK with rr examples

This repository contains a few DPDK applications used in research for talk "Debugging DPDK applications using rr" at DPDK Userspace Summit 2020.

This repository contains the following applications:

- `dpdk-eal` - initialize EAL, wait for SIGINT, clean up;
- `dpdk-rx-simple` - initialize EAL, configure port, receives single packet bursts, wait for SIGINT, clean up;
- `dpdk-rx-intr` - initialize EAL, configure port, wait for RX interrupt and receive 32 packet burst, wait for SIGINT, clean up;

Applications were built and tested with:

- DPDK v20.08,
- libibverbs v31.0.

Inside each directory there are two scripts:

- `run.sh` - start the application,
- `run-rr.sh` - start the application under `rr record`:
    - requires that `rr` is in `PATH`.

On top of that for `dpdk-rx-intr` run scripts take one optional command line parameter:

```bash
# Hexdump each received packet
run.sh
# or
run-rr.sh

# Only log when lcore was woken up
run.sh -n
# or
run-rr.sh -n
```
