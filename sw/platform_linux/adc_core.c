/***************************************************************************//**
 *   @file   adc_core.c
 *   @brief  Implementation of ADC Core Driver.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2013(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include "adc_core.h"
#include "parameters.h"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>

/******************************************************************************/
/************************ Variables Definitions *******************************/
/******************************************************************************/
int ad9361_uio_fd;
void *ad9361_uio_addr;
int rxdma_uio_fd;
void *rxdma_uio_addr;

/***************************************************************************//**
 * @brief adc_read
*******************************************************************************/
void adc_read(uint32_t regAddr, uint32_t *data)
{
	*data = (*((unsigned *) (ad9361_uio_addr + regAddr)));
}

/***************************************************************************//**
 * @brief adc_write
*******************************************************************************/
void adc_write(uint32_t regAddr, uint32_t data)
{
	*((unsigned *) (ad9361_uio_addr + regAddr)) = data;
}

/***************************************************************************//**
 * @brief adc_dma_read
*******************************************************************************/
void adc_dma_read(uint32_t regAddr, uint32_t *data)
{
	*data = (*((unsigned *) (rxdma_uio_addr + regAddr)));
}

/***************************************************************************//**
 * @brief adc_dma_write
*******************************************************************************/
void adc_dma_write(uint32_t regAddr, uint32_t data)
{
	*((unsigned *) (rxdma_uio_addr + regAddr)) = data;
}

/***************************************************************************//**
 * @brief adc_init
*******************************************************************************/
void adc_init(void)
{
	ad9361_uio_fd = open(AD9361_UIO_DEV, O_RDWR);
	if(ad9361_uio_fd < 1)
	{
		printf("%s: Can't open ad9361_uio device\n\r", __func__);
		return;
	}
	
	ad9361_uio_addr = mmap(NULL,
			       24576,
			       PROT_READ|PROT_WRITE,
			       MAP_SHARED,
			       ad9361_uio_fd,
			       0);
	
	rxdma_uio_fd = open(RXDMA_UIO_DEV, O_RDWR);
	if(rxdma_uio_fd < 1)
	{
		printf("%s: Can't open rxdma_uio device\n\r", __func__);
		return;
	}
	
	rxdma_uio_addr = mmap(NULL,
			      4096,
			      PROT_READ|PROT_WRITE,
			      MAP_SHARED,
			      rxdma_uio_fd,
			      0);

	adc_write(ADI_REG_RSTN, 0);
	adc_write(ADI_REG_RSTN, ADI_RSTN);

	adc_write(ADI_REG_CHAN_CNTRL(0),
		ADI_IQCOR_ENB | ADI_FORMAT_SIGNEXT | ADI_FORMAT_ENABLE | ADI_ENABLE);
	adc_write(ADI_REG_CHAN_CNTRL(1),
		ADI_IQCOR_ENB | ADI_FORMAT_SIGNEXT | ADI_FORMAT_ENABLE | ADI_ENABLE);
	adc_write(ADI_REG_CHAN_CNTRL(2),
		ADI_IQCOR_ENB | ADI_FORMAT_SIGNEXT | ADI_FORMAT_ENABLE | ADI_ENABLE);
	adc_write(ADI_REG_CHAN_CNTRL(3),
		ADI_IQCOR_ENB | ADI_FORMAT_SIGNEXT | ADI_FORMAT_ENABLE | ADI_ENABLE);
}

/***************************************************************************//**
 * @brief adc_capture
*******************************************************************************/
int32_t adc_capture(uint32_t size, uint32_t start_address)
{
	uint32_t reg_val;
	uint32_t transfer_id;

	adc_dma_write(AXI_DMAC_REG_CTRL, 0x0);
	adc_dma_write(AXI_DMAC_REG_CTRL, AXI_DMAC_CTRL_ENABLE);

	adc_dma_write(AXI_DMAC_REG_IRQ_MASK, 0x0);

	adc_dma_read(AXI_DMAC_REG_TRANSFER_ID, &transfer_id);
	adc_dma_read(AXI_DMAC_REG_IRQ_PENDING, &reg_val);
	adc_dma_write(AXI_DMAC_REG_IRQ_PENDING, reg_val);

	adc_dma_write(AXI_DMAC_REG_DEST_ADDRESS, start_address);
	adc_dma_write(AXI_DMAC_REG_DEST_STRIDE, 0x0);
	adc_dma_write(AXI_DMAC_REG_X_LENGTH, (size * 8) - 1);
	adc_dma_write(AXI_DMAC_REG_Y_LENGTH, 0x0);

	adc_dma_write(AXI_DMAC_REG_START_TRANSFER, 0x1);
	/* Wait until the new transfer is queued. */
	do {
		adc_dma_read(AXI_DMAC_REG_START_TRANSFER, &reg_val);
	}
	while(reg_val == 1);

	/* Wait until the current transfer is completed. */
	do {
		adc_dma_read(AXI_DMAC_REG_IRQ_PENDING, &reg_val);
	}
	while(reg_val != (AXI_DMAC_IRQ_SOT | AXI_DMAC_IRQ_EOT));
	adc_dma_write(AXI_DMAC_REG_IRQ_PENDING, reg_val);

	/* Wait until the transfer with the ID transfer_id is completed. */
	do {
		adc_dma_read(AXI_DMAC_REG_TRANSFER_DONE, &reg_val);
	}
	while((reg_val & (1 << transfer_id)) != (1 << transfer_id));

	return 0;
}

/***************************************************************************//**
 * @brief adc_save_csv_file
*******************************************************************************/
int32_t adc_capture_save_csv_file(uint32_t size, uint32_t start_address,
			  const char * filename)
{
	int dev_mem_fd;
	uint32_t mapping_length, page_mask, page_size;
	void *mapping_addr, *rx_buff_virt_addr;
	uint32_t index;
	uint32_t data, data_i1, data_q1, data_i2, data_q2;

	adc_capture(size, start_address);

	dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(dev_mem_fd == -1)
	{
		printf("%s: Can't open /dev/mem device\n\r", __func__);
		return -1;
	}

	page_size = sysconf(_SC_PAGESIZE);
	mapping_length = (((RX_BUFF_MEM_SIZE / page_size) + 1) * page_size);
	page_mask = (page_size - 1);
	mapping_addr = mmap(NULL,
			   mapping_length,
			   PROT_READ | PROT_WRITE,
			   MAP_SHARED,
			   dev_mem_fd,
			   (RX_BUFF_MEM_ADDR & ~page_mask));
	if(mapping_addr == MAP_FAILED)
	{
		printf("%s: mmap error\n\r", __func__);
		return -1;
	}

	rx_buff_virt_addr = (mapping_addr + (RX_BUFF_MEM_ADDR & page_mask));

	FILE *f = fopen(filename, "w");

	for(index = 0; index < size * 2; index += 2)
	{
		data = *((unsigned *) (rx_buff_virt_addr + (index * 4)));
		data_q1 = (data & 0xFFFF);
		data_i1 = (data >> 16) & 0xFFFF;
		data = *((unsigned *) (rx_buff_virt_addr + ((index + 1) * 4)));
		data_q2 = (data & 0xFFFF);
		data_i2 = (data >> 16) & 0xFFFF;
		fprintf(f, "%d,%d,%d,%d\n", data_q1, data_i1, data_q2, data_i2);
	}

	fclose(f);

	return 0;
}

