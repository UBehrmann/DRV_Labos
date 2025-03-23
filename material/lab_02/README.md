<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 2 : User-space Drivers <!-- omit in toc -->

# Auteur <!-- omit in toc -->

- Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Téléchargement du noyau Linux et ajout des modules pour le User-space I/O (UIO)](#téléchargement-du-noyau-linux-et-ajout-des-modules-pour-le-user-space-io-uio)
- [CONFIG\_UIO\_CIF is not set](#config_uio_cif-is-not-set)
- [CONFIG\_UIO\_DMEM\_GENIRQ is not set](#config_uio_dmem_genirq-is-not-set)
- [CONFIG\_UIO\_AEC is not set](#config_uio_aec-is-not-set)
- [CONFIG\_UIO\_SERCOS3 is not set](#config_uio_sercos3-is-not-set)
- [CONFIG\_UIO\_PCI\_GENERIC is not set](#config_uio_pci_generic-is-not-set)
- [CONFIG\_UIO\_NETX is not set](#config_uio_netx-is-not-set)
- [CONFIG\_UIO\_PRUSS is not set](#config_uio_pruss-is-not-set)
- [CONFIG\_UIO\_MF624 is not set](#config_uio_mf624-is-not-set)
- [Accès aux périphériques en utilisant /dev/mem](#accès-aux-périphériques-en-utilisant-devmem)
  - [Exercice 1](#exercice-1)
- [Accès aux périphériques en utilisant le UIO framework](#accès-aux-périphériques-en-utilisant-le-uio-framework)
  - [Exercice 2](#exercice-2)
  - [Exercice 3](#exercice-3)
- [UIO framework et interruptions](#uio-framework-et-interruptions)


# Téléchargement du noyau Linux et ajout des modules pour le User-space I/O (UIO)

Command a faire dans chaque terminal avant de compiler du code kernel 'manuellement'.

´´´bash
export TOOLCHAIN=/opt/toolchains/arm-linux-gnueabihf_6.4.1/bin/arm-linux-gnueabihf-
´´´

Configuration le noyau pour une nouvelle carte.

´´´bash
make ARCH=arm CROSS_COMPILE=$TOOLCHAIN socfpga_defconfig
´´´

**Pourquoi doit-on spécifier tous ces paramètres ?**

Modifier la configuration avec:

´´´bash
make ARCH=arm CROSS_COMPILE=$TOOLCHAIN menuconfig
´´´

Modifier 'Userspace I/O platform driver with generic IRQ handling' pour la compilation en tant que module.

Vérification de la configuration:

´´´bash
$ grep UIO .config
CONFIG_UIO=m
# CONFIG_UIO_CIF is not set
CONFIG_UIO_PDRV_GENIRQ=m
# CONFIG_UIO_DMEM_GENIRQ is not set
# CONFIG_UIO_AEC is not set
# CONFIG_UIO_SERCOS3 is not set
# CONFIG_UIO_PCI_GENERIC is not set
# CONFIG_UIO_NETX is not set
# CONFIG_UIO_PRUSS is not set
# CONFIG_UIO_MF624 is not set
´´´

**Pouvez-vous expliquer les lignes ci-dessous ?**

´´´bash	
#include "socfpga_cyclone5.dtsi"
#include <dt-bindings/interrupt-controller/irq.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>

/ {
    model = "Terasic SoCkit";
    compatible = "terasic,socfpga-cyclone5-sockit", "altr,socfpga-cyclone5", "altr,socfpga";

    soc {
        drv2025 {
            compatible = "drv2025";
            reg = <0xff200000 0x1000>;
            interrupts = <GIC_SPI 41 IRQ_TYPE_EDGE_RISING>;
            interrupt-parent = <&intc>;
        };
    };

    (...)
´´´

Compilation du noyau:

´´´bash
make ARCH=arm CROSS_COMPILE=$TOOLCHAIN -j8
rm ./tmp -rf
make ARCH=arm CROSS_COMPILE=$TOOLCHAIN INSTALL_MOD_PATH="./tmp" modules_install
´´´

Backup de l'ancien noyau/DT et copie des nouveaux fichiers:

´´´bash
cp $HOME/tftpboot/socfpga.dtb $HOME/tftpboot/socfpga.dtb.old
cp $HOME/tftpboot/zImage $HOME/tftpboot/zImage.old
cp arch/arm/boot/dts/socfpga_cyclone5_sockit.dtb $HOME/tftpboot/socfpga.dtb
cp arch/arm/boot/zImage $HOME/tftpboot/
cp ./tmp/lib/modules/* /export/drv/ -R
´´´

**Qu'est-ce que ces commandes font ? Est-il nécessaire de recompiler tout le noyau suite à nos changements ? Et si l'on modifie encore le Device Tree?**

**Quelle commmande pouvez-vous effectuer sur la carte afin d'avoir certaines informations sur le driver uio_pdrv_genirq, telles que l'auteur et la license ?**

___

# Accès aux périphériques en utilisant /dev/mem

## Exercice 1

[ex1.c](ex1.c)

Compilation :

´´´bash
arm-linux-gnueabihf-gcc-6.4.1 ex1.c -o lab2ex1
´´´

Mettre dans le répertoire '/export/drv/' et exécuter sur la carte.

```bash
~/drv/lab2ex1
```

___

# Accès aux périphériques en utilisant le UIO framework

## Exercice 2

**Pourquoi une région de 4096 bytes et non pas 5000 ou 10000 ? Et pourquoi on a spécifié cette adresse ?**

Car tous les chiffres qu'on utilise en programmation sont en base 2, car on fait des opérations binaires. 4096 est une puissance de 2, 2^12 = 4096. 

C'est probablement la taille standard pour une page mémoire.

**Quelles sont les différences dans le comportement de 'mmap()' susmentionnées ?**



**Effectuez des recherches avec Google/StackOverflow/... et résumez par écrit les avantages et les inconvénients des drivers user-space par rapport aux drivers kernel-space.**


## Exercice 3

**Ecrivez un driver user-space pour réaliser la même tâche que l'exercice 1 (mais gardez une copie de l'exercice 1). Vous pouvez par exemple prendre exemple sur le tutoriel disponible ici.**

Pas besoin de gérer les interruptions pour cet exercice.


Cette commande doit être exécutée après chaque boot pour utiliser les UIO !

´´´bash	
modprobe -r uio_pdrv_genirq; modprobe uio_pdrv_genirq of_id="drv2025"
´´´


___

# UIO framework et interruptions



