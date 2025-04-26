# Laboratoire Driver

Fork du projet [drv25_student](https://reds-gitlab.heig-vd.ch/reds-public/drv25_student) pour le cours de *Développement de Driver* à la HEIG-VD.

## Membres du groupe

- Urs Behrmann

# Setup de la DE1-SoC

## Configurer le MODE SELECT (MSEL)

![MODE SELECT (MSEL)](/docs/_images/msel.png)

# Commandes pour les labos

## Fetch des nouveaux Labos

```bash
git fetch upstream
git merge upstream/main -Xtheirs
git push origin main
```

## Connexion à la carte

Connexion UART :

```bash
sudo picocom /dev/ttyUSB0 --b 115200
```

MDP : 'heig' ou rien

Connexion SSH :

```bash
ssh root@192.168.0.2
```

## Compilation et run des exercices

Compilation :

```bash
arm-linux-gnueabihf-gcc-6.4.1 ex1.c -o lab1ex1
```

Mettre dans le répertoire '/export/drv/' et exécuter sur la carte.

```bash
~/drv/lab1ex1
```

# Feedback lab1

Bonsoir,
 
Voici un rapide feedback sur votre code de l'exercice 3 du laboratoire 1:
- Compile OK
- Consigne: fichier nommé ex<n>.c
- Bon code, bien commenté
- Déclarer les variables / fonctions "static" (sauf main())
- Les variables globales pourraient plutôt être passées en paramètre
- Il serait plus robuste / réactif de détecter le "flanc montant"lors de l'appui sur les boutons (transition 0 -> 1) - mais le délai300ms permet de répéter en laissant la touche appuyé alors why not
- MAP_SIZE hardcodé à 4kB, utiliser getpagesize() ou équivalent
- Attention au coding style (utiliser des tab pour l'indentation):https://www.kernel.org/doc/html/v6.1/process/coding-style.html

N'hésitez pas à me demander si vous avez des questions.
Bonne soirée
