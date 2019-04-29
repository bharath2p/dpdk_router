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



#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_byteorder.h>
#include <rte_lpm.h>
#include <rte_port.h>
#include "main.h"
#include "route.h"
#include "logutil.h"

static int32_t doRouting (data_t *pData);
static int32_t routeCreate (void);

/* Code taken from examples/l3fwd/l3fwd.h */
static inline int
is_valid_ipv4_pkt (struct ipv4_hdr *pkt, uint32_t link_len)
{
	/* From http://www.rfc-editor.org/rfc/rfc1812.txt section 5.2.2 */
	/*
	 * 1. The packet length reported by the Link Layer must be large
	 * enough to hold the minimum length legal IP datagram (20 bytes).
	 */
	if (link_len < sizeof(struct ipv4_hdr))
		return -1;

	/* 2. The IP checksum must be correct. */
	/* this is checked in H/W */

	/*
	 * 3. The IP version number must be 4. If the version number is not 4
	 * then the packet may be another version of IP, such as IPng or
	 * ST-II.
	 */
	if (((pkt->version_ihl) >> 4) != 4)
		return -3;
	/*
	 * 4. The IP header length field must be large enough to hold the
	 * minimum length legal IP datagram (20 bytes = 5 words).
	 */
	if ((pkt->version_ihl & 0xf) < 5)
		return -4;

	/*
	 * 5. The IP total length field must be large enough to hold the IP
	 * datagram header, whose length is specified in the IP header length
	 * field.
	 */
	if (rte_cpu_to_be_16(pkt->total_length) < sizeof(struct ipv4_hdr))
		return -5;

  /*
   * 6. If TTL is <=1, make it as error.
   * In the current implementation, we are merely acting as a router and
   * we won't accept the packet, even if it is destined to us.
   * So, TTL 1 is also invalid.
   */
  if (pkt->time_to_live <= 1)
    return -6;

	return 0;
}

int32_t routeCreate (void)
{
  struct rte_lpm_config lpmConfig;

  memset (&lpmConfig, 0, sizeof (struct rte_lpm_config));
  lpmConfig.max_rules = MAX_IPV4_ROUTES;
  lpmConfig.number_tbl8s = 1 << 8;
  vRouterg.pLpmv4 = rte_lpm_create ("ipv4Route", 0, &lpmConfig);

  if (vRouterg.pLpmv4 == NULL)
  {
    rte_panic ("Unable to create v4 Route table\n");
  }

  return SUCCESS;
}

int32_t doRouting (data_t *pData)
{
  struct ether_hdr *pEther = NULL;
  struct ipv4_hdr *pIpv4Hdr = NULL;
  metaData_t *pMetaData = NULL;
  uint32_t u32Ipv4Addr[MAX_MBUFS_TO_HOLD];
  uint32_t u32NHopIp[MAX_MBUFS_TO_HOLD];
  uint16_t u16Ipv4Cnt = 0;
  uint16_t u16Cnt = 0;
  uint16_t u16Idx = 0;
  struct rte_mbuf *pMbuf[MAX_MBUFS_TO_HOLD];


  for (u16Cnt = 0; u16Cnt < pData->u16MbufCnt; u16Cnt++)
  {
    pEther = (struct ether_hdr *)(rte_pktmbuf_mtod(pData->pMbuf[u16Cnt], 
                                  uint8_t *));

    if (rte_be_to_cpu_16 (pEther->ether_type) != ETHER_TYPE_IPv4)
    {
      LOG_WARN ("Invalid Ethertype");
      /* Don't do anything if the ethertype is not ipv4 */
      rte_pktmbuf_free (pData->pMbuf[u16Cnt]);
      pData->pMbuf[u16Cnt] = NULL;
      pData->u16MbufCnt--;
      continue;
    }

    if (is_multicast_ether_addr (&pEther->d_addr))
    {
      LOG_WARN ("currently multicast and broadcast is not supported");
      rte_pktmbuf_free (pData->pMbuf[u16Cnt]);
      pData->pMbuf[u16Cnt] = NULL;
      pData->u16MbufCnt--;
      continue;
    }

    pIpv4Hdr = rte_pktmbuf_mtod_offset (pData->pMbuf[u16Cnt], 
                                        struct ipv4_hdr *,
                                        sizeof (struct ether_hdr));

    if (is_valid_ipv4_pkt (pIpv4Hdr, pData->pMbuf[u16Cnt]->pkt_len) != SUCCESS)
    {
      LOG_WARN ("Invalid ipv4 pkt");
      rte_pktmbuf_free (pData->pMbuf[u16Cnt]);
      pData->pMbuf[u16Cnt] = NULL;
      pData->u16MbufCnt--;
      continue;
    }
    
    pIpv4Hdr->time_to_live--;
    pIpv4Hdr->hdr_checksum = 0;
    LOG_DEBUG ("Reduced TTL to %d", pIpv4Hdr->time_to_live);
#if 0
    /* Set hardware checksum related parameters here, in case of SR-IOV */
    pData->pMbuf[u16Cnt]->ol_flags = PKT_TX_IP_CKSUM;
    pData->pMbuf[u16Cnt]->l2_len = sizeof (struct ether_hdr);
    pData->pMbuf[u16Cnt]->l3_len = sizeof (struct ipv4_hdr);
#endif
    u32Ipv4Addr[u16Ipv4Cnt] = rte_cpu_to_be_32 (pIpv4Hdr->dst_addr);
    pMbuf[u16Ipv4Cnt] = pData->pMbuf[u16Cnt];
    u16Ipv4Cnt++;
  }

  if (u16Ipv4Cnt == 0)
  {
    return SUCCESS;
  }

  rte_lpm_lookup_bulk (vRouterg.pLpmv4,
                       u32Ipv4Addr,
                       u32NHopIp,
                       u16Ipv4Cnt);

  for (u16Cnt = 0; u16Cnt < u16Ipv4Cnt; u16Cnt++)
  {
    if ((u32NHopIp[u16Cnt] & RTE_LPM_LOOKUP_SUCCESS) == 0)
    {
      LOG_DEBUG ("route lookup failed");
      rte_pktmbuf_free (pMbuf[u16Cnt]);
      pMbuf[u16Cnt] = NULL;
      continue;
    }

    pMetaData = (metaData_t *) RTE_MBUF_METADATA_UINT8_PTR (pMbuf[u16Cnt], 
                                                            sizeof(struct rte_mbuf));
    pMetaData->u8OutPort = u32NHopIp[u16Cnt] & 0xff;
    pData->pMbuf[u16Idx] = pMbuf[u16Cnt];
    /* calculate checksum here */
    pIpv4Hdr = rte_pktmbuf_mtod_offset (pData->pMbuf[u16Idx], 
                                        struct ipv4_hdr *,
                                        sizeof (struct ether_hdr));
    pIpv4Hdr->hdr_checksum = rte_ipv4_cksum (pIpv4Hdr);
    LOG_DEBUG ("Recalculating checksum");
    u16Idx++;
  }

  pData->u16MbufCnt = u16Idx;

  return SUCCESS;
}

static modules_t RouteModule = {
  .name = "routing",
  .pProcess = doRouting,
  .pCreate = routeCreate,
};

void registerRoute (void)
{
  registerModules (&RouteModule);
  return;
}
