#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>

#include <rte_atomic.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_hexdump.h>
#include <rte_launch.h>
#include <rte_lcore.h>

#define APP_NAME "dpdk-rx-simple"

#define PKTMBUF_POOL_NAME "pktmbuf_pool"
#define PKTMBUF_POOL_SIZE ((1 << 10) - 1)

#define MASTER_LCORE (0)
#define SLAVE_LCORE (1)

/**
 * Default port configuration
 */
static uint16_t nb_rx_queues = 1;
static uint16_t nb_tx_queues = 1;

static uint16_t rx_queue_id = 0;
static uint16_t tx_queue_id = 0;

static uint16_t nb_rx_desc = 256;
static uint16_t nb_tx_desc = 256;

static struct rte_eth_dev_info dev_info;

static struct rte_eth_conf dev_conf = {
	.rxmode = {
		.mq_mode = ETH_MQ_RX_NONE,
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
	.intr_conf = {
		.rxq = 1,
	},
};

/**
 * Main loop control
 */
static rte_atomic16_t is_running = RTE_ATOMIC16_INIT(1);

/**
 * Signal handlers
 */
static void
sigint_handler(int signum)
{
	rte_atomic16_set(&is_running, 0);
}

/**
 * Configure passed port for RX/TX
 */
static int
configure_port(uint16_t port_id, struct rte_mempool *pktmbuf_pool)
{
	int ret;

	memset(&dev_info, 0, sizeof(dev_info));
	ret = rte_eth_dev_info_get(port_id, &dev_info);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_eth_dev_info_get() failed for port %u: (%d) %s\n",
			APP_NAME, port_id, ret, rte_strerror(-ret));
		return -1;
	}

	ret = rte_eth_dev_configure(port_id, nb_rx_queues, nb_tx_queues, &dev_conf);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_dev_dev_configure() failed for port %u: (%d) %s\n",
			APP_NAME, port_id, ret, rte_strerror(-ret));
		return -2;
	}
	printf("%s: port 0 configured\n", APP_NAME);

	ret = rte_eth_dev_adjust_nb_rx_tx_desc(port_id, &nb_rx_desc, &nb_tx_desc);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_eth_dev_adjust_nb_rx_tx_desc() failed for port %u: (%d) %s\n",
			APP_NAME, port_id, ret, rte_strerror(-ret));
		return -3;
	}
	printf("%s: port %u, rx/tx desc adjusted, rx=%u, tx=%u\n", APP_NAME, port_id,
		nb_rx_desc, nb_tx_desc);

	struct rte_eth_txconf txq_conf = dev_info.default_txconf;
	ret = rte_eth_tx_queue_setup(port_id, tx_queue_id, nb_tx_desc,
				     rte_eth_dev_socket_id(0), &txq_conf);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_eth_tx_queue_setup() failed for port %u, queue %u: (%d) %s\n",
			APP_NAME, port_id, tx_queue_id, ret, rte_strerror(-ret));
		return -4;
	}
	printf("%s: port %u, TX queue %u setup done\n", APP_NAME, port_id, tx_queue_id);

	struct rte_eth_rxconf rxq_conf = dev_info.default_rxconf;
	ret = rte_eth_rx_queue_setup(port_id, rx_queue_id, nb_rx_desc,
				     rte_eth_dev_socket_id(0), &rxq_conf, pktmbuf_pool);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_eth_rx_queue_setup() failed for port %u, queue %u: (%d) %s\n",
			APP_NAME, port_id, rx_queue_id, ret, rte_strerror(-ret));
		return -5;
	}
	printf("%s: port %u, RX queue %u setup done\n", APP_NAME, port_id, tx_queue_id);

	ret = rte_eth_dev_set_ptypes(port_id, RTE_PTYPE_UNKNOWN, NULL, 0);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_eth_dev_set_ptypes() failed for port %u: (%d) %s\n",
			APP_NAME, port_id, ret, rte_strerror(-ret));
		return -6;
	}
	printf("%s: port %u, Ptype set to UNKNOWN\n", APP_NAME, port_id);

	return 0;
}


/**
 * Start port and wait until link is UP
 */
static int
start_and_wait_for_link(uint16_t port_id)
{
	int ret;

	ret = rte_eth_dev_start(port_id);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_eth_dev_start() failed for port %u: (%d) %s\n",
			APP_NAME, port_id, ret, rte_strerror(-ret));
		return -1;
	}
	printf("%s: port %u started\n", APP_NAME, port_id);

	ret = rte_eth_promiscuous_enable(port_id);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_eth_promiscuous_enable() failed for port %u: (%d) %s\n",
			APP_NAME, port_id, ret, rte_strerror(-ret));
		return -2;
	}
	printf("%s: port %u promiscuous mode enabled\n", APP_NAME, port_id);

	struct rte_eth_link link = { 0 };
	ret = rte_eth_link_get_nowait(port_id, &link);
	if (ret < 0) {
		fprintf(stderr, "%s: rte_eth_link_get_nowait() failed for port %u: (%d) %s\n",
			APP_NAME, port_id, ret, rte_strerror(-ret));
		return -3;
	}
	printf("%s: port %u link speed   = %u\n", APP_NAME, port_id, link.link_speed);
	printf("%s: port %u link duplex  = %u\n", APP_NAME, port_id, link.link_duplex);
	printf("%s: port %u link autoneg = %u\n", APP_NAME, port_id, link.link_autoneg);
	printf("%s: port %u link status  = %u\n", APP_NAME, port_id, link.link_status);

	return 0;
}

/**
 * Stop and disable port
 */
static void
stop_and_close(uint16_t port_id)
{
	rte_eth_dev_stop(port_id);
	printf("%s: port %u stopped\n", APP_NAME, port_id);

	rte_eth_dev_close(port_id);
	printf("%s: port %u closed\n", APP_NAME, port_id);
}

/**
 * Worker routine
 */
static int
worker(__attribute__((unused)) void *dummy)
{
	uint16_t port_id = 0;

	if (rte_lcore_id() != SLAVE_LCORE) {
		return 0;
	}

	printf("%s: %s() running on lcore %u\n", APP_NAME, __func__, rte_lcore_id());

	uint64_t received = 0;
	while (rte_atomic16_read(&is_running)) {
		struct rte_mbuf *mbuf[32] = { 0 };
		uint16_t nb_rx = rte_eth_rx_burst(port_id, rx_queue_id, mbuf, 1);
		RTE_ASSERT(0 <= nb_rx && nb_rx <= 1);
		if (nb_rx == 1) {
			uint8_t buffer[2048] = { 0 };
			uint32_t len = rte_pktmbuf_pkt_len(mbuf[0]);
			const uint8_t *data = rte_pktmbuf_read(mbuf[0], 0, len, buffer);
			printf("%s: %s() packet received, len=%u\n", APP_NAME, __func__, len);
			rte_hexdump(stdout, NULL, data, len);
			rte_pktmbuf_free(mbuf[0]);
		}
		received += nb_rx;
	}

	printf("%s: %s() packets received = %llu\n",APP_NAME, __func__, received);
	printf("%s: %s() ending\n", APP_NAME, __func__);

	return 0;
}

int
main(int argc, char **argv)
{
	struct rte_mempool *pktmbuf_pool = NULL;
	int ret;

	signal(SIGINT, sigint_handler);

	ret = rte_eal_init(argc, argv);
	if (ret == -1)
		rte_panic("%s: EAL initialization failed: (%d) %s\n", APP_NAME, rte_errno,
		          rte_strerror(rte_errno));

	printf("%s: EAL initialized\n", APP_NAME);

	pktmbuf_pool = rte_pktmbuf_pool_create("pktmbuf_pool", PKTMBUF_POOL_SIZE,
		0, 0, RTE_MBUF_DEFAULT_BUF_SIZE, SOCKET_ID_ANY);
	if (pktmbuf_pool == NULL) {
		fprintf(stderr, "%s: rte_pktmbuf_pool_create() failed: (%d) %s\n",
		        APP_NAME, rte_errno, rte_strerror(rte_errno));
		goto cleanup_eal;
	}
	printf("%s: mempool '%s' initialized\n", APP_NAME, PKTMBUF_POOL_NAME);

	ret = configure_port(0, pktmbuf_pool);
	if (ret < 0) {
		rte_panic("%s: port configuration failed\n", APP_NAME);
		goto cleanup_mempool;
	}

	ret = start_and_wait_for_link(0);
	if (ret < 0) {
		rte_panic("%s: port startup failed\n", APP_NAME);
		goto cleanup_mempool;
	}

	rte_eal_mp_remote_launch(worker, NULL, SKIP_MASTER);

	unsigned int lcore_id = 0;
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			printf("%s: something bad happened on lcore %u\n", APP_NAME, lcore_id);
			break;
		}
	}

cleanup_port:
	stop_and_close(0);

cleanup_mempool:
	rte_mempool_free(pktmbuf_pool);
	printf("%s: mempool '%s' freed\n", APP_NAME, PKTMBUF_POOL_NAME);

cleanup_eal:
	ret = rte_eal_cleanup();
	if (ret < 0)
		rte_panic("%s: EAL cleanup failed: (%d) %s\n", APP_NAME, -ret,
		          rte_strerror(-ret));

	printf("%s: EAL cleaned up\n", APP_NAME);
	printf("%s: exiting\n", APP_NAME);

	return 0;
}
