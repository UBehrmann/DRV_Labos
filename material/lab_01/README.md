<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 1 : Introduction <!-- omit in toc -->

# Auteur <!-- omit in toc -->

- Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Préparation de la carte SD](#préparation-de-la-carte-sd)
- [Connexion console à la carte](#connexion-console-à-la-carte)
- [Configuration et utilisation de U-Boot](#configuration-et-utilisation-de-u-boot)
- [Vérification des toolchains](#vérification-des-toolchains)
- [Compilation sur la cible (native)](#compilation-sur-la-cible-native)
- [Compilation croisée sous DE1-SoC](#compilation-croisée-sous-de1-soc)
  - [Compilation statique](#compilation-statique)
  - [Run on DE1-SoC](#run-on-de1-soc)
- [Accès aux périphériques du CycloneV-SoC depuis U-Boot](#accès-aux-périphériques-du-cyclonev-soc-depuis-u-boot)
  - [Exercice 1](#exercice-1)
  - [Exercice 2](#exercice-2)
- [Accès aux périphériques du DE1-SoC depuis Linux](#accès-aux-périphériques-du-de1-soc-depuis-linux)
  - [Exercice 3](#exercice-3)

___

# Préparation de la carte SD

**dispositif associé avec le lecteur de cartes :** /dev/sdb

Commande de copie:

```bash
sudo dd if=drv-2024-sdcard.img of=/dev/sdb bs=4M conv=fsync status=progress
```

___

# Connexion console à la carte

**Connexion à la carte via USB**

Utilisez l'outil picocom (ou minicom, si vous le préférez) afin d'ouvrir une session de terminal avec la carte.

```bash
picocom -b 115200 /dev/ttyUSB0
```

Pour quitter Picocom, appuyez 'Ctrl+A' suivi de 'Ctrl+X'.

On devra donc mettre dans /home/reds/tftpboot l'image du noyau (zImage) et le fichier du Device Tree (socfpga.dtb) contenus dans l'archive drv-2025-boot.tar.gz. Ensuite, donnez les droits de lecture à tout le monde à ces fichiers avec

```bash	
cd /home/reds/tftpboot
chmod 777 *
```

___

# Configuration et utilisation de U-Boot

Pourriez-vous expliquer les lignes ci-dessus ?

```bash
setenv ethaddr "12:34:56:78:90:12"
setenv serverip "192.168.0.1"
setenv ipaddr "192.168.0.2"
setenv gatewayip "192.168.0.1"
setenv bootdelay "2"
setenv loadaddr "0xF000"
setenv bootcmd "mmc rescan; tftpboot ${loadaddr} zImage; tftpboot ${fdtaddr} ${fdtimage}; run fpgaload; run bridge_enable_handoff; run mmcboot"
setenv mmcboot "setenv bootargs console=ttyS0,115200 root=${mmcroot} rw rootwait ip=${ipaddr}:${serverip}:${serverip}:255.255.255.0:de1soclinux:eth0:on; bootz ${loadaddr} - ${fdtaddr}"
saveenv
```

___

# Vérification des toolchains

Pourriez-vous expliquer la signification des différents mots composant le nom de la toolchain? Pourquoi on a spécifié un numéro de version ? Qu'est-ce qui se passe si l'on prend tout simplement la toute dernière version de la toolchain?


Pourriez-vous expliquer ce que la ligne ci-dessus fait?

```bash
echo "int main(){}" | arm-linux-gnueabihf-gcc-6.4.1 -x c - -o /dev/stdout | file -
```	

**Réponse :** 

```bash	
/dev/stdin: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 2.6.32, BuildID[sha1]=b7a0db97fce5c152f9ae99800a40817aba3cfa1d, with debug_info, not stripped
```

Savez-vous interpréter ces réponses ?

Que signifie "not stripped" ?


___

# Compilation sur la cible (native)


Quelle version de GCC est installée sur la carte ?

```bash
root@de1soclinux:~# arm-linux-gnueabihf-gcc --version
arm-linux-gnueabihf-gcc (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3
Copyright (C) 2011 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

___

# Compilation croisée sous DE1-SoC

```bash	
$ arm-linux-gnueabihf-gcc-6.4.1 hello_cross.c -o hello_cross
$ ls -al hello_cross
-rwxrwx--- 1 root vboxsf 11888 Mar  5 10:44 hello_cross
$ file hello_cross
hello_cross: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 2.6.32, BuildID[sha1]=fe4b724f54424e522425b6a72d0eb6d0cdf60784, with debug_info, not stripped
```

Qu'est-ce qui se passerait si vous essayiez d'exécuter ce fichier sur votre PC ? Pourquoi ?

```bash
./hello_cross
bash: ./hello_cross: cannot execute binary file: Exec format error
```

## Compilation statique

```bash
$ arm-linux-gnueabihf-gcc-6.4.1 hello_cross.c -o hello_static -static
$ ls -al hello_static
-rwxrwx--- 1 root vboxsf 2789828 Mar  5 10:46 hello_static
$ file hello_static
hello_static: ELF 32-bit LSB executable, ARM, EABI5 version 1 (GNU/Linux), statically linked, for GNU/Linux 2.6.32, BuildID[sha1]=991a4409ada95761c68765a2b2b53b071eaeac0f, with debug_info, not stripped
```

## Run on DE1-SoC

```bash
root@de1soclinux:~# ~/drv/hello_cross
Hello, cross-compiled world!
```

___

# Accès aux périphériques du CycloneV-SoC depuis U-Boot

## Exercice 1

Expliquez les différences entre les lignes ci-dessous.

```bash
md.b 0x80008000 0x1
80008000: 46    F
md.w 0x80008000 0x1
80008000: 4c46    FL
md.l 0x80008000 0x1
80008000: eb004c46    FL..
md.b 0x80008000 0x4
80008000: 46 4c 00 eb    FL..
md.w 0x80008000 0x4
80008000: 4c46 eb00 9000 e10f    FL......
md.l 0x80008000 0x4
80008000: eb004c46 e10f9000 e229901a e319001f    FL........).....
```

Utilisez la commande md pour lire la valeur binaire écrite avec les switches et écrivez-la sur les LEDs.

| Base Address | End Address | I/O Peripheral  |
| ------------ | ----------- | --------------- |
| 0xFF200000   | 0xFF20000F  | Red LEDs        |
| 0xFF200040   | 0xFF20004F  | Slider Switches |

```bash
md.b 0xFF200040 0x1
ff200040: 03  .
mw.b 0xFF200000 0x3 0x1
```	

Qu'est-ce qui se passe si vous essayez d'accéder à une adresse qui n'est pas alignée (par exemple 0x01010101) et pourquoi ?

```bash
SOCFPGA_CYCLONE5 # md.b 0x01010101 0x1
01010101: ea    .
```

## Exercice 2

Écrivez un script (à l'aide d'une variable d'environnement) U-Boot qui va alterner, chaque seconde, entre afficher 🯰🯰🯰🯰🯰🯰 et 🯵🯵🯵🯵🯵🯵 sur les affichages à 7-segments.

**Adresse des affichages à 7-segments**

| Base Address | End Address | I/O Peripheral               |
| ------------ | ----------- | ---------------------------- |
| 0xFF200020   | 0xFF20002F  | 7-segment HEX3−HEX0 Displays |
| 0xFF200030   | 0xFF20003F  | 7-segment HEX5−HEX4 Displays |

**000000**

```bash
mw.b 0xFF200020 0x3f 0x4
mw.b 0xFF200030 0x3f 0x2
```

**555555**

```bash
mw.b 0xFF200020 0x6d 0x4
mw.b 0xFF200030 0x6d 0x2
```

**sleep N**

Attendez N secondes.

```bash
sleep 1
```

### Solution <!-- omit in toc -->

```bash	
setenv led_loop 'while true; do mw.b 0xFF200020 0x3f 0x4; mw.b 0xFF200030 0x3f 0x2; sleep 1; mw.b 0xFF200020 0x6d 0x4; mw.b 0xFF200030 0x6d 0x2; sleep 1; done'
saveenv
run led_loop
```
___

# Accès aux périphériques du DE1-SoC depuis Linux

Pouvez-vous identifier au moins deux gros problèmes de cette approche ?



## Exercice 3

Écrivez un logiciel user-space en C qui utilise /dev/mem pour accéder aux périphériques. Au démarrage, le logiciel commence par afficher A sur l’affichage HEX0. Ensuite, l’utilisateur peut contrôler la lettre affichée sur HEX0 de la façon suivante:

En appuyant sur KEY0, le caractère actuellement affiché est décrémenté
Z devient Y, ... , B devient A

Si A est affiché, on recommence à Z

En appuyant sur KEY1, le caractère actuellement affiché est incrémenté
A devient B, ... , Y devient Z

Si Z est déjà affiché, on recommence à A

De plus, le code ASCII de la lettre affichée doit être représenté en binaire sur les LEDs.

Exemple: si A est affiché (soit 0x41 / 0b1000001 en ASCII), LED0 et LED6 doivent être allumée

Note: vous pouvez trouver un exemple de police de caractère pour afficheurs 7-segments sur cette page Wikipedia. Libre à vous de vous en inspirer.

Bonus 1: éteignez les afficheurs 7-segments et les LEDs en partant (quand le programme est arrêté avec Ctrl+C) pour éviter de faire fondre la banquise inutilement.



Quel est le souci principal dans l'écriture de ce logiciel ?


</div>