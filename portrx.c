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
#include <rte_lcore.h>
#include "main.h"
#include "portrx.h"
#include "logutil.h"

static int32_t doPortRx (data_t *pData);
static int32_t portRxCreate (void);

int32_t portRxCreate (void)
{
  LOG_DEBUG ("Dummy function, as everything is configured in main.c");
  return SUCCESS;
}

int32_t doPortRx (data_t *pData)
{
  uint8_t u8Lcore = rte_lcore_id();
  uint8_t u8Cnt = 0;
  uint16_t u16Ret = 0;

  pData->u16MbufCnt = 0;
  /* Read the packet from the respective queue from all the ports */
  for (u8Cnt = 0; u8Cnt < vRouterg.u8PortCnt; u8Cnt++)
  {
    u16Ret = rte_eth_rx_burst (vRouterg.portConf[u8Cnt].u16PortId,
                               vRouterg.lcoreConf[u8Lcore].u16RxQ,
                               pData->pMbuf+pData->u16MbufCnt,
                               MAX_PKT_BURST);
    if (unlikely (u16Ret == 0))
    {
      continue;
    }

    pData->u16MbufCnt += u16Ret;

    if (unlikely (pData->u16MbufCnt >= MAX_MBUFS_TO_HOLD))
    {
      LOG_CRIT ("Allocate more mbufs or corrupt the program");
      rte_panic ("Its better to crash than to corrupt\n");
    }

  }
  
  if (pData->u16MbufCnt != 0)
  {
    LOG_DEBUG ("Rx'ed packets = %d",  pData->u16MbufCnt);
  }

  return SUCCESS;

}

static modules_t portRxModule = {
  .name = "portRx",
  .pProcess = doPortRx,
  .pCreate = portRxCreate,
};

void registerPortRx (void)
{
  registerModules (&portRxModule);
  return;
}
