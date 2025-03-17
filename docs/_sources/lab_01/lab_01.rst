.. _laboratoire1:

##############################
Laboratoire 1 --- Introduction
##############################

.. only:: latex

   .. figure:: ../images/banner_drv.png
      :width: 16cm
      :align: center


Le but de ces laboratoires est de vous familiariser avec les méthodologies de développement pour la création de drivers.
En effet (comme pour beaucoup de choses dans la vie réelle), il y a un gros décalage entre ce qu'on peut connaître sur la
théorie du fonctionnement d'un système d'exploitation et l'application de ces concepts dans la pratique.
En particulier, les drivers se situent dans une couche assez délicate, car ils sont à l'interface entre le monde utilisateur et le
matériel sous-jacent, donc ils sont affectés par les erreurs des deux mondes.

Dans ces consignes on vous invitera souvent à pousser vos investigations plus loin.
Essayez de donner une réponse par vous-mêmes aux questions qu'on vous posera,
cela vous aidera à mieux comprendre le sujet !

.. note:: Il est souvent très très difficile comprendre si une erreur est liée à une mauvaise manipulation au niveau logiciel utilisateur,
          à un bug dans le driver, à un bug du hardware, ... !

.. raw:: latex

    \newpage

=========
Objectifs
=========

Préparation de l'environnement de développement et prise en main de la carte DE1-SoC
====================================================================================

* Configurer l'écosystème de développement pour le cours DRV
* Communiquer avec la carte via le réseau et la console
* Connaître les fonctionnalités de base de U-Boot
* Utiliser U-Boot afin d'interagir directement avec les périphériques de la DE1-SoC
* Utiliser la toolchain de cross-compilation Linux afin de produire des exécutables destinés à la distribution Linux de la DE1-SoC

Premières interactions HW-SW
============================

* Manipuler le hardware DE1-SoC depuis un logiciel userspace

===================
Matériel nécessaire
===================

Dans ce laboratoire on préparera l'écosystème de développement qu'on utilisera pour toute la durée du cours.

Pour cela, il vous faudra télécharger les éléments suivants :

* une toolchain de cross-compilation Linux (déjà installée sur les machines de laboratoire et dans la
  machine virtuelle). On utilisera une toolchain de Linaro, que vous pouvez
  télécharger `ici <https://releases.linaro.org/components/toolchain/binaries/6.4-2018.05/arm-linux-gnueabihf/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz>`__
* une archive :file:`drv-2025-boot.tar.gz` contenant les fichiers nécessaires au boot de la DE1-SoC [disponible `ici <http://reds-data.heig-vd.ch/cours/2025_drv/drv-2025-boot.tar.gz>`__]
* une archive :file:`drv-2025-sdcard.tar.gz` contenant une image d'une carte SD pour la DE1-SoC [disponible `ici <http://reds-data.heig-vd.ch/cours/2025_drv/drv-2025-sdcard.tar.gz>`__]
* la documentation Altera, disponible dans le git du laboratoire (:file:`material/lab_01/`).

====================================================================================
Préparation de l'environnement de développement et prise en main de la carte DE1-SoC
====================================================================================

Présentation de la carte DE1-SoC
================================

La carte `DE1-SoC <https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=836>`__
est une carte de développement robuste et low-cost qui intègre un processeur ARM Cortex-A9 (dual core)
avec une FPGA (grâce au chip `Cyclone V SoC <https://www.intel.com/content/www/us/en/products/programmable/soc/cyclone-v.html>`__).

Dans ce cours on se focalisera sur le processeur ARM (aussi appelé HPS -- *Hard Processor System*).
Dans d'autres cours vous pourrez découvrir comment faire interagir ce processeur avec la partie FPGA
(aussi appelée PL -- *Programmable Logic*).

Au début du cours on utilisera l'image crée par Intel-Altera dans le contexte du
`Intel-Altera University Program <https://www.intel.com/content/www/us/en/developer/topic-technology/fpga-academic/overview.html>`__.
On customisera ensuite le noyau pour répondre à nos besoins.

.. warning:: Les outputs des commandes montrés ci-dessous sont ce que j'obtiens sur **ma** machine.
             Ces textes changent d'une machine à l'autre, si vous utilisez une machine virtuelle,
             si vous avez fait des mises à jour, ...
             Regardez donc ce qui le texte **dit**, et non seulement s'il est identique à ce qui
             est noté ci-dessous !!!

Préparation de la carte
=======================
Avant d'allumer la carte, assurez-vous que les switches du MODE SELECT (MSEL) sous
la carte soient comme dans la figure ci-dessous.

.. figure:: images/msel.png
   :width: 6cm
   :align: center

Ces switches sont utilisés pour dire à la DE1-SoC comment la FPGA sera configurée.
Avec la configuration ci-dessus, ce sera Linux qui se chargera de la tâche.

Configuration du PC
===================

Pour la suite du cours, on partira du principe que vous travaillez sous Linux
--- soit avec une machine virtuelle/machine de laboratoire, soit avec votre ordinateur portable.
Pour la connexion avec la carte, il faut que votre carte réseau ait l'adresse **192.168.0.1**.
La DE1-SoC aura, elle, l'adresse **192.168.0.2**.

.. important:: Les commandes ci-dessous ont été testées dans une distribution de
               Linux Ubuntu-like.
               Des différences sont possibles si vous utilisez une autre
               distribution.

Préparation de la carte SD
==========================

Une fois téléchargé l'archive contenant l'image de la carte SD, vous devez identifier le dispositif associé avec le lecteur de cartes avec la commande **dmesg**:

.. code-block:: console

   HOST:~$ dmesg
   ...
   130088.305550] sd 6:0:0:1: [sde] 31422464 512-byte logical blocks: (16.1 GB/15.0 GiB)
   [130088.306567] sd 6:0:0:1: [sde] Write Protect is off
   [130088.306575] sd 6:0:0:1: [sde] Mode Sense: 2f 00 00 00
   [130088.307716] sd 6:0:0:1: [sde] Write cache: disabled, read cache: enabled, doesn't support DPO or FUA
   [130088.313749]  sde: sde1 sde2 sde3
   [130088.316561] sd 6:0:0:1: [sde] Attached SCSI removable disk

.. figure:: images/skull.png
   :width: 6cm
   :align: center

Dans **mon** cas, il s'agit de :file:`/dev/sde`. FAITES BIEN ATTENTION AU NOM QUE
**VOUS** OBTENEZ !

:boldred:`Dans la commande ci-dessous, il faudra remplacer`
:file:`VOTREDISPOSITIF`
:boldred:`avec le nom du dispositif que vous avez obtenu grâce à dmesg.`

Vous pourrez ainsi préparer la carte avec les commandes :

.. code-block:: console

   HOST:~$ tar xzvf drv-2025-sdcard.tar.gz
   HOST:~$ sudo dd if=drv-2024-sdcard.img of=/dev/VOTREDISPOSITIF bs=4M conv=fsync status=progress

Replacez la carte SD dans la board DE1-SoC une fois la copie terminée !

Vous trouverez davantage d'informations sur la commande :code:`dd` `ici <https://en.wikipedia.org/wiki/Dd_(Unix)>`__.

Connexion console à la carte
============================

La connexion à la console série se fait sur le connecteur mini-USB situé près du lecteur de cartes SD.
Sur le même côté, une prise Ethernet est aussi disponible.
Connectez la carte et mettez-la sous tension.

Vérifiez d'abord que la carte soit bien détectée par l'ordinateur à l'aide de la commande :bash:`lsusb`.
La réponse devrait contenir la ligne suivante :

.. code-block:: text

   Bus 001 Device 008: ID 0403:6001 Future Technology Devices International, Ltd FT232 USB-Serial (UART) IC

Le convertisseur série/USB de la DE1-SoC apparaît sous la forme d'un fichier :file:`ttyUSBX` dans le répertoire :file:`/dev`.

.. note:: C'est où sur la carte ce chip ? N'hésitez pas à explorer les composants sur la
          carte à l'aide de la feuille incluse dans la boîte de la carte !

Utilisez l'outil **picocom** (ou **minicom**, si vous le préférez) afin d'ouvrir une session de terminal avec la carte.

.. code-block:: console

   HOST:~$ picocom -b 115200 /dev/ttyUSB0

Si vous avez des soucis de permissions, cela veut dire que l'utilisateur local n'est pas dans le groupe **dialout**.
Vous pouvez vérifier cela avec la commande :console:`HOST:~$ groups | grep dialout`.
Si cela est le cas, vous pouvez ajouter l'utilisateur au groupe avec la commande

.. code-block:: console

   HOST:~$ sudo usermod -aG dialout $USER

Ensuite il faudra fermer la session et la rouvrir, afin que les groupes soient
lus à nouveau par le système d'exploitation.

Picocom devrait vous retourner plusieurs lignes, pour terminer avec

.. code-block:: text

   [    2.269306] Freeing unused kernel memory: 468K (c0823000 - c0898000)
   [    2.338863] usb 1-1: new high-speed USB device number 2 using dwc2
   [    2.483361] random: init urandom read with 13 bits of entropy available
   [    2.548904] usb 1-1: New USB device found, idVendor=0424, idProduct=2512
   [    2.555595] usb 1-1: New USB device strings: Mfr=0, Product=0, SerialNumber=0
   [    2.563509] hub 1-1:1.0: USB hub found
   [    2.567430] hub 1-1:1.0: 2 ports detected
   [    2.689826] init: ureadahead main process (634) terminated with status 5
   [    3.138233] random: nonblocking pool is initialized

   Last login: Thu Jan  1 00:00:10 UTC 1970 on tty1
   root@de1soclinux:~# uname -ar
   Linux de1soclinux 3.18.0 #9 SMP Mon Aug 8 17:11:41 EDT 2016 armv7l armv7l armv7l GNU/Linux
   root@de1soclinux:~#

Si vous ne voyez rien, tapez sur la touche *enter*.

.. important::

    Veuillez noter que le noyau par défaut de la DE1-SoC n'est pas tout neuf... on remédiera bientôt à cela !

.. important::

    Pour quitter Picocom, appuyez **Ctrl+A** suivi de **Ctrl+X**.

Configuration des serveurs NFS et TFTP
======================================

Ces deux services, sur la machine hôte, vont vous permettre d'échanger des fichiers entre la carte cible et la machine hôte.
En particulier, on utilisera TFTP pour "envoyer" à la DE1-SoC le noyau Linux (et le Device Tree), alors qu'on utilisera
NFS pour garder une partie du filesystem sur notre ordinateur.
Cela a un double avantage :

* On travaillera directement sur l'ordinateur, sans devoir à chaque fois échanger explicitement les fichiers entre la DE1-SoC et le PC (plus efficace).
* On évitera d'écrire en continu sur la carte SD (donc on évitera de l'endommager pour rien).

Il faut d'abord vérifier que ces deux services soient installés et actifs.
Si ce n'est pas le cas sur votre machine, installez les paquets
**nfs-kernel-server** et **tftpd-hpa**.

Il faut ensuite éditer le fichier :file:`/etc/exports` pour dire à NFS quels répertoires on veut partager
et avec qui :

.. code-block:: text

    /export        192.168.0.0/24(rw,no_subtree_check,sync,no_root_squash)
    /export/drv    192.168.0.0/24(rw,no_subtree_check,sync,no_root_squash)

Bien sûr, maintenant il faudra créer le répertoire :file:`/export/drv`.

.. code-block:: console

    HOST:~$ sudo mkdir -p /export/drv
    HOST:~$ sudo chown reds:reds /export -R

Dans ce répertoire on pourra mettre des fichiers à partager avec la DE1-SoC.
Il faudra ensuite redémarrer NFS pour que la configuration soit relue.

.. code-block:: console

    HOST:~$ sudo service nfs-kernel-server restart

Maintenant, en regardant l'état du serveur NFS, vous devriez avoir
(**le message pourrait changer selon la version du serveur!!**) :

.. code-block:: console

    HOST:~$ service nfs-kernel-server status
    * nfs-server.service - NFS server and services
            Loaded: loaded (/lib/systemd/system/nfs-server.service; enabled; vendor preset: enabled)
        Drop-In: /run/systemd/generator/nfs-server.service.d
                    └─order-with-mounts.conf
            Active: active (exited) since Wed 2022-08-18 07:12:14 CEST; 1 day 3h ago
        Process: 1021 ExecStartPre=/usr/sbin/exportfs -r (code=exited, status=0/SUCCESS)
        Process: 1026 ExecStart=/usr/sbin/rpc.nfsd $RPCNFSDARGS (code=exited, status=0/SUCCESS)
        Main PID: 1026 (code=exited, status=0/SUCCESS)

    Aug 18 07:12:13 rri-nb01 systemd[1]: Starting NFS server and services...
    Aug 18 07:12:14 rri-nb01 systemd[1]: Finished NFS server and services.

Vous pouvez vérifier les répertoires partagés avec la commande

.. code-block:: console

    HOST:~$ sudo showmount -e localhost
    Export list for host:
    /export/drv 192.168.0.0/24
    /export     192.168.0.0/24

Vérifions ensuite que le serveur TFTP soit actif (le message pourrait changer selon la version du
serveur) :

.. code-block:: console

   HOST:~$ /etc/init.d/tftpd-hpa status
   * tftpd-hpa.service - LSB: HPA's tftp server
     Loaded: loaded (/etc/init.d/tftpd-hpa; generated)
     Active: active (running) since Thu 2022-08-19 11:36:18 CEST; 13min ago
       Docs: man:systemd-sysv-generator(8)
    Process: 2071 ExecStart=/etc/init.d/tftpd-hpa start (code=exited, status=0/SUCCESS)
      Tasks: 1 (limit: 9485)
     Memory: 1.2M
     CGroup: /system.slice/tftpd-hpa.service
             └─2095 /usr/sbin/in.tftpd --listen --user reds --address :69 -s /home/reds/tftpboot

   Aug 19 11:36:18 reds2022 systemd[1]: Starting LSB: HPA's tftp server...
   Aug 19 11:36:18 reds2022 tftpd-hpa[2071]:  * Starting HPA's tftpd in.tftpd
   Aug 19 11:36:18 reds2022 tftpd-hpa[2071]:    ...done.
   Aug 19 11:36:18 reds2022 systemd[1]: Started LSB: HPA's tftp server.

Si l'on regarde son fichier de configuration :file:`/etc/default/tftpd-hpa`, on verra les lignes suivantes :

.. code-block:: console

    HOST:~$ cat /etc/default/tftpd-hpa
    # /etc/default/tftpd-hpa

    TFTP_USERNAME="reds"
    TFTP_DIRECTORY="/home/reds/tftpboot"
    TFTP_ADDRESS=":69"
    TFTP_OPTIONS="-s"

On devra donc mettre dans :file:`/home/reds/tftpboot` l'image du noyau (:file:`zImage`) et
le fichier du Device Tree (:file:`socfpga.dtb`) contenus dans l'archive :file:`drv-2025-boot.tar.gz`.
Ensuite, donnez les droits de lecture à tout le monde à ces fichiers avec

.. code-block:: console

    HOST:~$ cd /home/reds/tftpboot
    HOST:~$ chmod 777 *


Configuration réseau de la machine virtuelle
============================================

.. warning:: Lisez cette section **uniquement** si vous travaillez avec la machine virtuelle !

Pour qu'il soit possible faire le boot et partager des répertoires à travers la machine virtuelle, il
faut que la configuration de la machine virtuelle soit adaptée.
En particulier, il faudra ajouter une deuxième carte réseau qui soit bridgée sur votre carte Ethernet
(celle que vous utilisez pour vous connecter à la DE1-SoC).
Le nom de votre carte pourrait changer, dans **mon** cas, il s'agissait de **eth2** (voir image ci-dessous).

.. figure:: images/virtualbox_net.png
   :width: 600px
   :align: center

   Ajout de la carte réseau dans Virtualbox.

Une fois cela fait (il faut que la machine virtuelle soit éteinte), vous devez la configurer afin
qu'elle ait l'adresse *192.168.0.1*, netmask *255.255.255.0*, ne mettez rien comme gateway (voir
figure ci-dessous). Redémarrez afin que les changements soient pris en compte (le NetworkManager d'Ubuntu
est souvent un peu têtu...). Si l'adresse n'est toujours pas la bonne, cliquez sur
la connexion correspondante dans le NetworkManager (cela devrait la recharger et tout devrait être bon).

.. figure:: images/network_device.png
   :width: 600px
   :align: center

   Configuration de la carte réseau dans la VM Linux.

Assurez-vous que la machine hôte n'ait pas l'adresse *192.168.0.1* pour la carte
que vous avez bridgé --- sinon il aura un conflit (vous pouvez changer cette
adresse, p.ex., en *192.168.0.11*).

De plus, vous devrez donner accès au port USB en cochant la case comme dans la figure qui suit.

.. figure:: images/usb_conf.png
   :width: 600px
   :align: center

   Configuration de la porte USB dans VirtualBox.

Si vous ne voyez la porte en cliquant, c'est possible que l'utilisateur de la
machine hôte ne soit pas dans le groupe *vboxusers*. Vous pouvez l'ajouter avec :

.. code-block:: console

   HOST:~$ sudo usermod -aG vboxusers $USER

Il faut ensuite faire logout/login (dans la machine hôte) pour que cela soit pris en compte.


Paramétrage réseau sur la machine hôte
======================================

La connexion réseau s'effectue en reliant le port RJ45 de la DE1-SoC à celui de la machine hôte via
le câble réseau fourni. Cette connexion permettra de communiquer rapidement avec l'hôte via SSH ainsi
que d'utiliser un système de fichier NFS distant et de télécharger le noyau à travers TFTP.

Afin que la carte puisse bien se connecter à la machine hôte, il faut que
l'interface réseau à laquelle la DE1-SoC est connectée ait l'adresse 192.168.0.1.

.. important::

    Comme marqué dans la section VM ci-dessus, si vous utilisez la VM
    alors cela s'applique uniquement à la VM, la machine hôte **doit**
    avoir une autre adresse.

Les machines de laboratoire possèdent deux cartes LAN : une reliée au réseau de l'école, l'autre, secondaire, restant disponible
pour connecter un périphérique. Utilisez :bash:`ifconfig` pour identifier laquelle est connectée à la DE1-SoC.

.. code-block:: console

   HOST:~$ ifconfig
   p1p1   Link encap:Ethernet  HWaddr 00:02:b3:9a:01:ef
          UP BROADCAST MULTICAST  MTU:1500  Metric:1
          ...
   em1    Link encap:Ethernet  HWaddr 00:27:0e:28:c2:0d
          inet addr:10.192.22.161  Bcast:10.192.22.255  Mask:255.255.248.0
          ...

Dans ce cas, *em1* est connectée au réseau LAN de l'école (réseau *10.192.22.255/21*), la carte secondaire est donc *p1p1*.
Par la suite, on configurera le bootloader de la DE1-SoC et le noyau Linux embarqué pour se connecter à l'adresse 192.168.0.1
en utilisant les protocoles NFS et TFTP lors du boot.
Si la carte secondaire n'est pas déjà initialisée à cette adresse une fois la DE1-SoC connectée,
activez manuellement *p1p1* et assignez-lui cette adresse.

.. warning::

    Si vous utilisez une distribution Ubuntu-derived (comme c'est le
    cas pour la machine virtuelle et les machines de laboratoire),
    changer les paramètres du réseau depuis la ligne de commande n'est pas forcément
    une bonne idée, car l'outil *NetworkManager* veut avoir le contrôle absolu des cartes réseau.
    **Vous devrez donc changer ces paramètres à l'aide de l'outil visuel.**
    Si cela ne devait pas fonctionner, redémarrez l'outil avec la commande :

    .. code-block:: console

        HOST:~$ sudo service NetworkManager restart

Configuration et utilisation de U-Boot
======================================

Dans cette section on verra comment configurer le système pour que le dispositif
de boot utilise TFTP et NFS pour récupérer le noyau et le filesystem depuis
l'ordinateur.

Au démarrage, vous pouvez appuyer sur une touche (via :code:`picocom`) pour arrêter le démarrage automatique
de U-Boot et jouer avec ses paramètres.
À l'écran vous devriez avoir un message du type :

.. code-block:: bash

   U-Boot 2013.01.01 (Apr 25 2014 - 04:59:35)

   CPU   : Altera SOCFPGA Platform
   BOARD : Altera SOCFPGA Cyclone V Board
   I2C:   ready
   DRAM:  1 GiB
   MMC:   ALTERA DWMMC: 0
   In:    serial
   Out:   serial
   Err:   serial
   Net:   mii0
   Hit any key to stop autoboot:  0
   Configuring PHY skew timing for Micrel ksz9021
   Waiting for PHY auto negotiation to complete.. done
   ENET Speed is 1000 Mbps - FULL duplex connection
   Using mii0 device
   TFTP from server 192.168.0.1; our IP address is 192.168.0.2
   Filename 'uImage'.
   Load address: 0x7fc0
   Loading: T T T T T T T T T T
   Retry count exceeded; starting again
   Using mii0 device
   TFTP from server 192.168.0.1; our IP address is 192.168.0.2
   Filename 'uImage'.
   Load address: 0x7fc0
   Loading: T T
   U-Boot SPL 2013.01.01 (Jan 28 2015 - 14:28:04)
   BOARD : Terasic DE1_SoC Board
   ################################################################################
   ##################################=                                           =#
   ########=-=#######################=                                     --    =#
   ######-   =#######################=                        ####-    =######   =#
   ######-   ###############- -=##=##=                        ####-   ########   =#
   ####=       =##-     -###       ##=   -#######    ######   ####-  #####-  -   =#
   ####        ##   ###   ##      -##=   -=--=###=  ####==#   ####-  ####        =#
   ######-   ###-  ##-  -###    =####=   =====###=  ####-     ####   ####        =#
   ######-   ###      =#####    #####=  ####-=###=    -=###-  ####-  #####       =#
   ######-    -#=     -  =##    #####=  #### -###=  #=-=####  ####-   ########-  ##
   #######    =##=        ##    #####=  -########=  #######   ####-    =######   =#
   #########==######====####==#######=     -         ---      --          ----   =#
   ##################################=                                           =#
   ################################################################################
   BOARD : Terasic  DE1_SoC Board
   CLOCK: EOSC1 clock 25000 KHz
   CLOCK: EOSC2 clock 25000 KHz
   CLOCK: F2S_SDR_REF clock 0 KHz
   CLOCK: F2S_PER_REF clock 0 KHz
   CLOCK: MPU clock 800 MHz
   CLOCK: DDR clock 400 MHz
   CLOCK: UART clock 100000 KHz
   CLOCK: MMC clock 50000 KHz
   CLOCK: QSPI clock 400000 KHz
   INFO : Watchdog enabled
   SDRAM: Initializing MMR registers
   SDRAM: Calibrating PHY
   SEQ.C: Preparing to start memory calibration
   SEQ.C: CALIBRATION PASSED
   SDRAM: 1024 MiB
   ALTERA DWMMC: 0


   U-Boot 2013.01.01 (Apr 25 2014 - 04:59:35)

   CPU   : Altera SOCFPGA Platform
   BOARD : Altera SOCFPGA Cyclone V Board
   I2C:   ready
   DRAM:  1 GiB
   MMC:   ALTERA DWMMC: 0
   In:    serial
   Out:   serial
   Err:   serial
   Net:   mii0
   Hit any key to stop autoboot:

.. note::

        Si vous l'avez raté, redémarrez simplement la carte avec la commande :console:`root@de1soclinux:~# reboot`.

.. warning::

    Pour le redémarrage, il y a un bouton *WARM_RST* sur la carte (à droite des gros boutons KEY 3-0), veuillez l'utiliser à la place
    du bouton *POWER ON/OFF*, votre carte sera bien plus heureuse !

Vous pouvez afficher la valeur actuelle des paramètres passés à U-Boot à l'aide de la commande :

.. code-block:: console

    SOCFPGA_CYCLONE5:~# printenv

U-Boot est un programme relativement complexe, mettant à la disposition de
l'utilisateur une console interactive avec support des variables d'environnement
et un jeu de commandes élaboré.
De plus, U-Boot fournit un support matériel basique pour certains périphériques,
comme les cartes réseau LAN et les périphériques flash, les cartes SD ou
la flash embarquée.
La commande :bash:`help` permet de visualiser les commandes disponibles.
Pour un manuel complet, se référer à `la documentation officielle <https://docs.u-boot.org/en/latest/>`__.

En donnant la commande :bash:`boot`, le système va continuer la procédure de démarrage
établie par défaut.

Vous pouvez le modifier pour qu'il fasse booter la carte depuis le noyau Linux
stocké sur votre PC (dans le répertoire :file:`/home/reds/tftpboot`)
en ajoutant les lignes qui suivent au prompt de U-Boot.

.. code-block:: console

    SOCFPGA_CYCLONE5:~# setenv ethaddr "12:34:56:78:90:12"
    SOCFPGA_CYCLONE5:~# setenv serverip "192.168.0.1"
    SOCFPGA_CYCLONE5:~# setenv ipaddr "192.168.0.2"
    SOCFPGA_CYCLONE5:~# setenv gatewayip "192.168.0.1"
    SOCFPGA_CYCLONE5:~# setenv bootdelay "2"
    SOCFPGA_CYCLONE5:~# setenv loadaddr "0xF000"
    SOCFPGA_CYCLONE5:~# setenv bootcmd "mmc rescan; tftpboot ${loadaddr} zImage; tftpboot ${fdtaddr} ${fdtimage}; run fpgaload; run bridge_enable_handoff; run mmcboot"
    SOCFPGA_CYCLONE5:~# setenv mmcboot "setenv bootargs console=ttyS0,115200 root=${mmcroot} rw rootwait ip=${ipaddr}:${serverip}:${serverip}:255.255.255.0:de1soclinux:eth0:on; bootz ${loadaddr} - ${fdtaddr}"
    SOCFPGA_CYCLONE5:~# saveenv

.. note::

    Pourriez-vous expliquer les lignes ci-dessus ?

    Le noyau Linux accepte une longue liste de paramètres, vous pouvez avoir
    plus d'informations en lisant le fichier :file:`Documentation/admin-guide/kernel-parameters.txt`
    dans les sources du noyau.

.. note::

    Est-ce que :bash:`${loadaddr} - ${fdtaddr}` est une soustraction de deux adresses dans la mémoire ?

.. note::

    La configuration de U-Boot sauvegardée dans la mémoire de la carte est
    assez complexe. Explorez les différentes valeurs et essayez
    d'effectuer le boot de la carte en changeant (de façon temporaire!! --- c.-à-d., sans utiliser la commande *saveenv*)
    l'adresse du PC, le répertoire où le noyau est stocké, ...


Maintenant, en démarrant la DE1-SoC, vous devriez pouvoir faire le boot du système en utilisant le
noyau qui vous a été fourni (celui dans :file:`/home/reds/tftpboot`). Vous devriez donc avoir un
output qui se termine par

.. code-block:: bash

    [    6.121032] IP-Config: Complete:
    [    6.124256]      device=eth0, hwaddr=12:34:56:78:90:12, ipaddr=192.168.0.123, mask=255.255.255.0, gw=192.168.0.1
    [    6.134418]      host=de1soclinux, domain=, nis-domain=(none)
    [    6.140145]      bootserver=192.168.0.1, rootserver=192.168.0.1, rootpath=
    [    6.140630] dw-apb-uart ffc02000.serial0: forbid DMA for kernel console
    [    6.169671] EXT4-fs (mmcblk0p2): mounted filesystem with ordered data mode. Quota mode: disabled.
    [    6.178615] VFS: Mounted root (ext4 filesystem) on device 179:2.
    [    6.195165] devtmpfs: mounted
    [    6.200981] Freeing unused kernel image (initmem) memory: 1024K
    [    6.207310] Run /sbin/init as init process
    [    9.001028] random: crng init done
    [    9.170095] init: hwclock main process (54) terminated with status 1
    [    9.184453] init: ureadahead main process (55) terminated with status 5

    Last login: Thu Jan  1 00:00:16 UTC 1970 on tty1
    root@de1soclinux:~# uname -ar
    Linux de1soclinux 6.1.55+ #4 SMP Wed Feb 21 11:30:34 CET 2024 armv7l armv7l armv7l GNU/Linux
    root@de1soclinux:~#


Vérification des toolchains
===========================

Pendant les laboratoires, la toolchain **arm-linux-gnueabifh** (version *6.4.1*)
sera utilisée pour la compilation d'exécutables Linux.

.. note::

    Pourriez-vous expliquer la signification des différents mots composant le nom de la toolchain?
    Pourquoi on a spécifié un numéro de version ?
    Qu'est-ce qui se passe si l'on prend tout simplement la toute dernière version de la toolchain?

Vérifiez la version de la toolchain par défaut :

.. code-block:: console

    HOST:~$ arm-linux-gnueabihf-gcc --version

S'il s'agit d'une version différente, vérifiez que la version *6.4.1* existe bien :

.. code-block:: console

    HOST:~$ arm-linux-gnueabihf-gcc-6.4.1 --version

Auquel cas, ajouter :bash:`-6.4.1` lors des appels suivants à :bash:`arm-linux-gnueabihf-gcc`.
Pour utiliser les autres outils fournis avec le compilateur (comme :bash:`objdump`), il faut mettre le chemin complet vers les exécutables qui se trouvent dans le dossier :file:`/opt/toolchains/arm-linux-gnueabihf_6.4.1/bin/` sur la VM.

Finalement, vérifiez que la toolchain fonctionne correctement.
Pour cela vous pouvez utiliser la commande suivante:

.. code-block:: console

    HOST:~$ echo "int main(){}" | arm-linux-gnueabihf-gcc -x c - -o /dev/stdout | file -

.. note::

    Pourriez-vous expliquer ce que la ligne ci-dessus fait?

Si tout marche bien, vous devriez avoir une réponse du type:

.. code-block:: bash

    /dev/stdin: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 2.6.32, BuildID[sha1]=b7a0db97fce5c152f9ae99800a40817aba3cfa1d, with debug_info, not stripped debug_info, not stripped


.. note::

    Savez-vous interpréter ces réponses ?

.. note::

    Que signifie *"not stripped"* ?

Compilation sur la cible (native)
=================================

Vos applications peuvent **parfois** être compilées directement sur la cible embarquée avec les mêmes commandes que vous
utilisez sur votre PC de développement --- pourvu que le compilateur et les
bibliothèques nécessaires soient installées et que le
processeur soit assez puissant, sinon vous allez attendre des siècles...
Avec votre éditeur préféré (soit avec **emacs** ou **vim** dans le terminal directement sur la cible), créez un
fichier :file:`hello_native.c` avec le contenu qui suit :

.. literalinclude:: hello_native.c
   :language: c

Ensuite, compilez-le avec la commande

.. code-block:: console

    root@de1soclinux:~# gcc hello_native.c -o hello_native

et testez-le avec

.. code-block:: console

    root@de1soclinux:~# ./hello_native

.. note::

    Quelle version de GCC est installée sur la carte ?

Compilation croisée sous DE1-SoC
================================

La compilation croisée standard nécessite quelques précautions supplémentaires
par rapport à une compilation native :

* La toolchain doit être compatible et paramétrée pour la cible choisie.
  Pour garantir une compatibilité entre le code généré par la toolchain et le
  système auquel il est destiné, il faut dans tous les cas que le compilateur
  utilisé pour compiler l'OS présent sur la cible et celui utilisé pour compiler
  l'application aient la même interface binaire (ABI).
* Le chemin des bibliothèques et des includes spécifiques à la carte cible
  doivent être précisés. Les bibliothèques présentes dans la machine hôte ayant
  été compilées pour une architecture différente, elles ne sont pas exécutables
  sur la carte cible.
* Les versions des bibliothèques et includes utilisés doivent être celles du
  système cible.
  Dans le cas de la compilation d'un driver Linux, les sources du noyau doivent
  être disponibles et leur version doit également correspondre.

Pour répondre au premier point de la problématique précédente, nous utilisons
ici la même toolchain qui a été utilisée pour générer la distribution Linux
cible, c'est-à-dire **arm-linux-gnueabihf** --- **gnueabihf** étant une ABI
normalisée pour les systèmes ARM possédant une unité de calcul hardware pour les
flottants ("hard float") --- et avec un numéro de version très proche.

Il est simple de cross-compiler notre première application, un "Hello world" classique.

.. literalinclude:: hello.c
   :language: c

Le fichier :file:`stdio.h` inclus dans le code est celui de la libc spécifique à
la toolchain installée. Pas besoin, pour les headers appartenant à des
bibliothèques fournies avec la toolchain (*libc*, *libm*, *libcrypt*,
*libpthread* et quelques autres) de préciser un dossier d'inclusion spécifique.
L'invocation du compilateur s'effectue donc de la même manière que l'invocation
d'un compilateur natif :

.. code-block:: console

   HOST:~$ arm-linux-gnueabihf-gcc hello.c -o hello
   HOST:~$ ls -al hello
   -rwxrwxr-x 1 rob rob 10228 Sep  1 14:56 hello
   HOST:~$ file hello
   hello: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 2.6.32, BuildID[sha1]=e295f4dc983156976c528d29073f98cade530ec8, with debug_info, not stripped

.. note::

    Qu'est-ce qui se passerait si vous essayiez d'exécuter ce fichier sur votre PC ? Pourquoi ?

Le fichier résultant est un exécutable linké dynamiquement.
Il est également possible de procéder à une compilation statique, si les
bibliothèques utilisées ne sont pas disponibles sur la cible (ce qui n'est
évidemment pas le cas ici, vu que seule la *libc* est utilisée).

.. code-block:: console

   HOST:~$ arm-linux-gnueabihf-gcc hello.c -o hello_static -static
   HOST:~$ ls -al hello_static
   -rwxrwxr-x 1 rob rob 3972176 Sep  1 14:57 hello_static
   HOST:~$ file hello_static
   hello_static: ELF 32-bit LSB executable, ARM, EABI5 version 1 (GNU/Linux), statically linked, for GNU/Linux 2.6.32, BuildID[sha1]=d2b9e57feaa2d0c24525a548f0cfdb7bf3d2f45a, with debug_info, not stripped

Comme la taille de l'exécutable s'en retrouve dramatiquement accrue, la
compilation statique est réservée aux cas spéciaux.

Pour tester, mettez l'exécutable dans le dossier :file:`/export/drv/` de la machine hôte et depuis la cible exécutez :

.. code-block:: console

    root@de1soclinux:~# ~/drv/hello

Si l'erreur suivante apparaît, la mauvaise toolchain a été utilisée lors de la compilation, assurez-vous de bien utiliser la version *6.4.1*.

.. code-block:: bash

    ~/drv/hello: /lib/arm-linux-gnueabihf/libc.so.6: version 'GLIBC_2.34' not found (required by ~/drv/hello)

=====================================================
Accès aux périphériques du CycloneV-SoC depuis U-Boot
=====================================================

La plupart des périphériques du CycloneV-SoC ne sont pas accessibles au core ARM
qu'à travers la FPGA:

.. figure:: images/schema_de1soc.png
   :width: 600px
   :align: center

   Diagramme du DE1-SoC computer (Figure 1 dans *"DE1-SoC Computer System with ARM Cortex-A9"*).

En particulier, grâce au bitstream chargé au démarrage, les différents
périphériques sont accessibles en *memory-map* en utilisant des accès mémoire.

La liste des adresses mémoire est disponible dans le chapitre 7 du document
*"DE1-SoC Computer System with ARM Cortex-A9"* (page 29).
La section 2.9 (page 7) de ce même document donne plus d'information sur les registres
des périphériques qui seront utilisés (LEDs, switchs, ...).

.. warning::

    Avant d'essayer les commandes ci-dessous, assurez-vous que le bon bitstream soit
    chargé en faisant un boot complète du système.
    A ce moment-là, dès que vous aurez le prompt Linux, tapez une commande

    .. code-block:: console

        root@de1soclinux:~# reboot

    et arrêtez le système au prompt U-Boot au redémarrage.

U-Boot possède un ensemble de commandes dédiées à la manipulation de données :

* la commande *memory display*

  .. code-block:: bash

        md.{b|w|l} addr length

  permet d'afficher
  :bash:`length` données depuis l'adresse :bash:`addr`, sous un certain format :

  * :bash:`md.b` affichant les données sous forme de mots de 8 bits
  * :bash:`md.w` affichant les données sous forme de mots de 16 bits
  * :bash:`md.l` affichant les données sous forme de mots de 32 bits

  Notez que si vous affichez 8 ou 16 bits d'un registre 32 bits,
  **vous allez récupérer les bytes de poids faible**,
  et non pas ceux de poids fort.

  .. note:: Est-ce que cela nous dit quelque chose sur l'endianness du MCU?

* la commande associée *memory write*

  .. code-block:: bash

      mw.{b|w|l} addr value (n)

  permet de
  modifier la valeur de :bash:`n` données à partir de :bash:`addr`, avec une
  taille d'élément de donnée de 1, 2 ou 4 bytes.

Exemple
=======

Les commandes ci-dessous affichent le contenu de la mémoire à l'adresse de
chargement de U-Boot sous chacun des trois formats disponibles.

.. code-block:: console

    SOCFPGA_CYCLONE5:~# md.b 0x80008000 0x1
    80008000: 46    F
    SOCFPGA_CYCLONE5:~# md.w 0x80008000 0x1
    80008000: 4c46    FL
    SOCFPGA_CYCLONE5:~# md.l 0x80008000 0x1
    80008000: eb004c46    FL..
    SOCFPGA_CYCLONE5:~# md.b 0x80008000 0x4
    80008000: 46 4c 00 eb    FL..
    SOCFPGA_CYCLONE5:~# md.w 0x80008000 0x4
    80008000: 4c46 eb00 9000 e10f    FL......
    SOCFPGA_CYCLONE5:~# md.l 0x80008000 0x4
    80008000: eb004c46 e10f9000 e229901a e319001f    FL........).....

.. admonition:: **Exercice 1**

    * Expliquez les différences entre les lignes ci-dessus.
    * Utilisez la commande :bash:`md` pour lire la valeur binaire écrite avec les
      switches et écrivez-la sur les LEDs.
    * Qu'est-ce qui se passe si vous essayez d'accéder à une adresse qui n'est pas alignée (par exemple *0x01010101*) et pourquoi ?

.. admonition:: **Exercice 2**

    Écrivez un script (à l'aide d'une variable d'environnement) U-Boot qui va alterner, chaque seconde, entre afficher 🯰🯰🯰🯰🯰🯰 et 🯵🯵🯵🯵🯵🯵 sur les affichages à 7-segments.

    Vous trouverez plus d'information sur le shell d'U-Boot via les liens suivants :

        - https://docs.u-boot.org/en/latest/usage/cmdline.html
        - https://mediawiki.compulab.com/w/index.php?title=U-Boot%3A_Quick_reference

===============================================
Accès aux périphériques du DE1-SoC depuis Linux
===============================================

Le même type d'accès aux ressources vu dans la section précédente est possible
depuis Linux.
En particulier, il est possible d'ouvrir un fichier spécial (:file:`/dev/mem`)
et, après l'avoir mappé en mémoire avec la fonction :c:`mmap()`, taper des
valeurs directement dans la mémoire.

Pour plus d'information sur :c:`mmap`, regardez dans sa documentation `man <https://www.man7.org/linux/man-pages/man2/mmap.2.html>`__
ainsi que cet `exemple <https://bakhi.github.io/devmem/>`__.
Attention cette exemple utilise :c:`PAGE_SIZE` comme adresse qui est défini à la compilation, or tous les systèmes n'ont pas la même taille de page. Regardez dans la description :c:`man` de :c:`mmap` pour trouver une alternative !

.. warning::

    En faisant ça, vous avez accès direct à **toute** la mémoire...
    Sans aucune supervision ni limitation !

Cette approche est très utile pour tester rapidement un dispositif --- vous pouvez
en effet accéder à ses registres et lire/écrire des valeurs, sans devoir écrire
un driver.

.. note::

    Pouvez-vous identifier au moins deux gros problèmes de cette approche ?

.. admonition:: **Exercice 3**
   :name: lab1-ex3

    Écrivez un logiciel user-space en C qui utilise :file:`/dev/mem` pour accéder aux périphériques.
    Au démarrage, le logiciel commence par afficher ``A`` sur l’affichage HEX0.
    Ensuite, l’utilisateur peut contrôler la lettre affichée sur HEX0 de la façon suivante:

      - En appuyant sur KEY0, le caractère actuellement affiché est décrémenté
          * ``Z`` devient ``Y``, ... , ``B`` devient ``A``
          * Si ``A`` est affiché, on recommence à ``Z``
      - En appuyant sur KEY1, le caractère actuellement affiché est incrémenté
          * ``A`` devient ``B``, ... , ``Y`` devient ``Z``
          * Si ``Z`` est déjà affiché, on recommence à ``A``

    De plus, le code ASCII de la lettre affichée doit être
    représenté en binaire sur les LEDs.

      - Exemple: si ``A`` est affiché (soit 0x41 / 0b1000001 en ASCII), LED0 et LED6 doivent être allumée

    **Note**: vous pouvez trouver un exemple de police de caractère pour afficheurs
    7-segments sur cette page `Wikipedia <https://en.wikipedia.org/wiki/Seven-segment_display_character_representations>`__.
    Libre à vous de vous en inspirer.

    **Bonus 1**: éteignez les afficheurs 7-segments et les LEDs en partant
    (quand le programme est arrêté avec Ctrl+C) pour éviter de faire fondre la banquise
    inutilement.

.. note::

    Quel est le souci principal dans l'écriture de ce logiciel ?


=========================================
Travail à rendre et critères d'évaluation
=========================================

Dans le cadre de ce laboratoire, vous devez rendre les deux exercices relatifs à U-Boot et les exercices sous Linux.

.. include:: ../consigne_rendu.rst.inc
