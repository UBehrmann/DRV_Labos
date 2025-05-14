<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 5 :  <!-- omit in toc -->

# Auteur <!-- omit in toc -->

- Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Exercice 1 : kfifo et kthread](#exercice-1--kfifo-et-kthread)
- [Exercice 2 : ktimer](#exercice-2--ktimer)
- [Exercice 3](#exercice-3)
- [Exercice 4 : synchronisation](#exercice-4--synchronisation)

___

# Exercice 1 : kfifo et kthread

Pour la 'kfifo', j'ai ajouté un verrouillage comme indiqué pour éviter les problèmes avec les accès concurrents.

Pour les commandes, il faut utiliser le fichier `/dev/lab5ex1` pour envoyer les commandes "up" et "down".

```bash
echo "up" > /dev/lab5ex1
echo "down" > /dev/lab5ex1
```

___

# Exercice 2 : ktimer

Pour les commandes, il faut utiliser le fichier `/dev/lab5ex2` pour envoyer les commandes "up" et "down".

```bash
echo "up" > /dev/lab5ex2
echo "down" > /dev/lab5ex2
```

___

# Exercice 3

## Utilisez le 'sysfs'  <!-- omit in toc -->

### Pour récupérer et modifier intervalle entre l'allumage et l'extinction de la LED: <!-- omit in toc -->

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/interval_ms
echo 200 > /sys/devices/platform/soc/ff200000.drv2025/interval_ms
```

### Pour récupérer le numéro de la LED actuellement allumée: <!-- omit in toc -->

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/led_current
```

### Pour récupérer le nombre de séquences jouées: <!-- omit in toc -->

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/seq_count
```

### Pour récupérer le nombre de séquences restantes dans la FIFO: <!-- omit in toc -->

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/kfifo_count
```

### Pour récupérer la séquence restante dans la FIFO: <!-- omit in toc -->

```bash
cat /sys/devices/platform/soc/ff200000.drv2025/sequence
```

___

# Exercice 4 : synchronisation

Compile et fonctionne comme l'exercice 3, mais je ne sais pas vraiment comment tester si la gestion des accès concurrents fonctionne correctement.

Si j'utilise deux terminales, je n'arrive pas à les envoyer exactement en même temps.

</div>