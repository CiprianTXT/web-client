Popescu Ciprian-Mihail, 322CD

Tema 4 PCom - Client Web. Comunicatie cu REST API.

    In cadrul acestei teme am avut de implementat un client web in C care
poate comunica cu un server. Pornind de la scheletul laboratorului 9
(https://pcom.pages.upb.ro/labs/lab9/http.html), am implementat in fisierul
client.c tema propriu-zisa: parser-ul de comenzi primite de la tastatura, cat
si comenzile in sine (register, login, enter_library, get_books, get_book,
add_book, delete_book, logout, exit).

    De-a lungul implementarii, am folosit biblioteca parson care imi permite cu
usurinta sa creez payload-urile JSON, cat si sa parsez un JSON primit de la
server. Functiile folosite sunt:

- json_value_init_object (aloca memorie pentru valoarea JSON)

- json_value_get_object (returneaza adresa la care se afla obiectul alocat)

- json_value_get_array (returneaza adresa la care se afla vectorul alocat)

- json_object_set_string (creaza in JSON o structura de tip "camp:valoare")

- json_serialize_to_string_pretty (converteste si formateaza JSON-ul final
intr-un sir de caractere)

- json_free_serialized_string si json_value_free (elibereaza memoria)

- json_parse_string (parseaza un sir de caractere si creaza un JSON)

- json_object_get_string_len (returneaza lungimea valorii campului dat ca
parametru)

- json_array_get_count (returneaza lungimea vectorului din JSON)

- json_array_get_object (returneaza adresa obiectului de la pozitia data ca
parametru)

    Pentru functionarea corecta a temei, a fost necesara modificarea fisierelor
requests.c si requests.h astfel:

- adaugarea functiei compute_delete_request() folosita la comanda de delete_book

- modificarea functiei compute_get_request() pentru a permite adaugarea
token-ului JWT in headerul cererii oricand este necesar

- modificarea functiei compute_post_request() pentru a permite gestionarea
payload-urilor de tip "application/json" oricand este necesar


Comenzi implementate:
> register si login:
    Aceste 2 comenzi le-am implementat in functia register_or_login(),
deoarece functioneaza intr-un mod similar, singurele diferente sunt:

- ruta de acces pentru register este "/api/v1/tema/auth/register", pe cand
ruta de acces pentru login este "/api/v1/tema/auth/login", deci difera
continutul POST request-ului

- gestionarea raspunsului primit de la server. Ambele comenzi afiseaza mesaje
sugestive in functie de response_code, insa comanda login va aloca si stoca in
memorie cookie-ul de sesiune daca primeste de la server codul HTTP 200 - OK.

!!! Daca este deja autentificat un user, cele 2 comenzi nu se vor executa !!!

> enter_library
    Aceasta comanda va apela functia enter_library() care va primi de la server
token-ul JWT doar daca in request este atasat cookie-ul de sesiune primit de la
comanda de login.

!!! Daca este deja permis accesul la biblioteca, comanda nu se va executa !!!

> get_books
    Aceasta comanda va apela functia get_books() care va primi de la server un
JSON cu toate cartile din biblioteca doar daca in header-ul request-ului este
atasat token-ul JWT. Pentru lizibilitate am parsat JSON-ul cu ajutorul
functiilor din biblioteca parson si am afisat pe ecran fiecare carte existenta
in biblioteca.

> get_book si delete_book
    Aceste 2 comenzi le-am implementat in functia get_or_delete_request(),
deoarece se comporta similar, insa diferentele sunt:

- get_book foloseste GET request, in timp ce delete_book foloseste DELETE
request

- cele 2 comenzi gestioneaza diferit raspunsul HTTP 200 - OK primit de la
server. get_book va primi de la server un JSON (parsat si afisat pe ecran) care
contine detalii despre cartea cautata dupa id, pe cand delete_book afiseaza pe
ecran un mesaj de succes.

!!! Daca pentru id nu se introduce un numar valid, comanda nu se va executa !!!

> add_book
    Acesta comanda apeleaza functia add_book() care va trimite catre server un
POST request cu JSON-ul generat in functie de ce a introdus utilizatorul. In
functie de raspunsul primit de la server se va afisa pe ecran fie un mesaj de
succes, fie un mesaj de eroare.

!!! Daca cel putin unul dintre campuri nu este completat sau in campul  !!!
!!! page_count nu se introduce un numar valid, comanda nu se va executa !!!

> logout
    Acesta comanda apeleaza functia logout() care va dezaloca memoria pentru
variabilele *token, *cookie[0] si **cookie, in functie de raspunsul primit de
la server.

> exit
    Aceasta comanda incheie executia programului.

!!! Daca nu se introduce una din comenzile de mai sus, se va afisa pe ecran !!!
!!!        un mesaj de eroare si o lista cu toate comenzile valabile        !!!

PS: Cartea lui Nutu Camataru "Dresor de Lei si de Fraieri" mi-a schimbat viata
radical!! https://www.youtube.com/watch?v=WMOCIbVOFBc