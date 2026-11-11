#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Source of the Suzuki trouble-code pack.

    python3 tools/brand_packs/suzuki.py      # writes src/core/data/dtc/brands/suzuki.json
    python3 tools/check_brand_pack.py src/core/data/dtc/brands/suzuki.json

Edit THIS file, not the JSON.

Scope: Suzuki's own engine and automatic-gearbox codes (P1xxx) on 2005-2014
cars: Swift (RS, 2005-2010), SX4 (GY) and Grand Vitara (JT/JB). These are the
codes a generic reader receives from the engine and gearbox computers, so they
help any owner straight away. ABS, airbag and body codes wait until those
modules' addresses are known (vehicles/suzuki.json).

Sources
-------
Every entry is "two-sources": the meaning appears in the workshop manuals of at
least two different Suzuki models, each a separately written book with its own
code tables. Copies of the same manual on different hosts count once (the two
Grand Vitara uploads), and so do code-list sites that copy each other. No
Suzuki document filed with NHTSA defines a P1xxx code (all 48 Suzuki
manufacturer communications for the 2006-2013 SX4, Grand Vitara and Kizashi were
checked), so nothing here is "official".

Not confirmed yet for the 2010-2017 Swift (AZG) or later: no workshop manual for
those is public. The SX4 of 2008 already reports network faults as standard
U-codes (U0073, U0101, ...) instead of P1674-P1685, and newer cars may too.

Left out on purpose:
  * P1615, P1616: meaning differs between models (key registration on most,
    steering-lock and keyless faults on keyless Grand Vitaras).
  * P1874, P1875 (4WD low switch): only one model's manual, and an older code list.
  * The 1.3 DDiS Swift diesel codes: one source, and some clash with the petrol
    meanings (P1530).
  * Codes from 1996-2005 cars that no in-scope manual lists.

All descriptions are written for Obdaryn in our own words; nothing is copied from
the manuals.
"""

import json
import pathlib

OUT = pathlib.Path(__file__).resolve().parents[2] / "src/core/data/dtc/brands/suzuki.json"


# ---- Sources ---------------------------------------------------------------
def sw(page):
    """Swift 2005-2010 (RS413/RS415/RS416) service manual, hosted as "Swift Sport 2008"."""
    return f"https://www.manualslib.com/manual/3835202/Suzuki-Swift-Sport-2008.html?page={page}"


def sx(page):
    """SX4 (GY) 2008 service manual."""
    return f"https://www.manualslib.com/manual/1128304/Suzuki-Sx4.html?page={page}"


def gv(page):
    """Grand Vitara JB416/JB420 service manual."""
    return f"https://www.manualslib.com/manual/1811928/Suzuki-Grand-Vitara-Jb416.html?page={page}"


# Code list for older Suzukis (zukiworld; troublecodes.net shows the same table, so it counts once).
ZUKIWORLD = "https://www2.zukiworld.com/?p=417"

PACK = {}


def add(code, module, src, en, ro, flags=()):
    """en / ro: dict(t=title, d=description, c=[(likelihood, cause)], s=[symptoms])."""
    def text(x):
        out = {"t": x["t"], "d": x["d"]}
        if x.get("c"):
            out["c"] = [list(c) for c in x["c"]]
        if x.get("s"):
            out["s"] = list(x["s"])
        return out
    assert code not in PACK, code
    refs = [{"u": u} for u in dict.fromkeys(src)]
    # Two different models' manuals, at least: never one book counted twice.
    assert len(refs) >= 2, f"{code}: 'two-sources' needs two references"
    PACK[code] = {"module": module, "confidence": "two-sources", "src": refs,
                  "f": list(flags), "en": text(en), "ro": text(ro)}


MIL = ("mil",)


# ============================================================================
# Engine computer: power supply and electrical load
# ============================================================================
add("P1510", "engine", [sw(80), sx(84), gv(40), ZUKIWORLD],
    dict(t="Engine computer back-up power supply fault",
         d="The engine computer keeps a permanent battery feed for its memory, which holds what it has learned and the "
           "stored codes. It sets this code when that feed is missing or out of range once the engine is running. Until "
           "it is fixed, the computer forgets what it learned every time the ignition is switched off.",
         c=[("high", "Back-up feed to the engine computer open or poor (fuse, wiring or connector)"),
            ("low", "Engine computer")],
         s=["Check-engine light on", "Learned values and stored codes lost when the ignition is switched off"]),
    dict(t="Defect al alimentării de rezervă a calculatorului motorului",
         d="Calculatorul motorului are o alimentare permanentă de la baterie pentru memorie, unde ține ce a învățat și "
           "codurile memorate. Înregistrează acest cod când alimentarea lipsește sau e în afara limitelor cu motorul "
           "pornit. Până la reparație, calculatorul uită ce a învățat de fiecare dată când se ia contactul.",
         c=[("high", "Alimentarea de rezervă a calculatorului întreruptă sau cu contact slab (siguranță, cablaj sau conector)"),
            ("low", "Calculatorul motorului")],
         s=["Martorul de motor aprins", "Valorile învățate și codurile memorate se pierd la luarea contactului"]),
    flags=MIL)

for code, level_en, level_ro in (("P1501", "low", "prea mic"), ("P1502", "high", "prea mare")):
    add(code, "engine", [sw(80), gv(40)],
        dict(t=f"Electric load current sensor: signal too {level_en}",
             d="The engine computer measures how much current the car's electrical equipment draws, with a sensor in the "
               f"main fuse box, and adjusts the alternator to it. The sensor's signal is too {level_en} to be real, so the "
               "computer stops adjusting the charging to the load.",
             c=[("high", "Open or short circuit in the sensor's wiring"),
                ("medium", "Electric load current sensor"),
                ("low", "Engine computer")],
             s=["Charging no longer follows the electrical load"]),
        dict(t=f"Senzorul de curent al consumatorilor: semnal {level_ro}",
             d="Calculatorul motorului măsoară cât curent consumă echipamentele electrice ale mașinii, cu un senzor din "
               f"cutia de siguranțe principală, și reglează alternatorul după el. Semnalul senzorului e {level_ro} ca să "
               "fie real, așa că calculatorul nu mai adaptează încărcarea la consum.",
             c=[("high", "Întrerupere sau scurtcircuit în cablajul senzorului"),
                ("medium", "Senzorul de curent al consumatorilor"),
                ("low", "Calculatorul motorului")],
             s=["Încărcarea bateriei nu mai urmează consumul electric"]))

add("P1603", "engine", [sw(80), gv(40)],
    dict(t="Automatic gearbox computer has a fault stored",
         d="On cars with an automatic gearbox, the engine computer uses some values measured by the gearbox computer. "
           "This code means the gearbox computer has stored a fault of its own, so those values can't be trusted. The "
           "real fault is in the gearbox computer: read its codes first.",
         c=[("high", "A fault stored in the automatic gearbox computer")]),
    dict(t="Calculatorul cutiei automate are un defect memorat",
         d="La mașinile cu cutie automată, calculatorul motorului folosește câteva valori măsurate de calculatorul cutiei. "
           "Codul înseamnă că acesta are un defect memorat, așa că valorile lui nu mai sunt de încredere. Defectul real e în "
           "calculatorul cutiei: citește-i mai întâi codurile.",
         c=[("high", "Un defect memorat în calculatorul cutiei automate")]))

# ============================================================================
# Engine computer: CAN network
# ============================================================================
add("P1674", "engine", [sw(80), gv(40), gv(49)],
    dict(t="CAN network: engine computer stopped transmitting (bus off)",
         d="The engine computer could not get its messages confirmed on the CAN network that links it to the other "
           "computers, so it stopped sending (a \"bus off\" state). The other computers then report that they lost "
           "contact with it.",
         c=[("high", "CAN wiring open or shorted, or a loose connector at one of the computers"),
            ("medium", "Another computer on the network disturbing it"),
            ("low", "Engine computer")],
         s=["Other computers store lost-communication codes"]),
    dict(t="Rețeaua CAN: calculatorul motorului a încetat să transmită (bus off)",
         d="Calculatorul motorului nu și-a putut confirma mesajele pe rețeaua CAN care îl leagă de celelalte calculatoare, "
           "așa că s-a oprit din transmis (starea „bus off”). Celelalte calculatoare raportează apoi că au pierdut legătura "
           "cu el.",
         c=[("high", "Cablaj CAN întrerupt sau în scurtcircuit, ori un conector slăbit la unul dintre calculatoare"),
            ("medium", "Alt calculator din rețea care o perturbă"),
            ("low", "Calculatorul motorului")],
         s=["Celelalte calculatoare memorează coduri de comunicație pierdută"]))

for code, flags, who_en, who_ro, fix_en, fix_ro, src, extra_s in (
        ("P1676", MIL, "the automatic gearbox computer", "calculatorul cutiei automate",
         ("high", "Gearbox computer supply or ground"), ("high", "Alimentarea sau masa calculatorului cutiei"),
         [sw(80), gv(40)], None),
        ("P1678", (), "the body computer (BCM)", "calculatorul de caroserie (BCM)",
         None, None, [sw(80), gv(40), gv(56)], None),
        ("P1685", MIL, "the ABS/ESP computer", "calculatorul ABS/ESP",
         None, None, [sw(80), gv(40), gv(60)],
         ("The ABS or ESP warning light may be on too", "Martorul ABS sau ESP poate fi și el aprins"))):
    causes_en = ([fix_en] if fix_en else []) + [("medium", f"CAN wiring to {who_en}"), ("low", who_en[0].upper() + who_en[1:])]
    causes_ro = ([fix_ro] if fix_ro else []) + [("medium", f"Cablajul CAN spre {who_ro}"), ("low", who_ro[0].upper() + who_ro[1:])]
    add(code, "engine", src,
        dict(t=f"CAN network: no valid data from {who_en}",
             d=f"The engine computer isn't receiving valid messages from {who_en} over the CAN network.",
             c=causes_en, s=[extra_s[0]] if extra_s else None),
        dict(t=f"Rețeaua CAN: nu sosesc date valide de la {who_ro}",
             d=f"Calculatorul motorului nu primește mesaje valide de la {who_ro} pe rețeaua CAN.",
             c=causes_ro, s=[extra_s[1]] if extra_s else None),
        flags=flags)

# ============================================================================
# Engine computer: immobiliser and keyless start
# ============================================================================
IMMO = "The immobiliser lets the engine run only with a key it knows. "
IMMO_RO = "Imobilizatorul lasă motorul să pornească doar cu o cheie pe care o cunoaște. "

add("P1614", "engine", [sw(1449), sx(85), gv(441)],
    dict(t="Immobiliser: no reply from the key's transponder",
         d=IMMO + "With the ignition on, it reads a code from the transponder chip in the key through a coil around the "
           "ignition lock. This code means the key's reply could not be read.",
         c=[("high", "Key transponder damaged, or a key that doesn't belong to this car"),
            ("medium", "Immobiliser coil (antenna) or its control module"),
            ("low", "Engine computer")],
         s=["The engine may not start", "The immobiliser light flashes"]),
    dict(t="Imobilizator: transponderul cheii nu răspunde",
         d=IMMO_RO + "Cu contactul pus, citește un cod din cipul transponder al cheii printr-o bobină din jurul "
           "contactului. Codul înseamnă că răspunsul cheii nu a putut fi citit.",
         c=[("high", "Transponderul cheii deteriorat sau o cheie care nu e a acestei mașini"),
            ("medium", "Bobina (antena) imobilizatorului sau modulul ei de control"),
            ("low", "Calculatorul motorului")],
         s=["Motorul poate să nu pornească", "Martorul imobilizatorului clipește"]))

add("P1618", "engine", [sw(1449), sw(1455), sx(85), gv(441)],
    dict(t="CAN network: no data from the keyless start computer",
         d="On cars with keyless start, the engine computer isn't receiving messages from the keyless start computer over "
           "the CAN network.",
         c=[("high", "CAN wiring or connectors"),
            ("medium", "Keyless start computer supply or ground"),
            ("low", "Keyless start computer")]),
    dict(t="Rețeaua CAN: nu sosesc date de la calculatorul de pornire fără cheie",
         d="La mașinile cu pornire fără cheie, calculatorul motorului nu primește mesaje de la calculatorul de pornire "
           "fără cheie pe rețeaua CAN.",
         c=[("high", "Cablajul sau conectorii CAN"),
            ("medium", "Alimentarea sau masa calculatorului de pornire fără cheie"),
            ("low", "Calculatorul de pornire fără cheie")]))

add("P1621", "engine", [sw(1449), sw(1455), sx(85), gv(441)],
    dict(t="Immobiliser: no communication between its module and the engine computer",
         d=IMMO + "This code means the immobiliser control module and the engine computer can't exchange data.",
         c=[("high", "Immobiliser module fuse, supply or ground"),
            ("medium", "Wiring between the immobiliser module and the engine computer"),
            ("low", "Immobiliser module or engine computer")],
         s=["The engine may not start"]),
    dict(t="Imobilizator: modulul lui nu comunică cu calculatorul motorului",
         d=IMMO_RO + "Codul înseamnă că modulul de control al imobilizatorului și calculatorul motorului nu pot schimba "
           "date.",
         c=[("high", "Siguranța, alimentarea sau masa modulului imobilizatorului"),
            ("medium", "Cablajul dintre modulul imobilizatorului și calculatorul motorului"),
            ("low", "Modulul imobilizatorului sau calculatorul motorului")],
         s=["Motorul poate să nu pornească"]))

add("P1622", "engine", [sw(1456), sx(85), gv(441)],
    dict(t="Engine computer: immobiliser memory fault",
         d="The engine computer could not read or write the memory where it keeps the immobiliser data. This is a fault "
           "in the computer itself.",
         c=[("high", "Engine computer (replacing it means registering the keys again)")],
         s=["The engine may not start"]),
    dict(t="Calculatorul motorului: defect al memoriei imobilizatorului",
         d="Calculatorul motorului nu a putut citi sau scrie memoria în care păstrează datele imobilizatorului. E un defect "
           "al calculatorului însuși.",
         c=[("high", "Calculatorul motorului (la înlocuire, cheile trebuie înregistrate din nou)")],
         s=["Motorul poate să nu pornească"]))

add("P1623", "engine", [sw(1456), sx(85), gv(441)],
    dict(t="Immobiliser: key not registered to this car",
         d=IMMO + "The key's transponder answered, but its code isn't one the car has registered.",
         c=[("high", "A key that isn't registered to this car"),
            ("low", "Immobiliser module or engine computer")],
         s=["The engine won't start with that key"]),
    dict(t="Imobilizator: cheia nu e înregistrată pe această mașină",
         d=IMMO_RO + "Transponderul cheii a răspuns, dar codul lui nu e unul înregistrat de mașină.",
         c=[("high", "O cheie care nu e înregistrată pe această mașină"),
            ("low", "Modulul imobilizatorului sau calculatorul motorului")],
         s=["Motorul nu pornește cu acea cheie"]))

add("P1625", "engine", [sw(1457), sx(85), gv(441)],
    dict(t="Immobiliser antenna (coil) fault",
         d=IMMO + "This code points to the coil around the ignition lock that talks to the key, or the module driving it.",
         c=[("high", "Immobiliser coil (antenna) or its module"),
            ("low", "Engine computer")],
         s=["The engine may not start"]),
    dict(t="Defect al antenei (bobinei) imobilizatorului",
         d=IMMO_RO + "Codul indică bobina din jurul contactului care comunică cu cheia sau modulul care o comandă.",
         c=[("high", "Bobina (antena) imobilizatorului sau modulul ei"),
            ("low", "Calculatorul motorului")],
         s=["Motorul poate să nu pornească"]))

add("P1636", "engine", [sw(1449), sw(1457), sx(85), gv(441)],
    dict(t="Immobiliser data couldn't be registered (engine and body computers)",
         d="The engine computer and the body computer share the immobiliser data. Registering it in both failed, "
           "usually because they can't talk to each other over the CAN network.",
         c=[("high", "CAN communication fault between the engine and body computers"),
            ("medium", "Body computer"),
            ("low", "Engine computer")],
         s=["The engine may not start"]),
    dict(t="Imobilizator: datele nu au putut fi înregistrate în calculatoarele motorului și caroseriei",
         d="Calculatorul motorului și cel de caroserie își împart datele imobilizatorului. Înregistrarea lor în ambele a "
           "eșuat, de obicei pentru că nu pot comunica pe rețeaua CAN.",
         c=[("high", "Defect de comunicație CAN între calculatorul motorului și cel de caroserie"),
            ("medium", "Calculatorul de caroserie"),
            ("low", "Calculatorul motorului")],
         s=["Motorul poate să nu pornească"]))

add("P1638", "engine", [sw(1449), sx(85), gv(441)],
    dict(t="Immobiliser data doesn't match between engine and body computers",
         d="The immobiliser data held by the engine computer and by the body computer differ. It happens when one of "
           "them was replaced with a part from another car, or when they can't communicate.",
         c=[("high", "An engine or body computer from another car fitted without being matched"),
            ("medium", "Communication fault between the engine and body computers")],
         s=["The engine may not start"]),
    dict(t="Datele imobilizatorului nu coincid între calculatorul motorului și cel de caroserie",
         d="Datele imobilizatorului din calculatorul motorului și din cel de caroserie diferă. Se întâmplă când unul "
           "dintre ele a fost înlocuit cu o piesă de pe altă mașină sau când nu pot comunica.",
         c=[("high", "Un calculator de motor sau de caroserie de pe altă mașină, montat fără împerechere"),
            ("medium", "Defect de comunicație între calculatorul motorului și cel de caroserie")],
         s=["Motorul poate să nu pornească"]))

# ============================================================================
# Automatic gearbox computer (4-speed automatic)
# ============================================================================
add("P1702", "transmission", [sw(663), sw(712), sx(721), gv(333)],
    dict(t="Gearbox computer: internal memory fault",
         d="The automatic gearbox computer found an error when checking its own memory. It goes into a fail-safe mode "
           "with a fixed gear and no torque-converter lock-up.",
         c=[("high", "Automatic gearbox computer")],
         s=["Gearbox stays in one gear (fail-safe)", "No torque-converter lock-up"]),
    dict(t="Calculatorul cutiei automate: defect al memoriei interne",
         d="Calculatorul cutiei automate a găsit o eroare la verificarea propriei memorii. Trece într-un mod de avarie, cu "
           "o treaptă fixă și fără blocarea convertizorului de cuplu.",
         c=[("high", "Calculatorul cutiei automate")],
         s=["Cutia rămâne într-o singură treaptă (mod de avarie)", "Convertizorul de cuplu nu se mai blochează"]),
    flags=("mil", "limp_mode"))

add("P1703", "transmission", [sw(663), sw(712), gv(333)],
    dict(t="Gearbox computer: engine data flagged as faulty",
         d="The gearbox computer receives engine data over the CAN network (throttle, coolant temperature, engine speed, "
           "torque). The engine computer has marked some of it as faulty, so the gearbox limits how it shifts. The cause is "
           "an engine fault: read the engine computer's codes first.",
         c=[("high", "A fault stored in the engine computer")],
         s=["No 4th gear or no torque-converter lock-up"]),
    dict(t="Calculatorul cutiei: date de la motor marcate ca defecte",
         d="Calculatorul cutiei primește pe rețeaua CAN date despre motor (clapetă, temperatura lichidului de răcire, "
           "turație, cuplu). Calculatorul motorului a marcat unele ca defecte, așa că cutia își limitează schimbările. "
           "Cauza e un defect de motor: citește mai întâi codurile calculatorului motorului.",
         c=[("high", "Un defect memorat în calculatorul motorului")],
         s=["Fără treapta a 4-a sau fără blocarea convertizorului de cuplu"]))

add("P1723", "transmission", [sw(663), sw(713), sx(722), gv(333)],
    dict(t="Gear selector \"3\" position switch: signal doesn't match",
         d="The switch that reports the selector's \"3\" position says something different from the gear range sensor.",
         c=[("high", "\"3\" position switch or its wiring"),
            ("low", "Automatic gearbox computer")]),
    dict(t="Contactul poziției „3” a selectorului: semnal necorespunzător",
         d="Contactul care raportează poziția „3” a selectorului spune altceva decât senzorul de poziție al selectorului.",
         c=[("high", "Contactul poziției „3” sau cablajul lui"),
            ("low", "Calculatorul cutiei automate")]))

add("P1774", "transmission", [sw(663), sw(715), gv(333), gv(341)],
    dict(t="CAN network: gearbox computer stopped transmitting (bus off)",
         d="The automatic gearbox computer could not get its messages confirmed on the CAN network, so it stopped sending "
           "(a \"bus off\" state) and limits shifting and lock-up.",
         c=[("high", "CAN wiring open or shorted, or a loose connector"),
            ("medium", "Another computer on the network disturbing it"),
            ("low", "Automatic gearbox computer")],
         s=["Limited shifting and no torque-converter lock-up"]),
    dict(t="Rețeaua CAN: calculatorul cutiei a încetat să transmită (bus off)",
         d="Calculatorul cutiei automate nu și-a putut confirma mesajele pe rețeaua CAN, așa că s-a oprit din transmis "
           "(starea „bus off”) și limitează schimbările și blocarea convertizorului.",
         c=[("high", "Cablaj CAN întrerupt sau în scurtcircuit, ori un conector slăbit"),
            ("medium", "Alt calculator din rețea care o perturbă"),
            ("low", "Calculatorul cutiei automate")],
         s=["Schimbări limitate și fără blocarea convertizorului de cuplu"]),
    flags=("limp_mode",))

for code, who_en, who_ro, src in (
        ("P1777", "the engine computer", "calculatorul motorului", [sw(664), sw(717), gv(333), gv(343)]),
        ("P1778", "the body computer (BCM)", "calculatorul de caroserie (BCM)", [sw(664), gv(333)])):
    add(code, "transmission", src,
        dict(t=f"Gearbox computer: no data from {who_en} (CAN)",
             d=f"The automatic gearbox computer isn't receiving messages from {who_en} over the CAN network.",
             c=[("high", f"CAN wiring to {who_en}"), ("low", who_en[0].upper() + who_en[1:])]),
        dict(t=f"Calculatorul cutiei: nu sosesc date de la {who_ro} (CAN)",
             d=f"Calculatorul cutiei automate nu primește mesaje de la {who_ro} pe rețeaua CAN.",
             c=[("high", f"Cablajul CAN spre {who_ro}"), ("low", who_ro[0].upper() + who_ro[1:])]))

add("P1878", "transmission", [sw(664), sw(719), sx(722), gv(333)],
    dict(t="Torque converter lock-up clutch shudder",
         d="While the torque converter's lock-up clutch was slipping on purpose, the gearbox computer saw the output speed "
           "oscillate: the clutch shudders. It stops using this partial lock-up.",
         c=[("high", "Worn or degraded automatic gearbox fluid"),
            ("medium", "Torque converter lock-up clutch")],
         s=["Shudder as the torque converter locks up"]),
    dict(t="Vibrații ale ambreiajului de blocare a convertizorului de cuplu",
         d="În timp ce ambreiajul de blocare a convertizorului patina controlat, calculatorul cutiei a văzut turația de "
           "ieșire oscilând: ambreiajul vibrează. Renunță la această blocare parțială.",
         c=[("high", "Ulei de cutie automată uzat sau degradat"),
            ("medium", "Ambreiajul de blocare a convertizorului de cuplu")],
         s=["Vibrații când se blochează convertizorul de cuplu"]))


def main():
    pack = {
        "brand": "suzuki",
        "label": "Obdaryn's Suzuki pack",
        "label_ro": "pachetul Obdaryn pentru Suzuki",
        "brandName": "Suzuki",
        "source": {
            "license": "GPL-3.0-or-later (text written for Obdaryn)",
            "note": "Meanings checked against the workshop manuals of at least two different Suzuki models; each code "
                    "lists its sources. Not yet confirmed for the 2010-2017 Swift and later. Verify with a workshop before "
                    "replacing parts.",
            "scope": "Engine and automatic-gearbox codes of 2005-2014 Suzuki Swift (RS), SX4 (GY), Grand Vitara (JT/JB)",
        },
        "codes": dict(sorted(PACK.items())),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(pack, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"wrote {len(PACK)} codes to {OUT}")


if __name__ == "__main__":
    main()
