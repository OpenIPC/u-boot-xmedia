/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#define SPI_NOR_NM_QE_SHIFT	 2
#define SPI_NOR_NM_QE_MASK	  (1 << SPI_NOR_NM_QE_SHIFT)
#define spi_nor_get_qe_by_nm(cr)	(((cr) & SPI_NOR_NM_QE_MASK) \
						>> SPI_NOR_NM_QE_SHIFT)

static int spi_nm25q128_entry_4addr(struct fmc_spi *spi, int enable)
{

	return 0;
}

static void spi_nm25q128_set_op(const struct fmc_spi *spi)
{
	unsigned int regval;
	struct fmc_host *host = (struct fmc_host *)spi->host;

	regval = fmc_cmd_cmd1(SPI_CMD_WRSR2);
	fmc_write(host, FMC_CMD, regval);
	fmc_pr(QE_DBG, "\t  Set CMD[%#x]%#x\n", FMC_CMD, regval);

	regval = op_cfg_fm_cs(spi->chipselect) | OP_CFG_OEN_EN;
	fmc_write(host, FMC_OP_CFG, regval);
	fmc_pr(QE_DBG, "\t  Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, regval);

	regval = fmc_data_num_cnt(SPI_NOR_SR_LEN);
	fmc_write(host, FMC_DATA_NUM, regval);
	fmc_pr(QE_DBG, "\t  Set DATA_NUM[%#x]%#x\n", FMC_DATA_NUM, regval);

	regval = fmc_op_cmd1_en(ENABLE) |
		 fmc_op_write_data_en(ENABLE) |
		 FMC_OP_REG_OP_START;
	fmc_write(host, FMC_OP, regval);
	fmc_pr(QE_DBG, "\t  Set OP[%#x]%#x\n", FMC_OP, regval);

	fmc_cmd_wait_cpu_finish(host);
}

/*
   enable QE bit if QUAD read write is supported by NM25Q128EVBSIG
*/
static int spi_nm25q128_qe_enable(struct fmc_spi *spi)
{
	unsigned char status, op;
	const char *str[] = {"Disable", "Enable"};
	struct fmc_host *host = NULL;
	if (!spi || !spi->driver)
		return -1;
	host = (struct fmc_host *)spi->host;
	if (!host || !host->iobase)
		return -1;
	op = spi_is_quad(spi);

	fmc_pr(QE_DBG, "\t* Start SPI Nor NM25Q(128/64)EVBSIG %s Quad.\n", str[op]);

	status = spi_general_get_flash_register(spi, SPI_CMD_RDSR2);
	fmc_pr(QE_DBG, "\t  Read Status Register-2[%#x]%#x\n", SPI_CMD_RDSR2,
	       status);
	if (op == spi_nor_get_qe_by_nm(status)) {
		fmc_pr(QE_DBG, "\t* Quad was %s status:%#x\n", str[op], status);
		goto QE_END;
	}

	spi->driver->write_enable(spi);

	if (op)
		status |= SPI_NOR_NM_QE_MASK;
	else
		status &= ~SPI_NOR_NM_QE_MASK;

	writeb(status, host->iobase);
	fmc_pr(QE_DBG, "\t  Write IO[%p]%#x\n", host->iobase,
	       *(unsigned char *)host->iobase);

	/* There is new cmd for Write Status Register 2 by NM25Q(128/64)EVBSIG */
	spi_nm25q128_set_op(spi);

	/* wait the flash have switched quad mode success */
	spi->driver->wait_ready(spi);

	status = spi_general_get_flash_register(spi, SPI_CMD_RDSR2);
	fmc_pr(QE_DBG, "\t  Read Status Register-2[%#x]:%#x\n",
	       SPI_CMD_RDSR2, status);
	if (op == spi_nor_get_qe_by_nm(status)) {
		fmc_pr(QE_DBG, "\t  %s Quad success. status:%#x\n",
		       str[op], status);
	} else {
		db_msg("Error: %s Quad failed! reg:%#x\n", str[op],
		       status);
	}
QE_END:
	/* Enable the reset pin when working on dual mode for 8PIN */
	if (!op)
		spi_nor_reset_pin_enable(spi, ENABLE);

	fmc_pr(QE_DBG, "\t* End SPI Nor NM25Q(128/64)EVBSIG %s Quad.\n", str[op]);

	return op;
}
