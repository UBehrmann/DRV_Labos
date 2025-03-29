<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 2 : User-space Drivers <!-- omit in toc -->

# Auteur <!-- omit in toc -->

- Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Exercice 2](#exercice-2)
- [Exercice 5](#exercice-5)

___

# Exercice 2

**Pourquoi une région de 4096 bytes et non pas 5000 ou 10000 ? Et pourquoi on a spécifié cette adresse ?**

Car tous les chiffres qu'on utilise en programmation sont en base 2, car on fait des opérations binaires. 4096 est une puissance de 2, 2^12 = 4096. 

L'adresse spécifiée est pour que la mémoire soit mappée à une adresse connue. Dans notre cas, c'est l'adresse 0xff200000 qui est la base des adresses pour la UIO.

**Quelles sont les différences dans le comportement de 'mmap()' susmentionnées ?**

Lorsqu'on utilise mmap() dans sur un périphérique UIO, on a un accès directe à la mémoire physique. Cela signifie que l'on peut lire et écrire directement dans la mémoire du périphérique sans passer par le noyau. Il n'y a pas de copie ou d'abstraction supplémentaire à faire.

**Effectuez des recherches avec Google/StackOverflow/... et résumez par écrit les avantages et les inconvénients des drivers user-space par rapport aux drivers kernel-space.**

Avantages user-space:

- Développement et débogage plus faciles 
- Stabilité accrue du système
- Sécurité renforcée

Inconvénients user-space:

- Performance réduites
- Accès limité au matériel

Sources :

- [linkedIn](https://www.linkedin.com/advice/0/what-performance-security-implications-user-mode?utm_source=chatgpt.com)
- [geeks for geeks](https://www.geeksforgeeks.org/difference-between-user-mode-and-kernel-mode/?utm_source=chatgpt.com)
- [stackoverflow]https://stackoverflow.com/questions/37081874/embedded-linux-kernel-drivers-vs-user-space-drivers

___

# Exercice 5

**Il y a au moins 3 façons pour attendre une interruption au moyen de /dev/uio0 (voir dans le tutoriel). Lesquelles ?**

- read()
- poll()
- select()

</div>