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



#include <rte_lpm.h>

/* As there is no ARP and KNI modules, pass outpot as u32NHop,
 */
static inline int32_t lpmAddv4Route (uint32_t u32DstIp, uint8_t u8Mask, 
                                     uint32_t u32Nhop)
{
  return (rte_lpm_add (vRouterg.pLpmv4, u32DstIp, u8Mask, u32Nhop));
}

static inline int32_t lpmDelv4Route (uint32_t u32DstIp, uint8_t u8Mask)
{
  return (rte_lpm_delete (vRouterg.pLpmv4, u32DstIp, u8Mask));
}
