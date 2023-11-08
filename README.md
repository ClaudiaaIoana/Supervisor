SuperManager

Dodot Andrei & Dascalescu Claudia
C113E
Academia Tehnica Militara “Ferdinand I”

Cuprins

Capitolul 1 - Introducere	
1.1. Scopul proiectului	
1.2. Lista definițiilor	
Capitolul 2 - Descrierea generală a produsului software	
2.1. Descrierea produsului software
2.2. Detaliera platformei HW/SW	
2.3. Constrângerile
Capitolul 3 - Detalierea cerințelor specifice	
3.2. Cerințele ne-funcționale	


Aplicatie tip Supervisor
Capitolul 1 – Introducere

1.1. Scopul proiectului
Scopul proiectului constă în dezvoltarea unei aplicații de management și monitorizare a proceselor lansate în execuție sub Supervisor. Această aplicație are ca obiectiv principal furnizarea unui mecanism transparent pentru gestionarea și personalizarea proceselor într-un mediu Linux. De asemenea, trebuie să fie ușor de utilizat, atât pentru utilizatorii locali, cât și pentru cei remote.
1.2. Lista definițiilor
Pentru a asigura o înțelegere clară a termenilor utilizați în cadrul documentației, sunt definite următoarele concepte:
•	fsup: Programul tip command-line-interface care permite interacțiunea utilizatorului cu backsup.
•	backsup: Serviciul systemd cu rol în managementul proceselor, reprezentând componenta principală a aplicației Supervisor.
•	POSIX: Familie de standarde specificate de Societatea de calculatoare IEEE pentru menținerea compatibilității dintre sistemele de operare.
•	Client: Utilizatorul sau entitatea care utilizează aplicația Supervisor pentru a gestiona și monitoriza procese.
•	Server (Manager): Programul care oferă utilizatorilor un mijloc de gestionare și monitorizare remote a proceselor rulate sub aplicația Supervisor.
Capitolul 2 - Descrierea generală a produsului software

2.1. Descrierea produsului software
Aplicația Supervisor este concepută ca un sistem Client-Server și constă din două entități principale:
•	Serverul (Manager): Acesta reprezintă un program care oferă utilizatorilor un mijloc de gestionare și monitorizare remote a proceselor rulate sub aplicația Supervisor. Managerul trebuie să ofere funcționalități avansate pentru controlul și configurarea proceselor.
•	Clientul (Supervisor): Este aplicația care rulează pe o stație și asigură execuția programelor conform criteriilor specificate în fișierele de configurare corespunzătoare. Acesta este compus din două componente principale:
•	fsup: Este un program care rulează în modul foreground și permite utilizatorilor să trimită comenzi și să interacționeze direct cu Supervisor.
•	Backsup: Este un program ce ruleaza in backgroud (sub forma de serviciu systemd).
•	Fisierele de configurare: Aceste fișiere reprezintă modalitatea structurată de transmitere a specificațiilor utilizatorului pentru executarea unui program. Fiecare program poate avea mai multe fișiere de configurare, iar acestea sunt create și gestionate de către utilizator.


2.2. Detaliera platformei HW/SW
Aplicația Supervisor necesită un mediu specific pentru a funcționa corect, care include:
•	Sistem de operare Linux-compatible: Aplicația este dezvoltată pentru a funcționa pe sistemele de operare compatibile cu Linux, asigurându-se că toate funcționalitățile sunt suportate corespunzător.
•	Respectarea standardului POSIX: Aplicația se bazează pe standardele POSIX pentru a asigura compatibilitatea între sistemele de operare și funcționalitățile implementate.
•	C Standard Library: Dezvoltarea aplicației se realizează utilizând biblioteca standard C pentru a asigura un cod portabil și eficient.
•	Biblioteca pthread.h: Această bibliotecă este utilizată pentru gestionarea firelor de execuție (thread-urilor) și pentru a permite multitasking în aplicație.
•	Biblioteca fcntl: Este utilizată pentru manipularea fișierelor și descriptorilor de fișiere, având un rol crucial în gestionarea fișierelor de configurare și a comunicării între entitățile Supervisor.


2.3. Constrângerile
Pentru ca aplicația Supervisor să funcționeze corespunzător, sunt definite o serie de constrângeri, incluzând:
Constrângeri ale fsup:
•	Rulează cu permisiunile utilizatorului curent, ceea ce înseamnă că comenzi precum crearea sau terminarea proceselor sunt limitate de permisiunile utilizatorului care a inițiat comanda.
Constrângeri ale backsup:
•	Fisierul de configurare trebuie să fie corect și să respecte structura specificată pentru a asigura o execuție corectă a proceselor.
•	backsup nu poate modifica sau interveni asupra resurselor procesului după suprascrierea imaginii acestuia, asigurând astfel stabilitatea procesului. De asemenea, **backsup** trebuie să ruleze cu permisiuni de root pentru a putea gestiona și controla procesele într-un mod eficient și pentru a asigura funcționalitatea corespunzătoare a aplicației Supervisor.

Constrângeri ale fișierelor de configurare:
•	- Fiecare fișier de configurare trebuie să conțină calea către executabil, un nume unic, o structură predefinită și să fie redactat de către utilizator. Aceste fișiere sunt esențiale pentru definirea comportamentului proceselor.

Constrângeri hardware:
•	- Aplicația Supervisor poate avea cerințe de resurse hardware, în special în ceea ce privește numărul de CPU cores, pentru a susține multitasking și multithreading într-un mod eficient.

Capitolul 3 - Detalierea cerințelor specifice


 3.1. Cerințele funcționale
Aplicația Supervisor are următoarele cerințe funcționale:

•	Comunicarea cu utilizatorul:
▫	fsup <program> <args>: Comanda fsup permite utilizatorilor să lanseze procese, unde "program" reprezintă calea către executabil, iar "args" reprezintă argumentele programului. Dacă nu se găsește un fișier de configurare specific pentru programul respectiv, fsup trebuie să trateze o excepție corespunzătoare.
▫	fsup <optiuni> <pid> (opțional): Cu ajutorul acestei comenzi, utilizatorii pot efectua operații specifice asupra unui proces folosind opțiuni precum "kill" pentru a termina procesul.

•	Managerul (Server): Managerul trebuie să aibă acces la o listă cu clienți și procesele rulate de fiecare în parte, precum și la fișierele de configurare. Acesta poate lua diverse decizii, inclusiv crearea, terminarea sau modificarea stării proceselor, cat si modificarea fisierelor de conf.

•	Jurnalizarea operațiunilor: Backsup are responsabilitatea de a jurnaliza toate acțiunile efectuate, atât cele proprii, cât și cele primite de la Manager. Această funcționalitate este esențială pentru urmărirea și analiza acțiunilor din trecut.


 3.2. Cerințele ne-funcționale
Aplicația Supervisor implică următoarele cerințe ne-funcționale:

•	Mecanism de tratare al erorilor: Aplicația trebuie să ofere un mecanism solid de tratare a erorilor pentru a gestiona situațiile neprevăzute și pentru a oferi feedback corespunzător utilizatorilor.

•	Gestionarea comenzilor utilizatorului local: fsup trebuie să preia numele programului transmis ca parametru și să caute fișierul de configurare corespunzător. În cazul în care fișierul nu este găsit, fsup trebuie să trimită un mesaj de eroare corespunzător.

•	Comunicarea între fsup și backsup: Comunicarea între client (fsup) și server (backsup) se realizează prin intermediul unor socket-uri, asigurând transmiterea datelor și comenzilor între entități.

•	Procesarea fișierelor de configurare: backsup trebuie să poată procesa fișierele de configurare și să creeze condițiile specificate pentru rularea programelor, respectând detaliile din fișierele .conf. Acest lucru se realizează prin suprascrierea imaginii procesului cu ajutorul funcției execlp.

•	Comunicarea intre backsup si server: printr-un socket de comunicatie vor comunica cele doua instante, thread-ul responsabil cu ascultarea de mesaje pe socket-ul respectiv al backsup-ului va capta mesaje de forma "<comanda> <pid>" sau "<nume .conf> <dimensiune>", acesta va analiza mesajul captat si respectiv va apela functii pentru prelucrarea starii procesului cerul sau va capta in continuare bucatile de fisier si le va asambla pentru a actualiza local fisierul de configurare precizat.

•	Gestionarea thread-urilor: Backsup trebuie să utilizeze thread-uri pentru gestionarea mesajelor primite de la Manager și pentru citirea numelor fișierelor de configurare dintr-un named pipe.

•	Rulare în background: Backsup trebuie să fie configurat pentru a rula în mod background încă de la bootarea sistemului, astfel încât toate programele rulate sub "fsup" să fie gestionate de aceeași instanță a backsup.

•	Securitate: Pentru programele care necesită permisiuni de root pentru rulare, comanda "fsup" trebuie să fie executată cu permisiuni de root. Cu toate acestea, backsup trebuie să aplice strict politicile de securitate pentru a preveni accesul neautorizat.

•	Interacțiunea cu utilizatorul local: Aplicația trebuie să ofere posibilitatea utilizatorilor de a interacționa cu procesele aflate în execuție și să efectueze acțiuni precum crearea, terminarea sau editarea fișierelor de configurare, oferind astfel o experiență de utilizare intuitivă.

•	Interactiunea cu utilizatorul remote: Managerul la conectare va astepta sa primeasca detalii despre procesele rulate sub supervisor de clienti si va putea lua diferite decizii (uciderea, crearea, stoparea unui proces) sau chiar editarea unui fisier de conf.

•	Jurnalizarea: programul backsup este responsabil cu jurnalizarea acctivitatii ce priveste statia pe care ruleaza, acesta va scrie mesaje intr-un fisier de tip .log atat referitoare la actiunile utilizatorului local cat si ale celui remote, care au efect asupra proceselor locale.


4. Arhitectura
![image](https://github.com/ClaudiaaIoana/Supervisor/assets/91790390/a6596b4e-baef-4a66-bbb5-b77b1d2c7abd)

