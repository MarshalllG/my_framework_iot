# framework_iot_tesi

DOCUMENTAZIONE

Il framework sviluppato è previsto per una rete con un nodo master e molteplici nodi slave.

--> Nodo Master
Il file master.exe è l'eseguibile per il nodo master e pone in esecuzione due thread su cui si mettono in attesa, rispettivamente, un server TCP parallelo e un server UDP in modalità non-blocking.
La comunicazione UDP prevede una periodica azione di polling, effettuata dal master per indagare gli stati dei dispositivi slave.
La comunicazione TCP permette al nodo master di rispondere alle richieste ricevute da uno slave.

- master.exe: contiene server TCP e server UDP
- masterlib.c: file di libreria contenente le funzioni necessarie alle comunicazioni e alcune funzioni di utility
- masterlib.h: header del file di libreria
- ip_list.txt: archivio a disposizione del nodo master, contenente gli ip già noti

--> Nodo Slave
La comunicazione TCP, per un nodo slave, è necessaria ad accedere alle informazioni registrate dal master sugli stati.
La comunicazione UDP, per un nodo slave, è necessaria a rendere note periodicamente al master le proprie informazioni di stato.

- slave_udpclnt.exe: programma che implementa il client UDP
- slave_tcpclnt.exe: programma che implementa il client TCP
