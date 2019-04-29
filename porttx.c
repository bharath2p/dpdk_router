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
#include <rte_port.h>
#include "main.h"
#include "porttx.h"
#include "logutil.h"

static int32_t doPortTx (data_t *pData);
static int32_t portTxCreate (void);

int32_t portTxCreate (void)
{
  LOG_DEBUG ("Dummy function, as everything is configured in main.c");
  return SUCCESS;
}

int32_t doPortTx (data_t *pData)
{
  uint16_t u16Cnt = 0;
  uint16_t u16Ret = 0;
  uint8_t u8Lcore = rte_lcore_id();

  metaData_t *pMetaData = NULL;

  for (u16Cnt = 0; u16Cnt < pData->u16MbufCnt; u16Cnt++)
  {
    pMetaData = (metaData_t *) RTE_MBUF_METADATA_UINT8_PTR (pData->pMbuf[u16Cnt], 
                                                            sizeof(struct rte_mbuf));
    /* If it is working fine, change it to burst mode.
     * In create call, add the buffer structure to store the mbuf
     */
    LOG_DEBUG ("Sending pkt out via port %d", pMetaData->u8OutPort);
    u16Ret = rte_eth_tx_burst (pMetaData->u8OutPort, 
                               vRouterg.lcoreConf[u8Lcore].u16TxQ,
                               &pData->pMbuf[u16Cnt], 1);

    if (unlikely (u16Ret == 0))
    {
      LOG_ERR ("failed to send pkt out via port %d", pMetaData->u8OutPort);
      rte_pktmbuf_free (pData->pMbuf[u16Cnt]);
      continue;
    }

  }
                              
  return SUCCESS;
}

static modules_t portTxModule = {
  .name = "portTx",
  .pProcess = doPortTx,
  .pCreate = portTxCreate,
};

void registerPortTx (void)
{
  registerModules (&portTxModule);
  return;
}
