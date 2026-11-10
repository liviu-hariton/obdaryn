#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Source of the Ford trouble-code pack.

    python3 tools/brand_packs/ford.py      # writes src/core/data/dtc/brands/ford.json
    python3 tools/check_brand_pack.py src/core/data/dtc/brands/ford.json

Edit THIS file, not the JSON. Families of codes (configuration codes, climate
sensors, ...) are written once and expanded, so English and Romanian stay
consistent.

Scope: 2008-2020 Ford sold in Europe and the US (Focus, Fiesta, Kuga/Escape,
Mondeo/Fusion, C-Max, Transit Connect; EcoBoost, Duratec and Duratorq/TDCi
engines; PowerShift DPS6 and 6F automatics).

Every meaning comes from public sources listed per code. "official" means a
Ford document filed with NHTSA (technical service bulletin, recall or
investigation file on static.nhtsa.gov) states it; "two-sources" means at least
two independent sources agree. Ford workshop-manual pages read through public
mirrors (charm.li for the 2012 Focus, vnx.su for the 2014 Fiesta, reichwein.it
for the 2014 Mustang) count as ordinary sources, not as official ones, the same
way the Hyundai/Kia pack treats its manual mirror. All descriptions are written
for Obdaryn in our own words; nothing is copied from service manuals. Codes whose
meaning was uncertain, contradictory or model-dependent, or whose sources could
only be read through search-engine summaries, were left out on purpose (see
docs/brand-packs.md).

Ford modules report 3-byte codes such as U3000:49. The pack is keyed on the
2-byte code (U3000); the failure-type byte is explained in the text where it
matters.
"""

import json
import pathlib
from urllib.parse import quote

OUT = pathlib.Path(__file__).resolve().parents[2] / "src/core/data/dtc/brands/ford.json"

# ---- Sources ---------------------------------------------------------------
# Ford documents filed with NHTSA (official).
TSB_16_0129 = "https://static.nhtsa.gov/odi/tsbs/2016/MC-10109047-9999.pdf"     # DPS6 TCM: U0100/U0101/U1013/P0606
DPS6_LETTER = "https://static.nhtsa.gov/odi/tsbs/2016/SB-10092576-5448.pdf"     # 14M01/15B22 owner programme, cites TSB 16-0044
RCL_14V284 = "https://static.nhtsa.gov/odi/rcl/2014/RCMN-14V284-2543.pdf"       # 2008-2011 Escape/Mariner EPAS recall 14S05
RCL_15V340 = "https://static.nhtsa.gov/odi/rcl/2015/RCMN-15V340-5844.pdf"       # 2011-2012 Fusion/MKZ/Milan EPAS recall 15S18
PE12017 = "https://static.nhtsa.gov/odi/inv/2012/INRD-PE12017-56154P.pdf"       # Ford EPAS hotline records (U3000:96)
TSB_13_6_17 = "https://static.nhtsa.gov/odi/tsbs/2013/SB-10053471-8297.pdf"     # SYNC, U3000:04 in the APIM
SSM_54288 = "https://static.nhtsa.gov/odi/tsbs/2025/MC-11024273-0001.pdf"       # 1.5L EcoBoost, P1285/P1299

OFFICIAL_PREFIXES = ("https://static.nhtsa.gov/",)
OFFICIAL = {TSB_16_0129, DPS6_LETTER, RCL_14V284, RCL_15V340, PE12017, TSB_13_6_17, SSM_54288}

# Ford code lists and TSB summaries (independent sites).
TCN = "https://www.troublecodes.net/fordcodes/"


def dtcdecode(code):
    return f"https://www.dtcdecode.com/Ford/{code}"


CC_TSB_16_0055 = "https://carcomplaints.com/Ford/Focus/2015/tsbs/tsb-16-0055.shtml"          # P1450, 1.0 EcoBoost
CC_TSB_14_0016 = "https://www.carcomplaints.com/Ford/Escape/2008/tsb/tsb-tsb-14-0016.shtml"   # B2278 in the PSCM
CC_SSM_46048 = "https://www.carcomplaints.com/Ford/Focus/2016/tsbs/tsb-ssm-46048.shtml"       # SYNC 3 APIM codes

# Ford workshop manuals read through public mirrors.
F12 = ("https://charm.li/Ford/2012/Focus%20L4-2.0L/Repair%20and%20Diagnosis/"
       "A%20L%20L%20%20Diagnostic%20Trouble%20Codes%20%28%20DTC%20%29/Testing%20and%20Inspection/"
       "Diagnostic%20Trouble%20Code%20Descriptions/")


def focus2012(*parts):
    """A page of the 2012 Focus workshop manual (DTC section) on charm.li."""
    return F12 + "/".join(quote(p, safe="") for p in parts) + "/"


def focus2012_p(code):
    return focus2012("Diagnostic Trouble Code (DTC) Charts and Descriptions", "P Code Charts", code)


F12_RCM = focus2012("Air Bag Systems", "DTC Chart: Restraint Control Module (RCM)")
F12_HVAC = focus2012("Heating and Air Conditioning", "Climate Control - DATC",
                     "DTC Chart: Heating Ventilation Air Conditioning (HVAC) Module - Dual Automatic Temperature Control (DATC)")
F12_APIM = focus2012("Accessories and Optional Equipment", "Information and Entertainment Systems - Premium",
                     "DTC Chart: Accessory Protocol Interface Module (APIM) - Premium")
F12_ABS = focus2012("Antilock Brakes / Traction Control Systems", "DTC Chart: Anti-Lock Brake System (ABS) Module")
F12_IPC = focus2012("Instrument Panel, Gauges and Warning Indicators", "Instrumentation, Message Center, and Warning Chimes",
                    "DTC Chart: Instrument Panel Cluster (IPC)")


def fiesta2014(page):
    """A page of the 2014 Fiesta workshop manual on vnx.su."""
    return f"https://vnx.su/content/avto/ford/fiesta-2014/Resources/Service_Manual/serg{page}.htm"


FI14_ABS = fiesta2014(1603152)    # 206-09 ABS and stability control, DTC chart
FI14_IPC = fiesta2014(1599890)    # 413-01 instrumentation, IPC DTC chart
FI14_HVAC = fiesta2014(1604706)   # 412-00 climate control (EATC), HVAC DTC chart
FI14_SYNC = fiesta2014(1602409)   # 415-00C SYNC, APIM DTC chart
FI14_RCM_B11D8 = fiesta2014(1603801)  # 501-20B pinpoint test for B11D8
MUSTANG14_SRS = ("https://reichwein.it/manuals/Mustang_2014/Workshop_Manual_Wiring_Diagrams/"
                 "Resources/Service_Manual/seb1kb03.htm")   # 501-20B, RCM and OCS DTC charts


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


def override(key):
    """Standard SAE code where Ford adds real information (which module, what it means there)."""
    PACK[key]["override"] = True


LAMP = ("warning_lamp",)
MIL = ("mil",)

# Ford's configuration and module-health codes, shared by every module. Each module gets its own
# entry (CODE@module), because Obdaryn never uses an entry written for another module.
MODULE_NAMES = {
    # module: (English name, Romanian name)
    "airbag": ("airbag control unit (RCM)", "unitatea de control airbag (RCM)"),
    "climate": ("climate control module", "modulul climatizării"),
    "infotainment": ("SYNC module (APIM)", "modulul SYNC (APIM)"),
    "abs": ("ABS/stability control module", "modulul ABS/controlul stabilității"),
    "cluster": ("instrument cluster", "bordul (instrument cluster)"),
}


def config_codes(module, src_u2100, src_u2101=None, src_u2200=None, extra_en="", extra_ro="", replace=True):
    """U2100 / U2101 / U2200: configuration after module replacement or programming."""
    name_en, name_ro = MODULE_NAMES[module]
    flags = LAMP if module in ("airbag", "abs") else ()
    tail_en = (" " + extra_en) if extra_en else ""
    tail_ro = (" " + extra_ro) if extra_ro else ""
    # Ford's charts for ABS, climate and SYNC replace a module that lost its configuration without any work done on it.
    repl_en = (" If no work was done on the module, Ford's chart treats it as no longer keeping its configuration and replaces it."
               if replace else "")
    repl_ro = (" Dacă la modul nu s-a lucrat, fișa Ford consideră că nu își mai păstrează configurația și îl înlocuiește."
               if replace else "")
    add("U2100", module, "two-sources", src_u2100,
        dict(t="Module configuration not completed",
             d=f"The {name_en} was never given its vehicle configuration (Ford's 'as-built' data), "
               "usually because it was replaced or reprogrammed and the set-up step was skipped or interrupted. "
               "Ford's procedure is to repeat the module set-up (PMI) with a dealer-level tool." + repl_en + tail_en,
             c=[("medium", "Module replaced or reprogrammed without completing the set-up"),
                ("low", "Module that no longer keeps its configuration")],
             s=["Some functions of the module missing or behaving as on another model"]),
        dict(t="Configurarea modulului nu este finalizată",
             d=f"{name_ro[0].upper() + name_ro[1:]} nu a primit configurația mașinii (datele „as-built” Ford), "
               "de obicei după o înlocuire sau o reprogramare la care pasul de configurare a fost sărit sau întrerupt. "
               "Procedura Ford este repetarea configurării modulului (PMI) cu un tester de reprezentanță." + repl_ro + tail_ro,
             c=[("medium", "Modul înlocuit sau reprogramat fără finalizarea configurării"),
                ("low", "Modul care nu își mai păstrează configurația")],
             s=["Unele funcții ale modulului lipsesc sau se comportă ca la alt model"]),
        flags, key="U2100@" + module)
    if src_u2101:
        add("U2101", module, "two-sources", src_u2101,
            dict(t="Module configuration does not match the car",
                 d=f"The configuration stored in the {name_en} doesn't fit this vehicle, typically after a used "
                   "or wrong module was fitted, or after incomplete programming. Ford's fix is to repeat the module "
                   "set-up with the car's as-built data; if no work was done on the module, it is replaced." + tail_en,
                 c=[("medium", "Used or wrong module fitted, or set-up done with the wrong data"),
                    ("low", "Module that no longer keeps its configuration")],
                 s=["Some functions of the module missing or wrong"]),
            dict(t="Configurația modulului nu se potrivește cu mașina",
                 d=f"Configurația pe care o păstrează {name_ro} nu se potrivește cu această mașină, de obicei după montarea "
                   "unui modul second-hand sau greșit ori după o programare incompletă. Soluția Ford este repetarea "
                   "configurării cu datele as-built ale mașinii; dacă la modul nu s-a lucrat, acesta se înlocuiește." + tail_ro,
                 c=[("medium", "Modul second-hand sau greșit, ori configurare făcută cu date greșite"),
                    ("low", "Modul care nu își mai păstrează configurația")],
                 s=["Unele funcții ale modulului lipsesc sau sunt greșite"]),
            flags, key="U2101@" + module)
    if src_u2200:
        add("U2200", module, "two-sources", src_u2200,
            dict(t="Module configuration memory corrupted",
                 d=f"The {name_en} found that the memory holding its vehicle configuration is corrupted. "
                   "Ford's chart calls for a new module, set up afterwards with the car's as-built data." + tail_en,
                 c=[("medium", "Internal memory fault of the module"),
                    ("low", "Interrupted programming or a voltage drop during programming")],
                 s=["Module warning light or message"]),
            dict(t="Memoria de configurare a modulului este coruptă",
                 d=f"{name_ro[0].upper() + name_ro[1:]} a constatat că memoria în care își ține configurația este coruptă. "
                   "Fișa Ford cere un modul nou, configurat apoi cu datele as-built ale mașinii." + tail_ro,
                 c=[("medium", "Defect intern de memorie al modulului"),
                    ("low", "Programare întreruptă sau cădere de tensiune în timpul programării")],
                 s=["Martor sau mesaj de avertizare al modulului"]),
            flags, key="U2200@" + module)


# ============================================================================
# Airbag (RCM)
# ============================================================================
SRS_EN = ("While an airbag fault is stored, the affected airbag or pretensioner may not deploy in a crash. "
          "Have it repaired before relying on it. Never probe airbag connectors with a meter: that can fire the airbag.")
SRS_RO = ("Cât timp un defect de airbag este memorat, airbagul sau pretensionatorul afectat s-ar putea să nu se declanșeze la impact. "
          "Repară-l înainte să te bazezi pe el. Nu măsura niciodată conectorii de airbag cu un aparat: poate declanșa airbagul.")
SRS_LAMP_EN = ["Airbag warning light"]
SRS_LAMP_RO = ["Martorul airbag aprins"]


def srs(code, src, title_en, title_ro, d_en, d_ro, causes_en, causes_ro, symptoms=(SRS_LAMP_EN, SRS_LAMP_RO),
        confidence="two-sources", key=None):
    add(code, "airbag", confidence, list(src),
        dict(t=title_en, d=d_en + " " + SRS_EN, c=causes_en, s=symptoms[0]),
        dict(t=title_ro, d=d_ro + " " + SRS_RO, c=causes_ro, s=symptoms[1]), LAMP, key=key)


srs("B1193", (F12_RCM, MUSTANG14_SRS),
    "Crash data memory full and locked", "Memoria de impact plină și blocată",
    "The airbag control unit (RCM) has stored crash records and its event memory is now full and locked. "
    "Ford treats this code as proof that the car has been in a crash: the RCM and the crash sensors are replaced, "
    "together with any parts that deployed. It cannot simply be cleared.",
    "Unitatea de control airbag (RCM) a memorat înregistrări de impact, iar memoria de evenimente este acum plină și blocată. "
    "Ford tratează acest cod ca dovadă că mașina a avut un accident: se înlocuiesc RCM-ul și senzorii de impact, "
    "împreună cu orice componentă declanșată. Codul nu se poate pur și simplu șterge.",
    [("medium", "Previous crash recorded by the airbag unit"), ("low", "Used airbag unit taken from a crashed car")],
    [("medium", "Accident anterior înregistrat de unitatea airbag"), ("low", "Unitate airbag second-hand provenită de la o mașină accidentată")])

PRETENSIONER_CAUSES_EN = [("medium", "Connector under the front seat loose or corroded"), ("medium", "Wiring under the seat (chafed when the seat moves)"),
                          ("low", "The seat-belt pretensioner itself"), ("low", "Wrong or incompatible part fitted")]
PRETENSIONER_CAUSES_RO = [("medium", "Conectorul de sub scaunul din față slăbit sau oxidat"), ("medium", "Cablajul de sub scaun (ros la mișcarea scaunului)"),
                          ("low", "Pretensionerul centurii în sine"), ("low", "Piesă greșită sau incompatibilă montată")]
for code, side_en, side_ro, short_ro in (("B1211", "driver", "șoferului", "șofer"),
                                         ("B1214", "front passenger", "pasagerului din față", "pasager față")):
    srs(code, (F12_RCM, MUSTANG14_SRS),
        f"Seat-belt pretensioner firing circuit, {side_en}", f"Circuit de declanșare pretensioner centură, {short_ro}",
        f"The airbag control unit found a fault in the firing circuit of the {side_en}'s seat-belt pretensioner "
        "(the charge that tightens the belt in a crash): a short to ground or to battery, an open circuit, too low a resistance, "
        "a circuit crossed with another one, or the wrong part fitted. The last two characters of the full Ford code say which.",
        f"Unitatea de control airbag a găsit un defect în circuitul de declanșare al pretensionatorului centurii {side_ro} "
        "(încărcătura care strânge centura la impact): scurtcircuit la masă sau la plus, circuit întrerupt, rezistență prea mică, "
        "circuit încrucișat cu altul sau piesă greșită montată. Ultimele două caractere ale codului Ford complet arată care dintre ele.",
        PRETENSIONER_CAUSES_EN, PRETENSIONER_CAUSES_RO)

srs("B11D8", (F12_RCM, FI14_RCM_B11D8),
    "Crash signal circuit to the fuel cut-off faulty", "Circuitul semnalului de impact către întreruperea combustibilului defect",
    "The airbag control unit sends a crash signal to the fuel pump module or the engine computer so that fuel is cut off after a crash. "
    "It found this signal wire shorted to battery, shorted to ground or open. On the Fiesta, Ford notes that this code does not "
    "turn on the airbag light, so it is easy to miss; the automatic fuel cut-off after a crash may not work until it is fixed.",
    "Unitatea de control airbag trimite un semnal de impact către modulul pompei de combustibil sau calculatorul motorului, "
    "ca alimentarea cu combustibil să fie oprită după un accident. A găsit firul acestui semnal în scurtcircuit la plus, "
    "la masă sau întrerupt. La Fiesta, Ford precizează că acest cod nu aprinde martorul airbag, așa că trece ușor neobservat; "
    "oprirea automată a combustibilului după impact s-ar putea să nu funcționeze până la reparare.",
    [("medium", "Wiring or connectors between the airbag unit and the fuel pump module or engine computer"),
     ("low", "Fuel pump module or engine computer"), ("low", "Airbag control unit")],
    [("medium", "Cablaj sau conectori între unitatea airbag și modulul pompei de combustibil ori calculatorul motorului"),
     ("low", "Modulul pompei de combustibil sau calculatorul motorului"), ("low", "Unitatea de control airbag")],
    symptoms=(["Often no warning light at all"], ["Adesea niciun martor aprins"]))

config_codes("airbag", [F12_RCM, MUSTANG14_SRS], src_u2200=[F12_RCM, MUSTANG14_SRS],
             extra_en=SRS_EN, extra_ro=SRS_RO, replace=False)

srs("U3000", (F12_RCM, MUSTANG14_SRS),
    "Airbag control unit internal fault", "Defect intern al unității de control airbag",
    "U3000 is the standard 'control module' code; in Ford's airbag unit (RCM) it reports a fault of the unit itself. "
    "With failure type 46 (calibration memory) or 49 (internal electronics) Ford's chart calls for a new RCM, set up with the car's "
    "as-built data; type 4A means a wrong part number is fitted.",
    "U3000 este codul standard pentru „modul de control”; la unitatea airbag Ford (RCM) raportează un defect al unității în sine. "
    "La tipul de defect 46 (memoria de calibrare) sau 49 (electronica internă), fișa Ford cere un RCM nou, configurat cu datele "
    "as-built ale mașinii; tipul 4A înseamnă că este montată o piesă cu cod greșit.",
    [("medium", "Internal fault of the airbag control unit"), ("low", "Wrong or second-hand airbag unit fitted")],
    [("medium", "Defect intern al unității de control airbag"), ("low", "Unitate airbag greșită sau second-hand montată")],
    key="U3000@airbag")
override("U3000@airbag")


# ============================================================================
# Steering (PSCM)
# ============================================================================
STEER_EN = ("Without assist the car can still be steered, but it needs much more effort, especially when parking or at low speed.")
STEER_RO = ("Fără asistență mașina se poate conduce în continuare, dar volanul cere mult mai mult efort, mai ales la parcare sau la viteză mică.")

add("U3000", "steering", "official", [PE12017, RCL_15V340],
    dict(t="Power steering module or steering gear internal fault",
         d="U3000 is the standard 'control module' code; Ford's electric power steering module (PSCM) uses it to report a fault "
           "inside the module or the electric steering gear it is built into. Ford's technical hotline states that a U3000:96 that "
           "keeps coming back means the electric steering gear has an internal failure and the gear is replaced, and Ford recall 15S18 "
           "(2011-2012 Fusion and related models) lists several U3000 failure types among the codes that call for a new steering gear. " + STEER_EN,
         c=[("medium", "Internal fault of the electric steering gear or its control module"),
            ("low", "Connector at the steering gear loose or unplugged")],
         s=["Loss of power steering assist", "Steering warning light or message"]),
    dict(t="Defect intern al modulului servodirecției sau al casetei de direcție",
         d="U3000 este codul standard pentru „modul de control”; modulul servodirecției electrice Ford (PSCM) îl folosește pentru a raporta "
           "un defect în interiorul modulului sau al casetei de direcție electrice în care este integrat. Linia tehnică Ford precizează că un "
           "U3000:96 care revine înseamnă un defect intern al casetei de direcție electrice, iar caseta se înlocuiește; rechemarea Ford 15S18 "
           "(Fusion 2011-2012 și modele înrudite) include mai multe tipuri de U3000 printre codurile care cer o casetă nouă. " + STEER_RO,
         c=[("medium", "Defect intern al casetei de direcție electrice sau al modulului ei"),
            ("low", "Conector la caseta de direcție slăbit sau decuplat")],
         s=["Pierderea asistenței la direcție", "Martor sau mesaj de avertizare pentru direcție"]),
    LAMP, key="U3000@steering")
override("U3000@steering")

add("B1342", "steering", "official", [RCL_14V284, MUSTANG14_SRS, dtcdecode("B1342")],
    dict(t="Power steering module internal fault (ECU faulted)",
         d="B1342 is Ford's long-standing code for 'the module has found a fault in itself'. In the power steering module of the "
           "2008-2011 Escape (and Mercury Mariner), whose electric assist sits on the steering column, Ford recall 14S05 replaces the "
           "whole steering column when B1342 is stored. " + STEER_EN,
         c=[("medium", "Internal fault of the power steering module / electric steering column")],
         s=["Loss of power steering assist", "Steering or wrench warning light"]),
    dict(t="Defect intern al modulului servodirecției (ECU defect)",
         d="B1342 este codul Ford folosit de mult timp pentru „modulul a găsit un defect în el însuși”. La modulul servodirecției de la "
           "Escape 2008-2011 (și Mercury Mariner), unde asistența electrică este pe coloana volanului, rechemarea Ford 14S05 înlocuiește "
           "întreaga coloană de direcție când B1342 este memorat. " + STEER_RO,
         c=[("medium", "Defect intern al modulului servodirecției / coloanei de direcție electrice")],
         s=["Pierderea asistenței la direcție", "Martorul de direcție sau cel cu cheie fixă aprins"]),
    LAMP, key="B1342@steering")

add("B2278", "steering", "official", [RCL_14V284, CC_TSB_14_0016],
    dict(t="Steering torque sensor fault (column-mounted electric steering)",
         d="Stored in the power steering module of the 2008-2012 Escape and 2008-2011 Mariner, whose electric assist sits on the steering "
           "column. The module uses the torque sensor to measure how hard the driver turns the wheel. When B2278 is the only code, Ford's "
           "recall 14S05 and bulletin 14-0016 replace just the torque sensor, not the whole column. " + STEER_EN,
         c=[("medium", "Steering torque sensor"), ("low", "Torque sensor wiring or connector")],
         s=["Loss of power steering assist, sometimes intermittent", "Steering or wrench warning light"]),
    dict(t="Defect la senzorul de cuplu al direcției (servodirecție electrică pe coloană)",
         d="Memorat în modulul servodirecției la Escape 2008-2012 și Mariner 2008-2011, unde asistența electrică este pe coloana volanului. "
           "Modulul folosește senzorul de cuplu ca să măsoare cât de tare rotește șoferul volanul. Când B2278 este singurul cod, rechemarea "
           "Ford 14S05 și buletinul 14-0016 înlocuiesc doar senzorul de cuplu, nu toată coloana. " + STEER_RO,
         c=[("medium", "Senzorul de cuplu al direcției"), ("low", "Cablajul sau conectorul senzorului de cuplu")],
         s=["Pierderea asistenței la direcție, uneori intermitent", "Martorul de direcție sau cel cu cheie fixă aprins"]),
    LAMP, key="B2278@steering")


# ============================================================================
# Transmission: PowerShift DPS6 (Fiesta 2011-2016, Focus 2012-2016)
# ============================================================================
DPS6_SYMPTOMS_EN = ["No start, or the car won't move off", "Loss of power while driving", "Gear indicator flashing on the cluster",
                    "'Hill start assist not available' or parking-aid messages", "Check-engine light"]
DPS6_SYMPTOMS_RO = ["Mașina nu pornește sau nu pleacă de pe loc", "Pierdere de putere în mers", "Indicatorul treptei clipește pe bord",
                    "Mesaje „Hill start assist indisponibil” sau despre senzorii de parcare", "Martorul de motor aprins"]

add("P0606", "transmission", "official", [TSB_16_0129, DPS6_LETTER],
    dict(t="PowerShift transmission computer internal failure",
         d="P0606 is the standard 'control module processor' code. On the DPS6 PowerShift gearbox, Ford's updated software uses it in the "
           "transmission computer (TCM) to flag an internal TCM failure. Ford bulletin 16-0129 replaces the TCM when P0606, U0100, U0101 "
           "or U1013 are stored, after first fixing any low-voltage problem, then reprograms it and runs the clutch adaptive learning.",
         c=[("medium", "Internal failure of the transmission computer (TCM)"), ("low", "Low battery voltage or poor TCM power/ground")],
         s=DPS6_SYMPTOMS_EN),
    dict(t="Defect intern al calculatorului cutiei PowerShift",
         d="P0606 este codul standard pentru „procesorul modulului de control”. La cutia DPS6 PowerShift, software-ul actualizat Ford îl "
           "folosește în calculatorul cutiei (TCM) pentru a semnala un defect intern al TCM-ului. Buletinul Ford 16-0129 înlocuiește TCM-ul "
           "când sunt memorate P0606, U0100, U0101 sau U1013, după ce se rezolvă mai întâi orice problemă de tensiune scăzută, apoi îl "
           "reprogramează și face învățarea adaptivă a ambreiajelor.",
         c=[("medium", "Defect intern al calculatorului cutiei (TCM)"), ("low", "Tensiune scăzută a bateriei sau alimentare/masă proastă la TCM")],
         s=DPS6_SYMPTOMS_RO),
    MIL, key="P0606@transmission")
override("P0606@transmission")

add("U1013", "engine", "two-sources", [dtcdecode("U1013"), TSB_16_0129, DPS6_LETTER],
    dict(t="Invalid self-check data from the transmission computer",
         d="The engine computer (PCM) received invalid internal monitoring data from the transmission computer (TCM). On cars with the "
           "DPS6 PowerShift gearbox, Ford bulletin 16-0129 links U1013, often together with U0100, U0101 or P0606, to a failed TCM "
           "and replaces it.",
         c=[("medium", "Failed PowerShift transmission computer (TCM)"), ("low", "Low battery voltage or TCM wiring")],
         s=DPS6_SYMPTOMS_EN),
    dict(t="Date de autocontrol invalide de la calculatorul cutiei",
         d="Calculatorul motorului (PCM) a primit date interne de monitorizare invalide de la calculatorul cutiei (TCM). La mașinile cu "
           "cutie DPS6 PowerShift, buletinul Ford 16-0129 leagă U1013, adesea împreună cu U0100, U0101 sau P0606, de un TCM defect "
           "și îl înlocuiește.",
         c=[("medium", "Calculatorul cutiei PowerShift (TCM) defect"), ("low", "Tensiune scăzută a bateriei sau cablajul TCM-ului")],
         s=DPS6_SYMPTOMS_RO),
    MIL)


# ============================================================================
# Climate control (HVAC module, automatic climate)
# ============================================================================
CLIM_SRC = [F12_HVAC, FI14_HVAC]
CLIMATE = [
    # code, title en, title ro, description en, description ro, causes en, causes ro, symptoms en, symptoms ro
    ("B10B5", "Left vent air temperature sensor fault", "Defect la senzorul de temperatură a aerului de la gurile de ventilație stânga",
     "The sensor that measures the temperature of the air leaving the left-hand dash vents reads out of range (short to ground, or open/short to battery).",
     "Senzorul care măsoară temperatura aerului ce iese prin gurile de ventilație din stânga planșei citește în afara limitelor (scurtcircuit la masă sau întrerupt/scurtcircuit la plus).",
     "Discharge air temperature sensor or its wiring", "Senzorul de temperatură a aerului refulat sau cablajul lui",
     "Automatic temperature control less accurate", "Reglajul automat al temperaturii mai puțin precis"),
    ("B10B6", "Left floor air temperature sensor fault", "Defect la senzorul de temperatură a aerului spre picioare stânga",
     "The sensor that measures the temperature of the air leaving the left-hand floor ducts reads out of range (short to ground, or open/short to battery).",
     "Senzorul care măsoară temperatura aerului ce iese prin gurile spre picioare din stânga citește în afara limitelor (scurtcircuit la masă sau întrerupt/scurtcircuit la plus).",
     "Discharge air temperature sensor or its wiring", "Senzorul de temperatură a aerului refulat sau cablajul lui",
     "Automatic temperature control less accurate", "Reglajul automat al temperaturii mai puțin precis"),
    ("B1081", "Left temperature blend door motor fault", "Defect la motorașul clapetei de amestec temperatură stânga",
     "The climate module found a fault in the small motor that moves the left (or only) temperature blend door, which mixes hot and cold air.",
     "Modulul climatizării a găsit un defect la motorașul care mișcă clapeta de amestec din stânga (sau singura), cea care amestecă aerul cald cu cel rece.",
     "Blend door motor (actuator) or its wiring", "Motorașul (actuatorul) clapetei sau cablajul lui",
     "Air temperature doesn't change, or stays hot or cold", "Temperatura aerului nu se schimbă sau rămâne caldă ori rece"),
    ("B1083", "Recirculation door motor fault", "Defect la motorașul clapetei de recirculare",
     "The climate module found a fault in the motor that moves the air inlet door between outside air and recirculation.",
     "Modulul climatizării a găsit un defect la motorașul care mută clapeta de admisie între aer din exterior și recirculare.",
     "Recirculation door motor (actuator) or its wiring", "Motorașul (actuatorul) clapetei de recirculare sau cablajul lui",
     "Recirculation doesn't switch, windows may mist up", "Recircularea nu comută, geamurile se pot aburi"),
    ("B1086", "Air distribution door motor fault", "Defect la motorașul clapetei de distribuție a aerului",
     "The climate module found a fault in the motor that sends the air to the windscreen, face vents or footwell.",
     "Modulul climatizării a găsit un defect la motorașul care dirijează aerul spre parbriz, gurile de la față sau spre picioare.",
     "Distribution door motor (actuator) or its wiring", "Motorașul (actuatorul) clapetei de distribuție sau cablajul lui",
     "Air comes out of the wrong vents", "Aerul iese pe alte guri decât cele alese"),
    ("B10B9", "Blower motor control fault", "Defect la comanda ventilatorului habitaclului",
     "The climate module found a fault in the control circuit of the cabin blower (short to battery, or short to ground/open).",
     "Modulul climatizării a găsit un defect în circuitul de comandă al ventilatorului habitaclului (scurtcircuit la plus sau scurtcircuit la masă/întrerupt).",
     "Blower motor speed controller or its wiring", "Regulatorul de turație al ventilatorului sau cablajul lui",
     "Blower doesn't run, or runs at the wrong speed", "Ventilatorul nu merge sau merge cu altă turație"),
    ("B1A61", "Cabin temperature sensor fault", "Defect la senzorul de temperatură din habitaclu",
     "The in-car temperature sensor used by the automatic climate control reads out of range.",
     "Senzorul de temperatură din habitaclu, folosit de climatizarea automată, citește în afara limitelor.",
     "Cabin temperature sensor or its wiring", "Senzorul de temperatură din habitaclu sau cablajul lui",
     "Automatic mode doesn't hold the set temperature", "Modul automat nu menține temperatura setată"),
    ("B105A", "Cabin temperature sensor fan fault", "Defect la ventilatorul senzorului de temperatură din habitaclu",
     "The small fan that draws cabin air over the in-car temperature sensor has an electrical fault, so the sensor no longer reads the real cabin temperature.",
     "Ventilatorul mic care trage aerul din habitaclu peste senzorul de temperatură are un defect electric, așa că senzorul nu mai citește temperatura reală.",
     "Sensor fan or its wiring", "Ventilatorul senzorului sau cablajul lui",
     "Automatic mode doesn't hold the set temperature", "Modul automat nu menține temperatura setată"),
    ("B1A63", "Right sunload sensor fault", "Defect la senzorul de radiație solară dreapta",
     "The right-hand sunload sensor on top of the dash, which tells the automatic climate control how strong the sun is, reads out of range.",
     "Senzorul de radiație solară din dreapta, de pe planșa de bord, care spune climatizării automate cât de puternic e soarele, citește în afara limitelor.",
     "Sunload sensor or its wiring", "Senzorul de radiație solară sau cablajul lui",
     "Automatic mode reacts poorly to sunshine", "Modul automat reacționează greșit la soare"),
    ("B1A64", "Left sunload sensor fault", "Defect la senzorul de radiație solară stânga",
     "The left-hand sunload sensor on top of the dash, which tells the automatic climate control how strong the sun is, reads out of range.",
     "Senzorul de radiație solară din stânga, de pe planșa de bord, care spune climatizării automate cât de puternic e soarele, citește în afara limitelor.",
     "Sunload sensor or its wiring", "Senzorul de radiație solară sau cablajul lui",
     "Automatic mode reacts poorly to sunshine", "Modul automat reacționează greșit la soare"),
    ("B10B8", "Climate control push-button stuck", "Buton al climatizării blocat",
     "A button on the climate control panel has been seen pressed for too long, as if stuck or blocked. In Ford's chart the fix is a new climate control module.",
     "Un buton de pe panoul climatizării a fost văzut apăsat prea mult timp, ca și cum ar fi blocat. În fișa Ford soluția este un modul nou al climatizării.",
     "Stuck or jammed button on the climate control panel", "Buton blocat sau înțepenit pe panoul climatizării",
     "One button doesn't respond", "Un buton nu răspunde"),
]
for code, t_en, t_ro, d_en, d_ro, c_en, c_ro, s_en, s_ro in CLIMATE:
    add(code, "climate", "two-sources", CLIM_SRC,
        dict(t=t_en, d=d_en, c=[("medium", c_en)], s=[s_en]),
        dict(t=t_ro, d=d_ro, c=[("medium", c_ro)], s=[s_ro]))

config_codes("climate", CLIM_SRC, src_u2101=CLIM_SRC)
add("U3000", "climate", "two-sources", CLIM_SRC,
    dict(t="Climate control module internal fault",
         d="U3000 is the standard 'control module' code; Ford's climate control module uses it for a fault inside the module. "
           "Ford's chart for a U3000 without a failure type calls for a new climate control module.",
         c=[("medium", "Internal fault of the climate control module")], s=["Climate control not working correctly"]),
    dict(t="Defect intern al modulului climatizării",
         d="U3000 este codul standard pentru „modul de control”; modulul climatizării Ford îl folosește pentru un defect în interiorul modulului. "
           "Fișa Ford pentru un U3000 fără tip de defect cere un modul nou al climatizării.",
         c=[("medium", "Defect intern al modulului climatizării")], s=["Climatizarea nu funcționează corect"]),
    key="U3000@climate")
override("U3000@climate")


# ============================================================================
# Infotainment (SYNC, APIM)
# ============================================================================
SYNC_SRC = [F12_APIM, FI14_SYNC]
add("B1252", "infotainment", "two-sources", SYNC_SRC,
    dict(t="SYNC USB port fault",
         d="The SYNC module (APIM) found a fault on its main USB port: an internal failure, a short to ground or an open circuit.",
         c=[("medium", "USB socket, its extension cable or connector"), ("low", "SYNC module")],
         s=["USB device not recognised or not charging"]),
    dict(t="Defect la portul USB SYNC",
         d="Modulul SYNC (APIM) a găsit un defect la portul USB principal: defect intern, scurtcircuit la masă sau circuit întrerupt.",
         c=[("medium", "Mufa USB, cablul ei prelungitor sau conectorul"), ("low", "Modulul SYNC")],
         s=["Dispozitivul USB nu este recunoscut sau nu se încarcă"]))
add("B12B8", "infotainment", "two-sources", SYNC_SRC,
    dict(t="SYNC second USB port fault",
         d="The SYNC module (APIM) found a fault on its second USB port. On the 2012 Focus this port is on the APIM itself and isn't used: "
           "Ford's chart says to check it for damage or debris, clear the code and replace the APIM only if the code comes back.",
         c=[("medium", "Debris or damage in the USB port"), ("low", "SYNC module")], s=["Often no symptom"]),
    dict(t="Defect la al doilea port USB SYNC",
         d="Modulul SYNC (APIM) a găsit un defect la al doilea port USB. La Focus 2012 acest port este chiar pe APIM și nu este folosit: "
           "fișa Ford cere verificarea lui pentru deteriorări sau impurități, ștergerea codului și înlocuirea APIM doar dacă revine.",
         c=[("medium", "Impurități sau deteriorări în portul USB"), ("low", "Modulul SYNC")], s=["Adesea fără simptome"]))
add("B108E", "infotainment", "two-sources", [FI14_SYNC, CC_SSM_46048],
    dict(t="SYNC display fault",
         d="The SYNC module (APIM) reports an electrical fault with the display it drives. For SYNC 3 cars (2016-2017) Ford's "
           "special service message 46048 says that if this code shows up with no symptoms, it is cleared and only investigated if it returns.",
         c=[("medium", "Display or its wiring/connector"), ("low", "Temporary fault that doesn't return")],
         s=["Blank or flickering screen", "Sometimes no symptom"]),
    dict(t="Defect la ecranul SYNC",
         d="Modulul SYNC (APIM) raportează un defect electric la ecranul pe care îl comandă. Pentru mașinile cu SYNC 3 (2016-2017), "
           "mesajul de service Ford 46048 spune că, dacă acest cod apare fără simptome, se șterge și se investighează doar dacă revine.",
         c=[("medium", "Ecranul sau cablajul/conectorul lui"), ("low", "Defect temporar care nu revine")],
         s=["Ecran negru sau care pâlpâie", "Uneori fără simptome"]))
add("U1A00", "infotainment", "two-sources", [FI14_SYNC, CC_SSM_46048],
    dict(t="SYNC private network message missing",
         d="The SYNC module (APIM) is missing a message on the private link it shares with its own display and controls. For SYNC 3 cars "
           "(2016-2017) Ford's special service message 46048 says that if this code shows up with no symptoms, it is cleared and only "
           "investigated if it returns.",
         c=[("medium", "Wiring or connector between the SYNC module and its display/controls"), ("low", "Temporary fault that doesn't return")],
         s=["Screen or SYNC controls not responding", "Sometimes no symptom"]),
    dict(t="Lipsește un mesaj pe rețeaua privată SYNC",
         d="Modulului SYNC (APIM) îi lipsește un mesaj pe legătura privată cu propriul ecran și propriile comenzi. Pentru mașinile cu SYNC 3 "
           "(2016-2017), mesajul de service Ford 46048 spune că, dacă acest cod apare fără simptome, se șterge și se investighează doar dacă revine.",
         c=[("medium", "Cablaj sau conector între modulul SYNC și ecranul/comenzile lui"), ("low", "Defect temporar care nu revine")],
         s=["Ecranul sau comenzile SYNC nu răspund", "Uneori fără simptome"]))
config_codes("infotainment", SYNC_SRC)
add("U3000", "infotainment", "official", [TSB_13_6_17, F12_APIM, FI14_SYNC],
    dict(t="SYNC module internal fault",
         d="U3000 is the standard 'control module' code; Ford's SYNC module (APIM) uses it for a fault inside the module. "
           "Ford bulletin 13-6-17 (2012-2014 Fiesta, Focus, Fusion, C-MAX, Escape and others with SYNC, not MyFord Touch) lists U3000:04 "
           "with SYNC slow or not responding and fixes it by updating the APIM software. For types 41 and 42 (checksum or memory) Ford's "
           "chart clears the code and replaces the APIM only if it returns; type 88 (bus off) is a passing network problem, usually wiring or "
           "a low battery.",
         c=[("medium", "SYNC software that needs updating"), ("low", "Internal fault of the SYNC module"),
            ("low", "Low battery voltage or damaged wiring")],
         s=["SYNC slow, partly or completely inoperative", "Phone connection drops"]),
    dict(t="Defect intern al modulului SYNC",
         d="U3000 este codul standard pentru „modul de control”; modulul SYNC Ford (APIM) îl folosește pentru un defect în interiorul modulului. "
           "Buletinul Ford 13-6-17 (Fiesta, Focus, Fusion, C-MAX, Escape 2012-2014 și altele cu SYNC, fără MyFord Touch) asociază U3000:04 "
           "cu un SYNC lent sau care nu răspunde și îl rezolvă prin actualizarea software-ului APIM. La tipurile 41 și 42 (sumă de control sau "
           "memorie), fișa Ford șterge codul și înlocuiește APIM doar dacă revine; tipul 88 (bus off) este o problemă trecătoare de rețea, "
           "de obicei cablaj sau baterie descărcată.",
         c=[("medium", "Software SYNC care trebuie actualizat"), ("low", "Defect intern al modulului SYNC"),
            ("low", "Tensiune scăzută a bateriei sau cablaj deteriorat")],
         s=["SYNC lent, parțial sau complet nefuncțional", "Conexiunea cu telefonul se întrerupe"]),
    key="U3000@infotainment")
override("U3000@infotainment")


# ============================================================================
# Engine (PCM): Ford P1xxx codes
# ============================================================================
add("P1000", "engine", "two-sources", [dtcdecode("P1000"), TCN],
    dict(t="OBD readiness tests not finished yet",
         d="Ford's engine computer reports P1000 while one or more of its emission self-tests (readiness monitors) have not yet run "
           "since the codes were cleared, the battery was disconnected or the computer was reprogrammed. It is a status message, not a "
           "fault: it goes away by itself after enough normal driving in varied conditions.",
         c=[("medium", "Codes recently cleared, battery disconnected or flat, or engine computer reprogrammed"),
            ("low", "Another fault keeping one of the self-tests from running")],
         s=["No warning light", "An emissions test may be refused until the monitors are ready"]),
    dict(t="Testele de pregătire OBD nu s-au încheiat încă",
         d="Calculatorul motorului Ford raportează P1000 cât timp unul sau mai multe dintre autotestele de emisii (monitoarele de "
           "pregătire) nu au rulat de la ștergerea codurilor, deconectarea bateriei sau reprogramarea calculatorului. Este un mesaj de "
           "stare, nu un defect: dispare singur după suficient condus normal, în condiții variate.",
         c=[("medium", "Coduri șterse recent, baterie deconectată sau descărcată ori calculator reprogramat"),
            ("low", "Alt defect care împiedică rularea unuia dintre autoteste")],
         s=["Niciun martor aprins", "Inspecția de emisii poate fi refuzată până când monitoarele sunt gata"]))

add("P1285", "engine", "two-sources", [focus2012_p("P1285"), dtcdecode("P1285"), TCN, SSM_54288],
    dict(t="Cylinder head over-temperature",
         d="The cylinder head temperature (CHT) sensor saw the engine overheat. Many Ford engines measure engine temperature in the "
           "cylinder head instead of the coolant, and the cluster may show an overheating warning or push the gauge to H. Stop as soon "
           "as it is safe, switch the engine off and let it cool before checking the coolant level; driving on can crack the head.",
         c=[("medium", "Low coolant level (leak)"), ("medium", "Cooling system fault: thermostat, water pump, radiator fan, blocked radiator"),
            ("low", "Cylinder head temperature sensor"), ("low", "Internal engine fault such as a head gasket")],
         s=["Engine temperature warning or gauge at H", "Check-engine light"]),
    dict(t="Supraîncălzirea chiulasei",
         d="Senzorul de temperatură a chiulasei (CHT) a văzut că motorul s-a supraîncălzit. Multe motoare Ford măsoară temperatura "
           "motorului în chiulasă, nu în lichidul de răcire, iar bordul poate afișa avertizarea de supraîncălzire sau poate duce acul la H. "
           "Oprește cât de curând poți în siguranță, oprește motorul și lasă-l să se răcească înainte să verifici nivelul lichidului "
           "de răcire; dacă mergi mai departe, chiulasa se poate fisura.",
         c=[("medium", "Nivel scăzut al lichidului de răcire (scurgere)"),
            ("medium", "Defect la răcire: termostat, pompă de apă, ventilatorul radiatorului, radiator înfundat"),
            ("low", "Senzorul de temperatură a chiulasei"), ("low", "Defect intern al motorului, de exemplu garnitura de chiulasă")],
         s=["Avertizare de temperatură motor sau acul la H", "Martorul de motor aprins"]),
    MIL + LAMP)

add("P1450", "engine", "two-sources", [dtcdecode("P1450"), TCN, CC_TSB_16_0055],
    dict(t="EVAP system: fuel tank vacuum does not bleed off",
         d="During its evaporative-emission test the engine computer pulls a slight vacuum in the fuel tank and then expects it to "
           "disappear when the canister vent opens. With P1450 the vacuum stays, which points to a blocked vent path. On early "
           "2015-2016 Focus 1.0 EcoBoost cars (built up to 19 October 2015) Ford bulletin 16-0055 fixes it with an engine computer update.",
         c=[("medium", "Canister vent valve stuck closed, or its filter blocked"), ("medium", "Kinked or blocked vent hose, dirt in the canister"),
            ("low", "Fuel tank pressure sensor"), ("low", "Engine computer software (early 1.0 EcoBoost Focus)")],
         s=["Check-engine light"]),
    dict(t="Sistemul EVAP: vacuumul din rezervor nu dispare",
         d="În timpul testului de emisii evaporative, calculatorul motorului creează un vacuum ușor în rezervor și se așteaptă ca acesta "
           "să dispară când se deschide aerisirea canistrului. La P1450 vacuumul rămâne, ceea ce indică o cale de aerisire blocată. "
           "La primele Focus 1.0 EcoBoost 2015-2016 (fabricate până la 19 octombrie 2015), buletinul Ford 16-0055 rezolvă problema "
           "printr-o actualizare a calculatorului motorului.",
         c=[("medium", "Supapa de aerisire a canistrului blocată închisă sau filtrul ei înfundat"),
            ("medium", "Furtun de aerisire strivit sau înfundat, murdărie în canistru"),
            ("low", "Senzorul de presiune din rezervor"), ("low", "Software-ul calculatorului motorului (primele Focus 1.0 EcoBoost)")],
         s=["Martorul de motor aprins"]),
    MIL)

add("P1635", "engine", "two-sources", [dtcdecode("P1635"), TCN],
    dict(t="Tyre size / axle ratio setting out of range",
         d="The tyre size or final-drive ratio stored in the engine computer is outside the values it accepts. The computer uses it to "
           "work out road speed, so speedometer, cruise control and shifting can be affected. It usually follows a module replacement or "
           "a configuration change that wasn't completed correctly.",
         c=[("medium", "Wrong or missing vehicle configuration in the engine computer"), ("low", "Engine computer replaced without full set-up")],
         s=["Speedometer, cruise control or shifting may be affected"]),
    dict(t="Valoarea dimensiunii anvelopelor / raportului de transmisie în afara limitelor",
         d="Dimensiunea anvelopelor sau raportul transmisiei finale memorate în calculatorul motorului sunt în afara valorilor acceptate. "
           "Calculatorul le folosește ca să calculeze viteza, așa că vitezometrul, pilotul automat și schimbarea treptelor pot fi afectate. "
           "De obicei apare după înlocuirea unui modul sau după o modificare de configurare neterminată corect.",
         c=[("medium", "Configurație greșită sau lipsă în calculatorul motorului"), ("low", "Calculatorul motorului înlocuit fără configurare completă")],
         s=["Vitezometrul, pilotul automat sau schimbarea treptelor pot fi afectate"]))

add("P1639", "engine", "two-sources", [dtcdecode("P1639"), TCN],
    dict(t="Vehicle ID block corrupted or not programmed",
         d="The vehicle identification block in the engine computer, which holds data such as the VIN and the car's configuration, is "
           "missing or corrupted. It is typically seen after the engine computer was replaced or reprogrammed and the set-up was not finished.",
         c=[("medium", "Engine computer replaced or reprogrammed without completing the set-up"), ("low", "Internal memory fault of the engine computer")],
         s=["Some functions may behave as on another model"]),
    dict(t="Blocul de identificare a vehiculului corupt sau neprogramat",
         d="Blocul de identificare din calculatorul motorului, care conține date precum seria de șasiu (VIN) și configurația mașinii, lipsește "
           "sau este corupt. Apare de obicei după înlocuirea sau reprogramarea calculatorului motorului fără finalizarea configurării.",
         c=[("medium", "Calculatorul motorului înlocuit sau reprogramat fără finalizarea configurării"), ("low", "Defect intern de memorie al calculatorului motorului")],
         s=["Unele funcții se pot comporta ca la alt model"]))

# ============================================================================
# ABS / stability control
# ============================================================================
ABS_SRC = [F12_ABS, FI14_ABS]
ABS_EN = "Ford's ABS module and hydraulic unit (HCU) are serviced as one assembly. While the fault is active, ABS and stability control may be switched off; normal braking still works."
ABS_RO = "Modulul ABS și unitatea hidraulică (HCU) Ford se înlocuiesc ca un singur ansamblu. Cât timp defectul este activ, ABS-ul și controlul stabilității pot fi dezactivate; frânarea normală funcționează în continuare."

add("B10DF", "abs", "two-sources", ABS_SRC,
    dict(t="ABS master cylinder isolation valve calibration lost",
         d="The ABS module has lost, or never matched, the calibration data for the valve that isolates the brake master cylinder. "
           "Ford sees it as an internal ABS module fault or a calibration mismatch between the ABS module and the hydraulic unit: the code is "
           "cleared (on the Fiesta after running the valve calibration routines with a scan tool) and, if it returns, the ABS module/HCU "
           "assembly is replaced. " + ABS_EN,
         c=[("medium", "Calibration mismatch between ABS module and hydraulic unit"), ("low", "Internal fault of the ABS module")],
         s=["ABS and stability control warning lights"]),
    dict(t="Calibrarea supapei de izolare a pompei centrale ABS pierdută",
         d="Modulul ABS a pierdut sau nu are potrivite datele de calibrare pentru supapa care izolează pompa centrală de frână. Ford tratează "
           "codul ca defect intern al modulului ABS sau ca nepotrivire de calibrare între modulul ABS și unitatea hidraulică: codul se șterge "
           "(la Fiesta după rularea rutinelor de calibrare a supapelor cu testerul) și, dacă revine, se înlocuiește ansamblul modul ABS/HCU. " + ABS_RO,
         c=[("medium", "Nepotrivire de calibrare între modulul ABS și unitatea hidraulică"), ("low", "Defect intern al modulului ABS")],
         s=["Martorii ABS și control stabilitate aprinși"]),
    LAMP)

add("U2007", "abs", "two-sources", ABS_SRC,
    dict(t="ABS valve calibration or signal check failed",
         d="The ABS module's check of its own valves failed (calibration memory or a signal comparison). Ford puts it down to an internal ABS "
           "module fault or a calibration mismatch with the hydraulic unit: clear the code and, if it returns, replace the ABS module/HCU "
           "assembly. " + ABS_EN,
         c=[("medium", "Internal fault of the ABS module / hydraulic unit"), ("low", "Calibration mismatch between ABS module and hydraulic unit")],
         s=["ABS and stability control warning lights"]),
    dict(t="Verificarea calibrării sau semnalului supapelor ABS a eșuat",
         d="Verificarea pe care modulul ABS o face propriilor supape a eșuat (memoria de calibrare sau o comparație de semnale). Ford o pune pe "
           "seama unui defect intern al modulului ABS sau a unei nepotriviri de calibrare cu unitatea hidraulică: se șterge codul și, dacă revine, "
           "se înlocuiește ansamblul modul ABS/HCU. " + ABS_RO,
         c=[("medium", "Defect intern al modulului ABS / unității hidraulice"), ("low", "Nepotrivire de calibrare între modulul ABS și unitatea hidraulică")],
         s=["Martorii ABS și control stabilitate aprinși"]),
    LAMP)

config_codes("abs", ABS_SRC, extra_en=ABS_EN, extra_ro=ABS_RO)
add("U3000", "abs", "two-sources", ABS_SRC,
    dict(t="ABS module internal fault",
         d="U3000 is the standard 'control module' code; Ford's ABS module uses it for a fault inside the module (type 49: internal "
           "electronics). Ford's chart clears the code and repeats the self-test; if it returns, the ABS module/HCU assembly is replaced. " + ABS_EN,
         c=[("medium", "Internal fault of the ABS module"), ("low", "Intermittent fault that doesn't return after clearing")],
         s=["ABS and stability control warning lights"]),
    dict(t="Defect intern al modulului ABS",
         d="U3000 este codul standard pentru „modul de control”; modulul ABS Ford îl folosește pentru un defect în interiorul modulului (tipul 49: "
           "electronica internă). Fișa Ford șterge codul și repetă autotestul; dacă revine, se înlocuiește ansamblul modul ABS/HCU. " + ABS_RO,
         c=[("medium", "Defect intern al modulului ABS"), ("low", "Defect intermitent care nu revine după ștergere")],
         s=["Martorii ABS și control stabilitate aprinși"]),
    LAMP, key="U3000@abs")
override("U3000@abs")

# ============================================================================
# Instrument cluster (IPC)
# ============================================================================
IPC_SRC = [F12_IPC, FI14_IPC]
add("B1A14", "cluster", "two-sources", IPC_SRC,
    dict(t="Airbag warning lamp circuit fault in the cluster",
         d="The instrument cluster found its airbag warning lamp circuit shorted to ground or open. If this lamp can't light, the car "
           "can't warn you about an airbag fault, so check the airbag unit for stored codes as well. Ford's chart replaces the cluster "
           "(on the Fiesta only if the code returns after clearing).",
         c=[("medium", "Instrument cluster fault")], s=["Airbag warning light doesn't light during the bulb check, or stays on"]),
    dict(t="Defect la circuitul martorului airbag din bord",
         d="Bordul a găsit circuitul martorului airbag în scurtcircuit la masă sau întrerupt. Dacă acest martor nu se poate aprinde, mașina "
           "nu te poate avertiza despre un defect de airbag, așa că verifică și codurile memorate în unitatea airbag. Fișa Ford înlocuiește "
           "bordul (la Fiesta doar dacă, după ștergere, codul revine).",
         c=[("medium", "Defect al bordului")], s=["Martorul airbag nu se aprinde la testul de pornire sau rămâne aprins"]),
    LAMP)
add("B1A84", "cluster", "two-sources", IPC_SRC,
    dict(t="Vehicle configuration data in the cluster missing or corrupted",
         d="The 'car configuration' block that the instrument cluster stores (and on many Fords shares with other modules) is not "
           "programmed or fails its checksum. It usually follows a cluster replacement or an incomplete set-up; Ford's fix is to repeat "
           "the module set-up (PMI), or to fit a new cluster if no work was done and it no longer keeps its data.",
         c=[("medium", "Cluster replaced or programmed without completing the set-up"), ("low", "Cluster that no longer keeps its configuration")],
         s=["Wrong or missing displays and warnings", "Other modules may also report configuration codes"]),
    dict(t="Datele de configurare a mașinii din bord lipsesc sau sunt corupte",
         d="Blocul „configurația mașinii” pe care îl păstrează bordul (și pe care la multe Ford îl împarte cu alte module) nu este programat "
           "sau nu trece verificarea sumei de control. Apare de obicei după înlocuirea bordului sau o configurare incompletă; soluția Ford "
           "este repetarea configurării modulului (PMI) sau un bord nou, dacă la el nu s-a lucrat și nu își mai păstrează datele.",
         c=[("medium", "Bord înlocuit sau programat fără finalizarea configurării"), ("low", "Bord care nu își mai păstrează configurația")],
         s=["Afișaje și avertizări greșite sau lipsă", "Și alte module pot raporta coduri de configurare"]))
add("P1607", "cluster", "two-sources", IPC_SRC,
    dict(t="Check-engine lamp request from the engine computer missing or invalid",
         d="The instrument cluster lights the check-engine lamp (MIL) when the engine computer asks it to over the network. P1607 means "
           "that request message is missing, not updated or invalid. Ford's chart clears the code and retests; if it returns without a "
           "lost-communication code (U0100) for the engine computer, the engine computer is replaced. Until it is fixed, the cluster may "
           "be unable to warn you about engine faults.",
         c=[("medium", "Network wiring or connectors between engine computer and cluster"), ("low", "Engine computer (PCM)")],
         s=["Check-engine light on, or not working", "Often set together with U0100"]),
    dict(t="Cererea de aprindere a martorului de motor lipsește sau este invalidă",
         d="Bordul aprinde martorul de motor (MIL) când calculatorul motorului îi cere asta prin rețea. P1607 înseamnă că acest mesaj de "
           "cerere lipsește, nu se actualizează sau este invalid. Fișa Ford șterge codul și retestează; dacă revine fără un cod de pierdere a "
           "comunicației (U0100) cu calculatorul motorului, se înlocuiește calculatorul motorului. Până la reparare, bordul s-ar putea să nu "
           "te poată avertiza despre defectele motorului.",
         c=[("medium", "Cablajul sau conectorii rețelei între calculatorul motorului și bord"), ("low", "Calculatorul motorului (PCM)")],
         s=["Martorul de motor aprins sau nefuncțional", "Apare adesea împreună cu U0100"]),
    LAMP)
config_codes("cluster", IPC_SRC, src_u2101=IPC_SRC)
add("U3000", "cluster", "two-sources", IPC_SRC,
    dict(t="Instrument cluster internal fault",
         d="U3000 is the standard 'control module' code; Ford's instrument cluster uses it for a fault inside the cluster (on the Fiesta, "
           "type 41: checksum). Ford's chart clears the code and repeats the self-test; if it returns, the cluster is replaced.",
         c=[("medium", "Internal fault of the instrument cluster"), ("low", "Intermittent fault that doesn't return after clearing")],
         s=["Gauges, warnings or messages not working correctly"]),
    dict(t="Defect intern al bordului",
         d="U3000 este codul standard pentru „modul de control”; bordul Ford îl folosește pentru un defect în interiorul lui (la Fiesta, "
           "tipul 41: sumă de control). Fișa Ford șterge codul și repetă autotestul; dacă revine, bordul se înlocuiește.",
         c=[("medium", "Defect intern al bordului"), ("low", "Defect intermitent care nu revine după ștergere")],
         s=["Ceasurile, avertizările sau mesajele nu funcționează corect"]),
    key="U3000@cluster")
override("U3000@cluster")

# ============================================================================
# Engine (PCM): Ford P1xxx codes from the 2012 Focus workshop manual
# ============================================================================
add("P1001", "engine", "two-sources", [focus2012_p("P1001"), TCN],
    dict(t="Engine-running self-test could not be completed",
         d="The key-on, engine-running self-test, started from a scan tool, did not finish in the time allowed and was aborted. It concerns "
           "only that workshop test and doesn't turn on the check-engine light; repeat the test following the scan tool's instructions.",
         c=[("medium", "Self-test procedure not followed (for example throttle touched, or engine speed not stable)"),
            ("low", "A fault that makes the engine respond unexpectedly during the test")],
         s=["No warning light"]),
    dict(t="Autotestul cu motorul pornit nu s-a putut încheia",
         d="Autotestul cu contactul pus și motorul pornit, lansat dintr-un tester, nu s-a încheiat în timpul permis și a fost abandonat. "
           "Privește doar acest test de atelier și nu aprinde martorul de motor; repetă testul urmând instrucțiunile testerului.",
         c=[("medium", "Procedura de autotest nerespectată (de exemplu pedala de accelerație atinsă sau turație instabilă)"),
            ("low", "Un defect care face ca motorul să răspundă neașteptat în timpul testului")],
         s=["Niciun martor aprins"]))

add("P1299", "engine", "two-sources", [focus2012_p("P1299"), TCN, SSM_54288],
    dict(t="Cylinder head over-temperature protection active",
         d="The engine computer saw the cylinder head overheat and switched to its protection mode, which can cut power or run the engine "
           "on fewer cylinders to cool it. It usually comes with P1285. Stop as soon as it is safe, switch the engine off and let it cool "
           "before checking the coolant level.",
         c=[("medium", "Low coolant level (leak)"), ("medium", "Cooling system fault: thermostat, water pump, radiator fan, blocked radiator"),
            ("low", "Cylinder head temperature sensor")],
         s=["Engine temperature warning", "Marked loss of power", "Check-engine light"]),
    dict(t="Protecția la supraîncălzirea chiulasei este activă",
         d="Calculatorul motorului a văzut că chiulasa s-a supraîncălzit și a trecut în modul de protecție, care poate reduce puterea sau "
           "poate opri alimentarea unor cilindri ca motorul să se răcească. De obicei apare împreună cu P1285. Oprește cât de curând poți în "
           "siguranță, oprește motorul și lasă-l să se răcească înainte să verifici nivelul lichidului de răcire.",
         c=[("medium", "Nivel scăzut al lichidului de răcire (scurgere)"),
            ("medium", "Defect la răcire: termostat, pompă de apă, ventilatorul radiatorului, radiator înfundat"),
            ("low", "Senzorul de temperatură a chiulasei")],
         s=["Avertizare de temperatură motor", "Pierdere vizibilă de putere", "Martorul de motor aprins"]),
    MIL + LAMP + ("limp_mode",))

add("P1288", "engine", "two-sources", [focus2012_p("P1288"), TCN],
    dict(t="Cylinder head temperature sensor out of self-test range",
         d="During a self-test the cylinder head temperature (CHT) sensor did not read a normal operating temperature. Most often the "
           "engine was simply not warm yet; repeat the test hot. If the engine was overheating, check the cooling system instead.",
         c=[("medium", "Engine not at operating temperature during the test"), ("low", "Engine overheating or low coolant"),
            ("low", "Damaged sensor or connector")],
         s=["Usually none"]),
    dict(t="Senzorul de temperatură a chiulasei în afara domeniului de autotest",
         d="În timpul unui autotest, senzorul de temperatură a chiulasei (CHT) nu a citit o temperatură normală de funcționare. Cel mai des "
           "motorul pur și simplu nu era încă cald; repetă testul cu motorul încălzit. Dacă motorul se supraîncălzea, verifică sistemul de răcire.",
         c=[("medium", "Motorul nu era la temperatura de funcționare în timpul testului"), ("low", "Motor supraîncălzit sau lichid de răcire puțin"),
            ("low", "Senzor sau conector deteriorat")],
         s=["De obicei niciunul"]))

for code, high, en_t, ro_t, en_d, ro_d, en_c, ro_c in (
        ("P1289", True, "Cylinder head temperature sensor circuit high", "Circuitul senzorului de temperatură a chiulasei: semnal ridicat",
         "The cylinder head temperature (CHT) sensor signal is stuck high, which normally means the circuit is open or shorted to voltage. "
         "P0118 may be stored with it.",
         "Semnalul senzorului de temperatură a chiulasei (CHT) este blocat la valoare ridicată, ceea ce înseamnă de obicei circuit întrerupt "
         "sau scurtcircuit la plus. Poate fi memorat împreună cu P0118.",
         [("medium", "Open circuit or poor connection at the sensor"), ("low", "Short to voltage"), ("low", "Failed sensor")],
         [("medium", "Circuit întrerupt sau contact slab la senzor"), ("low", "Scurtcircuit la plus"), ("low", "Senzor defect")]),
        ("P1290", False, "Cylinder head temperature sensor circuit low", "Circuitul senzorului de temperatură a chiulasei: semnal scăzut",
         "The cylinder head temperature (CHT) sensor signal is stuck low, which normally means the circuit is shorted to ground. "
         "P0117 may be stored with it.",
         "Semnalul senzorului de temperatură a chiulasei (CHT) este blocat la valoare scăzută, ceea ce înseamnă de obicei scurtcircuit la masă. "
         "Poate fi memorat împreună cu P0117.",
         [("medium", "Sensor wiring shorted to ground"), ("low", "Failed sensor"), ("low", "Wrong harness connection")],
         [("medium", "Cablajul senzorului în scurtcircuit la masă"), ("low", "Senzor defect"), ("low", "Conexiune greșită a cablajului")])):
    add(code, "engine", "two-sources", [focus2012_p(code), TCN],
        dict(t=en_t, d=en_d + " Many Ford engines use this sensor as their only engine temperature reading, so the cooling fan and the "
                              "temperature gauge may not work normally.",
             c=en_c, s=["Check-engine light", "Temperature gauge or cooling fan behaving oddly"]),
        dict(t=ro_t, d=ro_d + " Multe motoare Ford folosesc acest senzor ca singură măsurare a temperaturii motorului, așa că ventilatorul "
                              "de răcire și indicatorul de temperatură pot să nu funcționeze normal.",
             c=ro_c, s=["Martorul de motor aprins", "Indicatorul de temperatură sau ventilatorul se comportă ciudat"]),
        MIL)

add("P1336", "engine", "two-sources", [focus2012_p("P1336"), TCN],
    dict(t="Crankshaft or camshaft position sensor signal erratic",
         d="The signal from the crankshaft position (CKP) or camshaft position (CMP) sensor is erratic. Ford also points at wiring routed "
           "too close to other systems or badly shielded, which can pick up interference.",
         c=[("medium", "Crankshaft or camshaft position sensor"), ("medium", "Sensor wiring: routing, shielding, chafing"),
            ("low", "Mechanical engine problem, e.g. timing")],
         s=["Hesitation, misfire or stalling", "Hard starting"]),
    dict(t="Semnal neregulat de la senzorul de poziție arbore cotit sau ax cu came",
         d="Semnalul senzorului de poziție a arborelui cotit (CKP) sau a axului cu came (CMP) este neregulat. Ford indică și cablajul trasat "
           "prea aproape de alte sisteme sau ecranat necorespunzător, care poate prelua interferențe.",
         c=[("medium", "Senzorul de poziție arbore cotit sau ax cu came"), ("medium", "Cablajul senzorului: traseu, ecranare, frecare"),
            ("low", "Problemă mecanică a motorului, de exemplu distribuția")],
         s=["Ezitări, rateuri sau opriri ale motorului", "Pornire grea"]))

add("P1397", "engine", "two-sources", [focus2012_p("P1397"), TCN],
    dict(t="Battery voltage out of range during the self-test",
         d="The 12-volt system voltage went too high or too low while a scan-tool self-test (engine off or running) was running. Ford asks "
           "for 11 to 18 volts before starting the test; check the battery and charging system.",
         c=[("medium", "Weak or discharged battery"), ("medium", "Charging system fault (low charge or overcharging)")],
         s=["Usually none; set only during a workshop self-test"]),
    dict(t="Tensiunea bateriei în afara limitelor în timpul autotestului",
         d="Tensiunea sistemului de 12 volți a fost prea mare sau prea mică în timpul unui autotest lansat din tester (cu motorul oprit sau "
           "pornit). Ford cere 11-18 volți înainte de test; verifică bateria și sistemul de încărcare.",
         c=[("medium", "Baterie slabă sau descărcată"), ("medium", "Defect la încărcare (încărcare slabă sau supraîncărcare)")],
         s=["De obicei niciunul; apare doar la un autotest în atelier"]))

add("P1474", "engine", "two-sources", [focus2012_p("P1474"), TCN],
    dict(t="Low-speed cooling fan control circuit fault",
         d="The engine computer checks the circuit it uses to switch on the low-speed radiator fan relay and found it open, shorted or "
           "drawing too much current. If the fan can't run, the engine may overheat in traffic.",
         c=[("medium", "Fan relay control wiring open or shorted"), ("medium", "Low-speed fan relay"), ("low", "No power feed to the relay")],
         s=["Cooling fan not running at low speed", "Engine running hot at idle or in traffic"]),
    dict(t="Defect în circuitul de comandă al ventilatorului de răcire (treapta mică)",
         d="Calculatorul motorului verifică circuitul cu care cuplează releul ventilatorului radiatorului pe treapta mică și l-a găsit întrerupt, "
           "în scurtcircuit sau cu consum prea mare. Dacă ventilatorul nu poate porni, motorul se poate supraîncălzi în trafic.",
         c=[("medium", "Cablajul de comandă al releului întrerupt sau în scurtcircuit"), ("medium", "Releul ventilatorului pe treapta mică"),
            ("low", "Lipsa alimentării releului")],
         s=["Ventilatorul nu pornește pe treapta mică", "Motorul se încălzește la ralanti sau în trafic"]))
add("P1479", "engine", "two-sources", [focus2012_p("P1479"), TCN],
    dict(t="High-speed cooling fan control circuit fault",
         d="The engine computer checks the circuit it uses to switch on the high-speed radiator fan relay and found it open, shorted or "
           "drawing too much current. Without the high fan speed the engine may run hot under load or with the air conditioning on.",
         c=[("medium", "Fan relay control wiring open or shorted"), ("medium", "High-speed fan relay"), ("low", "No power feed to the relay")],
         s=["Cooling fan not reaching high speed", "Engine running hot under load"]),
    dict(t="Defect în circuitul de comandă al ventilatorului de răcire (treapta mare)",
         d="Calculatorul motorului verifică circuitul cu care cuplează releul ventilatorului radiatorului pe treapta mare și l-a găsit întrerupt, "
           "în scurtcircuit sau cu consum prea mare. Fără treapta mare a ventilatorului, motorul se poate încălzi la sarcină sau cu aerul "
           "condiționat pornit.",
         c=[("medium", "Cablajul de comandă al releului întrerupt sau în scurtcircuit"), ("medium", "Releul ventilatorului pe treapta mare"),
            ("low", "Lipsa alimentării releului")],
         s=["Ventilatorul nu ajunge pe treapta mare", "Motorul se încălzește la sarcină"]))

add("P1572", "engine", "two-sources", [focus2012_p("P1572"), TCN],
    dict(t="Brake pedal switch signals don't agree",
         d="The engine computer compares the brake pedal position switch with the brake pressure switch (on some cars the ABS module "
           "passes the brake signal on). One or both did not change state when the pedal was pressed and released.",
         c=[("medium", "Brake pedal switch badly adjusted or faulty"), ("medium", "Brake pressure switch faulty"),
            ("low", "Blown fuse, or open/short in the switch wiring")],
         s=["Possibly the check-engine light"]),
    dict(t="Semnalele contactelor pedalei de frână nu concordă",
         d="Calculatorul motorului compară contactul de poziție al pedalei de frână cu contactul de presiune din sistemul de frânare (la unele "
           "mașini semnalul de frână vine prin modulul ABS). Unul sau ambele nu și-au schimbat starea la apăsarea și eliberarea pedalei.",
         c=[("medium", "Contactul pedalei de frână dereglat sau defect"), ("medium", "Contactul de presiune al frânei defect"),
            ("low", "Siguranță arsă sau cablaj întrerupt/în scurtcircuit la contacte")],
         s=["Posibil martorul de motor aprins"]))

add("P1633", "engine", "two-sources", [focus2012_p("P1633"), TCN],
    dict(t="Engine computer permanent power supply interrupted",
         d="The engine computer's keep-alive power feed, which stays live with the ignition off to hold learned values, was interrupted. "
           "Losing this feed turns on the check-engine light straight away. Learned values such as fuel trims and idle are lost.",
         c=[("medium", "Open or intermittent keep-alive power circuit (fuse, wiring, connector)"),
            ("low", "Battery terminals loose or corroded")],
         s=["Check-engine light"]),
    dict(t="Alimentarea permanentă a calculatorului motorului întreruptă",
         d="Alimentarea permanentă (keep-alive) a calculatorului motorului, care rămâne sub tensiune cu contactul tăiat ca să păstreze valorile "
           "învățate, a fost întreruptă. Pierderea ei aprinde imediat martorul de motor. Valorile învățate, precum corecțiile de amestec și "
           "ralantiul, se pierd.",
         c=[("medium", "Circuit de alimentare permanentă întrerupt sau intermitent (siguranță, cablaj, conector)"),
            ("low", "Borne de baterie slăbite sau oxidate")],
         s=["Martorul de motor aprins"]),
    MIL)

add("P1674", "engine", "two-sources", [focus2012_p("P1674"), TCN],
    dict(t="Engine computer software corrupted",
         d="The engine computer found an error in its own software; on the 2012 Focus it is set together with P2105 (throttle "
           "control forced engine shutdown). Ford's first step is to make sure the computer has the latest software.",
         c=[("medium", "Software problem: the engine computer needs reprogramming"), ("low", "Damaged engine computer")],
         s=["Engine may shut down (P2105 is set with it)"]),
    dict(t="Software-ul calculatorului motorului este corupt",
         d="Calculatorul motorului a găsit o eroare în propriul software; la Focus 2012 apare împreună cu P2105 (oprirea forțată a motorului de "
           "către controlul clapetei). Primul pas Ford este verificarea că acel calculator are cel mai nou software.",
         c=[("medium", "Problemă de software: calculatorul motorului trebuie reprogramat"), ("low", "Calculatorul motorului deteriorat")],
         s=["Motorul se poate opri (apare împreună cu P2105)"]))

add("P1703", "engine", "two-sources", [focus2012_p("P1703"), TCN],
    dict(t="Brake switch signal wrong during the self-test",
         d="During the scan-tool self-test the brake pedal switch signal was on with the engine off, or did not switch on and off with the "
           "engine running. Ford suggests checking that the brake lights work with the pedal.",
         c=[("medium", "Brake pedal switch faulty or badly adjusted"), ("medium", "Brake switch or brake light wiring open or shorted"),
            ("low", "Another module connected to the brake switch circuit")],
         s=["Brake lights not working correctly", "Usually set only during a workshop self-test"]),
    dict(t="Semnalul contactului de frână greșit în timpul autotestului",
         d="În timpul autotestului din tester, semnalul contactului pedalei de frână era activ cu motorul oprit sau nu s-a activat și dezactivat "
           "cu motorul pornit. Ford recomandă verificarea funcționării stopurilor la apăsarea pedalei.",
         c=[("medium", "Contactul pedalei de frână defect sau dereglat"), ("medium", "Cablajul contactului sau al stopurilor întrerupt ori în scurtcircuit"),
            ("low", "Alt modul conectat la circuitul contactului de frână")],
         s=["Stopurile nu funcționează corect", "De obicei apare doar la un autotest în atelier"]))

add("P1780", "engine", "two-sources", [focus2012_p("P1780"), TCN],
    dict(t="Transmission control switch out of self-test range",
         d="During the scan-tool self-test the engine computer didn't see the expected signal from the transmission control switch, the "
           "button or selector switch the driver uses to change the automatic gearbox mode (for example overdrive cancel or manual shifting). "
           "Ford sends the diagnosis to the gearbox external controls section.",
         c=[("medium", "Switch operated or held during the self-test"), ("low", "Faulty switch or its wiring")],
         s=["Usually none; set only during a workshop self-test"]),
    dict(t="Contactul de comandă al cutiei automate în afara domeniului de autotest",
         d="În timpul autotestului din tester, calculatorul motorului nu a văzut semnalul așteptat de la contactul de comandă al cutiei, "
           "butonul sau contactul din selector cu care șoferul schimbă modul cutiei automate (de exemplu anularea overdrive-ului sau schimbarea "
           "manuală). Ford trimite diagnosticarea la secțiunea comenzilor exterioare ale cutiei.",
         c=[("medium", "Contactul acționat sau ținut apăsat în timpul autotestului"), ("low", "Contact defect sau cablajul lui")],
         s=["De obicei niciunul; apare doar la un autotest în atelier"]))

add("P1116", "engine", "two-sources", [focus2012_p("P1116"), TCN],
    dict(t="Coolant temperature sensor out of self-test range",
         d="During a scan-tool self-test the coolant temperature (ECT) sensor reading was outside the range the test expects. Ford asks for "
           "more than about 10 °C for the engine-off test and more than about 82 °C (fully warm) for the engine-running test, so a cold engine "
           "is a common reason.",
         c=[("medium", "Engine not warm enough during the test"), ("low", "Thermostat stuck open, or engine overheating / low coolant"),
            ("low", "Coolant temperature sensor or its connector")],
         s=["Usually none; set only during a workshop self-test"]),
    dict(t="Senzorul de temperatură a lichidului de răcire în afara domeniului de autotest",
         d="În timpul unui autotest din tester, valoarea senzorului de temperatură a lichidului de răcire (ECT) a fost în afara domeniului "
           "așteptat. Ford cere peste circa 10 °C pentru testul cu motorul oprit și peste circa 82 °C (motor complet încălzit) pentru testul "
           "cu motorul pornit, așa că un motor rece este un motiv frecvent.",
         c=[("medium", "Motorul nu era suficient de cald în timpul testului"),
            ("low", "Termostat blocat deschis, ori motor supraîncălzit / lichid de răcire puțin"),
            ("low", "Senzorul de temperatură a lichidului de răcire sau conectorul lui")],
         s=["De obicei niciunul; apare doar la un autotest în atelier"]))

add("P1117", "engine", "two-sources", [focus2012_p("P1117"), TCN],
    dict(t="Coolant temperature sensor circuit intermittent",
         d="While the engine was running, the coolant temperature (ECT) sensor circuit briefly went open or shorted. Ford suggests watching "
           "the live coolant temperature on a scan tool while wiggling the wiring or tapping the sensor, looking for sudden jumps.",
         c=[("medium", "Damaged wiring or connector"), ("medium", "Coolant temperature sensor"), ("low", "Low coolant level")],
         s=["Engine temperature reading that jumps on a scan tool"]),
    dict(t="Circuitul senzorului de temperatură a lichidului de răcire intermitent",
         d="Cu motorul pornit, circuitul senzorului de temperatură a lichidului de răcire (ECT) a fost scurt timp întrerupt sau în scurtcircuit. "
           "Ford recomandă urmărirea temperaturii în timp real pe tester în timp ce se mișcă cablajul sau se lovește ușor senzorul, căutând salturi bruște.",
         c=[("medium", "Cablaj sau conector deteriorat"), ("medium", "Senzorul de temperatură a lichidului de răcire"), ("low", "Nivel scăzut al lichidului de răcire")],
         s=["Temperatura motorului citită pe tester sare brusc"]))
add("P1101", "engine", "two-sources", [focus2012_p("P1101"), TCN],
    dict(t="Mass air flow sensor out of self-test range",
         d="During a scan-tool self-test the mass air flow (MAF) sensor signal was too high with the engine off, or outside the expected "
           "range with it running. Ford asks to fix any MAF circuit codes first.",
         c=[("medium", "MAF sensor dirty, damaged or not fully plugged in"), ("medium", "Air leak in the intake after the MAF sensor"),
            ("low", "Open ground or signal-return wire"), ("low", "Low battery charge")],
         s=["Usually set during a workshop self-test"]),
    dict(t="Debitmetrul de aer în afara domeniului de autotest",
         d="În timpul unui autotest din tester, semnalul debitmetrului de aer (MAF) a fost prea mare cu motorul oprit sau în afara domeniului "
           "așteptat cu motorul pornit. Ford cere rezolvarea mai întâi a codurilor de circuit ale debitmetrului.",
         c=[("medium", "Debitmetru murdar, deteriorat sau conectat incomplet"), ("medium", "Aer fals în admisie după debitmetru"),
            ("low", "Fir de masă sau de retur al semnalului întrerupt"), ("low", "Baterie descărcată")],
         s=["De obicei apare la un autotest în atelier"]))

add("P1124", "engine", "two-sources", [focus2012_p("P1124"), TCN],
    dict(t="Throttle position sensor out of self-test range",
         d="During a scan-tool self-test the electronic throttle's position sensors read higher than expected. Ford's only listed cause is "
           "the accelerator pedal being pressed during the test: repeat it without touching the pedal and check that the floor mat isn't "
           "pushing on it.",
         c=[("medium", "Accelerator pedal pressed, or floor mat touching it, during the self-test")],
         s=["Usually none; set only during a workshop self-test"]),
    dict(t="Senzorul de poziție a clapetei în afara domeniului de autotest",
         d="În timpul unui autotest din tester, senzorii de poziție ai clapetei electronice au citit mai mult decât era de așteptat. Singura "
           "cauză indicată de Ford este pedala de accelerație apăsată în timpul testului: repetă-l fără să atingi pedala și verifică să nu "
           "apese covorașul pe ea.",
         c=[("medium", "Pedala de accelerație apăsată sau atinsă de covoraș în timpul autotestului")],
         s=["De obicei niciunul; apare doar la un autotest în atelier"]))

add("P1270", "engine", "two-sources", [focus2012_p("P1270"), TCN],
    dict(t="Engine speed or vehicle speed limiter reached",
         d="The engine computer recorded that engine speed or road speed went past its programmed limit, so the limiter stepped in. It "
           "describes how the car was driven, not a fault: Ford's advice is to tell the owner about it and only investigate if there is "
           "another symptom.",
         c=[("medium", "Wheel spin on water, ice, mud or snow"), ("medium", "High revs in neutral or in the wrong gear"),
            ("medium", "Car driven at very high speed")],
         s=["Usually none"]),
    dict(t="Limitatorul de turație sau de viteză a intervenit",
         d="Calculatorul motorului a înregistrat că turația motorului sau viteza mașinii a depășit limita programată, așa că limitatorul a "
           "intervenit. Descrie modul în care a fost condusă mașina, nu un defect: Ford recomandă informarea proprietarului și investigarea "
           "doar dacă există și alt simptom.",
         c=[("medium", "Patinarea roților pe apă, gheață, noroi sau zăpadă"), ("medium", "Turație mare în neutru sau în treapta greșită"),
            ("medium", "Mașină condusă cu viteză foarte mare")],
         s=["De obicei niciunul"]))

add("P1489", "engine", "two-sources", [focus2012_p("P1489"), TCN],
    dict(t="Crankcase ventilation (PCV) heater circuit fault",
         d="The engine computer controls a small heater that keeps the crankcase ventilation (PCV) valve from freezing in cold weather, and "
           "found its circuit voltage too low or too high. Ford also asks to check that the right PCV valve is fitted and the heater is plugged in.",
         c=[("medium", "PCV heater wiring open or shorted, or connector unplugged"), ("medium", "PCV heater assembly"),
            ("low", "Wrong PCV valve fitted")],
         s=["Check-engine light possible"]),
    dict(t="Defect în circuitul încălzitorului ventilației carterului (PCV)",
         d="Calculatorul motorului comandă un mic încălzitor care împiedică înghețarea supapei de ventilație a carterului (PCV) pe vreme rece și "
           "a găsit tensiunea din circuitul lui prea mică sau prea mare. Ford cere și verificarea faptului că este montată supapa PCV corectă "
           "și că încălzitorul este conectat.",
         c=[("medium", "Cablajul încălzitorului PCV întrerupt sau în scurtcircuit, ori conector decuplat"), ("medium", "Ansamblul încălzitorului PCV"),
            ("low", "Supapă PCV greșită montată")],
         s=["Posibil martorul de motor aprins"]))

add("P1588", "engine", "two-sources", [focus2012_p("P1588"), TCN],
    dict(t="Electronic throttle does not spring back to its default position",
         d="The engine computer tests whether the throttle plate returns by spring force to its default 'limp-home' position from both "
           "open and closed, and it did not. Ford suggests looking at the throttle plate for anything blocking it.",
         c=[("medium", "Something obstructing the throttle plate (dirt, deposits)"), ("medium", "Damaged electronic throttle body")],
         s=["Check-engine light possible"]),
    dict(t="Clapeta electronică nu revine prin arc în poziția implicită",
         d="Calculatorul motorului verifică dacă clapeta de accelerație revine prin forța arcului în poziția implicită de avarie, atât din "
           "poziția deschisă, cât și din cea închisă, iar aceasta nu a revenit. Ford recomandă verificarea clapetei pentru orice o blochează.",
         c=[("medium", "Ceva care blochează clapeta (murdărie, depuneri)"), ("medium", "Corpul clapetei electronice deteriorat")],
         s=["Posibil martorul de motor aprins"]))
add("P1260", "engine", "two-sources", [focus2012_p("P1260"), TCN],
    dict(t="Theft detected, engine immobilised",
         d="The passive anti-theft system (PATS) decided there was a theft attempt and disabled the engine, or someone tried to start the "
           "car with a key that isn't programmed to it. Ford notes it can also appear after a new instrument cluster or engine computer was "
           "fitted without the anti-theft parameters being reset in both. Check the anti-theft system for its own codes.",
         c=[("medium", "Start attempted with a key not programmed to the car"),
            ("medium", "Instrument cluster or engine computer replaced without the anti-theft parameter reset"),
            ("low", "Earlier theft attempt"), ("low", "Fault in the anti-theft system")],
         s=["Engine won't start, or starts and dies", "Anti-theft light flashing quickly or staying on"]),
    dict(t="Furt detectat, motor imobilizat",
         d="Sistemul antifurt pasiv (PATS) a considerat că a avut loc o tentativă de furt și a blocat motorul, sau cineva a încercat să pornească "
           "mașina cu o cheie neprogramată pentru ea. Ford precizează că poate apărea și după montarea unui bord sau a unui calculator de motor "
           "nou fără resetarea parametrilor antifurt în ambele. Verifică și codurile proprii ale sistemului antifurt.",
         c=[("medium", "Pornire încercată cu o cheie neprogramată pentru mașină"),
            ("medium", "Bord sau calculator de motor înlocuit fără resetarea parametrilor antifurt"),
            ("low", "Tentativă de furt anterioară"), ("low", "Defect în sistemul antifurt")],
         s=["Motorul nu pornește sau pornește și se oprește", "Martorul antifurt clipește rapid sau rămâne aprins"]))

add("P1548", "engine", "two-sources", [focus2012_p("P1548"), TCN],
    dict(t="Engine air filter restricted",
         d="At various engine speeds and at full throttle the measured air flow stayed below what the engine computer expects, which points "
           "to a restricted intake. Ford's advice is to inspect the intake and, if nothing is blocking it, replace the air filter.",
         c=[("medium", "Clogged air filter"), ("medium", "Something blocking the air intake")]),
    dict(t="Filtrul de aer al motorului înfundat",
         d="La diferite turații și la accelerație maximă, debitul de aer măsurat a rămas sub cel așteptat de calculatorul motorului, ceea ce "
           "indică o admisie obturată. Recomandarea Ford este verificarea admisiei și, dacă nimic nu o blochează, înlocuirea filtrului de aer.",
         c=[("medium", "Filtru de aer înfundat"), ("medium", "Ceva care blochează priza de aer")]))

add("P1575", "engine", "two-sources", [focus2012_p("P1575"), TCN],
    dict(t="Accelerator pedal position out of self-test range",
         d="During the engine-off scan-tool self-test the accelerator pedal sensors read higher than expected. Ford's only listed cause is "
           "the pedal being pressed during the test: repeat it without touching the pedal and check that the floor mat isn't pushing on it.",
         c=[("medium", "Accelerator pedal pressed, or floor mat touching it, during the self-test")],
         s=["Usually none; set only during a workshop self-test"]),
    dict(t="Poziția pedalei de accelerație în afara domeniului de autotest",
         d="În timpul autotestului din tester cu motorul oprit, senzorii pedalei de accelerație au citit mai mult decât era de așteptat. "
           "Singura cauză indicată de Ford este pedala apăsată în timpul testului: repetă-l fără să atingi pedala și verifică să nu apese "
           "covorașul pe ea.",
         c=[("medium", "Pedala de accelerație apăsată sau atinsă de covoraș în timpul autotestului")],
         s=["De obicei niciunul; apare doar la un autotest în atelier"]))

add("P1640", "engine", "two-sources", [focus2012_p("P1640"), TCN],
    dict(t="Emission-related codes stored in another module",
         d="Another engine-related module asked the engine computer to turn on the check-engine light because it found an "
           "emission-related fault. P1640 itself is only a pointer: read the codes stored in the other modules.",
         c=[("medium", "Fault stored in a secondary engine-related module")],
         s=["Check-engine light"]),
    dict(t="Coduri legate de emisii memorate în alt modul",
         d="Un alt modul legat de motor a cerut calculatorului motorului să aprindă martorul de motor pentru că a găsit un defect care afectează "
           "emisiile. P1640 este doar o trimitere: citește codurile memorate în celelalte module.",
         c=[("medium", "Defect memorat într-un modul secundar legat de motor")],
         s=["Martorul de motor aprins"]),
    MIL)


def main():
    pack = {
        "brand": "ford",
        "label": "Obdaryn's Ford pack",
        "label_ro": "pachetul Obdaryn pentru Ford",
        "brandName": "Ford",
        "source": {
            "license": "GPL-3.0-or-later (text written for Obdaryn)",
            "note": "Meanings compiled from Ford documents filed with NHTSA, Ford workshop-manual pages read through public mirrors "
                    "and independent code lists; each code lists its sources. Verify with a workshop before replacing parts.",
            "scope": "2008-2020 Ford (Europe and US): Focus, Fiesta, Kuga/Escape, Mondeo/Fusion, C-Max, Transit Connect",
        },
        "codes": dict(sorted(PACK.items())),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(pack, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"wrote {len(PACK)} codes to {OUT}")


if __name__ == "__main__":
    main()
