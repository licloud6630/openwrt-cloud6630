#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/utsname.h>

#define PAGE_ROUND_UP(x)	PAGE_ROUND_DOWN((x) + getpagesize() - 1)
#define PAGE_ROUND_DOWN(x)	((x) & ~(getpagesize() - 1))
#define DEV_MEM_DEFAULT		"/dev/mem"
#define PHY_MMAP_LENGTH  0x80000

static struct {
	int fd;
	off_t offset;
	size_t len;
	void *vaddr;
} dev_mem = { -1, 0, 0, NULL };


struct phy_dev {
	int index;
	unsigned int cpu_type;
	bool is_little_endian;
	void *phy_regs;
	void *priv;
};


const char *dev_mem_file;

static void unmap_dev_mem(void)
{
	if (dev_mem.vaddr != NULL)
		munmap(dev_mem.vaddr, dev_mem.len);
	if (dev_mem.fd != -1)
		close(dev_mem.fd);

	dev_mem.fd = -1;
	dev_mem.offset = 0;
	dev_mem.len = 0;
	dev_mem.vaddr = NULL;
}

static void *map_dev_mem(uint64_t addr, uint32_t size)
{
	int fd;
	void *vaddr;
	off_t offset;
	size_t len;

	offset = PAGE_ROUND_DOWN(addr);
	len = PAGE_ROUND_UP(size);

	if ((offset == dev_mem.offset) && (len == dev_mem.len))
		return dev_mem.vaddr;

	unmap_dev_mem();
	
	if ((fd = open(DEV_MEM_DEFAULT, O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "open '%s' failed\n", DEV_MEM_DEFAULT);
		goto err;
	}

	vaddr = mmap64(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);

	if ((vaddr == NULL) || (vaddr == MAP_FAILED)) {
		fprintf(stderr, "mmap '%s' failed\n", dev_mem_file);
		goto err;
	}
	
	dev_mem.offset = offset;
	dev_mem.len = len;
	dev_mem.vaddr = vaddr;

	return vaddr;
err :
	unmap_dev_mem();
	return NULL;
}

/*****************************MDIO******************************************/
#define MDIO_DEVAD_NONE			(-1)

/****************************ppc64****************************/
#define CONFIG_SYS_FSL_FM0_ADDR  0xFFE400000

#define CONFIG_SYS_FSL_FM0_EMAC1   (CONFIG_SYS_FSL_FM0_ADDR + 0xE1000)
#define CONFIG_SYS_FSL_FM0_EMAC2   (CONFIG_SYS_FSL_FM0_ADDR + 0xE3000)
#define CONFIG_SYS_FSL_FM0_EMAC3   (CONFIG_SYS_FSL_FM0_ADDR + 0xE5000)
#define CONFIG_SYS_FSL_FM0_EMAC4   (CONFIG_SYS_FSL_FM0_ADDR + 0xE7000)
#define CONFIG_SYS_FSL_FM0_EMAC5   (CONFIG_SYS_FSL_FM0_ADDR + 0xE9000)
#define CONFIG_SYS_FSL_FM0_EMAC6   (CONFIG_SYS_FSL_FM0_ADDR + 0xEB000)
#define CONFIG_SYS_FSL_FM0_EMAC7   (CONFIG_SYS_FSL_FM0_ADDR + 0xED000)
#define CONFIG_SYS_FSL_FM0_EMAC8   (CONFIG_SYS_FSL_FM0_ADDR + 0xEF000)
#define CONFIG_SYS_FSL_FM0_EMAC9   (CONFIG_SYS_FSL_FM0_ADDR + 0xF1000)
#define CONFIG_SYS_FSL_FM0_EMAC10  (CONFIG_SYS_FSL_FM0_ADDR + 0xE3000)
/**********Dedicated MDIO**************************/
#define CONFIG_SYS_FSL_FM0_EMAC11   (CONFIG_SYS_FSL_FM0_ADDR + 0xFC000)
#define CONFIG_SYS_FSL_FM0_EMAC12   (CONFIG_SYS_FSL_FM0_ADDR + 0xFD000)

/****************************aarch64****************************/
#define CONFIG_SYS_FSL_FM1_ADDR  0x1A00000

#define CONFIG_SYS_FSL_FM1_EMAC1   (CONFIG_SYS_FSL_FM1_ADDR + 0xE1000)
#define CONFIG_SYS_FSL_FM1_EMAC2   (CONFIG_SYS_FSL_FM1_ADDR + 0xE3000)
#define CONFIG_SYS_FSL_FM1_EMAC3   (CONFIG_SYS_FSL_FM1_ADDR + 0xE5000)
#define CONFIG_SYS_FSL_FM1_EMAC4   (CONFIG_SYS_FSL_FM1_ADDR + 0xE7000)
#define CONFIG_SYS_FSL_FM1_EMAC5   (CONFIG_SYS_FSL_FM1_ADDR + 0xE9000)
#define CONFIG_SYS_FSL_FM1_EMAC6   (CONFIG_SYS_FSL_FM1_ADDR + 0xEB000)
#define CONFIG_SYS_FSL_FM1_EMAC9   (CONFIG_SYS_FSL_FM1_ADDR + 0xF1000)
/*only ls1046*/
#define CONFIG_SYS_FSL_FM1_EMAC10  (CONFIG_SYS_FSL_FM1_ADDR + 0xE3000)

/**********Dedicated MDIO**************************/
#define CONFIG_SYS_FSL_FM1_EMAC11   (CONFIG_SYS_FSL_FM1_ADDR + 0xFC000)
#define CONFIG_SYS_FSL_FM1_EMAC12   (CONFIG_SYS_FSL_FM1_ADDR + 0xFD000)


#define MDIO_STAT_CLKDIV(x)	(((x>>1) & 0xff) << 8)
#define MDIO_STAT_BSY		(1 << 0)
#define MDIO_STAT_RD_ER		(1 << 1)
#define MDIO_STAT_PRE		(1 << 5)
#define MDIO_STAT_ENC		(1 << 6)
#define MDIO_STAT_HOLD_15_CLK	(7 << 2)
#define MDIO_STAT_NEG		(1 << 23)

#define MDIO_CTL_DEV_ADDR(x)	(x & 0x1f)
#define MDIO_CTL_PORT_ADDR(x)	((x & 0x1f) << 5)
#define MDIO_CTL_PRE_DIS	(1 << 10)
#define MDIO_CTL_SCAN_EN	(1 << 11)
#define MDIO_CTL_POST_INC	(1 << 14)
#define MDIO_CTL_READ		(1 << 15)

#define MDIO_DATA(x)		(x & 0xffff)
#define MDIO_DATA_BSY		(1 << 31)



#define __force	
#define __iomem	
#define ___mdio_swab32(x) ((unsigned int)(				\
	(((unsigned int)(x) & (unsigned int)0x000000ffUL) << 24) |		\
	(((unsigned int)(x) & (unsigned int)0x0000ff00UL) <<  8) |		\
	(((unsigned int)(x) & (unsigned int)0x00ff0000UL) >>  8) |		\
	(((unsigned int)(x) & (unsigned int)0xff000000UL) >> 24)))
	
#define __le32_to_cpu(x) ___mdio_swab32((__force unsigned int)(unsigned int)(x))


#define FM_DTSEC_INFO_INITIALIZER(idx,endian,n) \
{									\
	.index		= idx,						\
	.is_little_endian = endian ,               \
	.phy_regs		=  (void *)CONFIG_SYS_FSL_FM##n##_EMAC##idx,				\
}


#define  FM0_MAX_INFO_INITIALIZER   11
#define  FM1_MAX_INFO_INITIALIZER   9

static struct phy_dev phy_info_ppc64[] = {
	FM_DTSEC_INFO_INITIALIZER(1,0,0),
	FM_DTSEC_INFO_INITIALIZER(2,0,0),
	FM_DTSEC_INFO_INITIALIZER(3,0,0),
	FM_DTSEC_INFO_INITIALIZER(4,0,0),
	FM_DTSEC_INFO_INITIALIZER(5,0,0),
	FM_DTSEC_INFO_INITIALIZER(6,0,0),
	FM_DTSEC_INFO_INITIALIZER(7,0,0),
	FM_DTSEC_INFO_INITIALIZER(8,0,0),
	FM_DTSEC_INFO_INITIALIZER(9,0,0),
	FM_DTSEC_INFO_INITIALIZER(10,0,0),
	FM_DTSEC_INFO_INITIALIZER(11,0,0),
	FM_DTSEC_INFO_INITIALIZER(12,0,0),
};

static struct phy_dev phy_info_aarch64[] = {
	FM_DTSEC_INFO_INITIALIZER(1,1,1),
	FM_DTSEC_INFO_INITIALIZER(2,1,1),
	FM_DTSEC_INFO_INITIALIZER(3,1,1),
	FM_DTSEC_INFO_INITIALIZER(4,1,1),
	FM_DTSEC_INFO_INITIALIZER(5,1,1),
	FM_DTSEC_INFO_INITIALIZER(6,1,1),
	FM_DTSEC_INFO_INITIALIZER(9,1,1),
	FM_DTSEC_INFO_INITIALIZER(10,1,1),
	FM_DTSEC_INFO_INITIALIZER(11,1,1),
	FM_DTSEC_INFO_INITIALIZER(12,1,1),
};


struct memac_mdio_controller {
	unsigned int res0[0xc];
	unsigned int	mdio_stat;	/* MDIO configuration and status */
	unsigned int	mdio_ctl;	/* MDIO control */
	unsigned int	mdio_data;	/* MDIO data */
	unsigned int	mdio_addr;	/* MDIO address */
};


static inline unsigned int ioread32(void __iomem *addr)
{
	return __le32_to_cpu(*((volatile unsigned int *)addr));
}

static inline unsigned int ioread32be(void __iomem *addr)
{
	return *((volatile unsigned int *)addr);
}

static inline void iowrite32(unsigned int value, void __iomem *addr)
{
	*((volatile unsigned int *)addr) = __le32_to_cpu(value);
}

static inline void iowrite32be(unsigned int value, void __iomem *addr)
{
	*((volatile unsigned int *)addr) = value;
}


static unsigned int memac_in_32(struct phy_dev *phy_dev, unsigned int *reg)
{
	if (phy_dev->is_little_endian)
		return ioread32(reg);
	else
		return ioread32be(reg);
}

static void memac_out_32(struct phy_dev *phy_dev, unsigned int *reg,unsigned int value)	
{
	if (phy_dev->is_little_endian)
		iowrite32(value, reg);
	else
		iowrite32be(value, reg);
}

static void memac_setbits_32(struct phy_dev *phy_dev, unsigned int *reg,unsigned int bit)	
{
	if (phy_dev->is_little_endian)
		iowrite32((ioread32(reg) | bit ), reg);
	else
		iowrite32be((ioread32be(reg) | bit ), reg);
}

static void memac_clrbits_32(struct phy_dev *phy_dev, unsigned int *reg,unsigned int bit)	
{
	if (phy_dev->is_little_endian)
		iowrite32((ioread32(reg) & ~bit) , reg);
	else
		iowrite32be((ioread32be(reg) & ~bit), reg);
}


int memac_mdio_write(struct phy_dev *phy_dev, int port_addr, int dev_addr,int regnum, int value)
{
	uint32_t mdio_ctl;
	struct memac_mdio_controller *regs = phy_dev->priv;
	uint32_t c45 = 1; /* Default to 10G interface */

	if (dev_addr == MDIO_DEVAD_NONE) {
		c45 = 0; /* clause 22 */
		dev_addr = regnum & 0x1f;
		memac_clrbits_32(phy_dev, &regs->mdio_stat, MDIO_STAT_ENC);
	} else
		memac_setbits_32(phy_dev, &regs->mdio_stat, MDIO_STAT_ENC);

	/* Wait till the bus is free */
	while ((memac_in_32(phy_dev, &regs->mdio_stat)) & MDIO_STAT_BSY)
		;

	/* Set the port and dev addr */
	mdio_ctl = MDIO_CTL_PORT_ADDR(port_addr) | MDIO_CTL_DEV_ADDR(dev_addr);
	memac_out_32(phy_dev, &regs->mdio_ctl, mdio_ctl);

	/* Set the register address */
	if (c45)
		memac_out_32(phy_dev, &regs->mdio_addr, regnum & 0xffff);

	/* Wait till the bus is free */
	while ((memac_in_32(phy_dev, &regs->mdio_stat)) & MDIO_STAT_BSY)
		;

	/* Write the value to the register */
	memac_out_32(phy_dev,&regs->mdio_data, MDIO_DATA(value));

	/* Wait till the MDIO write is complete */
	while ((memac_in_32(phy_dev,&regs->mdio_data)) & MDIO_DATA_BSY)
		;

	return 0;
}

int memac_mdio_read(struct phy_dev *phy_dev, int port_addr, int dev_addr,int regnum)
{
	
	uint32_t mdio_ctl;
	struct memac_mdio_controller *regs = phy_dev->priv;
	uint32_t c45 = 1;

	if (dev_addr == MDIO_DEVAD_NONE) {
		c45 = 0; /* clause 22 */
		dev_addr = regnum & 0x1f;
		memac_clrbits_32(phy_dev,&regs->mdio_stat, MDIO_STAT_ENC);
	} else
		memac_setbits_32(phy_dev,&regs->mdio_stat, MDIO_STAT_ENC);

	/* Wait till the bus is free */
	while ((memac_in_32(phy_dev,&regs->mdio_stat)) & MDIO_STAT_BSY)
		;

	/* Set the Port and Device Addrs */
	mdio_ctl = MDIO_CTL_PORT_ADDR(port_addr) | MDIO_CTL_DEV_ADDR(dev_addr);
	memac_out_32(phy_dev, &regs->mdio_ctl, mdio_ctl);

	/* Set the register address */
	if (c45)
		memac_out_32(phy_dev,&regs->mdio_addr, regnum & 0xffff);

	/* Wait till the bus is free */
	while ((memac_in_32(phy_dev,&regs->mdio_stat)) & MDIO_STAT_BSY)
		;

	/* Initiate the read */
	mdio_ctl |= MDIO_CTL_READ;
	memac_out_32(phy_dev, &regs->mdio_ctl, mdio_ctl);

	/* Wait till the MDIO write is complete */
	while ((memac_in_32(phy_dev, &regs->mdio_data)) & MDIO_DATA_BSY)
		;

	/* Return all Fs if nothing was there */
	if (memac_in_32(phy_dev, &regs->mdio_stat) & MDIO_STAT_RD_ER)
		return 0xffff;

	return memac_in_32(phy_dev,&regs->mdio_data) & 0xffff;
}

static struct phy_dev *phy_init(unsigned int idx)
{
	struct utsname utsname;
	struct phy_dev *p;
	
	int result = uname(&utsname);
	if (result < 0) {
		printf( "uname failed\n");
		return NULL;
	}
	
	if (strcmp(utsname.machine, "aarch64") == 0){
			if(idx > FM1_MAX_INFO_INITIALIZER) {
				printf( "idx invaild\n");
				return NULL;
			}
			p = &phy_info_aarch64[idx];
	}
	else if (strcmp(utsname.machine, "ppc64") == 0)
	{	
			if(idx > FM0_MAX_INFO_INITIALIZER) {
				printf( "idx invaild\n");
				return NULL;
			}
			p = &phy_info_ppc64[idx];
	}
	else
	{
			printf("Unsupported machine type\n");
			return NULL;
	}
	
	printf("p->phy_regs = %p  p->is_little_endian = 0x%x\n",p->phy_regs,p->is_little_endian);
	p->priv = map_dev_mem((uint64_t)p->phy_regs,PHY_MMAP_LENGTH);
	if(!p->priv)
		return NULL;
	
	return p;
	
}


#define help() \
    printf("mdio:\n");                  \
    printf("read operation: mdio read idx phy_addr phy_reg\n");          \
    printf("write operation: mdio write idx phy_addr phy_reg value\n");    \
	printf("idx range 0-11\n");    \
    printf("For example:\n");            \
    printf("mdio read 0x08 0x0 0x02\n");             \
    printf("mdio write 0x08 0x0 0x02 0x8140\n\n");      \
    exit(0);

	
	
int main(int argc, char *argv[])
{
	int idx,addr,data,reg;
	struct phy_dev *phy_info;

	if(argc < 5) {
		 help();
		 return 0;
	}
	
	idx =  (int)strtoull(argv[2], NULL, 16);
	addr = (int)strtoull(argv[3], NULL, 16);
	reg = (int)strtoull(argv[4], NULL, 16);
	
	phy_info = phy_init(idx);
	if(!phy_info)
	{
		printf("phy_init error\n");
		return 0;
	}
	
	if (strcmp("read", argv[1]) == 0) {

		printf("Read idx 0x%x phy addr 0x%x:reg 0x%x vlaue:0x%x\n",idx,addr,reg,memac_mdio_read(phy_info,addr,MDIO_DEVAD_NONE,reg));
		return 0;		
	}
	else if(strcmp("write", argv[1]) == 0){
			if(argc < 6) {
				help();
				return 0;
			}
		data = (int)strtoull(argv[5], NULL, 16);
		memac_mdio_write(phy_info,addr,MDIO_DEVAD_NONE,reg,data);
		printf("Write idx 0x%x phy addr 0x%x: reg 0x%x vlaue:0x%x\n",idx,addr,reg,data);
		return 0;
	}
	else{
		help();	
		return 0;
	}
}
