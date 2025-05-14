<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 4 : Développement de drivers kernel-space I <!-- omit in toc -->

# Auteur <!-- omit in toc -->

- Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Exercice 1 : kfifo et kthread](#exercice-1--kfifo-et-kthread)
- [Exercice 2 : ktimer](#exercice-2--ktimer)

# Exercice 1 : kfifo et kthread

Compilation :

```bash
export TOOLCHAIN=/opt/toolchains/arm-linux-gnueabihf_6.4.1/bin/arm-linux-gnueabihf-
```

```bash
make
```

arm-linux-gnueabihf-gcc-6.4.1 lab5ex1.c -o lab5ex1


Montez le module:

```bash
sudo insmod ~/drv/lab5ex1.ko
```

```bash
lsmod | grep lab5ex1
```

echo "up" > /dev/lab5ex1
echo "down" > /dev/lab5ex1

sudo dmesg | tail


Démontez le module:

```bash
sudo rmmod lab5ex1
```

# Exercice 2 : ktimer

Montez le module:

```bash
sudo insmod ~/drv/lab5ex2.ko
```

```bash
lsmod | grep lab5ex2
```

echo "up" > /dev/lab5ex2
echo "down" > /dev/lab5ex2

sudo dmesg | tail


Démontez le module:

```bash
sudo rmmod lab5ex2
```

</div>