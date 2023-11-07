# Supervisor

    Aplicatie tip Supervisor

    Capitol 1 - Introducere
1.1. Scopul proiectului 
    Proiectul presupune dezvoltarea unei aplicatii de management si monitorizare a proceselor lansate in executie sub Supervisor. Aplicatia trebuie sa fie un mecanism transparent de gestiune si personalizare a executiei diverselor programe, si totodata usor de utilizat, atat pentru utilizatorul local cat si pentru cel remote.

1.2. Lista definițiilor
    - fsup = program tip command-line-interface ce permite interactiunea utilizatorului cu backsup;
    - backsup = serviciu systemd cu rol in managementul proceselor;
    - POSIX = o familie de standarde specificate de Societatea de calculatoare IEEE pentru menținerea compatibilității dintre sisteme de operare.
    - client
    - server
    - Supervisor 

Capitolul 2
2.1. Descrierea produsului software 
    Arhitectura aplicatiei este de Client (Supervisor) - Server (Manager)
    Serverul (Manager) reprezinta un program ce ofera utilizatorului un mijloc de remote management al proceselor rulate sub aplicatia Supervisor. 
    Clientul (Supervisor) este aplicatia care ruleaza pe o statie si asigura executia programelor dupa criteriile fisierelor de configurare corespunzatoare. 
    Acesta este alcatuit din doua entitati distincte: un program ce ruleaza in backgroud (sub forma de serviciu systemd) - backsup si un program ce ruleaza in foreground si primeste comanda de la utilizator - fsup.
    Fisierele de configurare reprezinta o modalitate structurata de transmitere a specificatiilor utilizatorului pentru executia unui program. Un program poate avea mai multe fisiere de configurare.

2.2. Detaliera platformei HW/SW - descrierea elementelor Hardware și Software pentru care produsul software este dezvoltat (cu alte cuvinte, descrierea platformei pe care aplicația va putea fi rulată).
    * Aplicatia este Linux-compatible;
    * Aplicatia respecta standarul POSIX;
    * C Standard Library
    *  biblioteca pthread.h
    * biblioteca fcntl

2.3. Constrângeri - descrierea unor eventuale constrângeri pe care produsul software trebuie să le satisfacă (limitări privind datele introduse de utilizator, limitări datorită mediului în care aplicația va fi rulată etc.)
    Constrangeri ale fsup:
        * ruleaza cu permisiunile utilizatorului curent;

    Constrangeri ale backsup:
        * fisierul de configurare este corect;
        * nu poate modifica sau interveni asupra resurselor procesului dupa suprascrierea imaginii acestuia
        * trebuie sa ruleze cu permisiuni de root

    Constrangeri ale fisierelor de configurare:
        * sa contina path-ul catre executabil;
        * nume unic;
        * structura predefinita;
        * sunt redactate de catre utilizator;
        * sa fie create intr-un director prestabilit.

    Constrangeri hardware:
        * CPU cores pentru multithreading


Capitolul 3 - Detalierea cerințelor specifice
3.1. Cerințele funcționale
        * fsup<program> <args> , fsup urmand sa caute fisierul de configurare in directorul ./conf prestabilit. 
            * daca nu se gaseste, fsup trateaza o exceptie
            * altfel, se ruleaza programul cerut conform detaliilor din configurare
        * --args: argumentele programului ce ulmeaza a fi rulat
        *fsup <optiuni> <pid> (optional)
        * --optiuni: kill (daca procesul este in executie), ...

        * Managerul trebuie sa aibe acces la o lista cu clienti si procesele rulate de fiecare in parte, cat si la fisierele de conf
        * jurnalizarea operatiunilor de catre backsup in /var/log
        * managerul poate lua diverse decizii care trebuie executate de catre fiecare aplicatie client 
        * totodata, fiecare utilizator al unei statii poate lua decizii privitoate la procesele aflate in executie si, evident, pot crea si edita fisiere de configurare

3.2. Cerințele ne-funcționale
        * mecanism de tratare al erorilor
        * fsup trebuie sa preia numele transmis ca parametru si sa caute fisierul de configurare corespunzator, in caz contrar va transmite un mesaj de eroare
        * ulterior, fisierul de configurare va fi trimis printr-un mecanism IPC catre backsup, un named pipe
        * backsup va crea un proces copil cu ajutorul functiei fork
        * va parsa fisierul de configurare si va crea conditiile de rulare specificate in fisierul .conf dupa care va suprascrie imaginea procesului copil cu ajutorul functiei execlp cu imaginea programului executabil precizat in conf
        * comunicarea dintre aplicatia client si cea server se va realiza deasemenea via backsup prin avand ca mecanism de IPC socketi
        * va exista un thread dedicat ascultarii mesajelor de la Manager si un thread care va citi din pipe numele fisierelor de conf
        * backsup va fi configurat astfel incat sa ruleze in background de la bootare astfel orice program rulat sub "fsub" va fi gestionat de acceasi entitate a programului backsup
        * backsup este responsabil cu jurnalizarea actiunilor, atat proprii cat si cele primite remote de la server
        * Managerul la conectare va astepta sa primeasca detalii despre procesele rulate sub supervisor de clienti si va putea lua diferite decizii (uciderea, crearea, stoparea unui proces) sau chiar editarea unui fisier de conf
        * pentru a rula un program ce necesita permisiuni de root comanda fsup va trebui data cu permisiune de root deoarece backsup va crea procesul copil cu permisiunile, ownerul si group ownerul vor fi ale celui care ruleaza comanda fsup, exceptie facand cazuri in care se presizeaza altfel in fisierul de conf
        * totodata, fsup va putea interoga backsup in legatura cu procesele ce ruleaza sub supervisor la momentul curent si ii va putea transmite si comenzi prin intermediul optiunilor