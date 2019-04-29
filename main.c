/*
MIT License

Copyright (c) 2019 Bharath Paulraj <bharathpaul@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_common.h>
#include <rte_malloc.h>
#include <string.h>
#include <signal.h>
#include "xml/ezxml.h"
#include "main.h"
#include "portrx.h"
#include "porttx.h"
#include "route.h"
#include "netlink.h"
#include "logutil.h"

static struct rte_eth_conf ethConf = {
  .rxmode = {
    .mq_mode = ETH_MQ_RX_RSS,
    .max_rx_pkt_len = ETHER_MAX_LEN,
    .split_hdr_size = 0,
    .offloads = (DEV_RX_OFFLOAD_CRC_STRIP |
#if 0 /* Use it only for SR-IOV */
           DEV_RX_OFFLOAD_CHECKSUM),
#else
    0),
#endif
  },
  .rx_adv_conf = {
    .rss_conf = {
      .rss_key = NULL,
#if 0 /* Use it only for SR-IOV */
      .rss_hf = ETH_RSS_IP,
#endif
    },
  },
  .txmode = {
    .mq_mode = ETH_MQ_TX_NONE,
  },
};

static int
mainLoop (__attribute__((unused)) void *arg)
{
  uint8_t u8Index = 0;
  data_t *pData = rte_zmalloc ("data", sizeof (data_t), 0);
  while (1)
  {
    for (u8Index = 0; u8Index < vRouterg.u8ModuleCnt; u8Index++)
    {
      vRouterg.pModule[u8Index]->pProcess(pData);
    }
  }
	return SUCCESS;
}

void registerModules (modules_t *pModules)
{
  if (vRouterg.u8ModuleCnt > MAX_MODULES)
  {
   rte_panic ("Unable to add new module\n");
  }

  vRouterg.pModule[vRouterg.u8ModuleCnt] = pModules;
  vRouterg.u8ModuleCnt++;
}

static void setLogLevel (void)
{
  uint32_t u32Level = 0;
  ezxml_t cfg = ezxml_parse_file("route.xml");

  if (cfg == NULL)
  {
    rte_panic ("Unable to parse route.xml file\n");
  }

  char *pBuff = ezxml_child(cfg, "log_level")->txt;

  if (strcmp (pBuff, "debug") == 0)
  {
    u32Level = RTE_LOG_DEBUG;
  }
  else if (strcmp (pBuff, "error") == 0)
  {
    u32Level = RTE_LOG_ERR;
  }
  else if (strcmp (pBuff, "warn") == 0)
  {
    u32Level = RTE_LOG_WARNING;
  }
  else
  {
    u32Level = RTE_LOG_CRIT;
  }

  vRouterg.u32LogLevel = u32Level;
  rte_log_set_level (RTE_LOGTYPE_VROUTER, vRouterg.u32LogLevel);

  return;
}

static const char * loglevel_to_string (void)
{
  switch (vRouterg.u32LogLevel) 
  {
    case 0: 
      return "disabled";
    case RTE_LOG_CRIT: 
      return "critical";
    case RTE_LOG_ERR: 
      return "error";
    case RTE_LOG_WARNING: 
      return "warning";
    case RTE_LOG_DEBUG: 
      return "debug";
    default: 
      return "unknown";
  }
}


static void openLogFile (void)
{
  FILE *fp = fopen ("vrouter.log", "a");

  if (fp == NULL)
  {
    rte_panic ("Unable to open log file\n");
  }

  if (rte_openlog_stream (fp) != 0)
  {
    rte_panic ("Unable to open log stream\n");
  }

  LOG_CRIT ("vRouter started with log-level %s", loglevel_to_string ());

  return;
}


static void fetchPortStats (int32_t i32Signal)
{
  struct rte_eth_stats ethStats;
  uint8_t u8Count = 0;

  LOG_DEBUG ("Received signal %d", i32Signal);

  for (u8Count = 0; u8Count < vRouterg.u8PortCnt; u8Count++)
  {
    if (rte_eth_stats_get (vRouterg.portConf[u8Count].u16PortId, &ethStats) < 0)
    {
      LOG_CRIT ("Unable to fetch port statistics for port (%d)", 
                 vRouterg.portConf[u8Count].u16PortId);
      continue;
    }

    LOG_CRIT ("Port (%d) - Rx Count: %lu", vRouterg.portConf[u8Count].u16PortId, 
               ethStats.ipackets);
    LOG_CRIT ("Port (%d) - Tx Count: %lu", vRouterg.portConf[u8Count].u16PortId, 
               ethStats.opackets);
    LOG_CRIT ("Port (%d) - Rx Bytes: %lu", vRouterg.portConf[u8Count].u16PortId, 
               ethStats.ibytes);
    LOG_CRIT ("Port (%d) - Tx Bytes: %lu", vRouterg.portConf[u8Count].u16PortId, 
               ethStats.obytes);
    LOG_CRIT ("Port (%d) - Rx Errors: %lu", vRouterg.portConf[u8Count].u16PortId, 
               ethStats.ierrors);
    LOG_CRIT ("Port (%d) - Tx Errors: %lu", vRouterg.portConf[u8Count].u16PortId, 
               ethStats.oerrors);
    LOG_CRIT ("Port (%d) - Rx NoMbuf: %lu", vRouterg.portConf[u8Count].u16PortId, 
               ethStats.rx_nombuf);
  }

  return;
}


int
main(int argc, char **argv)
{
  struct rte_eth_txconf *pTxConf;
  struct rte_eth_rxconf *pRxConf;
	int32_t i32Ret = 0;
  uint32_t u32MpSize = 0;
  uint16_t u16PortId = 0;
	uint8_t u8CoreId = 0;
  uint8_t u8QCnt = 0;
  struct rte_eth_dev_info dev_info;
  struct rte_eth_link link;
  uint8_t u8Index = 0;

  memset (&vRouterg, 0, sizeof (vRouterg));
  memset (&dev_info, 0, sizeof (struct rte_eth_dev_info));
  memset (&link, 0, sizeof (struct rte_eth_link));

  signal (SIGUSR1, fetchPortStats);

	i32Ret = rte_eal_init(argc, argv);

	if (i32Ret < 0)
  {
    rte_panic("Cannot init EAL\n");
  }

  setLogLevel ();
  openLogFile ();

  /* based on the coremask, derive the no of queues */
  for (u8CoreId = 0; u8CoreId < RTE_MAX_LCORE; u8CoreId++) 
  {
    if (rte_lcore_is_enabled(u8CoreId))
    {
      vRouterg.lcoreConf[vRouterg.u8CoreCnt].u8CoreId = u8CoreId;
      /* Assing the Rx and Tx Queue for the port */
      vRouterg.lcoreConf[vRouterg.u8CoreCnt].u16RxQ = u8QCnt;
      vRouterg.lcoreConf[vRouterg.u8CoreCnt].u16TxQ = u8QCnt;
      u8QCnt++;
      vRouterg.u8CoreCnt++;
    }
  }
  vRouterg.u8PortCnt = rte_eth_dev_count_avail ();
  LOG_CRIT ("vRouter started with %d number of ports", vRouterg.u8PortCnt);

  /* Calculation of mempool size is based on the l3fwd appln */
  u32MpSize = RTE_MAX ((vRouterg.u8PortCnt * u8QCnt * RX_DESC) +
                       (vRouterg.u8PortCnt * vRouterg.u8CoreCnt * MAX_PKT_BURST) +
                       (vRouterg.u8PortCnt *  u8QCnt * TX_DESC) +
                       (vRouterg.u8CoreCnt * MEMPOOL_CACHE_SIZE),
                       8192);

  /* Socket aware is not supported as of now*/
  vRouterg.pMempool = rte_pktmbuf_pool_create ("mbufpool", u32MpSize,
                                               MEMPOOL_CACHE_SIZE, 0,
                                               RTE_MBUF_DEFAULT_BUF_SIZE, 0);

  if (vRouterg.pMempool == NULL)
  {
    LOG_CRIT ("Unable to initialize mempools. Exiting...");
    rte_panic ("Cannot initialize mempool\n");
  }


  /* Prepare port config structure */

  uint8_t u8Count = 0;
  RTE_ETH_FOREACH_DEV (u16PortId)
  {
    i32Ret = rte_eth_dev_configure (u16PortId, u8QCnt, u8QCnt, &ethConf);

    if (i32Ret < 0)
    {
      LOG_CRIT ("Unable to configure port %d. Exiting...", u16PortId);
      rte_panic("Cannot configure port\n");
    }
    /* Get the mac address of the port and store it in port conf */
    rte_eth_macaddr_get (u16PortId, &vRouterg.portConf[u8Count].macAddr);
    vRouterg.portConf[u8Count].u8RxQCnt = u8QCnt;
    vRouterg.portConf[u8Count].u8TxQCnt = u8QCnt;
    vRouterg.portConf[u8Count].u16PortId = u16PortId;
    u8Count++;
  }

  if (vRouterg.u8PortCnt != u8Count)
  {
    LOG_CRIT ("Some issue with port db. Exiting...");
    rte_panic ("Some issue with port db\n");
  }

  for (u8Count = 0; u8Count < vRouterg.u8PortCnt; u8Count++)
  {
    /* In current implementation, TxQcount = RxQcount. So
     * Don't worry about the differences
     */
    rte_eth_dev_info_get (vRouterg.portConf[u8Count].u16PortId, &dev_info);
    pTxConf = &dev_info.default_txconf;
    pTxConf->offloads =  ethConf.txmode.offloads;
    pRxConf = &dev_info.default_rxconf;
    pRxConf->offloads =  ethConf.rxmode.offloads;

    for (u8QCnt = 0; u8QCnt < vRouterg.portConf[u8Count].u8TxQCnt; u8QCnt++)
    {
      i32Ret = rte_eth_tx_queue_setup (vRouterg.portConf[u8Count].u16PortId,
                                       u8QCnt, TX_DESC, 0, pTxConf);
      if (i32Ret < 0)
      {
        LOG_CRIT ("Unable to configure Tx Queue. Exiting...");
        rte_panic ("Unable to configure Tx Queue\n");
      }
      
      i32Ret = rte_eth_rx_queue_setup (vRouterg.portConf[u8Count].u16PortId,
                                       u8QCnt, RX_DESC, 0, pRxConf,
                                       vRouterg.pMempool);
      if (i32Ret < 0)
      {
        LOG_CRIT ("Unable to configure Rx Queue. Exiting...");
        rte_panic ("Unable to configure Rx Queue\n");
      }

      i32Ret = rte_eth_dev_start (vRouterg.portConf[u8Count].u16PortId);

      if (i32Ret < 0)
      {
        LOG_CRIT ("Unable to start port. Exiting...");
        rte_panic ("Unable to start port\n");
      }
      
      rte_eth_promiscuous_enable (vRouterg.portConf[u8Count].u16PortId);
    }

    rte_eth_link_get (vRouterg.portConf[u8Count].u16PortId, &link);

    if (link.link_status == ETH_LINK_DOWN)
    {
      LOG_CRIT ("Port (%d) is in down state.", vRouterg.portConf[u8Count].u16PortId);
    }
    else
    {
      LOG_CRIT ("Port (%d) is in up state with speed %u.", 
                vRouterg.portConf[u8Count].u16PortId,
                link.link_speed);
    }
  }

  /* Order of the registration matters, as the modules will be called
   * as per the order of registration
   */
  registerPortRx();
  registerRoute();
  registerNetLink ();
  registerPortTx();

  for (u8Index = 0; u8Index < vRouterg.u8ModuleCnt; u8Index++)
  {
    vRouterg.pModule[u8Index]->pCreate ();
  }

	RTE_LCORE_FOREACH_SLAVE (u8CoreId) 
  {
		rte_eal_remote_launch(mainLoop, NULL, u8CoreId);
	}

	mainLoop (NULL);

	rte_eal_mp_wait_lcore();

}
