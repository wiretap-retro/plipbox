/*
 * param.c - handle device parameters
 *
 * Written by
 *  Christian Vogelgsang <chris@vogelgsang.org>
 *
 * This file is part of plip2slip.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "param.h"
#include "uartutil.h"
#include "uart.h"
#include "net/net.h"

#include <avr/eeprom.h>
#include <util/crc16.h>
#include <avr/pgmspace.h>
#include <string.h>

// current memory RAM param
param_t param;

// eeprom param
param_t eeprom_param EEMEM;
uint16_t eeprom_crc16 EEMEM;

// default 
static const param_t PROGMEM default_param = {
  .ip_net_mask = { 255,255,255,0 },
  .ip_gw_addr = { 192,168,2,1 },
  .ip_eth_addr = { 192,168,2,133 },
  .ip_srv_addr = { 192,168,2,99 }, // a typical server in your net
  .ip_plip_addr = { 192,168,0,2 },
  .ip_amiga_addr = { 192,168,0,1 },
  .mac_addr = { 0x74,0x69,0x69,0x2D,0x30,0x31 },
  .dhcp_enabled = 0
};

// dump all params
void param_dump(void)
{
  uart_send_pstring(PSTR("nn)et mask:  "));
  net_dump_ip(param.ip_net_mask);
  uart_send_crlf();
  
  uart_send_pstring(PSTR("ng)ateway:   "));
  net_dump_ip(param.ip_gw_addr);
  uart_send_crlf();
  
  uart_send_pstring(PSTR("ns)erver     "));
  net_dump_ip(param.ip_srv_addr);
  uart_send_crlf();
  
  uart_send_pstring(PSTR("ne)th port:  "));
  net_dump_ip(param.ip_eth_addr);
  uart_send_crlf();
  
  uart_send_pstring(PSTR("np)lip port: "));
  net_dump_ip(param.ip_plip_addr);
  uart_send_crlf();
  
  uart_send_pstring(PSTR("na)miga p2p: "));
  net_dump_ip(param.ip_amiga_addr);
  uart_send_crlf();
  
  uart_send_pstring(PSTR("nm)ac addr:  "));
  net_dump_mac(param.mac_addr);
  uart_send_crlf();
  
  uart_send_pstring(PSTR("nd)hcp on:   "));
  uart_send_hex_byte_crlf(param.dhcp_enabled);
  
  for(u08 i=0;i<PARAM_NUM_ARP_IP;i++) {
    uart_send('a');
    uart_send('1'+i);
    uart_send_pstring(PSTR(") arp ip:  "));
    net_dump_ip(param.arp_ip[i]);
    uart_send_crlf();
  }
}

// build check sum for parameter block
static uint16_t calc_crc16(param_t *p)
{
  uint16_t crc16 = 0xffff;
  u08 *data = (u08 *)p;
  for(u16 i=0;i<sizeof(param_t);i++) {
    crc16 = _crc16_update(crc16,*data);
    data++;
  }
  return crc16;
}

u08 param_save(void)
{
  // check that eeprom is writable
  if(!eeprom_is_ready())
    return PARAM_EEPROM_NOT_READY;

  // write current param to eeprom
  eeprom_write_block(&param,&eeprom_param,sizeof(param_t));

  // calc current parameter crc
  uint16_t crc16 = calc_crc16(&param);
  eeprom_write_word(&eeprom_crc16,crc16);

  return PARAM_OK;
}

u08 param_load(void)
{
  // check that eeprom is readable
  if(!eeprom_is_ready())
    return PARAM_EEPROM_NOT_READY;
  
  // read param
  eeprom_read_block(&param,&eeprom_param,sizeof(param_t));
  
  // read crc16
  uint16_t crc16 = eeprom_read_word(&eeprom_crc16);
  uint16_t my_crc16 = calc_crc16(&param);
  if(crc16 != my_crc16) {
    param_reset();
    return PARAM_EEPROM_CRC_MISMATCH;
  }
  
  return PARAM_OK;
}

void param_reset(void)
{
  // restore default param
  u08 *out = (u08 *)&param;
  const u08 *in = (const u08 *)&default_param;
  for(u08 i=0;i<sizeof(param_t);i++) {
    *(out++) = pgm_read_byte_near(in++);
  }
}

void param_init(void)
{
  if(param_load()!=PARAM_OK)
    param_reset();
}
