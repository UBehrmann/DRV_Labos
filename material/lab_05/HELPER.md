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

# Exercice 1 : kfifo et kthread

## Compilation :

```bash
make
```

## Montez le module:

```bash
sudo insmod ~/drv/lab5ex1.ko
```

```bash
lsmod | grep lab5ex1
```
## Utilisez le fichier `/dev/lab5ex2` pour envoyer les commandes "up" et "down".

```bash
echo "up" > /dev/lab5ex1
echo "down" > /dev/lab5ex1
```

## Démontez le module:

```bash
sudo rmmod lab5ex1
```

___

# Exercice 2 : ktimer

## Montez le module:

```bash
sudo insmod ~/drv/lab5ex2.ko
```

```bash
lsmod | grep lab5ex2
```

## Utilisez le fichier `/dev/lab5ex2` pour envoyer les commandes "up" et "down".

```bash
echo "up" > /dev/lab5ex2
echo "down" > /dev/lab5ex2
```

## Démontez le module:

```bash
sudo rmmod lab5ex2
```

___

# Exercice 3 : sysfs

## Montez le module:

```bash
sudo insmod ~/drv/lab5ex3.ko
```

```bash
lsmod | grep lab5ex3
```

## Utilisez le fichier `/dev/lab5ex3` pour envoyer les commandes "up" et "down".

```bash
echo "up" > /dev/lab5ex3
echo "down" > /dev/lab5ex3
```

## Utilisez le 'sysfs'

### Pour récupérer et modifier intervalle entre l'allumage et l'extinction de la LED:

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/interval_ms
echo 200 > /sys/devices/platform/soc/ff200000.drv2025/interval_ms
```

### Pour récupérer le numéro de la LED actuellement allumée:

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/led_current
```

### Pour récupérer le nombre de séquences jouées:

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/seq_count
```

### Pour récupérer le nombre de séquences restantes dans la FIFO:

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/kfifo_count
```

### Pour récupérer la séquence restante dans la FIFO:

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/sequence
```

## Démontez le module:

```bash
sudo rmmod labex3
```

___

# Exercice 4 : synchronisation

## Montez le module:

```bash
sudo insmod ~/drv/lab5ex4.ko
```

```bash
lsmod | grep lab5ex4
```

## Utilisez le fichier `/dev/lab5ex4` pour envoyer les commandes "up" et "down".

```bash
echo "up" > /dev/lab5ex4
echo "down" > /dev/lab5ex4
```

## Démontez le module:

```bash
sudo rmmod lab5ex4
```

</div>