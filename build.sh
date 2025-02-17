#!/bin/sh

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabi-

export BOOT_SRCDIR=$(pwd)

mkdir -p output

mkflags(){
    BOOT_BUILDDIR=${BOOT_SRCDIR}/output/build-${soc}
    MAKE_OPTS="-C ${BOOT_SRCDIR} O=${BOOT_BUILDDIR} -j`nproc`"
}

for soc in hi3516dv200 hi3516ev200 hi3516ev300 hi3518ev300; do
    mkflags

    mkdir -p ${BOOT_BUILDDIR}/drivers/ddr/xmedia/
    mkdir -p ${BOOT_BUILDDIR}/drivers/ddr/xmedia/xmedia/default/cmd_bin/
    cp -arf ${BOOT_SRCDIR}/drivers/ddr/xmedia/* ${BOOT_BUILDDIR}/drivers/ddr/xmedia/
    cp openipc/${soc}_reginfo.bin ${BOOT_BUILDDIR}/.reg
    # NOR
    cat ${BOOT_SRCDIR}/openipc/gk7205xxx_config > ${BOOT_BUILDDIR}/.config && \
    cat ${BOOT_SRCDIR}/openipc/${soc}_config >> ${BOOT_BUILDDIR}/.config && \
    make ${MAKE_OPTS} KCFLAGS="-DPRODUCT_SOC=${soc} -DVENDOR_HISILICON" && \
    make ${MAKE_OPTS} SRCDIR=${BOOT_SRCDIR} u-boot-z.bin && \
    cp ${BOOT_BUILDDIR}/u-boot-xm*.bin ${BOOT_BUILDDIR}/../u-boot-${soc}-nor.bin
    # NAND
    cat ${BOOT_SRCDIR}/openipc/gk7205xxx_config > ${BOOT_BUILDDIR}/.config && \
    cat ${BOOT_SRCDIR}/openipc/${soc}_config >> ${BOOT_BUILDDIR}/.config && \
    cat ${BOOT_SRCDIR}/openipc/nand_config >> ${BOOT_BUILDDIR}/.config && \
    make ${MAKE_OPTS} KCFLAGS="-DPRODUCT_SOC=${soc} -DVENDOR_HISILICON" && \
    make ${MAKE_OPTS} SRCDIR=${BOOT_SRCDIR} u-boot-z.bin && \
    cp ${BOOT_BUILDDIR}/u-boot-xm*.bin ${BOOT_BUILDDIR}/../u-boot-${soc}-nand.bin
done

for soc in gk7205v200 gk7205v300 gk7202v300 gk7605v100; do
    mkflags

    mkdir -p ${BOOT_BUILDDIR}/drivers/ddr/xmedia/
    mkdir -p ${BOOT_BUILDDIR}/drivers/ddr/xmedia/xmedia/default/cmd_bin/
    cp -arf ${BOOT_SRCDIR}/drivers/ddr/xmedia/* ${BOOT_BUILDDIR}/drivers/ddr/xmedia/
    cp openipc/${soc}_reginfo.bin ${BOOT_BUILDDIR}/.reg
    # NOR
    cat ${BOOT_SRCDIR}/openipc/gk7205xxx_config > ${BOOT_BUILDDIR}/.config && \
    cat ${BOOT_SRCDIR}/openipc/${soc}_config >> ${BOOT_BUILDDIR}/.config && \
    make ${MAKE_OPTS} KCFLAGS="-DPRODUCT_SOC=${soc}" && \
    make ${MAKE_OPTS} SRCDIR=${BOOT_SRCDIR} u-boot-z.bin && \
    cp ${BOOT_BUILDDIR}/u-boot-xm*.bin ${BOOT_BUILDDIR}/../u-boot-${soc}-nor.bin
    # NAND
    cat ${BOOT_SRCDIR}/openipc/gk7205xxx_config > ${BOOT_BUILDDIR}/.config && \
    cat ${BOOT_SRCDIR}/openipc/${soc}_config >> ${BOOT_BUILDDIR}/.config && \
    cat ${BOOT_SRCDIR}/openipc/nand_config >> ${BOOT_BUILDDIR}/.config && \
    make ${MAKE_OPTS} KCFLAGS="-DPRODUCT_SOC=${soc}" && \
    make ${MAKE_OPTS} SRCDIR=${BOOT_SRCDIR} u-boot-z.bin && \
    cp ${BOOT_BUILDDIR}/u-boot-xm*.bin ${BOOT_BUILDDIR}/../u-boot-${soc}-nand.bin
done

for soc in gk7205v500 gk7205v510 gk7205v530; do
    mkflags

    mkdir -p ${BOOT_BUILDDIR}/drivers/ddr/xmedia/
    mkdir -p ${BOOT_BUILDDIR}/drivers/ddr/xmedia/xmedia/default/cmd_bin/
    cp -arf ${BOOT_SRCDIR}/drivers/ddr/xmedia/* ${BOOT_BUILDDIR}/drivers/ddr/xmedia/
    cp openipc/${soc}_reginfo.bin ${BOOT_BUILDDIR}/.reg
    # NOR
    cat ${BOOT_SRCDIR}/openipc/xm720xxx_config > ${BOOT_BUILDDIR}/.config && \
    make ${MAKE_OPTS} KCFLAGS="-DPRODUCT_SOC=${soc}" && \
    make ${MAKE_OPTS} SRCDIR=${BOOT_SRCDIR} u-boot-z.bin && \
    cp ${BOOT_BUILDDIR}/u-boot-xm*.bin ${BOOT_BUILDDIR}/../u-boot-${soc}-nor.bin
    # NAND
    cat ${BOOT_SRCDIR}/openipc/xm720xxx_config > ${BOOT_BUILDDIR}/.config && \
    cat ${BOOT_SRCDIR}/openipc/nand_config >> ${BOOT_BUILDDIR}/.config && \
    make ${MAKE_OPTS} KCFLAGS="-DPRODUCT_SOC=${soc}" && \
    make ${MAKE_OPTS} SRCDIR=${BOOT_SRCDIR} u-boot-z.bin && \
    cp ${BOOT_BUILDDIR}/u-boot-xm*.bin ${BOOT_BUILDDIR}/../u-boot-${soc}-nand.bin
done
