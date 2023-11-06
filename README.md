# Supervisor

    Aplicatie tip Supervisor

    Capitol 1 - Introducere
1.1. Scopul proiectului - Explicație sumară a produsului software și ce anume va realiza.
    Proiectul presupune dezvoltarea unei aplicatii de management si monitorizare a proceselor lansate in executie sub Supervisor pe o statie.

1.2. Lista definițiilor - Un tabel în care vor fi incluse toate abrevierile folosite în cadrul DCS. 
    - fsup = program tip command-line-interface ce permite interactiunea utilizatorului cu backsup;
    - backsup = serviciu systemd cu rol in managementul proceselor;
    - 

Capitolul 2 - Descrierea generală a produsului software
2.1. Descrierea produsului software - descrierea funcționalităților produsului software și încadrarea acestora în cerințele unui mediu de lucru.
    Arhitectura aplicatiei este de Client (Supervisor) - Server (Manager)
    Serverul (Manager) reprezinta un program ce ofera utilizatorului un mijloc de remote management al proceselor rulate sub aplicatia Supervisor. 
    Clientul (Supervisor) este aplicatia care ruleaza pe o statie si asigura executia programelor dupa criteriile fisierelor de configurare corespunzatoare. 
    Acesta este alcatuit din doua entitati distincte: un program ce ruleaza in backgroud (sub forma de serviciu systemd) - backsup si un program ce ruleaza in foreground si primeste comanda de la utilizator - fsup.
    Fisierele de configurare reprezinta o modalitate structurata de transmitere a specificatiilor utilizatorului pentru executia unui program. Un program poate avea mai multe fisiere de configurare.

2.2. Detaliera platformei HW/SW - descrierea elementelor Hardware și Software pentru care produsul software este dezvoltat (cu alte cuvinte, descrierea platformei pe care aplicația va putea fi rulată).
    * Aplicatia este Linux-compatible;
    * Aplicatia respecta standarul POSIX;
    * C Standard Library

2.3. Constrângeri - descrierea unor eventuale constrângeri pe care produsul software trebuie să le satisfacă (limitări privind datele introduse de utilizator, limitări datorită mediului în care aplicația va fi rulată etc.)
    Constrangeri ale fsup:
        * ruleaza cu permisiunile utilizatorului curent;

    Constrangeri ale backsup:
        * fisierul de configurare este corect;

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
        * fsup <optiuni> <program> <args> , fsup urmand sa caute fisierul de configurare in directorul ./conf prestabilit. 
            * daca nu se gaseste, fsup trateaza o exceptie
            * altfel, se transmite fisierul .conf catre backsup care va urma criteriile din fisier
        * --optiuni: create=default, kill (daca procesul este in executie), ...
        * --args: argumentele programului
        * backsup comunica prin unix-sockets cu o Manager-ul
        * jurnalizarea operatiunilor de catre backsup in /var/log
        * managerul implementeaza un mecanism multithreading de interogare si receptie informatii despre procesele rulate de un back-sup
        * posibilitatea utilizatorului de a interactiona cu back-sup prin intermediul unei interfete web (prin serviciul apache2)
        * 

3.2. Cerințele ne-funcționale
        * mecanism de tratare al erorilor
        * multithreading
        * comunicare prin unix/https sockets
        * POSIX is everywhere 
        
                
        
        
        
        
    

