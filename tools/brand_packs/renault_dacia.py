#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Source of the Renault/Dacia trouble-code pack.

    python3 tools/brand_packs/renault_dacia.py      # writes src/core/data/dtc/brands/renault-dacia.json
    python3 tools/check_brand_pack.py src/core/data/dtc/brands/renault-dacia.json

Edit THIS file, not the JSON. Families of codes (wheel-speed sensors, ...) are
written once and expanded, so English and Romanian stay consistent.

Scope: 2010-2020 Dacia Logan, Sandero, Duster, Lodgy, Dokker and Renault Clio,
Megane, Captur, Kangoo (K7M, K9K dCi, H4Bt/H5Ft TCe and K4M engines).

How Renault codes reach a generic reader
----------------------------------------
Renault's own tools (CLIP, DDT4All) show internal "DFxxx" fault numbers. Each DF
number has an "associated DTC" in Renault's fault-finding tables, and that DTC is
what a generic OBD/UDS reader receives: Renault writes it as four hex digits
(9007, 50C3, 5608, ...), which a reader shows as B1007, C10C3, C1608, ...
(first hex digit 0-3 = P, 4-7 = C, 8-B = B, C-F = U). The DF number itself never
leaves the car and changes from one computer version to the next (DF165 on a
Duster airbag unit is DF010 on another), so DF numbers are not in this pack.

Engine computers use standard SAE P0/P2 codes for almost everything; only a few
P1xxx codes are Renault-specific. Renault's own numbers are mostly in the chassis
and body computers, and their meaning can change between generations of the same
computer (for example B1036 is a side-impact sensor on older airbag units but a
supply-voltage fault on Logan II/Duster units), so only codes whose meaning was
confirmed without contradiction are included.

No Renault/Dacia document is filed with NHTSA (the brands are not sold in the
US), so every entry is "two-sources": at least two independent public pages,
each covering that exact code, agree on the meaning. All descriptions are written
for Obdaryn in our own words; nothing is copied from service manuals. Codes that
were disputed, generation- or engine-dependent, or backed by only one source were
left out on purpose (see docs/brand-packs.md).
"""

import json
import pathlib

OUT = pathlib.Path(__file__).resolve().parents[2] / "src/core/data/dtc/brands/renault-dacia.json"

# ---- Sources ---------------------------------------------------------------
# Renault fault-finding tables (DF number -> associated DTC), compiled by a French tool seller.
AUTODIAG = ("https://autodiag-france.com/en/blogs/infos/"
            "la-liste-complete-des-codes-defaut-renault-comprendre-et-diagnostiquer-efficacement")


def autoxuga(hexcode):
    """Spanish workshop's per-code page for a Renault 4-hex DTC (gives the reader-format code)."""
    return f"https://www.autoxuga.com/averiasrecibidas/faultcars/faultsrenault/renaultdtc{hexcode}.html"


# Real scan reports (Autel MaxiSys, published by DEKRA / MFC auctions) of in-scope cars.
DEKRA_LOGAN2_A = "https://www.mfcauctions.co.za/dekra-reports/DIGVF15SRAA455903438.pdf"   # Logan II / Sandero II
DEKRA_LOGAN2_B = "https://mfcauctions.co.za/dekra-reports/DIGAAPV0342010672620.pdf"       # Logan II / Sandero II
DEKRA_LOGAN2_C = "https://mfcauctions.co.za/dekra-reports/DIGVF15SRLA463065623.pdf"       # Logan II / Sandero II
DEKRA_CLIO4 = "https://mfcauctions.co.za/dekra-reports/DIGVF15R400E59090389.pdf"          # Clio IV
DEKRA_DUSTER = "https://mfcauctions.co.za/dekra-reports/DIGVF1HJD20462169972.pdf"         # Duster
DEKRA_CAPTUR = "https://mfcauctions.co.za/dekra-reports/DIGVF12R141A58555675.pdf"         # Captur
# Scan reports from a different tool maker (ThinkCar).
THINKCAR_LOGAN2 = "https://reportview.thinkcar.com/report/think_car/?id=76be4be7geTd1uAEAE2YLroG54&type=diag&lan=en-us"
THINKCAR_SANDERO3 = "https://reportview.thinkcar.com/report/think_car/?id=289129aage3b3bTd8cTdLrIFOM&type=diag&lan=en-us"
# Forum threads and other pages covering one code.
PLANETE_P1525 = "https://www.planeterenault.com/forum/post1067302.html"            # Megane III 1.5 dCi, DTC1525F3
ELEKTRODA_P1560 = "https://www.elektroda.pl/rtvforum/topic4149141.html"              # Twingo 3, DTC156013 / P1560
CLIOSPORT_9040 = "https://cliosport.net/threads/airbag-fault-code-df165-restraint-system-malfunction-warnlight.682452/"
CRASHDATA_907F = ("https://crashdata.co.uk/product/"
                  "a2c80613911-renault-clio-dacia-duster-airbag-module-repair-and-reset-985101389r-a/")

# No Renault/Dacia document filed with a regulator was found, so nothing is flagged official.
OFFICIAL_PREFIXES = ()
OFFICIAL = set()


PACK = {}


def add(code, module, confidence, src, en, ro, flags=(), key=None):
    """en / ro: dict(t=title, d=description, c=[(likelihood, cause)], s=[symptoms])."""
    def text(x):
        out = {"t": x["t"], "d": x["d"]}
        if x.get("c"):
            out["c"] = [list(c) for c in x["c"]]
        if x.get("s"):
            out["s"] = list(x["s"])
        return out
    k = key or code
    assert k not in PACK, k
    refs = [{"u": u, "o": True} if u in OFFICIAL or u.startswith(OFFICIAL_PREFIXES) else {"u": u}
            for u in dict.fromkeys(src)]
    n_official = sum(1 for r in refs if r.get("o"))
    # Confidence comes from the references, never from wishful thinking.
    if confidence == "official":
        assert n_official >= 1, f"{k}: 'official' without an official reference"
    else:
        assert len(refs) >= 2, f"{k}: 'two-sources' needs two references"
    PACK[k] = {"module": module, "confidence": confidence, "src": refs,
               "f": list(flags), "en": text(en), "ro": text(ro)}


LAMP = ("warning_lamp",)

# ============================================================================
# Engine
# ============================================================================
add("P1525", "engine", "two-sources", [DEKRA_LOGAN2_A, DEKRA_CLIO4, DEKRA_DUSTER, PLANETE_P1525],
    dict(t="Cruise control / speed limiter: inconsistent network data",
         d="The engine computer received data for the cruise control or speed limiter over the CAN network that does not "
           "make sense, so it switches both functions off. It is often a side effect of another stored fault (network, ABS "
           "or engine), so read every module and repair the other faults first, then clear the codes.",
         c=[("medium", "Another stored fault that makes the cruise-control data invalid (CAN network, ABS or engine fault)"),
            ("low", "Faulty cruise control / speed limiter switch, or brake or clutch pedal switch")],
         s=["Cruise control and speed limiter not available"]),
    dict(t="Pilot automat / limitator de viteză: date incoerente pe rețea",
         d="Calculatorul motorului a primit pe rețeaua CAN date pentru pilotul automat sau limitatorul de viteză care nu "
           "au sens, așa că dezactivează ambele funcții. Apare des ca urmare a altui defect memorat (rețea, ABS sau motor), "
           "deci citește toate modulele, repară întâi celelalte defecte și apoi șterge codurile.",
         c=[("medium", "Alt defect memorat care face invalide datele pilotului automat (rețea CAN, ABS sau motor)"),
            ("low", "Comutator defect al pilotului automat / limitatorului sau contactor defect la pedala de frână ori de ambreiaj")],
         s=["Pilotul automat și limitatorul de viteză nu sunt disponibile"]),
    key="P1525@engine")

add("P1560", "engine", "two-sources", [DEKRA_LOGAN2_A, ELEKTRODA_P1560],
    dict(t="Oil pump solenoid valve circuit",
         d="Petrol engines with a variable-pressure oil pump (such as the TCe and SCe engines) let the engine computer "
           "control the pump through a solenoid valve. The computer found an electrical fault in that circuit, most often "
           "an open circuit. The valve gets +12 V from the fuse and relay box and is switched to ground by the engine "
           "computer. Older diesel computers outside this pack's range used P1560 for a supply-voltage fault instead.",
         c=[("medium", "Broken wire or poor connector between the valve, the fuse/relay box and the engine computer"),
            ("medium", "No +12 V supply at the valve connector"),
            ("low", "Failed oil pump solenoid valve")]),
    dict(t="Circuitul electrovalvei pompei de ulei",
         d="La motoarele pe benzină cu pompă de ulei cu presiune variabilă (cum sunt motoarele TCe și SCe), calculatorul "
           "motorului comandă pompa printr-o electrovalvă. Calculatorul a găsit un defect electric în acest circuit, cel "
           "mai des un circuit întrerupt. Electrovalva primește +12 V de la cutia de siguranțe și relee, iar calculatorul "
           "motorului îi comandă masa. Calculatoarele diesel mai vechi, din afara acestui pachet, foloseau P1560 pentru un "
           "defect de tensiune de alimentare.",
         c=[("medium", "Fir întrerupt sau conector cu contact slab între electrovalvă, cutia de siguranțe/relee și calculatorul motorului"),
            ("medium", "Lipsește alimentarea de +12 V la conectorul electrovalvei"),
            ("low", "Electrovalva pompei de ulei defectă")]),
    key="P1560@engine")

# Without module context (e.g. a manual lookup) the same text applies.
for _code in ("P1525", "P1560"):
    PACK[_code] = dict(PACK[_code + "@engine"])

# ============================================================================
# ABS / ESP (Renault numbering of the older Bosch ABS/ESP 8.0 units)
# ============================================================================
ABS_EN = ("This meaning comes from Renault's tables for its older Bosch ABS/ESP 8.0 units; newer ABS units (for example "
          "on Logan II and Clio IV) report wheel faults under other codes. While the fault is present, ABS (and ESP, if "
          "fitted) is usually switched off; normal braking still works.")
ABS_RO = ("Acest sens provine din tabelele Renault pentru unitățile mai vechi Bosch ABS/ESP 8.0; unitățile ABS mai noi (de "
          "exemplu pe Logan II și Clio IV) raportează defectele roților cu alte coduri. Cât timp defectul este prezent, "
          "ABS-ul (și ESP-ul, dacă există) este de obicei dezactivat; frânarea normală funcționează în continuare.")
ABS_S_EN = ["ABS (and ESP) warning light", "ABS does not work"]
ABS_S_RO = ["Martorul ABS (și ESP) aprins", "ABS-ul nu funcționează"]

WHEELS = [  # (sensor circuit code, target code, English, Romanian)
    ("500F", "5041", "front-right", "față dreapta"),
    ("501F", "5042", "front-left", "față stânga"),
    ("502F", "5043", "rear-right", "spate dreapta"),
    ("503F", "5044", "rear-left", "spate stânga"),
]


def reader(hexcode):
    """Renault 4-hex DTC -> the code a reader shows (9007 -> B1007)."""
    n = int(hexcode, 16)
    return "PCBU"[n >> 14] + str((n >> 12) & 3) + hexcode[1:].upper()


for circuit, target, wen, wro in WHEELS:
    code = reader(circuit)
    add(code, "abs", "two-sources", [AUTODIAG, autoxuga(circuit)],
        dict(t=f"Wheel-speed sensor {wen}: circuit fault",
             d=f"The ABS unit sees an electrical fault in the {wen} wheel-speed sensor circuit. " + ABS_EN,
             c=[("medium", "Damaged sensor wire or connector (chafing, corrosion)"), ("medium", "Failed sensor"),
                ("low", "ABS unit fault")],
             s=ABS_S_EN),
        dict(t=f"Senzor de turație roată {wro}: defect de circuit",
             d=f"Unitatea ABS vede un defect electric în circuitul senzorului de turație al roții {wro}. " + ABS_RO,
             c=[("medium", "Fir sau conector al senzorului deteriorat (frecare, coroziune)"), ("medium", "Senzor defect"),
                ("low", "Defect al unității ABS")],
             s=ABS_S_RO),
        LAMP, key=code + "@abs")
    code = reader(target)
    add(code, "abs", "two-sources", [AUTODIAG, autoxuga(target)],
        dict(t=f"Wheel-speed signal {wen}: sensor target (tone ring) fault",
             d=f"The {wen} wheel-speed signal is not plausible. Renault's tables link this code to the sensor target, the "
               "toothed or magnetic ring the sensor reads. " + ABS_EN,
             c=[("medium", "Damaged, dirty or rusted tone ring"), ("medium", "Loose sensor or too large a gap to the ring"),
                ("low", "Wheel bearing play")],
             s=ABS_S_EN),
        dict(t=f"Semnal de turație roată {wro}: defect la ținta senzorului (inel dințat)",
             d=f"Semnalul de turație al roții {wro} nu este plauzibil. Tabelele Renault leagă acest cod de ținta senzorului, "
               "inelul dințat sau magnetic pe care îl citește senzorul. " + ABS_RO,
             c=[("medium", "Inel dințat deteriorat, murdar sau ruginit"), ("medium", "Senzor slăbit sau distanță prea mare față de inel"),
                ("low", "Joc la rulmentul roții")],
             s=ABS_S_RO),
        LAMP, key=code + "@abs")

ABS_OTHER = [
    ("5046",
     "Wheel speeds inconsistent",
     "The speeds measured at the four wheels do not agree with each other.",
     [("medium", "Mixed tyre sizes, or one tyre very worn or under-inflated"), ("medium", "Wheel-speed sensor or tone ring problem")],
     "Turațiile roților sunt incoerente",
     "Turațiile măsurate la cele patru roți nu se potrivesc între ele.",
     [("medium", "Anvelope de dimensiuni diferite sau o anvelopă foarte uzată ori dezumflată"), ("medium", "Problemă la un senzor de turație sau la un inel dințat")]),
    ("50C3",
     "ABS/ESP computer fault or configuration missing",
     "The ABS/ESP computer reports an internal fault, or that its configuration (such as the tyre-size index or other "
     "vehicle parameters) has not been programmed. A workshop with Renault-compatible software can check and program it.",
     [("medium", "Configuration not programmed after the ABS unit was replaced"), ("low", "ABS/ESP computer internal fault")],
     "Defect al calculatorului ABS/ESP sau configurare lipsă",
     "Calculatorul ABS/ESP raportează un defect intern sau faptul că nu i s-a programat configurarea (de exemplu indicele "
     "de dimensiune a anvelopelor sau alți parametri ai mașinii). Un service cu soft compatibil Renault îl poate verifica "
     "și programa.",
     [("medium", "Configurarea nu a fost programată după înlocuirea unității ABS"), ("low", "Defect intern al calculatorului ABS/ESP")]),
    ("50C6",
     "Brake light switch circuit",
     "The ABS/ESP unit sees a fault in the brake light switch signal.",
     [("medium", "Badly adjusted or failed brake light switch"), ("low", "Wiring or connector at the brake pedal")],
     "Circuitul contactorului lămpilor de frână",
     "Unitatea ABS/ESP vede un defect la semnalul contactorului lămpilor de frână.",
     [("medium", "Contactorul lămpilor de frână dereglat sau defect"), ("low", "Cablaj sau conector la pedala de frână")]),
    ("50CC",
     "ABS/ESP supply voltage fault",
     "The supply voltage of the ABS/ESP unit is out of range.",
     [("medium", "Weak battery or charging-system fault"), ("low", "Corroded ground or supply connection, or a blown fuse")],
     "Defect la tensiunea de alimentare ABS/ESP",
     "Tensiunea de alimentare a unității ABS/ESP este în afara limitelor.",
     [("medium", "Baterie slabă sau defect la sistemul de încărcare"), ("low", "Masă sau alimentare oxidată, ori o siguranță arsă")]),
    ("50E6",
     "ABS/ESP network (CAN) fault",
     "The ABS/ESP unit reports a fault on the CAN network it uses to talk to the other computers.",
     [("medium", "CAN wiring or connector fault"), ("low", "Another computer on the network disturbing it")],
     "Defect de rețea (CAN) la ABS/ESP",
     "Unitatea ABS/ESP raportează un defect pe rețeaua CAN prin care comunică cu celelalte calculatoare.",
     [("medium", "Defect de cablaj sau de conector pe rețeaua CAN"), ("low", "Alt calculator din rețea care o perturbă")]),
    ("50F4",
     "ESP yaw-rate / lateral-acceleration sensor fault",
     "The combined sensor that measures how fast the car turns and how hard it is pushed sideways gives an implausible "
     "signal or has a circuit fault. Stability control needs it, so ESP is switched off.",
     [("medium", "Sensor wiring or connector"), ("medium", "Failed combined sensor"), ("low", "Sensor loose on its mounting")],
     "Defect la senzorul de rotire / accelerație laterală ESP",
     "Senzorul combinat care măsoară cât de repede se rotește mașina și cât de tare este împinsă lateral dă un semnal "
     "neplauzibil sau are un defect de circuit. Controlul stabilității are nevoie de el, așa că ESP-ul se dezactivează.",
     [("medium", "Cablajul sau conectorul senzorului"), ("medium", "Senzor combinat defect"), ("low", "Senzor slăbit pe suport")]),
    ("5180",
     "No engine-computer data on the network",
     "The ABS/ESP unit is not receiving the messages the engine computer sends over the CAN network. Check the engine "
     "computer for its own faults first.",
     [("medium", "CAN wiring fault between the ABS unit and the engine computer"), ("medium", "Engine computer fault or unpowered"),
      ("low", "Battery voltage dropped during starting")],
     "Lipsesc datele calculatorului de motor din rețea",
     "Unitatea ABS/ESP nu primește mesajele pe care calculatorul motorului le trimite pe rețeaua CAN. Verifică întâi "
     "defectele memorate în calculatorul motorului.",
     [("medium", "Defect de cablaj CAN între unitatea ABS și calculatorul motorului"), ("medium", "Calculatorul motorului defect sau nealimentat"),
      ("low", "Tensiunea bateriei a scăzut la pornire")]),
]
for hexcode, t_en, d_en, c_en, t_ro, d_ro, c_ro in ABS_OTHER:
    code = reader(hexcode)
    add(code, "abs", "two-sources", [AUTODIAG, autoxuga(hexcode)],
        dict(t=t_en, d=d_en + " " + ABS_EN, c=c_en, s=ABS_S_EN),
        dict(t=t_ro, d=d_ro + " " + ABS_RO, c=c_ro, s=ABS_S_RO), LAMP, key=code + "@abs")

# ============================================================================
# Airbag (SRS)
# ============================================================================
SRS_EN = ("While an airbag fault is stored, the affected airbag or pretensioner may not deploy in a crash. "
          "Have it repaired before relying on it. Never probe airbag connectors with a meter: that can fire the airbag.")
SRS_RO = ("Cât timp un defect de airbag este memorat, airbagul sau pretensionatorul afectat s-ar putea să nu se declanșeze la impact. "
          "Repară-l înainte să te bazezi pe el. Nu măsura niciodată conectorii de airbag cu un aparat: poate declanșa airbagul.")
SRS_LAMP_EN = ["Airbag warning light"]
SRS_LAMP_RO = ["Martorul airbag aprins"]


def srs(code, src, title_en, title_ro, d_en, d_ro, causes_en, causes_ro):
    add(code, "airbag", "two-sources", list(src),
        dict(t=title_en, d=d_en + " " + SRS_EN, c=causes_en, s=SRS_LAMP_EN),
        dict(t=title_ro, d=d_ro + " " + SRS_RO, c=causes_ro, s=SRS_LAMP_RO), LAMP, key=code + "@airbag")


srs("B1040", [AUTODIAG, autoxuga("9040"), CLIOSPORT_9040],
    "Airbag warning light circuit fault", "Defect la circuitul martorului airbag",
    "The airbag computer found a fault in the circuit of the airbag warning light, so the light may not warn you about "
    "other airbag faults. Seen on airbag units such as the first-generation Duster; on Logan II and Clio IV a warning-light "
    "fault is reported as B00D2 instead.",
    "Calculatorul airbag a găsit un defect la circuitul martorului airbag, așa că martorul s-ar putea să nu te avertizeze "
    "despre alte defecte de airbag. Apare la unități airbag precum cea a primei generații Duster; pe Logan II și Clio IV "
    "un defect al martorului este raportat în schimb ca B00D2.",
    [("medium", "Warning light or its wiring in the instrument cluster faulty, or the light removed"),
     ("low", "Instrument cluster or airbag computer fault")],
    [("medium", "Martorul sau cablajul lui din bordul de instrumente defect, ori martorul scos"),
     ("low", "Defect al bordului de instrumente sau al calculatorului airbag")])
srs("B107E", [AUTODIAG, autoxuga("907E"), THINKCAR_LOGAN2, CRASHDATA_907F],
    "Airbag computer locked", "Calculatorul airbag blocat",
    "The airbag computer reports that it is locked. Renault workshops lock it with the diagnostic tool before working on "
    "the airbag system, and it has to be unlocked with a Renault-compatible tool afterwards.",
    "Calculatorul airbag raportează că este blocat. Service-urile Renault îl blochează cu aparatul de diagnoză înainte de "
    "a lucra la sistemul airbag, iar după lucrare trebuie deblocat cu un aparat compatibil Renault.",
    [("medium", "Computer left locked after work on the airbags, steering wheel or seats"),
     ("low", "Airbag computer internal fault")],
    [("medium", "Calculatorul a rămas blocat după o lucrare la airbaguri, volan sau scaune"),
     ("low", "Defect intern al calculatorului airbag")])
srs("B107F", [AUTODIAG, autoxuga("907F"), CRASHDATA_907F],
    "Crash recorded: airbag computer must be replaced", "Impact înregistrat: calculatorul airbag trebuie înlocuit",
    "The airbag computer has recorded a crash. Renault's rule is that the computer is replaced after an impact; until then "
    "the airbag system cannot be trusted. Have the whole system checked by a workshop.",
    "Calculatorul airbag a înregistrat un impact. Regula Renault este ca după un impact calculatorul să fie înlocuit; "
    "până atunci nu te poți baza pe sistemul airbag. Cere unui service să verifice tot sistemul.",
    [("medium", "A crash in which airbags or pretensioners fired"), ("low", "Airbag computer internal fault")],
    [("medium", "Un impact în care s-au declanșat airbaguri sau pretensionatoare"), ("low", "Defect intern al calculatorului airbag")])

# ============================================================================
# Electric power steering
# ============================================================================
EPS_EN = ("Steering assistance may be reduced or lost; the car can still be steered, but the wheel is much heavier. "
          "Battery or charging problems can set power-steering faults, so check them first.")
EPS_RO = ("Asistența direcției poate fi redusă sau pierdută; mașina poate fi condusă în continuare, dar volanul devine mult mai greu. "
          "Problemele de baterie sau de încărcare pot genera defecte la servodirecție, așa că verifică-le întâi.")
EPS_S_EN = ["Power steering warning light", "Heavy steering"]
EPS_S_RO = ["Martorul servodirecției aprins", "Volan greu"]

add("C1607", "steering", "two-sources", [AUTODIAG, DEKRA_LOGAN2_A],
    dict(t="Power steering computer memory fault",
         d="The electric power steering computer reports an internal memory or electronic fault. " + EPS_EN,
         c=[("medium", "Power steering computer internal fault"), ("low", "Supply voltage dropped while the computer was working")],
         s=EPS_S_EN),
    dict(t="Defect de memorie al calculatorului servodirecției",
         d="Calculatorul servodirecției electrice raportează un defect intern de memorie sau electronic. " + EPS_RO,
         c=[("medium", "Defect intern al calculatorului servodirecției"), ("low", "Tensiunea de alimentare a scăzut în timp ce calculatorul funcționa")],
         s=EPS_S_RO), LAMP, key="C1607@steering")
add("C1608", "steering", "two-sources", [AUTODIAG, DEKRA_LOGAN2_B, DEKRA_LOGAN2_C],
    dict(t="Power steering computer fault",
         d="The electric power steering computer reports an internal electronic failure. " + EPS_EN,
         c=[("medium", "Power steering computer internal fault"), ("low", "Poor supply or ground connection at the computer")],
         s=EPS_S_EN),
    dict(t="Defect al calculatorului servodirecției",
         d="Calculatorul servodirecției electrice raportează o defecțiune electronică internă. " + EPS_RO,
         c=[("medium", "Defect intern al calculatorului servodirecției"), ("low", "Alimentare sau masă cu contact slab la calculator")],
         s=EPS_S_RO), LAMP, key="C1608@steering")

# ============================================================================
# Instrument cluster
# ============================================================================
add("B1411", "cluster", "two-sources", [DEKRA_LOGAN2_A, DEKRA_LOGAN2_C, THINKCAR_LOGAN2],
    dict(t="Oil pressure sensor circuit fault",
         d="The instrument cluster, which shows the oil-pressure warning, found a fault on the oil-pressure sensor circuit "
           "(on the cars checked, a short to ground). The oil warning light cannot be trusted until this is fixed: check the "
           "oil level, and stop the engine if the oil light comes on while driving.",
         c=[("medium", "Damaged wire or connector at the oil-pressure sensor"), ("medium", "Failed oil-pressure sensor"),
            ("low", "Instrument cluster fault")],
         s=["Oil warning light stays on, or never comes on"]),
    dict(t="Defect la circuitul senzorului de presiune a uleiului",
         d="Bordul de instrumente, care afișează avertizarea de presiune a uleiului, a găsit un defect la circuitul "
           "senzorului de presiune a uleiului (la mașinile verificate, un scurtcircuit la masă). Nu te poți baza pe martorul "
           "de ulei până nu se repară: verifică nivelul uleiului și oprește motorul dacă martorul de ulei se aprinde în mers.",
         c=[("medium", "Fir sau conector deteriorat la senzorul de presiune a uleiului"), ("medium", "Senzor de presiune a uleiului defect"),
            ("low", "Defect al bordului de instrumente")],
         s=["Martorul de ulei rămâne aprins sau nu se aprinde deloc"]),
    LAMP, key="B1411@cluster")
add("B1408", "cluster", "two-sources", [DEKRA_CLIO4, DEKRA_CAPTUR, THINKCAR_SANDERO3],
    dict(t="Seat-belt warning light circuit fault",
         d="The instrument cluster found a fault on the circuit of the seat-belt reminder light, so the reminder may not work.",
         c=[("medium", "Wiring or connector fault"), ("low", "Instrument cluster fault")],
         s=["Seat-belt reminder light does not work correctly"]),
    dict(t="Defect la circuitul martorului de centură",
         d="Bordul de instrumente a găsit un defect la circuitul martorului de centură de siguranță, așa că avertizarea s-ar "
           "putea să nu funcționeze.",
         c=[("medium", "Defect de cablaj sau de conector"), ("low", "Defect al bordului de instrumente")],
         s=["Martorul de centură nu funcționează corect"]),
    key="B1408@cluster")


def main():
    pack = {
        "brand": "renault-dacia",
        "label": "Obdaryn's Renault/Dacia pack",
        "label_ro": "pachetul Obdaryn pentru Renault/Dacia",
        "brandName": "Renault/Dacia",
        "source": {
            "license": "GPL-3.0-or-later (text written for Obdaryn)",
            "note": "Meanings compiled from public Renault fault-finding tables, scan-tool reports of real cars and owner "
                    "forums; each code lists its sources. Renault's internal DF numbers are not used. Verify with a "
                    "workshop before replacing parts.",
            "scope": "2010-2020 Dacia Logan, Sandero, Duster, Lodgy, Dokker; Renault Clio, Megane, Captur, Kangoo",
        },
        "codes": dict(sorted(PACK.items())),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(pack, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"wrote {len(PACK)} codes to {OUT}")


if __name__ == "__main__":
    main()
