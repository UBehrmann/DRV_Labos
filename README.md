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
git merge upstream/main
git push origin main
```

Connexion UART :

```bash
sudo picocom /dev/ttyUSB0 --b 115200
´´´

MDP : heig

Connexion SSH :

´´´bash
ssh root@192.168.0.2
´´´