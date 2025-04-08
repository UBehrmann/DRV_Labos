<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 3 : Introduction aux drivers kernel-space <!-- omit in toc -->

# Auteur <!-- omit in toc -->

- Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Exercice 1](#exercice-1)
- [Exercice 2](#exercice-2)
- [Exercice 3](#exercice-3)
- [Exercice 4](#exercice-4)

# Exercice 1

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


# Exercice 2

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

# Exercice 3

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

# Exercice 4

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

</div>