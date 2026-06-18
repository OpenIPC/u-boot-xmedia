#!/bin/sh

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabi-

export BOOT_SRCDIR=$(pwd)

mkdir -p output

mkflags(){
    BOOT_BUILDDIR=${BOOT_SRCDIR}/output/build-${soc}
    MAKE_OPTS="-C ${BOOT_SRCDIR} O=${BOOT_BUILDDIR} -j`nproc`"
}

# Per-SoC build settings. The original three loops differed only in:
#   - config base   (gk7205xxx_config vs xm720xxx_config)
#   - per-SoC config (concatenated for Hisi/Goke V4, none for xm720 V5)
#   - KCFLAGS       (-DVENDOR_HISILICON only on the hi35xx parts)
soc_base(){
    case "$1" in
        gk7205v500|gk7205v510|gk7205v530) echo xm720xxx ;;
        *) echo gk7205xxx ;;
    esac
}

soc_kcflags(){
    case "$1" in
        hi3516dv200|hi3516ev200|hi3516ev300|hi3518ev300)
            echo "-DPRODUCT_SOC=$1 -DVENDOR_HISILICON" ;;
        *) echo "-DPRODUCT_SOC=$1" ;;
    esac
}

# Whether a per-SoC ${soc}_config fragment is appended on top of the base.
soc_has_socconfig(){
    case "$1" in
        gk7205v500|gk7205v510|gk7205v530) return 1 ;;
        *) return 0 ;;
    esac
}

build_soc(){
    soc=$1
    mkflags
    base=$(soc_base ${soc})
    kcflags=$(soc_kcflags ${soc})

    mkdir -p ${BOOT_BUILDDIR}/drivers/ddr/xmedia/
    mkdir -p ${BOOT_BUILDDIR}/drivers/ddr/xmedia/xmedia/default/cmd_bin/
    cp -arf ${BOOT_SRCDIR}/drivers/ddr/xmedia/* ${BOOT_BUILDDIR}/drivers/ddr/xmedia/
    cp openipc/${soc}_reginfo.bin ${BOOT_BUILDDIR}/.reg

    # NOR
    cat ${BOOT_SRCDIR}/openipc/${base}_config > ${BOOT_BUILDDIR}/.config && \
    { ! soc_has_socconfig ${soc} || cat ${BOOT_SRCDIR}/openipc/${soc}_config >> ${BOOT_BUILDDIR}/.config; } && \
    make ${MAKE_OPTS} KCFLAGS="${kcflags}" && \
    make ${MAKE_OPTS} SRCDIR=${BOOT_SRCDIR} u-boot-z.bin && \
    cp ${BOOT_BUILDDIR}/u-boot-xm*.bin ${BOOT_BUILDDIR}/../u-boot-${soc}-nor.bin || return 1

    # NAND
    cat ${BOOT_SRCDIR}/openipc/${base}_config > ${BOOT_BUILDDIR}/.config && \
    { ! soc_has_socconfig ${soc} || cat ${BOOT_SRCDIR}/openipc/${soc}_config >> ${BOOT_BUILDDIR}/.config; } && \
    cat ${BOOT_SRCDIR}/openipc/nand_config >> ${BOOT_BUILDDIR}/.config && \
    make ${MAKE_OPTS} KCFLAGS="${kcflags}" && \
    make ${MAKE_OPTS} SRCDIR=${BOOT_SRCDIR} u-boot-z.bin && \
    cp ${BOOT_BUILDDIR}/u-boot-xm*.bin ${BOOT_BUILDDIR}/../u-boot-${soc}-nand.bin || return 1
}

# Build the SoCs named on the command line, or every supported SoC when
# invoked with no arguments (preserves the original behaviour).
SOCS="$@"
[ -z "${SOCS}" ] && SOCS="hi3516dv200 hi3516ev200 hi3516ev300 hi3518ev300 \
                          gk7205v200 gk7205v300 gk7202v300 gk7605v100 \
                          gk7205v500 gk7205v510 gk7205v530"

rc=0
for soc in ${SOCS}; do
    build_soc ${soc} || { echo "build failed for ${soc}" >&2; rc=1; }
done
exit ${rc}
