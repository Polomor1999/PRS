# PRS (Programmation Réseaux et Systèmes)

## Voici le magnifique projet de PRS du célèbre groupe : les CodeWarriors (aka Paul Mortier et Claire Feral). 

### CONTEXTE: 
Nous sommes 2 étudiants en 4ème année de l'école d'ingénieur INSA Lyon. 

### DATE: 
16 Décembre 2022

### STATUT: 
Fini 

### OBJECTIF:
- Implémenter le contrôle de congestion de TCP sur UDP 
- Avoir 3 serveurs pour 3 scénarios différents
 - Scénario 1: Client n°1 qui perd rarement des trames 
 - Scénario 2: Client n° 2 qui perd quasiment toutes les trames
 - Scénario 3: Connexion simultanée de plusieurs client n° 1
                        
### ORGANISATION:
- *bin*/ --contient 3 fichiers .c appelés serveurX-CodeWarriors, X étant le numéro du scénario
- *src*/ -- contient tous les fichiers utilisés pour le projet et le Makefile qui génère les 3 fichier .c dans bin
- *pres*/ --contient la présentation du projet, en pdf

### UTILISATION:

Pour run les fichiers .c vous devez tapper dans un terminal:
```
./CodeWarriorsX <numéro du port>
//X = <numéro du scénario>
```

Pour connecter un client vous devez tapper dans un autre terminal:
```
./clientX <@ IP> <numéro de port du serveur> <nom du fichier>
//X = <numéro du client>
```

Pour compiler un serveur.c vous devez faire dans un terminal:
```
make
./serveurX <numéro du port>
```
