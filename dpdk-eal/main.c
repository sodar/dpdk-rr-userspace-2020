#include <assert.h>
#include <stdio.h>

#include <signal.h>

#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_eal.h>

static rte_atomic16_t is_running = RTE_ATOMIC16_INIT(1);

static void
sigint_handler(int signum)
{
	printf("dpdk-example: SIGINT caught!\n");
	rte_atomic16_set(&is_running, 0);
}

int
main(int argc, char **argv)
{
	int ret;

	signal(SIGINT, sigint_handler);

	ret = rte_eal_init(argc, argv);
	assert(ret >= 0);

	while (rte_atomic16_read(&is_running)) {}

	ret = rte_eal_cleanup();
	assert(ret == 0);

	return 0;
}
