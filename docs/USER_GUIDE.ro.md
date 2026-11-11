# Ghidul utilizatorului Obdaryn

Același ghid e și în aplicație, la **Ajutor** (⌘?). Pentru ce îi cere fiecare funcție mașinii și adaptorului, în detaliu, vezi [cum comunică Obdaryn cu mașina](CAR_INTERACTION.md) (în engleză).

## Instalarea și prima deschidere

1. Descarcă programul de instalare de pe [obdaryn.eu](https://obdaryn.eu/ro/descarca/), deschide-l și trage **Obdaryn** în **Aplicații**. Merge pe macOS 14.4 sau mai nou, pe Apple Silicon și Intel.
2. Deschide Obdaryn. Pentru că nu e încă notarizat de Apple, macOS spune că nu poate verifica aplicația de programe malițioase. Apasă **Gata**.
3. Deschide **Configurări sistem → Confidențialitate și securitate**. La Securitate, lângă „Obdaryn a fost blocat”, apasă **Deschide oricum** și confirmă cu parola. Faci asta o singură dată; după aceea se deschide normal.

## Primii pași

Obdaryn citește ce raportează calculatoarele mașinii printr-un adaptor Wi-Fi compatibil ELM327, de exemplu Vgate iCar Pro. Doar citește: nu poate modifica nimic în mașină.

1. Pune adaptorul în priza OBD-II, sub bord, pe partea șoferului.
2. Pune contactul. Motorul poate rămâne oprit; pornește-l pentru datele în timp real cu motorul în funcțiune.
3. Conectează-te la rețeaua Wi-Fi a adaptorului (de obicei „V-LINK”, fără parolă). Cât timp e conectat, calculatorul nu are internet; e normal.
4. Apasă **Conectează**. Prima dată, macOS îți cere să-i permiți lui Obdaryn să găsească dispozitive în rețeaua locală: permite.
5. Închide mai întâi celelalte aplicații OBD (Car Scanner, Torque): un adaptor Wi-Fi acceptă o singură conexiune odată.

Nu ai mașina la îndemână? Încearcă **mașina demo** sau redă o înregistrare: toate ecranele merg cu amândouă.

## Mașini compatibile

Merge orice mașină cu priză OBD-II standard: în UE, mașinile pe benzină din 2001 și cele diesel din 2004; în SUA, toate mașinile din 1996. Pe toate, Obdaryn citește codurile de eroare, pregătirea pentru ITP, instantaneul, rezultatele autotestelor, datele în timp real și informațiile vehiculului, și șterge codurile când ceri.

Pentru aceste mărci știe mai mult, din datele care vin cu aplicația:

| Marcă | Toate modulele | Explicații de coduri | Valori de producător |
|---|---|---|---|
| Ford | Lista de module a mărcii | Da, în română și engleză | |
| Hyundai / Kia / Genesis | Lista de module a mărcii | Da, în română și engleză | Peste 50 de modele Hyundai și Kia (OBDb) |
| Renault / Dacia | Lista de module a mărcii | Da, în română și engleză | |
| Toyota / Lexus | Lista de module a mărcii | Da, în română și engleză | |
| Grupul Volkswagen (VW, Audi, Škoda, SEAT, Cupra) | Lista de module a mărcii | Da, în română și engleză | |

Orice altă marcă primește tot ce e standard și o scanare a tuturor modulelor la adresele standard OBD-II. Adresele modulelor și valorile de producător vin din surse publice și ale comunității; cele neconfirmate încă pe o mașină reală sunt marcate ca atare. **Ajutor → Mașini compatibile** din aplicație arată numerele și modelele exacte din versiunea pe care o ai.

## Ce poți face în fiecare stare a mașinii

| Starea mașinii | Ce poți face |
|---|---|
| Contact pus, motor oprit | Citești codurile de eroare, instantaneul, pregătirea pentru ITP și rezultatele autotestelor, informațiile vehiculului și scanezi toate modulele. E singura stare în care se pot șterge codurile. |
| Motor pornit, parcat | Tot ce e mai sus, în afară de ștergere, plus date în timp real cu sens: temperaturi, corecții de combustibil, tensiunea de încărcare. |
| În mers | Date în timp real, grafice, computerul de bord și înregistrarea unei sesiuni. Ecranul îl urmărește un pasager, niciodată șoferul. |
| Neconectat | Redai sau vezi în grafice înregistrările, le exporți datele ca CSV și te uiți prin călătoriile salvate. |

Cu contactul pus și motorul oprit, mașina își descarcă bateria. Ține astfel de sesiuni sub 20–30 de minute sau pornește motorul. Obdaryn arată tensiunea bateriei în Prezentare generală.

## Ce face de fapt ștergerea codurilor

Ștergerea e singurul lucru prin care Obdaryn schimbă ceva în mașină, și doar după ce confirmi, cu motorul oprit. Nu repară nimic:

- Martorul se stinge, dar se aprinde din nou dacă defectul e tot acolo, uneori după câteva drumuri.
- Instantaneul se șterge. Citește-l sau salvează un raport înainte.
- Toate monitoarele de pregătire se resetează. Mașina pică testul de emisii de la ITP până când e condusă suficient, de obicei 50–150 km de drum mixt, în câteva zile. Nu șterge niciodată codurile chiar înainte de ITP.
- Codurile permanente rămân. Se șterg singure după ce mașina retestează sistemul și îl găsește reparat.

## Ecranele

| Ecran | La ce folosește |
|---|---|
| **Prezentare generală** | Martorul de motor, pregătirea pentru ITP, bateria, codurile și principalele valori în timp real, dintr-o privire. **Salvează raportul…** (⌘S) face un PDF sau o pagină web de dat unui cumpărător sau mecanic. |
| **Coduri de eroare** | Codurile de la motor și transmisie, fiecare explicat: ce înseamnă, cauze probabile, dacă contează la ITP. |
| **Toate modulele** | Cere codurile de la ABS, airbag, caroserie și celelalte module, folosind lista de module a mărcii mașinii. **Caută la toate adresele** găsește modulele pe care lista nu le cunoaște (cam un minut, cu contactul pus). |
| **Pregătire ITP** | Ce autoteste s-au încheiat de la ultima ștergere a codurilor. **Citește rezultatele testelor** arată rezultatele autotestelor (Mode 06) cu limitele lor: un rezultat aproape de limită indică o piesă care se uzează. |
| **Instantaneu** | Valorile senzorilor salvate când a fost memorat un defect. |
| **Date în timp real** | Toate valorile raportate de mașină, cu istoricul recent. Fixează valori în Prezentare generală. La Hyundai și Kia, **Valori Hyundai** (sau **Valori Kia**) arată ce raportează modulele peste valorile standard: alege modelul, apoi **Caută valorile**. Aceste definiții vin de la comunitatea OBDb și nu sunt verificate pe mașina ta. |
| **Grafice** | Până la șase valori în timp, câte un grafic pentru fiecare, pe o axă de timp comună, în timp real sau dintr-o înregistrare. Compară două înregistrări ca să vezi un drum înainte și după o reparație. Treci cu mouse-ul sau dă focus graficelor și folosește săgețile ca să citești toate valorile dintr-un moment. |
| **Călătorie** | Distanța, combustibilul și consumul. La mașinile fără debitmetru, introdu capacitatea motorului. Calibrează estimarea consumului cu două alimentări și setează un preț al combustibilului ca să vezi costul. |
| **Informații vehicul** | Seria de șasiu (VIN) și ce spune ea, versiunile software și modulele care răspund. |
| **Jurnal adaptor** | Ce suportă adaptorul, cât de repede răspunde și fiecare comandă și răspuns, pentru depanare. |
| **Ajutor**, **Setări** | Acest ghid; limba (română, engleză), aspectul, unitățile de măsură (metric, UK, SUA). |

### Înregistrare și redare

Apasă **Înregistrează** în bara laterală cât timp ești conectat. Obdaryn salvează tot ce spune mașina în `~/Documents/Obdaryn/Recordings`. Pe ecranul de conectare, **Redă o înregistrare** o redă ca și cum mașina ar fi acolo, iar **CSV** îi exportă datele în timp real. Graficele pot arăta o înregistrare sau pot compara două. Înregistrările conțin seria de șasiu (VIN), așa că verifică-le înainte să le trimiți cuiva.

## Când ceva nu merge

| Problemă | Ce faci |
|---|---|
| Nu ajunge la adaptor | Verifică dacă calculatorul e în rețeaua Wi-Fi a adaptorului; macOS se întoarce uneori la rețeaua obișnuită. Pe macOS 15 și mai nou, permite-i lui Obdaryn accesul la Configurări sistem → Confidențialitate și securitate → Rețea locală. |
| Adaptorul refuză | E conectată altă aplicație, adesea una de pe telefon. Închide-o și conectează-te din nou. |
| Mașina nu răspunde | Pune contactul. Dacă l-ai luat, Obdaryn reia singur legătura când îl pui la loc. |
| Conexiunea se întrerupe | Adaptorul poate fi intrat în repaus sau s-a schimbat rețeaua Wi-Fi. Obdaryn se reconectează singur; dacă renunță, ia contactul și pune-l la loc, apoi apasă din nou Conectează. |
| Un modul apare „Negăsit” | Modelul tău poate nu îl are sau folosește altă adresă. Adresele marcate „neconfirmată” vin din liste ale comunității. Încearcă **Caută la toate adresele**. |
| Datele în timp real vin încet | Adaptoarele ieftine și mașinile mai vechi răspund încet. Fixează mai puține valori în Prezentare generală; valorile fixate și cele din grafice se actualizează primele. |
| Un cod a revenit | Defectul e tot acolo. Ștergerea doar stinge martorul; cauza trebuie reparată. |

## Termeni pe care îi vei vedea

| Termen | Ce înseamnă |
|---|---|
| Martorul de motor (MIL) | Lampa de avertizare a motorului, aprinsă de calculatorul motorului pentru defecte care afectează emisiile. |
| Cod de eroare (DTC) | Un cod din cinci caractere, de exemplu P0420: P motor și transmisie, C șasiu, B caroserie, U rețea. |
| Memorat, în așteptare, permanent | Memorat: confirmat. În așteptare: văzut o dată, memorat dacă se repetă. Permanent: nu poate fi șters cu un tester, se șterge singur după reparație. |
| Instantaneu (freeze frame) | Valorile senzorilor salvate în momentul în care a fost memorat un cod. |
| Monitoare de pregătire | Autotestele sistemelor de emisii. Monitoarele neîncheiate pot pica testul de emisii. |
| Rezultatele autotestelor (Mode 06) | Valorile din spatele monitoarelor, fiecare cu limitele după care o judecă mașina. |
| ITP | Inspecția tehnică periodică din România, care include un test de emisii. |
| VIN | Seria de șasiu a mașinii, din 17 caractere. |

## Confidențialitate și siguranță

- Obdaryn nu trimite nimic online. Comunică doar cu adaptorul.
- Înregistrările și rapoartele rămân pe calculatorul tău și conțin seria de șasiu (VIN). Verifică-le înainte să le trimiți cuiva.
- O protecție verifică fiecare comandă și refuză orice ar putea scrie, coda, reseta sau rescrie un modul. Singura modificare permisă e ștergerea codurilor de eroare, după ce confirmi, cu motorul oprit.
- Explicațiile codurilor sunt orientative și vin din surse publice. Confirmă la un service înainte să schimbi piese.
