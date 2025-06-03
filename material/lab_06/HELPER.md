<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 4 : Développement de drivers kernel-space I <!-- omit in toc -->

# Auteur <!-- omit in toc -->

- Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Exercice 1 : kfifo et kthread](#exercice-1--kfifo-et-kthread)
  - [Compilation :](#compilation-)
  - [Montez le module:](#montez-le-module)
  - [Utilisez le fichier `/dev/lab5ex2` pour envoyer les commandes "up" et "down".](#utilisez-le-fichier-devlab5ex2-pour-envoyer-les-commandes-up-et-down)
  - [Démontez le module:](#démontez-le-module)
- [Exercice 2 : ktimer](#exercice-2--ktimer)
  - [Montez le module:](#montez-le-module-1)
  - [Utilisez le fichier `/dev/lab5ex2` pour envoyer les commandes "up" et "down".](#utilisez-le-fichier-devlab5ex2-pour-envoyer-les-commandes-up-et-down-1)
  - [Démontez le module:](#démontez-le-module-1)
- [Exercice 3 : sysfs](#exercice-3--sysfs)
  - [Montez le module:](#montez-le-module-2)
  - [Utilisez le fichier `/dev/lab5ex3` pour envoyer les commandes "up" et "down".](#utilisez-le-fichier-devlab5ex3-pour-envoyer-les-commandes-up-et-down)
  - [Utilisez le 'sysfs'](#utilisez-le-sysfs)
    - [Pour récupérer et modifier intervalle entre l'allumage et l'extinction de la LED:](#pour-récupérer-et-modifier-intervalle-entre-lallumage-et-lextinction-de-la-led)
    - [Pour récupérer le numéro de la LED actuellement allumée:](#pour-récupérer-le-numéro-de-la-led-actuellement-allumée)
    - [Pour récupérer le nombre de séquences jouées:](#pour-récupérer-le-nombre-de-séquences-jouées)
    - [Pour récupérer le nombre de séquences restantes dans la FIFO:](#pour-récupérer-le-nombre-de-séquences-restantes-dans-la-fifo)
    - [Pour récupérer la séquence restante dans la FIFO:](#pour-récupérer-la-séquence-restante-dans-la-fifo)
  - [Démontez le module:](#démontez-le-module-2)
- [Exercice 4 : synchronisation](#exercice-4--synchronisation)
  - [Montez le module:](#montez-le-module-3)
  - [Utilisez le fichier `/dev/lab5ex4` pour envoyer les commandes "up" et "down".](#utilisez-le-fichier-devlab5ex4-pour-envoyer-les-commandes-up-et-down)
  - [Démontez le module:](#démontez-le-module-3)

___

# Exercice 1

## Check si le DTB a été changé correctement

```bash
i2cdetect -y -r 0
```

| Adresse | Nom du périphérique | Valeur a setter / attendu |
| ------- | ------------------- | ------------------------- |
| 0x00    | DEVID               | 0xE5                      |
| 0x2D    | POWER_CTL           | 0x08                      |
| 0x31    | DATA_FORMAT         | 0x01                      |
| 0x32    | DATAX0              |
| 0x33    | DATAX1              |
| 0x34    | DATAY0              |
| 0x35    | DATAY1              |
| 0x36    | DATAZ0              |
| 0x37    | DATAZ1              |

## Fonctions utiles

```c
// Lecture d'un octet
i2c_smbus_read_byte_data()
// Écriture d'un octet
i2c_smbus_write_byte_data()
// Lecture de plusieurs octets
i2c_smbus_read_i2c_block_data()
```

## Montez le module:

```bash
sudo insmod ~/drv/lab6ex1.ko
```

```bash
lsmod | grep lab6ex1
```

## Démontez le module:

```bash
sudo rmmod lab6ex1
```

# Exercice 2

| Adresse | Nom du périphérique |
| ------- | ------------------- |
| 0x1D    | THRESH_TAP          |
| 0x21    | DUR                 |
| 0x22    | Latent              |
| 0x23    | Window              |
| 0x2A    | TAP_AXES            |
| 0x2B    | ACT_TAP_STATUS      |
| 0x2E    | INT_ENABLE          |
| 0x30    | INT_SOURCE          |

</div>