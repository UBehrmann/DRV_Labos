<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 2 : User-space Drivers <!-- omit in toc -->

# Auteur <!-- omit in toc -->

- Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Introductions aux périphériques sous Linux](#introductions-aux-périphériques-sous-linux)
  - [Exercice 1](#exercice-1)
  - [Exercice 2](#exercice-2)
  - [Exercice 3](#exercice-3)
- [Compilation de modules](#compilation-de-modules)
  - [Makefile pour hôte](#makefile-pour-hôte)
  - [Makefile pour la cible](#makefile-pour-la-cible)
  - [Exercice 4](#exercice-4)
- [Module simple](#module-simple)
  - [Exercice 5](#exercice-5)

# Introductions aux périphériques sous Linux

## Exercice 1

**Utilisez la page de manuel de la commande 'mknod' pour en comprendre le fonctionnement. Créez ensuite un fichier virtuel de type caractère avec le même couple majeur/mineur que le fichier '/dev/random'. Qu'est-ce qui se passe lorsque vous lisez son contenu avec la commande 'cat' ?**

```bash
ls -la /dev/random
```

Output:

```bash
crw-rw-rw- 1 root root 1, 8 Apr  2 10:39 /dev/random
```

[mknod](https://man7.org/linux/man-pages/man2/mknod.2.html) est la page de manuel de la commande 'mknod'.

```bash
mknod /dev/myrandom c 1 8sudo mknod /dev/myrandom c 1 8
ls -la /dev/myrandom
```

Output:

```bash
crw-r--r-- 1 root root 1, 8 Apr  2 10:44 /dev/myrandom
```

```bash
cat /dev/myrandom
```
Output:

```bash
�LG;���ga��9 �jπ�7:��.`�T��BQ����<���r��4,Zp\<�R��PEI�9�D������ MyQдH��A�uF���q��7��W����ƚwG�N�\j<K6�����M�I�L9$Q��������O�KwC���ؘ���ݴ]����4&�t��r���ϊ�Mw1�����B��o�̃��}uI
�$Q5��Bռy9���p�x�%���������Kn5
```

Lorsqu'on fait un 'cat' sur le fichier, on obtient un flux de données aléatoires.  '/dev/random' est un générateur de nombres aléatoires qui fournit des données aléatoires à partir d'une source d'entropie. En lisant le fichier, on obtient donc un flux de données aléatoires.


## Exercice 2

**Retrouvez cette information dans le fichier '/proc/devices'.**

```bash
cat /proc/devices | grep ttyUSB0
```

Output:

```bash
Character devices:
(...)
188 ttyUSB
```

## Exercice 3

**'sysfs' contient davantage d'informations sur le périphérique. Retrouvez-le dans l'arborescence de 'sysfs', en particulier pour ce qui concerne le nom du driver utilisé et le type de connexion. Ensuite, utilisez la commande 'lsmod' pour confirmer que le driver utilisé est bien celui identifié auparavant et cherchez si d'autres modules plus génériques sont impliqués. (en fonction de la distribution Linux, il se peut qu'aucuns modules plus génériques ne soient impliqués, car ils ont été configurés en mode "built-in" lors de la compilation du kernel)**

read sysfs:

```bash
ls /sys/bus/usb-serial/devices
```
Output:

```bash
total 0
lrwxrwxrwx 1 root root 0 Apr  2 11:02 ttyUSB0 -> ../../../devices/pci0000:00/0000:00:0c.0/usb1/1-2/1-2:1.0/ttyUSB0
```

```bash
ls /sys/bus/usb-serial/drivers
```

Output:

```bash
total 0
drwxr-xr-x 2 root root 0 Apr  2 10:52 ftdi_sio
```

```bash
lsmod | grep usbserial
```
Output:

```bash
usbserial              57344  1 ftdi_sio
```

# Compilation de modules

## Makefile pour hôte	

Le 'Makefile' suivant peut être utilisé pour compiler un module pour l'hôte:

```makefile
obj-m = empty.o

KVERSION = $(shell uname -r)
KERNELSRC = /lib/modules/$(KVERSION)/build/

all:
	make -C $(KERNELSRC) M=$(PWD) modules
clean:
	make -C $(KERNELSRC) M=$(PWD) clean
```

## Makefile pour la cible

Le 'Makefile' suivant peut être utilisé pour compiler un module pour la cible:

```makefile
### Put here the path to kernel sources! ###
KERNELDIR := /home/reds/linux-socfpga/
TOOLCHAIN := /opt/toolchains/arm-linux-gnueabihf_6.4.1/bin/arm-linux-gnueabihf-

obj-m := empty.o

PWD := $(shell pwd)
WARN := -W -Wall -Wstrict-prototypes -Wmissing-prototypes

all: empty

empty:
	@echo "Building with kernel sources in $(KERNELDIR)"
	$(MAKE) ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(KERNELDIR) M=$(PWD) ${WARN}

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers
```

## Exercice 4

**Sur votre machine hôte (laptop, machine de labo), pas sur la DE1-SoC. Compilez le module empty disponible dans les sources de ce laboratoire. Ensuite, montez-le dans le noyau, démontez-le, et analysez les messages enregistrés dans les logs.**

Montez le module:

```bash
sudo insmod empty.ko
```

Démontez le module:

```bash
sudo rmmod empty
```

Logs:

```bash
sudo dmesg | tail
```

```bash
[ 2446.882912] Hello there!
[ 2462.560665] Good bye!
```

# Module simple

## Exercice 5

Le module accumulate (disponible dans les sources de ce laboratoire), est un driver minimal de périphérique virtuel de type caractère. Il possède un majeur de 97. Une fois un device node associé à ce module, il est possible d'y écrire une série de nombres qui seront accumulés soit par addition, soit par multiplication. La remise à zéro ainsi que le choix de l'opération utilisée sont fait grâce à un appel à ioctl() sur le device node.

Un appel ioctl() prends deux paramètres entiers et renvoie un entier. Le programme ioctl.c permet d'effectuer ces appels facilement. Afin d'utiliser les bonnes pratiques, les macros _IO et _IOW ont été utilisées pour générer la valeur du premier paramètre (= numéro de l'ioctl). Pour ce module, ces valeurs sont écrites dans les logs à l'insertion. Plus d'information sur ces macros sont disponibles sur la doc officiel ainsi que dans le chapitre 6 de Linux Device Drivers, Third Edition (dans les premières pages du PDF correspondant).

Le module fonctionne de la façon suivante lors des appels ioctl :

- Si le premier paramètre de ioctl vaut ACCUMULATE_CMD_RESET, la valeur est remise à zéro.
- Si le premier paramètre vaut ACCUMULATE_CMD_CHANGE_OP, l'opération est modifiée en fonction du second paramètre (0 pour l'addition et 1 pour la multiplication).

Pour cet exercice :

- Compilez et cross-compilez ce driver pour votre machine et pour la DE1-SoC. Vérifiez que le driver soit bien inséré sur les deux plates-formes et récupérez un maximum d'informations sur ce périphérique grâce aux outils précédemment vus.
- Créez un device node afin de communiquer avec le driver (à choix sur votre machine ou sur la carte). Donnez les bons droits sur ce fichier afin que l'utilisateur courant puisse y accéder. Rendez un listing du device node. (c.-à-d. ls -la /dev/mynode).
- Effectuez une écriture (echo) et une lecture (cat) sur ce device node. Grâce à ioctl.c, testez la configuration du périphérique, puis démontez-le. Vérifiez également que le démontage du noyau ait bien été effectué. Rendez une copie texte de votre console.
- Ce driver a été écrit par un ingénieur pressé qui n'a pas trop bien fait son boulot : les valeurs de retour ne sont pas contrôlées ! Aidez-le en rajoutant ces vérifications !
- L'ingénieur qui a écrit ce driver doit se croire à l'âge de la pierre, car il a utilisé la fonction register_chrdev() qui ne devrait pas être utilisée à partir du noyau 2.6... Modernisez ce driver et rendez-le plus agréable à utiliser (p. ex., ce serait très bien de ne pas devoir créer à la main le fichier du dispositif). Vous pouvez vous référer à cet article pour une utilisation de alloc_chrdev_region. Il est également possible d'utiliser le framework misc dont un exemple est donné par la V1 de reds_adder dans les tutoriels, ainsi que sur la page Cyberlearn du cours.
- Modifiez ensuite les fonctions accumulate_read() et accumulate_write() pour qu'on puisse les utiliser pour lire/écrire les valeurs en format binaire (et donc pas en ASCII) à travers des appels read() et write() d'un logiciel user-space. Ecrivez un petit logiciel userspace qui permet valider le fonctionnement du driver.

Montez le module:

```bash
sudo insmod accumulate.ko
```

```bash
lsmod | grep accumulate
```

Créez un device node:

```bash
sudo mknod /dev/mynode c 97 0
sudo chmod 666 /dev/mynode
```

```bash
ls -la /dev/mynode
```

écriture:
  
```bash
echo 5 > /dev/mynode
```

Lecture:

```bash
cat /dev/mynode
```

Démontez le module:

```bash
sudo rmmod accumulate
```

```bash
lsmod | grep accumulate
```

```bash
sudo dmesg | tail
```

```bash
[ 3297.736837] Acumulate ready!
[ 3297.736840] ioctl ACCUMULATE_CMD_RESET: 11008
[ 3297.736841] ioctl ACCUMULATE_CMD_CHANGE_OP: 1074014977
[ 3687.973040] Acumulate done!
```

Compilez le programme 'test_accumulate.c' et exécutez-le:

```bash
gcc -o test_accumulate test_accumulate.c -Wall
```

</div>