#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Source of the Hyundai/Kia trouble-code pack.

    python3 tools/brand_packs/hyundai_kia.py      # writes src/core/data/dtc/brands/hyundai-kia.json
    python3 tools/check_brand_pack.py src/core/data/dtc/brands/hyundai-kia.json

Edit THIS file, not the JSON. Families of codes (wheel-speed sensors, airbag
firing circuits, ...) are written once and expanded, so English and Romanian
stay consistent.

Scope: 2010-2016 Hyundai/Kia (Elantra MD/UD, i30 GD, Ceed JD, Cerato/Forte).
Every meaning comes from public sources listed per code. "official" means a
Hyundai/Kia document (technical service bulletin, recall or chart filed with
NHTSA) states it; "two-sources" means at least two independent sources agree.
All descriptions are written for Obdaryn in our own words; nothing is copied
from service manuals. Codes whose meaning was uncertain or contradictory, or whose sources could only be
read through search-engine summaries, were left out on purpose (see docs/brand-packs.md).
"""

import json
import pathlib

OUT = pathlib.Path(__file__).resolve().parents[2] / "src/core/data/dtc/brands/hyundai-kia.json"

# ---- Sources ---------------------------------------------------------------
YCN = "https://www.youcanic.com/guide/hyundai-fault-code-dtc"
CMH = "https://carmanualshub.com/kia-diagnostic-trouble-codes/"
SRS_CHART = "https://static.nhtsa.gov/odi/inv/2019/INRD-EA19001-76579.pdf"   # Kia Forte TD SRS chart (NHTSA EA19-001)
MDPS_TSB = "https://static.nhtsa.gov/odi/tsbs/2020/MC-10179244-0001.pdf"      # Hyundai MDPS diagnosis TSB
MDPS_TSB2 = "https://static.nhtsa.gov/odi/tsbs/2022/MC-10209439-0001.pdf"
ESC_RECALL = "https://static.nhtsa.gov/odi/rcl/2022/RCRIT-22V800-5083.pdf"     # Kia SC253 HECU
WSS_REAR = "https://static.nhtsa.gov/odi/tsbs/2024/MC-11005362-0002.pdf"       # Kia SC301Y
WSS_FR = "https://static.nhtsa.gov/odi/tsbs/2015/MC-10089848-5448.pdf"         # Kia TT066 ESC harness
YAW_RECALL = "https://static.nhtsa.gov/odi/rcl/2015/RCMN-15V871-1878.pdf"      # Elantra recall 137
BRAKE_SW_TSB = "https://static.nhtsa.gov/odi/tsbs/2016/SB-10082449-0699.pdf"
BRAKE_SW_RCL = "https://static.nhtsa.gov/odi/rcl/2017/RCRIT-17V769-4932.pdf"
CLOCKSPRING = "https://static.nhtsa.gov/odi/tsbs/2026/MC-11036632-0001.pdf"    # Hyundai TXXA warranty extension
CLOCKSPRING_KIA = "https://static.nhtsa.gov/odi/tsbs/2020/MC-10184998-0001.pdf"
OCS_TSB = "https://static.nhtsa.gov/odi/tsbs/2026/MC-11032525-0001.pdf"
TPMS_TSB = "https://www.slideshare.net/slideshow/hyundai-tpms-tire-pressure-monitoring-system-tsb/256996681"
BCM_TSB = ("https://workshop-manuals.com/hyundai/genesis_sedan/v6-3.8l/powertrain_management/"
           "computers_and_control_systems/body_control_module/component_information/"
           "technical_service_bulletins/body_controls_dtcs_b1602/b1603/battery_drain/")

OFFICIAL_PREFIXES = ("https://vehicleservicepros.com/tsbs/", "https://www.vehicleservicepros.com/tsbs/")
OFFICIAL = {SRS_CHART, MDPS_TSB, MDPS_TSB2, ESC_RECALL, WSS_REAR, WSS_FR, YAW_RECALL, BRAKE_SW_TSB, BRAKE_SW_RCL,
            CLOCKSPRING, CLOCKSPRING_KIA, OCS_TSB, TPMS_TSB, BCM_TSB,
            "https://static.nhtsa.gov/odi/tsbs/2020/MC-10183332-0001.pdf"}
MANUALS = "https://manuals.autorepairshq.com"


def manual(make, year, model, letter, code, page="General Information"):
    """Exact page of a code in a service-manual mirror (spaces encoded)."""
    path = (f"{make}/{year}/{model}/Repair and Diagnosis/A L L  Diagnostic Trouble Codes ( DTC )/"
            f"Testing and Inspection/{letter} Code Charts/{code}" + (f"/{page}" if page else ""))
    return MANUALS + "/" + path.replace(" ", "%20").replace("(", "%28").replace(")", "%29")


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
    refs = [{"u": u, "o": True} if u in OFFICIAL or "static.nhtsa.gov" in u or u.startswith(OFFICIAL_PREFIXES) else {"u": u}
            for u in dict.fromkeys(src)]
    n_official = sum(1 for r in refs if r.get("o"))
    # Confidence comes from the references, never from wishful thinking.
    if confidence == "official":
        assert n_official >= 1, f"{k}: 'official' without an official reference"
    else:
        assert len(refs) >= 2, f"{k}: 'two-sources' needs two references"
    PACK[k] = {"module": module, "confidence": confidence, "src": refs,
               "f": list(flags), "en": text(en), "ro": text(ro)}


# ============================================================================
# ABS / ESC
# ============================================================================
ABS_EN = "The ABS/ESC unit has switched off ABS, traction and stability control for safety; normal braking still works."
ABS_RO = "Unitatea ABS/ESC a dezactivat, din motive de siguranță, ABS-ul, controlul tracțiunii și al stabilității; frânarea normală funcționează în continuare."
LAMP = ("warning_lamp",)

add("C1101", "abs", "two-sources", [YCN, CMH],
    dict(t="ABS/ESC supply voltage too high",
         d="The ABS/ESC unit measured too high a supply voltage, usually from a charging-system fault. " + ABS_EN,
         c=[("medium", "Alternator voltage regulator fault (overcharging)"), ("low", "Jump-starting or a charger connected with the engine running")],
         s=["ABS/ESC warning light"]),
    dict(t="Tensiune de alimentare ABS/ESC prea mare",
         d="Unitatea ABS/ESC a măsurat o tensiune de alimentare prea mare, de obicei din cauza unui defect la încărcare. " + ABS_RO,
         c=[("medium", "Regulatorul de tensiune al alternatorului defect (supraîncărcare)"), ("low", "Pornire cu cabluri sau încărcător conectat cu motorul pornit")],
         s=["Martorul ABS/ESC aprins"]), LAMP)
add("C1102", "abs", "two-sources", [YCN, CMH],
    dict(t="ABS/ESC supply voltage too low",
         d="The ABS/ESC unit measured too low a supply voltage. A weak battery, a poor ground or a charging fault are the usual reasons. " + ABS_EN,
         c=[("medium", "Weak or discharged battery"), ("medium", "Charging-system fault"), ("low", "Corroded ground or supply connection")],
         s=["ABS/ESC warning light", "Often set together with other low-voltage codes"]),
    dict(t="Tensiune de alimentare ABS/ESC prea mică",
         d="Unitatea ABS/ESC a măsurat o tensiune de alimentare prea mică. Cauzele obișnuite sunt o baterie slabă, o masă proastă sau un defect la încărcare. " + ABS_RO,
         c=[("medium", "Baterie slabă sau descărcată"), ("medium", "Defect al sistemului de încărcare"), ("low", "Conexiune de masă sau de alimentare oxidată")],
         s=["Martorul ABS/ESC aprins", "Apare adesea împreună cu alte coduri de tensiune scăzută"]), LAMP)
add("C1112", "abs", "two-sources", [YCN, CMH],
    dict(t="ESC sensor supply voltage fault",
         d="The supply the ESC unit provides to its sensors is out of range. " + ABS_EN,
         c=[("medium", "Shorted sensor or sensor wiring"), ("low", "ESC unit internal fault")], s=["ABS/ESC warning light"]),
    dict(t="Defect la tensiunea de alimentare a senzorilor ESC",
         d="Tensiunea pe care unitatea ESC o furnizează senzorilor săi este în afara limitelor. " + ABS_RO,
         c=[("medium", "Senzor sau cablaj de senzor scurtcircuitat"), ("low", "Defect intern al unității ESC")], s=["Martorul ABS/ESC aprins"]), LAMP)

# Wheel-speed sensors: three codes per wheel.
WHEELS = [
    ("C1200", "front left", "față stânga"),
    ("C1203", "front right", "față dreapta"),
    ("C1206", "rear left", "spate stânga"),
    ("C1209", "rear right", "spate dreapta"),
]
# Sources per code: the two Hyundai/Kia code lists cover every code; extra pages only where they cover that exact code.
WSS_SOURCES = {
    "C1202": ("two-sources", [YCN, CMH, "https://www.autocodes.com/c1202_kia.html"]),
    "C1203": ("official", [WSS_FR, YCN, CMH]),
    "C1205": ("two-sources", [YCN, CMH, "https://www.autocodes.com/c1205_kia.html"]),
    "C1206": ("official", [WSS_REAR, YCN, CMH]), "C1207": ("official", [WSS_REAR, YCN, CMH]),
    "C1208": ("official", [WSS_REAR, YCN, CMH]), "C1209": ("official", [WSS_REAR, YCN, CMH]),
    "C1210": ("official", [WSS_REAR, YCN, CMH]),
    "C1211": ("official", [WSS_REAR, YCN, "https://www.innova.com/blogs/fix-advices/hyundai-elantra-replace-anti-lock-brake-system-abs-wheel-speed-sensor-wss-to-fix-dtc-c1211"]),
}
WSS_KINDS = [
    ("open or short circuit", "circuit întrerupt sau în scurtcircuit",
     "The electrical circuit of the {w} wheel-speed sensor is open or shorted.",
     "Circuitul electric al senzorului de turație al roții {w} este întrerupt sau în scurtcircuit.",
     [("high", "Damaged sensor wire or connector (chafing, corrosion)"), ("medium", "Failed sensor")],
     [("high", "Fir sau conector al senzorului deteriorat (frecare, coroziune)"), ("medium", "Senzor defect")]),
    ("range, performance or intermittent signal", "semnal în afara limitelor sau intermitent",
     "The {w} wheel-speed sensor signal is out of range or comes and goes.",
     "Semnalul senzorului de turație al roții {w} este în afara limitelor sau intermitent.",
     [("medium", "Loose sensor or too large an air gap"), ("medium", "Damaged or dirty tone ring"), ("medium", "Intermittent wiring fault")],
     [("medium", "Senzor slăbit sau distanță prea mare față de inelul dințat"), ("medium", "Inel dințat (tone ring) deteriorat sau murdar"), ("medium", "Defect intermitent de cablaj")]),
    ("invalid or no signal", "semnal invalid sau lipsă",
     "The {w} wheel-speed sensor sends no usable signal while the car moves.",
     "Senzorul de turație al roții {w} nu trimite un semnal utilizabil în timpul mersului.",
     [("high", "Failed sensor"), ("medium", "Missing, cracked or rusted tone ring"), ("medium", "Wheel bearing play")],
     [("high", "Senzor defect"), ("medium", "Inel dințat lipsă, fisurat sau ruginit"), ("medium", "Joc la rulmentul roții")]),
]
for first, wen, wro in WHEELS:
    base = int(first[1:], 16)
    for i, (ken, kro, den, dro, cen, cro) in enumerate(WSS_KINDS):
        code = "C%04X" % (base + i)
        conf, src = WSS_SOURCES.get(code, ("two-sources", [YCN, CMH]))
        add(code, "abs", conf, src,
            dict(t=f"Wheel-speed sensor {wen}: {ken}", d=den.format(w=wen) + " " + ABS_EN, c=cen,
                 s=["ABS, traction and ESC warning lights", "ABS does not work"]),
            dict(t=f"Senzor de turație roată {wro}: {kro}", d=dro.format(w=wro) + " " + ABS_RO, c=cro,
                 s=["Martorii ABS, control tracțiune și ESC aprinși", "ABS-ul nu funcționează"]), LAMP)

# C1213 (wheel-speed difference implausible) is left out: its sources could only be
# read through search-engine summaries.
add("C1235", "abs", "two-sources", [YCN, CMH],
    dict(t="Brake pressure sensor circuit fault",
         d="The sensor that measures brake-fluid pressure in the master cylinder has an electrical fault. Stability control needs it, so ESC is switched off.",
         c=[("medium", "Pressure sensor (often inside the hydraulic unit)"), ("medium", "Wiring or connector")], s=["ESC warning light"]),
    dict(t="Defect la circuitul senzorului de presiune de frânare",
         d="Senzorul care măsoară presiunea lichidului de frână în pompa centrală are un defect electric. Controlul stabilității are nevoie de el, așa că ESC-ul este dezactivat.",
         c=[("medium", "Senzorul de presiune (adesea în unitatea hidraulică)"), ("medium", "Cablaj sau conector")], s=["Martorul ESC aprins"]), LAMP)
add("C1237", "abs", "two-sources", [YCN, CMH, "https://2carpros.com/questions/abs-esc-light"],
    dict(t="Brake pressure sensor signal implausible",
         d="The brake-pressure reading doesn't match what the unit expects, for example pressure without the pedal pressed. ESC is switched off. Often appears together with C1513 (brake light switch).",
         c=[("medium", "Pressure sensor"), ("medium", "Brake light switch out of adjustment")], s=["ABS and ESC warning lights"]),
    dict(t="Semnalul senzorului de presiune de frânare neplauzibil",
         d="Presiunea de frânare citită nu corespunde cu ce așteaptă unitatea, de exemplu presiune fără pedala apăsată. ESC-ul este dezactivat. Apare adesea împreună cu C1513 (contactorul stopurilor).",
         c=[("medium", "Senzorul de presiune"), ("medium", "Contactorul stopurilor dereglat")], s=["Martorii ABS și ESC aprinși"]), LAMP)
add("C1260", "abs", "two-sources", [YCN, CMH],
    dict(t="Steering angle sensor signal fault (seen by ESC)",
         d="Stability control can't trust the steering angle, so ESC is switched off. ABS keeps working.",
         c=[("medium", "Steering angle sensor needs calibration after alignment or battery work"), ("medium", "Sensor or wiring fault")], s=["ESC warning light"]),
    dict(t="Defect la semnalul senzorului de unghi al volanului (văzut de ESC)",
         d="Controlul stabilității nu se poate baza pe unghiul volanului, așa că ESC-ul este dezactivat. ABS-ul funcționează în continuare.",
         c=[("medium", "Senzorul de unghi al volanului trebuie calibrat după geometrie sau lucrări la baterie"), ("medium", "Defect de senzor sau cablaj")], s=["Martorul ESC aprins"]), LAMP)
add("C1261", "abs", "two-sources", [CMH, manual("Hyundai", 2008, "Elantra L4-2.0L", "C", "C1261", page="")],
    dict(t="Steering angle sensor not calibrated",
         d="The steering angle sensor has lost its zero point, typically after the battery, the sensor or the steering column was disconnected, or after a wheel alignment. ESC stays off until the sensor is calibrated with a scan tool.",
         c=[("high", "Battery or steering angle sensor disconnected"), ("medium", "Steering or alignment work without recalibration")], s=["ESC warning light"]),
    dict(t="Senzorul de unghi al volanului necalibrat",
         d="Senzorul de unghi al volanului și-a pierdut punctul zero, de obicei după deconectarea bateriei, a senzorului sau a coloanei de direcție, ori după geometrie. ESC-ul rămâne dezactivat până la calibrarea senzorului cu un tester.",
         c=[("high", "Bateria sau senzorul de unghi deconectate"), ("medium", "Lucrări la direcție sau geometrie fără recalibrare")], s=["Martorul ESC aprins"]), LAMP)
for code, en_t, ro_t in (("C1282", "Yaw-rate / lateral acceleration sensor electrical fault", "Defect electric la senzorul de rotație / accelerație laterală"),
                         ("C1283", "Yaw-rate / lateral acceleration sensor signal implausible", "Semnal neplauzibil de la senzorul de rotație / accelerație laterală")):
    add(code, "abs", "official", [YAW_RECALL, YCN],
        dict(t=en_t, d="The sensor that tells the stability control how the car turns and slides is faulty. On 2011-2012 Elantra this was covered by recall 137 (sensor replacement and a software update for the ABS/ESC unit).",
             c=[("high", "Yaw-rate sensor"), ("medium", "Sensor wiring")], s=["ESC warning light", "Unwanted stability-control intervention"]),
        dict(t=ro_t, d="Senzorul care îi spune controlului stabilității cum virează și derapează mașina este defect. La Elantra 2011-2012 a existat rechemarea 137 (înlocuirea senzorului și actualizarea software-ului unității ABS/ESC).",
             c=[("high", "Senzorul de rotație (yaw)"), ("medium", "Cablajul senzorului")], s=["Martorul ESC aprins", "Intervenții nedorite ale controlului stabilității"]), LAMP)
add("C1503", "abs", "official", [ESC_RECALL],
    dict(t="TCS/ESC OFF switch fault", d="The button that switches traction/stability control off reports an impossible state.",
         c=[("medium", "Switch or its wiring"), ("low", "ABS/ESC unit internal fault")], s=["ESC warning light"]),
    dict(t="Defect la butonul TCS/ESC OFF", d="Butonul care dezactivează controlul tracțiunii/stabilității raportează o stare imposibilă.",
         c=[("medium", "Butonul sau cablajul lui"), ("low", "Defect intern al unității ABS/ESC")], s=["Martorul ESC aprins"]), LAMP)
add("C1513", "abs", "official", [BRAKE_SW_TSB, BRAKE_SW_RCL],
    dict(t="Brake light switch fault",
         d="The brake light switch says the pedal is pressed (or released) while the brake pressure says otherwise. On Elantra MD/UD a broken pedal stopper pad can keep the switch on; Hyundai fixed this under a recall.",
         c=[("high", "Brake light switch faulty or out of adjustment"), ("medium", "Missing or broken brake pedal stopper pad")],
         s=["ESC warning light", "Brake lights stay on, or don't light"]),
    dict(t="Defect la contactorul stopurilor",
         d="Contactorul stopurilor indică pedala apăsată (sau eliberată) în timp ce presiunea de frânare arată altceva. La Elantra MD/UD un tampon opritor al pedalei rupt poate ține contactorul acționat; Hyundai a remediat asta printr-o rechemare.",
         c=[("high", "Contactorul stopurilor defect sau dereglat"), ("medium", "Tamponul opritor al pedalei de frână lipsă sau rupt")],
         s=["Martorul ESC aprins", "Stopurile rămân aprinse sau nu se aprind"]), LAMP)
for code, conf, src, en_t, ro_t in (
        ("C1604", "official", [ESC_RECALL], "ABS/ESC unit internal hardware fault", "Defect hardware intern al unității ABS/ESC"),
        ("C1605", "two-sources", [YCN, CMH], "ABS/ESC unit CAN hardware fault", "Defect hardware CAN al unității ABS/ESC"),
        ("C2112", "official", [ESC_RECALL], "Valve relay fault in the hydraulic unit", "Defect la releul electrovalvelor din unitatea hidraulică"),
        ("C2380", "official", [ESC_RECALL], "ABS/ESC solenoid valve fault", "Defect la electrovalvele ABS/ESC"),
        ("C2402", "official", [ESC_RECALL], "ABS pump motor electrical fault", "Defect electric la motorul pompei ABS")):
    add(code, "abs", conf, src,
        dict(t=en_t, d="The ABS/ESC control unit found a fault in its own electronics or hydraulics. " + ABS_EN + " Check its fuses and connector first; otherwise the unit usually needs repair or replacement.",
             c=[("medium", "ABS/ESC unit (HECU) internal fault"), ("low", "Blown fuse or poor power/ground connection")], s=["ABS and ESC warning lights"]),
        dict(t=ro_t, d="Unitatea de control ABS/ESC a găsit un defect în propria electronică sau hidraulică. " + ABS_RO + " Verifică întâi siguranțele și conectorul; altfel unitatea trebuie de obicei reparată sau înlocuită.",
             c=[("medium", "Defect intern al unității ABS/ESC (HECU)"), ("low", "Siguranță arsă sau conexiune de alimentare/masă proastă")], s=["Martorii ABS și ESC aprinși"]), LAMP)
for code, en_from, ro_from in (("C1611", "the engine computer (ECM)", "calculatorul motorului (ECM)"),
                               ("C1612", "the transmission computer (TCM)", "calculatorul cutiei de viteze (TCM)")):
    add(code, "abs", "two-sources", [YCN, CMH, MDPS_TSB],
        dict(t=f"CAN timeout: no messages from {en_from}",
             d=f"The ABS/ESC unit stopped receiving data from {en_from} over the CAN network. Usually a wiring, connector or power problem at either module rather than a faulty ABS unit.",
             c=[("high", "CAN wiring or connector pins"), ("medium", f"Power or ground of {en_from}"), ("low", "Faulty module")],
             s=["ABS/ESC warning light", "Other warning lights at the same time"]),
        dict(t=f"Timeout CAN: niciun mesaj de la {ro_from}",
             d=f"Unitatea ABS/ESC nu mai primește date de la {ro_from} prin rețeaua CAN. De obicei e o problemă de cablaj, conector sau alimentare la unul dintre module, nu o unitate ABS defectă.",
             c=[("high", "Cablaj CAN sau pini de conector"), ("medium", f"Alimentarea sau masa pentru {ro_from}"), ("low", "Modul defect")],
             s=["Martorul ABS/ESC aprins", "Alți martori aprinși în același timp"]), LAMP)
add("C1613", "abs", "two-sources", [YCN, CMH],
    dict(t="Invalid CAN message received", d="The ABS/ESC unit received a CAN message with impossible content.",
         c=[("medium", "Network or sending-module fault"), ("low", "Mismatched software after a module replacement")], s=["ABS/ESC warning light"]),
    dict(t="Mesaj CAN invalid recepționat", d="Unitatea ABS/ESC a primit un mesaj CAN cu conținut imposibil.",
         c=[("medium", "Defect de rețea sau al modulului emițător"), ("low", "Software nepotrivit după înlocuirea unui modul")], s=["Martorul ABS/ESC aprins"]), LAMP)
add("C1616", "abs", "two-sources", [YCN, CMH, MDPS_TSB],
    dict(t="CAN bus-off (ABS/ESC unit)", d="The ABS/ESC unit dropped off the CAN network because of too many communication errors.",
         c=[("high", "Short or open in the chassis CAN wiring"), ("medium", "A faulty module disturbing the network")], s=["Several warning lights at once"]),
    dict(t="CAN bus-off (unitatea ABS/ESC)", d="Unitatea ABS/ESC s-a deconectat din rețeaua CAN din cauza prea multor erori de comunicație.",
         c=[("high", "Scurtcircuit sau întrerupere în cablajul CAN de șasiu"), ("medium", "Un modul defect care perturbă rețeaua")], s=["Mai mulți martori aprinși deodată"]), LAMP)
add("C2227", "abs", "two-sources", [YCN, CMH],
    dict(t="Brake disc over-temperature (estimated)",
         d="The ESC unit estimates the brake discs got very hot from repeated traction or stability interventions, and limits its braking interventions until they cool. Usually clears by itself.",
         c=[("high", "Long wheel spin on a slippery surface, or hard driving"), ("low", "Dragging brake")], s=["Temporary ESC/TCS limitation"]),
    dict(t="Supratemperatură estimată la discurile de frână",
         d="Unitatea ESC estimează că discurile de frână s-au încălzit foarte tare din cauza intervențiilor repetate ale controlului tracțiunii sau stabilității și își limitează intervențiile până se răcesc. De obicei dispare singur.",
         c=[("high", "Patinare îndelungată pe suprafață alunecoasă sau condus agresiv"), ("low", "Frână care rămâne frecată")], s=["Limitare temporară ESC/TCS"]))

# ============================================================================
# MDPS electric power steering (some codes share numbers with ABS: keyed @steering)
# ============================================================================
STEER_EN = "When the power steering limits or stops its assistance, the steering becomes heavy but still works mechanically."
STEER_RO = "Când servodirecția își limitează sau oprește asistența, volanul devine greu, dar direcția funcționează în continuare mecanic."


def steer(code, title_en, title_ro, d_en, d_ro, causes_en, causes_ro, symptoms=("Power steering warning light",),
          symptoms_ro=("Martorul servodirecției aprins",), src=(MDPS_TSB,), keyed=False):
    add(code, "steering", "official", list(src),
        dict(t=title_en, d=d_en + " " + STEER_EN, c=causes_en, s=list(symptoms)),
        dict(t=title_ro, d=d_ro + " " + STEER_RO, c=causes_ro, s=list(symptoms_ro)),
        LAMP, key=(code + "@steering") if keyed else None)


steer("C1102", "Power steering supply voltage problem", "Problemă la tensiunea de alimentare a servodirecției",
      "The power steering unit measured a supply voltage outside its working range.", "Unitatea servodirecției a măsurat o tensiune de alimentare în afara domeniului de lucru.",
      [("high", "Weak battery or charging fault"), ("medium", "Poor power or ground connection")], [("high", "Baterie slabă sau defect la încărcare"), ("medium", "Conexiune de alimentare sau masă proastă")],
      src=(MDPS_TSB, "https://static.nhtsa.gov/odi/tsbs/2020/MC-10183332-0001.pdf"), keyed=True)
steer("C1112", "Torque sensor supply voltage fault", "Defect la alimentarea senzorului de cuplu",
      "The supply to the steering torque sensor is out of range.", "Alimentarea senzorului de cuplu al direcției este în afara limitelor.",
      [("high", "Wiring or connector between torque sensor and unit")], [("high", "Cablaj sau conector între senzorul de cuplu și unitate")], keyed=True)
steer("C1259", "Steering angle sensor signal fault (in the column)", "Defect la semnalul senzorului de unghi (în coloana de direcție)",
      "The steering angle signal from the column sensor is faulty.", "Semnalul de unghi de la senzorul din coloana de direcție este defect.",
      [("medium", "Torque/angle sensor wiring"), ("medium", "Steering column assembly")], [("medium", "Cablajul senzorului de cuplu/unghi"), ("medium", "Ansamblul coloanei de direcție")])
steer("C1260", "Steering angle sensor signal fault", "Defect la semnalul senzorului de unghi al volanului",
      "The steering angle signal is implausible, often because the sensor needs a calibration (reset).", "Semnalul de unghi al volanului este neplauzibil, adesea pentru că senzorul trebuie calibrat (resetat).",
      [("high", "Sensor needs calibration"), ("medium", "Wiring")], [("high", "Senzorul trebuie calibrat"), ("medium", "Cablaj")], keyed=True)
steer("C1261", "Steering angle zero point not set", "Punctul zero al unghiului de virare nesetat",
      "The steering angle sensor has no valid zero point and must be calibrated with a scan tool.", "Senzorul de unghi al volanului nu are un punct zero valid și trebuie calibrat cu un tester.",
      [("high", "Calibration lost after battery or steering work")], [("high", "Calibrare pierdută după lucrări la baterie sau direcție")], keyed=True)
steer("C1290", "Torque sensor signal fault", "Defect la semnalul senzorului de cuplu",
      "The sensor that measures how hard you turn the wheel gives an implausible signal, so assistance may stop.", "Senzorul care măsoară cât de tare rotești volanul dă un semnal neplauzibil, așa că asistența se poate opri.",
      [("high", "Torque sensor cable or wiring"), ("medium", "Steering column / sensor assembly")], [("high", "Cablul sau cablajul senzorului de cuplu"), ("medium", "Ansamblul coloanei / senzorului")],
      symptoms=("Power steering warning light", "Heavy steering"), symptoms_ro=("Martorul servodirecției aprins", "Volan greu"), src=(MDPS_TSB, MDPS_TSB2))
steer("C1603", "Power steering thermal protection active", "Protecția termică a servodirecției activă",
      "The electric motor got hot from a lot of steering at low speed (for example repeated parking manoeuvres) and assistance is reduced to protect it. Nothing needs replacing; it recovers as the motor cools.",
      "Motorul electric s-a încălzit din cauza multor manevre de direcție la viteză mică (de exemplu parcări repetate) și asistența este redusă pentru a-l proteja. Nu trebuie înlocuit nimic; revine pe măsură ce motorul se răcește.",
      [("high", "Heavy steering use at standstill or low speed")], [("high", "Utilizare intensă a direcției pe loc sau la viteză mică")],
      symptoms=("Temporarily heavier steering",), symptoms_ro=("Volan temporar mai greu",))
steer("C1604", "Power steering unit over/discharge fault", "Defect de supraîncărcare/descărcare a unității servodirecției",
      "The power steering control unit reports an internal power-stage fault.", "Unitatea de control a servodirecției raportează un defect intern în etajul de putere.",
      [("medium", "Unit wiring or connector"), ("medium", "Power steering control unit")], [("medium", "Cablajul sau conectorul unității"), ("medium", "Unitatea de control a servodirecției")], keyed=True)
for code, en_from, ro_from in (("C1611", "the engine computer (ECM)", "calculatorul motorului (ECM)"),
                               ("C1612", "the transmission computer (TCM)", "calculatorul cutiei de viteze (TCM)"),
                               ("C1646", "the transmission computer (TCM)", "calculatorul cutiei de viteze (TCM)")):
    steer(code, f"Signal from {en_from} missing or invalid (CAN)", f"Semnal lipsă sau invalid de la {ro_from} (CAN)",
          f"The power steering unit doesn't get valid data from {en_from} over CAN; it uses engine and speed data to decide how much to assist.",
          f"Unitatea servodirecției nu primește date valide de la {ro_from} prin CAN; folosește datele de motor și viteză ca să decidă cât să asiste.",
          [("high", "CAN wiring or connectors"), ("medium", f"Power or ground of {en_from}")], [("high", "Cablaj sau conectori CAN"), ("medium", f"Alimentarea sau masa pentru {ro_from}")],
          keyed=code in ("C1611", "C1612"))
steer("C1616", "CAN bus-off (power steering)", "CAN bus-off (servodirecție)",
      "The power steering unit dropped off the CAN network.", "Unitatea servodirecției s-a deconectat din rețeaua CAN.",
      [("high", "Chassis CAN wiring")], [("high", "Cablajul CAN de șasiu")], keyed=True)
steer("C1622", "Vehicle speed signal fault from the engine computer", "Defect la semnalul de viteză de la calculatorul motorului",
      "The power steering unit receives an invalid vehicle-speed signal, which it needs to reduce assistance at speed.", "Unitatea servodirecției primește un semnal de viteză invalid, de care are nevoie ca să reducă asistența la viteză mare.",
      [("medium", "Engine computer or CAN wiring")], [("medium", "Calculatorul motorului sau cablajul CAN")])
steer("C1623", "CAN timeout from the steering angle sensor", "Timeout CAN de la senzorul de unghi al volanului",
      "The steering angle sensor stopped sending data over CAN.", "Senzorul de unghi al volanului nu mai trimite date prin CAN.",
      [("medium", "Power steering fuse, power or ground"), ("medium", "CAN wiring"), ("low", "Power steering unit")], [("medium", "Siguranța, alimentarea sau masa servodirecției"), ("medium", "Cablaj CAN"), ("low", "Unitatea servodirecției")])
for code in ("C1628", "C1656"):
    steer(code, "Instrument cluster signal fault (CAN)", "Defect la semnalul de la bord (CAN)",
          "The power steering unit receives an invalid signal from the instrument cluster.", "Unitatea servodirecției primește un semnal invalid de la bord.",
          [("medium", "Cluster or CAN wiring")], [("medium", "Bordul sau cablajul CAN")])
for code, d_en, d_ro in (("C1687", "No data from the stability control (VSM/ESC) reaches the power steering unit.", "Nu ajung date de la controlul stabilității (VSM/ESC) la unitatea servodirecției."),
                         ("C1688", "The stability control (VSM/ESC) sends invalid data to the power steering unit.", "Controlul stabilității (VSM/ESC) trimite date invalide către unitatea servodirecției."),
                         ("C1692", "The stability control (VSM/ESC) signal on CAN is invalid.", "Semnalul controlului stabilității (VSM/ESC) pe CAN este invalid."),
                         ("C1693", "The stability control (VSM/ESC) signal on CAN is invalid.", "Semnalul controlului stabilității (VSM/ESC) pe CAN este invalid.")):
    steer(code, "Stability control (VSM) signal fault", "Defect la semnalul controlului stabilității (VSM)", d_en, d_ro,
          [("medium", "CAN wiring between power steering and ESC"), ("medium", "Power steering power or ground")],
          [("medium", "Cablajul CAN între servodirecție și ESC"), ("medium", "Alimentarea sau masa servodirecției")])
for code, title_en, title_ro, src in (
        ("C1704", "Power steering fail-safe relay fault", "Defect la releul de siguranță al servodirecției", (MDPS_TSB, MDPS_TSB2)),
        ("C1705", "Power steering control unit hardware fault", "Defect hardware al unității servodirecției", (MDPS_TSB,)),
        ("C2413", "Power steering control unit fault (motor current)", "Defect al unității servodirecției (curent motor)", (MDPS_TSB,))):
    steer(code, title_en, title_ro, "The power steering control unit found an internal fault and may stop assisting.",
          "Unitatea de control a servodirecției a găsit un defect intern și poate opri asistența.",
          [("medium", "Unit wiring or connector"), ("medium", "Power steering control unit")], [("medium", "Cablajul sau conectorul unității"), ("medium", "Unitatea de control a servodirecției")],
          symptoms=("Power steering warning light", "Heavy steering"), symptoms_ro=("Martorul servodirecției aprins", "Volan greu"), src=src)
for code, title_en, title_ro in (("C2400", "Power steering motor signal fault", "Defect la semnalul motorului servodirecției"),
                                 ("C2401", "Power steering motor circuit fault", "Defect la circuitul motorului servodirecției"),
                                 ("C2412", "Power steering motor supply fault", "Defect la alimentarea motorului servodirecției")):
    steer(code, title_en, title_ro, "The electric assist motor or its wiring has a fault.", "Motorul electric de asistare sau cablajul lui are un defect.",
          [("medium", "Motor wiring between unit and motor"), ("medium", "Assist motor")], [("medium", "Cablajul dintre unitate și motor"), ("medium", "Motorul de asistare")],
          symptoms=("Power steering warning light", "Heavy steering"), symptoms_ro=("Martorul servodirecției aprins", "Volan greu"))

# ============================================================================
# Airbag (SRS)
# ============================================================================
SRS_EN = ("While an airbag fault is stored, the affected airbag or pretensioner may not deploy in a crash. "
          "Have it repaired before relying on it. Never probe airbag connectors with a meter: that can fire the airbag.")
SRS_RO = ("Cât timp un defect de airbag este memorat, airbagul sau pretensionatorul afectat s-ar putea să nu se declanșeze la impact. "
          "Repară-l înainte să te bazezi pe el. Nu măsura niciodată conectorii de airbag cu un aparat: poate declanșa airbagul.")
SRS_LAMP_EN = ["Airbag warning light"]
SRS_LAMP_RO = ["Martorul airbag aprins"]


def srs(code, title_en, title_ro, d_en, d_ro, causes_en, causes_ro, src=(SRS_CHART, YCN), key=None):
    add(code, "airbag", "official", list(src),
        dict(t=title_en, d=d_en + " " + SRS_EN, c=causes_en, s=SRS_LAMP_EN),
        dict(t=title_ro, d=d_ro + " " + SRS_RO, c=causes_ro, s=SRS_LAMP_RO), LAMP, key=key)


srs("B1101", "Airbag unit supply voltage too high", "Tensiune de alimentare a unității airbag prea mare",
    "The airbag control unit measured more than about 16 V.", "Unitatea de control airbag a măsurat peste aproximativ 16 V.",
    [("high", "Charging-system overvoltage")], [("high", "Supratensiune de la sistemul de încărcare")])
srs("B1102", "Airbag unit supply voltage too low", "Tensiune de alimentare a unității airbag prea mică",
    "The airbag control unit measured less than about 9 V.", "Unitatea de control airbag a măsurat sub aproximativ 9 V.",
    [("high", "Weak battery or charging fault"), ("medium", "Poor supply or ground connection")], [("high", "Baterie slabă sau defect la încărcare"), ("medium", "Conexiune de alimentare sau masă proastă")])
for code, side_en, side_ro, kind in (("B1328", "driver", "șofer", "defect"), ("B1329", "driver", "șofer", "comm"),
                                     ("B1333", "passenger", "pasager", "defect"), ("B1334", "passenger", "pasager", "comm")):
    srs(code, f"Front impact sensor ({side_en} side): " + ("sensor fault" if kind == "defect" else "communication fault"),
        f"Senzor de impact frontal (partea {side_ro}): " + ("defect de senzor" if kind == "defect" else "defect de comunicație"),
        f"The front crash sensor on the {side_en} side " + ("reports an internal fault." if kind == "defect" else "doesn't communicate correctly with the airbag unit."),
        f"Senzorul de impact frontal din partea {side_ro} " + ("raportează un defect intern." if kind == "defect" else "nu comunică corect cu unitatea airbag."),
        [("medium", "Sensor"), ("medium", "Sensor wiring or connector (front-end damage)")], [("medium", "Senzorul"), ("medium", "Cablajul sau conectorul senzorului (avarie în față)")])

# Firing circuits ("squibs"): four codes each.
SQUIB_KINDS = [
    ("resistance too high", "rezistență prea mare", "The circuit's resistance is too high, as from a loose connector or a broken wire.",
     "Rezistența circuitului este prea mare, ca la un conector slăbit sau un fir rupt."),
    ("resistance too low", "rezistență prea mică", "The circuit's resistance is too low, as from a short between its two wires.",
     "Rezistența circuitului este prea mică, ca la un scurtcircuit între cele două fire."),
    ("short to ground", "scurtcircuit la masă", "The circuit is shorted to ground.", "Circuitul este în scurtcircuit la masă."),
    ("short to battery", "scurtcircuit la plus", "The circuit is shorted to a supply voltage.", "Circuitul este în scurtcircuit la o tensiune de alimentare."),
]
SQUIBS = [
    ("B1346", "Driver front airbag, stage 1", "Airbag frontal șofer, treapta 1", True),
    ("B1481", "Driver front airbag, stage 2", "Airbag frontal șofer, treapta 2", True),
    ("B1352", "Passenger front airbag, stage 1", "Airbag frontal pasager, treapta 1", False),
    ("B1485", "Passenger front airbag, stage 2", "Airbag frontal pasager, treapta 2", False),
    ("B1361", "Driver seat-belt pretensioner", "Pretensioner centură șofer", False),
    ("B1367", "Passenger seat-belt pretensioner", "Pretensioner centură pasager", False),
    ("B1378", "Driver seat side airbag", "Airbag lateral scaun șofer", False),
    ("B1382", "Passenger seat side airbag", "Airbag lateral scaun pasager", False),
    ("B1473", "Driver-side curtain airbag", "Airbag cortină partea șoferului", False),
    ("B1477", "Passenger-side curtain airbag", "Airbag cortină partea pasagerului", False),
]
for first, name_en, name_ro, driver_wheel in SQUIBS:
    base = int(first[1:], 16)
    for i, (k_en, k_ro, d_en, d_ro) in enumerate(SQUIB_KINDS):
        code = "B%04X" % (base + i)
        if driver_wheel:
            # Hyundai extended the warranty of the clock spring (the coil behind the
            # steering wheel) for 2011-2014 Elantra MD/UD over exactly these codes.
            causes_en = [("high", "Clock spring behind the steering wheel (warranty extension on 2011-2014 Elantra)"),
                         ("medium", "Airbag connector"), ("low", "Airbag module")]
            causes_ro = [("high", "Spirala (clock spring) din spatele volanului (garanție extinsă la Elantra 2011-2014)"),
                         ("medium", "Conectorul airbagului"), ("low", "Modulul airbag")]
            src = (CLOCKSPRING, SRS_CHART, CLOCKSPRING_KIA)
        else:
            causes_en = [("medium", "Connector (often under the seat or behind trim)"), ("medium", "Wiring"), ("low", "The airbag or pretensioner itself")]
            causes_ro = [("medium", "Conector (adesea sub scaun sau după ornamente)"), ("medium", "Cablaj"), ("low", "Airbagul sau pretensionatorul propriu-zis")]
            src = (SRS_CHART, YCN)
        srs(code, f"{name_en}: {k_en}", f"{name_ro}: {k_ro}",
            f"Firing circuit of the {name_en.lower()}. {d_en}", f"Circuitul de declanșare pentru {name_ro.lower()}. {d_ro}",
            causes_en, causes_ro, src=src)
srs("B1395", "Airbag firing circuits cross-connected", "Circuite de declanșare airbag interconectate",
    "Two airbag firing circuits are electrically connected to each other.", "Două circuite de declanșare a airbagurilor sunt conectate electric între ele.",
    [("high", "Harness damage or a wrongly mated connector")], [("high", "Cablaj deteriorat sau conector montat greșit")])
for code, side_en, side_ro, kind in (("B1400", "driver", "șofer", "defect"), ("B1409", "driver", "șofer", "comm"),
                                     ("B1403", "passenger", "pasager", "defect"), ("B1410", "passenger", "pasager", "comm")):
    srs(code, f"Side impact sensor ({side_en} side): " + ("sensor fault" if kind == "defect" else "communication fault"),
        f"Senzor de impact lateral (partea {side_ro}): " + ("defect de senzor" if kind == "defect" else "defect de comunicație"),
        f"The side crash sensor on the {side_en} side " + ("reports an internal fault." if kind == "defect" else "doesn't communicate correctly with the airbag unit."),
        f"Senzorul de impact lateral din partea {side_ro} " + ("raportează un defect intern." if kind == "defect" else "nu comunică corect cu unitatea airbag."),
        [("medium", "Sensor"), ("medium", "Wiring or connector in the door pillar")], [("medium", "Senzorul"), ("medium", "Cablaj sau conector în stâlpul ușii")])
for code, seat_en, seat_ro, kind_en, kind_ro in (
        ("B1511", "Driver", "șofer", "open or short to battery", "circuit întrerupt sau scurtcircuit la plus"),
        ("B1512", "Driver", "șofer", "short to ground", "scurtcircuit la masă"),
        ("B1515", "Driver", "șofer", "switch fault", "contact defect"),
        ("B1513", "Passenger", "pasager", "open or short to battery", "circuit întrerupt sau scurtcircuit la plus"),
        ("B1514", "Passenger", "pasager", "short to ground", "scurtcircuit la masă"),
        ("B1516", "Passenger", "pasager", "switch fault", "contact defect")):
    srs(code, f"{seat_en} seat-belt buckle switch: {kind_en}", f"Contactul cataramei centurii {seat_ro}: {kind_ro}",
        "The airbag unit uses the buckle switch to know whether the seat belt is fastened.", "Unitatea airbag folosește contactul cataramei ca să știe dacă centura este cuplată.",
        [("high", "Buckle switch or the connector under the seat")], [("high", "Contactul cataramei sau conectorul de sub scaun")])
srs("B1620", "Airbag control unit internal fault", "Defect intern al unității de control airbag",
    "The airbag control unit found a fault in itself and must be replaced.", "Unitatea de control airbag a găsit un defect propriu și trebuie înlocuită.",
    [("high", "Airbag control unit")], [("high", "Unitatea de control airbag")])
for code, en_t, ro_t, d_en, d_ro in (
        ("B1650", "Frontal crash recorded (stage 1 deployed)", "Impact frontal înregistrat (treapta 1 declanșată)",
         "The airbag unit recorded a frontal crash in which the first stage deployed.", "Unitatea airbag a înregistrat un impact frontal în care s-a declanșat prima treaptă."),
        ("B1670", "Frontal crash recorded (both stages deployed)", "Impact frontal înregistrat (ambele trepte declanșate)",
         "The airbag unit recorded a frontal crash in which both stages deployed.", "Unitatea airbag a înregistrat un impact frontal în care s-au declanșat ambele trepte."),
        ("B1651", "Side crash recorded (driver side)", "Impact lateral înregistrat (partea șoferului)",
         "The airbag unit recorded a side crash on the driver side.", "Unitatea airbag a înregistrat un impact lateral pe partea șoferului."),
        ("B1652", "Side crash recorded (passenger side)", "Impact lateral înregistrat (partea pasagerului)",
         "The airbag unit recorded a side crash on the passenger side.", "Unitatea airbag a înregistrat un impact lateral pe partea pasagerului.")):
    srs(code, en_t, ro_t, d_en + " Crash data cannot be cleared: the unit must be replaced, together with every deployed part. On a used car, this tells you it has been in a crash.",
        d_ro + " Datele de impact nu se pot șterge: unitatea trebuie înlocuită, împreună cu toate componentele declanșate. La o mașină second-hand, îți spune că a fost implicată într-un accident.",
        [("high", "A crash took place")], [("high", "A avut loc un impact")])
srs("B1763", "Passenger occupant detection unit fault", "Defect al unității de detectare a pasagerului",
    "The unit that detects whether someone sits in the passenger seat (to switch the passenger airbag on or off) reports a fault. Not every model has it.",
    "Unitatea care detectează dacă cineva stă pe scaunul pasagerului (ca să activeze sau să dezactiveze airbagul pasagerului) raportează un defect. Nu toate modelele o au.",
    [("medium", "Occupant detection unit or its wiring")], [("medium", "Unitatea de detectare sau cablajul ei")], src=(OCS_TSB,))
srs("B1764", "Passenger occupant detection mat fault", "Defect al senzorului de ocupare din scaunul pasagerului",
    "The sensor mat in the passenger seat cushion reports a fault. Not every model has it.", "Senzorul din perna scaunului pasagerului raportează un defect. Nu toate modelele îl au.",
    [("medium", "Seat sensor mat"), ("medium", "Seat wiring")], [("medium", "Senzorul din scaun"), ("medium", "Cablajul scaunului")], src=(OCS_TSB,))
srs("B2500", "Airbag warning lamp circuit fault", "Defect la circuitul martorului airbag",
    "The circuit of the airbag warning lamp itself is shorted, so the lamp may not be able to warn you.", "Circuitul martorului airbag este în scurtcircuit, așa că martorul s-ar putea să nu te poată avertiza.",
    [("medium", "Lamp circuit in the instrument cluster or wiring")], [("medium", "Circuitul martorului din bord sau cablajul")])
srs("B2502", "Passenger airbag ON/OFF lamp fault", "Defect la indicatorul ON/OFF al airbagului pasager",
    "The lamp that shows whether the passenger airbag is enabled has a circuit fault.", "Indicatorul care arată dacă airbagul pasagerului este activ are un defect de circuit.",
    [("medium", "Indicator lamp circuit")], [("medium", "Circuitul indicatorului")])

# ============================================================================
# TPMS
# ============================================================================
# C1121-C1124 (sensor battery low) are left out: their sources could only be read
# through search-engine summaries, which is not enough to cite them.
for i in range(4):
    add("C%04X" % (0x1312 + i), "tpms", "official", [TPMS_TSB],
        dict(t=f"Tyre pressure sensor {i + 1}: no radio signal",
             d=f"The TPMS unit stopped receiving tyre-pressure sensor number {i + 1}. Radio interference often sets this code as a history code that clears itself; a sensor that was never learned after a wheel change, or a failed sensor, keeps it active.",
             c=[("medium", "Radio interference (chargers, dash cams, aftermarket electronics)"), ("medium", "Sensor not learned after a wheel or tyre change"), ("medium", "Failed sensor")],
             s=["TPMS warning light after about 25 minutes of driving", "Low-pressure lamp blinks, then stays on (models without a separate TPMS lamp)"]),
        dict(t=f"Senzor de presiune anvelopă {i + 1}: lipsă semnal radio",
             d=f"Unitatea TPMS nu mai recepționează senzorul de presiune numărul {i + 1}. Interferențele radio setează adesea acest cod ca istoric, care dispare singur; un senzor neînvățat după schimbarea roților sau un senzor defect îl țin activ.",
             c=[("medium", "Interferențe radio (încărcătoare, camere de bord, electronice adăugate)"), ("medium", "Senzor neînvățat după schimbarea roților sau anvelopelor"), ("medium", "Senzor defect")],
             s=["Martorul TPMS se aprinde după aproximativ 25 de minute de mers", "Martorul de presiune clipește, apoi rămâne aprins (modele fără martor TPMS separat)"]), LAMP)

# ============================================================================
# Body control / smart key
# ============================================================================
add("B1602", "body", "official", [BCM_TSB, YCN, CMH],
    dict(t="Body CAN communication error",
         d="A module on the body CAN network (body computer, smart key, cluster...) reports communication errors. A module that keeps the network awake can drain the battery overnight.",
         c=[("medium", "Body CAN wiring"), ("medium", "A failing module on the body network")], s=["Battery drained after standing", "Odd behaviour of body functions"]),
    dict(t="Eroare de comunicație pe CAN-ul de caroserie",
         d="Un modul din rețeaua CAN de caroserie (calculatorul de caroserie, smart key, bord...) raportează erori de comunicație. Un modul care ține rețeaua trează poate descărca bateria peste noapte.",
         c=[("medium", "Cablajul CAN de caroserie"), ("medium", "Un modul defect din rețeaua de caroserie")], s=["Bateria descărcată după staționare", "Funcții de caroserie care se comportă ciudat"]))
add("B1603", "body", "official", [BCM_TSB, YCN, "https://www.go-parts.com/garage/obd-b1603"],
    dict(t="Body CAN bus-off",
         d="A module dropped off the body CAN network. On cars with a smart key, a failing smart-key unit can also cause a 'key not detected' no-start.",
         c=[("medium", "Body CAN wiring"), ("medium", "Smart-key unit or another body module")], s=["Battery drain", "Possible no-start on smart-key cars"]),
    dict(t="CAN bus-off pe rețeaua de caroserie",
         d="Un modul s-a deconectat din rețeaua CAN de caroserie. La mașinile cu smart key, o unitate smart key defectă poate provoca și nepornirea cu mesajul „cheie nedetectată”.",
         c=[("medium", "Cablajul CAN de caroserie"), ("medium", "Unitatea smart key sau alt modul de caroserie")], s=["Descărcarea bateriei", "Posibilă nepornire la mașinile cu smart key"]))
add("B1978", "body", "two-sources", [CMH, "https://www.tat.net.au/scandata/codes/code-view.php?id=8407"],
    dict(t="Electronic steering lock fault",
         d="On push-button-start cars with an electronic steering column lock, the lock reports a fault or its locked/unlocked feedback doesn't match.",
         c=[("medium", "Steering lock unit"), ("low", "Wiring")], s=["Possible no-start", "Steering stays locked"]),
    dict(t="Defect la blocajul electronic al volanului",
         d="La mașinile cu pornire din buton și blocaj electronic al coloanei de direcție, blocajul raportează un defect sau starea blocat/deblocat nu corespunde.",
         c=[("medium", "Unitatea de blocare a volanului"), ("low", "Cablaj")], s=["Posibilă nepornire", "Volanul rămâne blocat"]))

# ============================================================================
# Automatic climate control (self-diagnosis codes)
# ============================================================================
CLIMATE = [
    ("B1208", "Recirculation door position sensor: open or low", "Senzor poziție clapetă recirculare: întrerupt sau semnal mic"),
    ("B1209", "Recirculation door position sensor: short or high", "Senzor poziție clapetă recirculare: scurtcircuit sau semnal mare"),
    ("B1233", "In-car temperature sensor: short or low", "Senzor temperatură habitaclu: scurtcircuit sau semnal mic"),
    ("B1234", "In-car temperature sensor: open or high", "Senzor temperatură habitaclu: întrerupt sau semnal mare"),
    ("B1237", "Outside temperature sensor: short or low", "Senzor temperatură exterioară: scurtcircuit sau semnal mic"),
    ("B1238", "Outside temperature sensor: open or high", "Senzor temperatură exterioară: întrerupt sau semnal mare"),
    ("B1241", "Evaporator temperature sensor: short or low", "Senzor temperatură evaporator: scurtcircuit sau semnal mic"),
    ("B1242", "Evaporator temperature sensor: open or high", "Senzor temperatură evaporator: întrerupt sau semnal mare"),
    ("B1245", "Temperature blend door position sensor (driver): open or low", "Senzor poziție clapetă amestec temperatură (șofer): întrerupt sau semnal mic"),
    ("B1246", "Temperature blend door position sensor (driver): short or high", "Senzor poziție clapetă amestec temperatură (șofer): scurtcircuit sau semnal mare"),
    ("B1249", "Air distribution door position sensor (driver): open or low", "Senzor poziție clapetă distribuție aer (șofer): întrerupt sau semnal mic"),
    ("B1250", "Air distribution door position sensor (driver): short or high", "Senzor poziție clapetă distribuție aer (șofer): scurtcircuit sau semnal mare"),
]
CLIMATE_SRC = [YCN, CMH]
for code, en_t, ro_t in CLIMATE:
    add(code, "climate", "two-sources", CLIMATE_SRC + (["https://www.go-parts.com/garage/obd-b1241"] if code == "B1241" else []),
        dict(t=en_t, d="The automatic climate control found a fault in one of its sensors or flap position sensors. It keeps working with a substitute value, but temperature or air direction may not follow the settings.",
             c=[("medium", "The sensor or actuator"), ("medium", "Wiring or connector behind the dashboard")], s=["Temperature or airflow doesn't respond as set"]),
        dict(t=ro_t, d="Climatizarea automată a găsit un defect la unul dintre senzori sau la un senzor de poziție a clapetelor. Funcționează în continuare cu o valoare de rezervă, dar temperatura sau direcția aerului s-ar putea să nu urmeze setările.",
             c=[("medium", "Senzorul sau actuatorul"), ("medium", "Cablaj sau conector din spatele bordului")], s=["Temperatura sau fluxul de aer nu răspund cum sunt setate"]))
for code, en_t, ro_t in (("B2406", "Temperature blend door motor fault (driver)", "Defect la motorul clapetei de amestec temperatură (șofer)"),
                         ("B2408", "Recirculation door motor fault", "Defect la motorul clapetei de recirculare"),
                         ("B2409", "Air distribution door motor fault (driver)", "Defect la motorul clapetei de distribuție a aerului (șofer)")):
    add(code, "climate", "two-sources", CLIMATE_SRC,
        dict(t=en_t, d="A flap motor of the climate control doesn't reach the position it was told to. A clicking noise behind the dashboard often points to a broken actuator gear.",
             c=[("high", "Actuator motor or its gears"), ("medium", "Jammed flap or linkage"), ("low", "Wiring")], s=["Clicking behind the dashboard", "Air temperature or direction stuck"]),
        dict(t=ro_t, d="Un motor de clapetă al climatizării nu ajunge în poziția comandată. Un clicăit din spatele bordului indică adesea un pinion de actuator rupt.",
             c=[("high", "Motorul actuatorului sau pinioanele lui"), ("medium", "Clapetă sau tijă blocată"), ("low", "Cablaj")], s=["Clicăit în spatele bordului", "Temperatura sau direcția aerului blocate"]))


# ============================================================================
# Engine: knock sensor detection (KSDS), immobilizer (SMARTRA), battery sensor
# ============================================================================
KSDS = ["https://static.nhtsa.gov/odi/tsbs/2021/MC-10200346-0001.pdf",
        "https://static.nhtsa.gov/odi/tsbs/2020/MC-10172065-0002.pdf",
        "https://static.nhtsa.gov/odi/tsbs/2024/MC-11010507-0001.pdf"]
add("P1326", "engine", "official", KSDS,
    dict(t="Knock Sensor Detection System: possible connecting-rod bearing wear",
         d="Engines with the Knock Sensor Detection System (KSDS) software listen with the knock sensor for the vibration that early "
           "connecting-rod bearing wear makes. When they hear it, the check-engine light flashes and the engine goes into limp mode to "
           "protect itself. Stop driving soon and have a dealer check it: Hyundai/Kia cover this under campaigns and extended warranties "
           "on many Theta, Nu and Gamma GDI engines. Not every 2010-2016 model has KSDS.",
         c=[("high", "Connecting-rod bearing wear"), ("low", "Interference on the knock sensor wiring (checked first by the dealer)")],
         s=["Flashing check-engine light", "Limp mode: engine speed limited to about 2000 rpm", "Possible knocking noise"]),
    dict(t="Sistemul de detectare prin senzorul de detonație: posibilă uzură a cuzineților de bielă",
         d="Motoarele cu software KSDS (Knock Sensor Detection System) ascultă prin senzorul de detonație vibrația pe care o produce "
           "uzura incipientă a cuzineților de bielă. Când o detectează, martorul de motor clipește și motorul intră în modul de avarie ca "
           "să se protejeze. Oprește-te curând și mergi la dealer: Hyundai/Kia acoperă problema prin campanii și garanții extinse la multe "
           "motoare Theta, Nu și Gamma GDI. Nu toate modelele 2010-2016 au KSDS.",
         c=[("high", "Uzura cuzineților de bielă"), ("low", "Interferențe pe cablajul senzorului de detonație (verificate întâi de dealer)")],
         s=["Martorul de motor clipește", "Mod de avarie: turația limitată la aproximativ 2000 rot/min", "Posibil zgomot de bătaie"]),
    ("mil", "limp_mode"), key="P1326@engine")
# Without module context (e.g. a manual lookup) the same text applies.
PACK["P1326"] = dict(PACK["P1326@engine"])

IMMO_EN = ("The immobilizer (SMARTRA) checks the chip in the key before the engine computer allows a start. ")
IMMO_RO = ("Imobilizatorul (SMARTRA) verifică cipul din cheie înainte ca calculatorul motorului să permită pornirea. ")
IMMO = [
    ("P1676", [manual("Hyundai", 2012, "Accent L4-1.6L", "P", "P1676"), "https://www.youcanic.com/hyundai-fault-code-dtc/"],
     "Invalid message from the immobilizer unit", "Mesaj invalid de la unitatea de imobilizator",
     "The engine computer received a faulty message from the SMARTRA immobilizer unit.",
     "Calculatorul motorului a primit un mesaj defect de la unitatea de imobilizator SMARTRA.",
     [("medium", "SMARTRA unit"), ("medium", "Wiring or connector between SMARTRA and engine computer")],
     [("medium", "Unitatea SMARTRA"), ("medium", "Cablaj sau conector între SMARTRA și calculatorul motorului")], ["Possible no-start"], ["Posibilă nepornire"]),
    ("P1690", [manual("Kia", 2012, "Rio L4-1.6L", "P", "P1690"), "https://www.youcanic.com/hyundai-fault-code-dtc/",
               "https://iaaf.co.uk/news/tecrmi-what-the-fix-hyundai-i30"],
     "No answer from the immobilizer unit", "Niciun răspuns de la unitatea de imobilizator",
     "The engine computer gets no response from the SMARTRA immobilizer unit. On i30 FD, a corroded immobilizer connector in the A-pillar is a known cause.",
     "Calculatorul motorului nu primește răspuns de la unitatea de imobilizator SMARTRA. La i30 FD, un conector de imobilizator oxidat în stâlpul A este o cauză cunoscută.",
     [("medium", "SMARTRA unit"), ("medium", "Corroded connector or wiring"), ("low", "Antenna coil circuit")],
     [("medium", "Unitatea SMARTRA"), ("medium", "Conector sau cablaj oxidat"), ("low", "Circuitul bobinei antenei")],
     ["Cranks but won't start, often intermittently"], ["Demarorul se învârte, dar motorul nu pornește, adesea intermitent"]),
    ("P1691", [manual("Kia", 2012, "Rio L4-1.6L", "P", "P1691"), "https://blog.upfix.com/hyundai-factory-dtc-code-definitions"],
     "Immobilizer antenna coil open or shorted", "Bobina antenei imobilizatorului întreruptă sau în scurtcircuit",
     "The antenna coil around the ignition lock, which reads the key chip, has an open or short circuit.",
     "Bobina antenei din jurul contactului, care citește cipul cheii, are circuitul întrerupt sau în scurtcircuit.",
     [("high", "Antenna coil"), ("medium", "Wiring or connector to the SMARTRA unit")], [("high", "Bobina antenei"), ("medium", "Cablaj sau conector spre unitatea SMARTRA")], ["No start"], ["Nu pornește"]),
    ("P1694", [manual("Hyundai", 2011, "Accent L4-1.6L", "P", "P1694"), "https://blog.upfix.com/hyundai-factory-dtc-code-definitions"],
     "Invalid request from the engine computer to the immobilizer", "Cerere invalidă de la calculatorul motorului către imobilizator",
     "The communication line between the engine computer and the SMARTRA unit carried an invalid request.",
     "Linia de comunicație dintre calculatorul motorului și unitatea SMARTRA a transmis o cerere invalidă.",
     [("medium", "Wiring between engine computer and SMARTRA"), ("low", "Engine computer")], [("medium", "Cablajul dintre calculatorul motorului și SMARTRA"), ("low", "Calculatorul motorului")],
     ["Possible no-start"], ["Posibilă nepornire"]),
    ("P1695", [manual("Kia", 2012, "Rio L4-1.6L", "P", "P1695"), "https://www.youcanic.com/hyundai-fault-code-dtc/", "https://obdadvisor.com/codes/code/kia-p1695"],
     "Immobilizer memory in the engine computer inconsistent", "Memoria imobilizatorului din calculatorul motorului neconsistentă",
     "The immobilizer data stored in the engine computer (key count, PIN state) doesn't add up. Re-teaching all keys usually fixes it; the computer is replaced only if the code returns.",
     "Datele de imobilizator salvate în calculatorul motorului (numărul de chei, starea PIN) nu se potrivesc. De obicei se rezolvă prin reînvățarea tuturor cheilor; calculatorul se înlocuiește doar dacă codul revine.",
     [("high", "Corrupted immobilizer data")], [("high", "Date de imobilizator corupte")], ["No start possible"], ["Pornirea poate fi imposibilă"]),
    ("P1696", [manual("Kia", 2012, "Forte L4-2.4L", "P", "P1696"), "https://www.youcanic.com/hyundai-fault-code-dtc/", "https://www.elektroda.com/rtvforum/topic3204706.html"],
     "Key authentication failed", "Autentificarea cheii a eșuat",
     "The key was read but not accepted: it isn't taught to this car, or its transponder is faulty.",
     "Cheia a fost citită, dar nu a fost acceptată: nu este învățată pe această mașină sau transponderul ei este defect.",
     [("high", "Key not taught to this car"), ("medium", "Faulty key transponder")], [("high", "Cheie neînvățată pe această mașină"), ("medium", "Transponderul cheii defect")],
     ["Cranks but won't start, or starts only sometimes"], ["Demarorul se învârte fără pornire, sau pornește doar uneori"]),
    ("P1699", [manual("Kia", 2012, "Rio L4-1.6L", "P", "P1699"), "https://www.youcanic.com/hyundai-fault-code-dtc/", "https://obdadvisor.com/codes/code/kia-p1699"],
     "Limp-start mode used too often", "Modul de pornire fără chei învățate folosit de prea multe ori",
     "A new or replaced engine computer allows a limited number of starts before keys are taught. That limit has been reached.",
     "Un calculator de motor nou sau înlocuit permite un număr limitat de porniri înainte de învățarea cheilor. Limita a fost atinsă.",
     [("high", "Engine computer replaced but keys never taught")], [("high", "Calculator de motor înlocuit, dar cheile nu au fost învățate")], ["No start"], ["Nu pornește"]),

]
for code, src, t_en, t_ro, d_en, d_ro, c_en, c_ro, s_en, s_ro in IMMO:
    add(code, "engine", "two-sources", src,
        dict(t=t_en, d=IMMO_EN + d_en + " Applies to cars with an immobilizer.", c=c_en, s=s_en),
        dict(t=t_ro, d=IMMO_RO + d_ro + " Se aplică mașinilor cu imobilizator.", c=c_ro, s=s_ro))

add("P1529", "engine", "two-sources", ["https://blog.upfix.com/hyundai-factory-dtc-code-definitions",
                                        manual("Hyundai", 2004, "Tiburon L4-2.0L", "P", "P1529", page="")],
    dict(t="Transmission computer requests the check-engine light",
         d="This code only says that the transmission computer stored a fault and asked the engine computer to light the check-engine lamp. "
           "The real fault is in the transmission computer: read its codes too (All modules). Mostly seen on older Hyundai/Kia.",
         c=[("medium", "Any fault stored in the transmission computer, often a speed sensor")], s=["Depends on the transmission fault: harsh shifts or limp mode"]),
    dict(t="Calculatorul cutiei de viteze cere aprinderea martorului de motor",
         d="Acest cod spune doar că calculatorul cutiei de viteze a memorat un defect și i-a cerut calculatorului motorului să aprindă martorul. "
           "Defectul real este în calculatorul cutiei: citește și codurile lui (Toate modulele). Apare mai ales la Hyundai/Kia mai vechi.",
         c=[("medium", "Orice defect memorat în calculatorul cutiei, adesea un senzor de turație")], s=["Depinde de defectul cutiei: schimbări bruște sau mod de avarie"]),
    ("mil",))

AMS_TSB = "https://vehicleservicepros.com/tsbs/article/53093078/kia-no-more-grounds-please-2011-12-28"
for code, t_en, t_ro, d_en, d_ro, src2 in (
        ("U1111", "Battery sensor fault", "Defect la senzorul de baterie",
         "The engine computer found a fault in the battery sensor on the negative battery terminal, which it uses to control the alternator.",
         "Calculatorul motorului a găsit un defect la senzorul de baterie de pe borna negativă, pe care îl folosește ca să controleze alternatorul.",
         "https://www.go-parts.com/garage/obd-u1111"),
        ("U1112", "Battery sensor communication fault", "Defect de comunicație cu senzorul de baterie",
         "The engine computer can't talk to the battery sensor over its single-wire (LIN) link.",
         "Calculatorul motorului nu poate comunica cu senzorul de baterie prin legătura pe un fir (LIN).",
         "https://obdadvisor.com/codes/code/hyundai-u1112")):
    add(code, "engine", "official", [AMS_TSB, src2],
        dict(t=t_en, d=d_en + " An extra ground cable bolted to the negative terminal (for accessories) is a known cause: Kia asks not to add one. Only on cars with a battery sensor.",
             c=[("high", "Extra ground cable on the negative battery terminal"), ("medium", "Battery sensor or its connector"), ("low", "Wiring")],
             s=["Alternator charges wrongly", "Stop-start (ISG) misbehaves"]),
        dict(t=t_ro, d=d_ro + " Un cablu de masă suplimentar prins pe borna negativă (pentru accesorii) este o cauză cunoscută: Kia cere să nu se adauge. Doar la mașinile cu senzor de baterie.",
             c=[("high", "Cablu de masă suplimentar pe borna negativă"), ("medium", "Senzorul de baterie sau conectorul lui"), ("low", "Cablaj")],
             s=["Alternatorul încarcă greșit", "Sistemul start-stop (ISG) funcționează anormal"]))

# ============================================================================
# 6-speed automatic (A6): standard codes, with Hyundai/Kia's solenoid names
# ============================================================================
A6_TSB = "https://static.nhtsa.gov/odi/tsbs/2014/SB-10059670-2273.pdf"
A6 = [
    ("P0741", [A6_TSB, "https://www.vehicleservicepros.com/service-repair/underhood/article/21189506/a-little-more-about-the-6-speed-from-korea"],
     "Torque converter clutch: performance or stuck off", "Ambreiajul convertizorului: performanță slabă sau blocat decuplat",
     "The torque converter lock-up (damper) clutch doesn't engage as commanded.", "Ambreiajul de blocare (damper) al convertizorului nu cuplează cum este comandat.",
     "Hyundai advises replacing the transmission if this returns after the basic checks.", "Hyundai recomandă înlocuirea cutiei dacă revine după verificările de bază."),
    ("P0743", [A6_TSB, "https://static.nhtsa.gov/odi/tsbs/2024/MC-10253275-0001.pdf"],
     "Torque converter clutch solenoid: electrical fault", "Electrovalva ambreiajului convertizorului: defect electric",
     "The solenoid that controls the converter lock-up clutch has an electrical fault.", "Electrovalva care comandă ambreiajul de blocare al convertizorului are un defect electric.", "", ""),
    ("P0748", [A6_TSB, "https://static.nhtsa.gov/odi/tsbs/2014/SB-10059669-2273.pdf"],
     "Line pressure control solenoid (VFS) fault", "Defect la electrovalva de control al presiunii (VFS)",
     "The variable-force solenoid that sets the transmission's hydraulic line pressure has a fault.", "Electrovalva care stabilește presiunea hidraulică din cutie are un defect.", "", ""),
    ("P0753", [A6_TSB, "https://static.nhtsa.gov/odi/tsbs/2014/SB-10059669-2273.pdf"],
     "Shift solenoid A (UD brake) fault", "Defect la electrovalva A (frâna UD)",
     "On Hyundai/Kia 6-speed automatics, solenoid A controls the underdrive (UD) brake.", "La cutiile automate cu 6 trepte Hyundai/Kia, electrovalva A comandă frâna underdrive (UD).", "", ""),
    ("P0758", [A6_TSB, "https://static.nhtsa.gov/odi/tsbs/2014/SB-10059669-2273.pdf"],
     "Shift solenoid B (2-6 brake) fault", "Defect la electrovalva B (frâna 2-6)",
     "On Hyundai/Kia 6-speed automatics, solenoid B controls the 2nd/6th gear brake.", "La cutiile automate cu 6 trepte Hyundai/Kia, electrovalva B comandă frâna treptelor 2/6.", "", ""),
    ("P0763", [A6_TSB, "https://static.nhtsa.gov/odi/tsbs/2014/SB-10059669-2273.pdf"],
     "Shift solenoid C (3-5-R clutch) fault", "Defect la electrovalva C (ambreiajul 3-5-R)",
     "On Hyundai/Kia 6-speed automatics, solenoid C controls the 3rd/5th/reverse clutch.", "La cutiile automate cu 6 trepte Hyundai/Kia, electrovalva C comandă ambreiajul treptelor 3/5/marșarier.", "", ""),
    ("P0768", [A6_TSB, "https://static.nhtsa.gov/odi/tsbs/2014/SB-10059669-2273.pdf"],
     "Shift solenoid D (overdrive clutch) fault", "Defect la electrovalva D (ambreiajul overdrive)",
     "On Hyundai/Kia 6-speed automatics, solenoid D controls the overdrive (OD) clutch.", "La cutiile automate cu 6 trepte Hyundai/Kia, electrovalva D comandă ambreiajul overdrive (OD).", "", ""),
    ("P0773", [A6_TSB, "https://static.nhtsa.gov/odi/tsbs/2022/MC-10215602-0001.pdf"],
     "Shift solenoid E (SS-A) fault", "Defect la electrovalva E (SS-A)",
     "On Hyundai/Kia 6-speed automatics, solenoid E is the on/off shift solenoid SS-A.", "La cutiile automate cu 6 trepte Hyundai/Kia, electrovalva E este electrovalva de schimbare SS-A (pornit/oprit).", "", ""),
    ("P2709", [A6_TSB, "https://static.nhtsa.gov/odi/tsbs/2022/MC-10215602-0001.pdf"],
     "Shift solenoid F (SS-B) fault", "Defect la electrovalva F (SS-B)",
     "On Hyundai/Kia 6-speed automatics, solenoid F is the on/off shift solenoid SS-B.", "La cutiile automate cu 6 trepte Hyundai/Kia, electrovalva F este electrovalva de schimbare SS-B (pornit/oprit).", "", ""),
]
for code, src, t_en, t_ro, d_en, d_ro, extra_en, extra_ro in A6:
    en = dict(t=t_en, d=d_en + " The transmission may go into a fail-safe gear. " + extra_en + " Check the wiring between computer and transmission before the solenoid or valve body.",
              c=[("medium", "The solenoid"), ("medium", "Valve-body internal harness or the external transmission harness"), ("low", "Engine/transmission computer")],
              s=["Check-engine light", "Harsh shifts or stuck in one gear"])
    ro = dict(t=t_ro, d=d_ro + " Cutia poate trece într-o treaptă de avarie. " + extra_ro + " Verifică cablajul dintre calculator și cutie înaintea electrovalvei sau a blocului de valve.",
              c=[("medium", "Electrovalva"), ("medium", "Cablajul intern al blocului de valve sau cablajul exterior al cutiei"), ("low", "Calculatorul motorului/cutiei")],
              s=["Martorul de motor aprins", "Schimbări bruște sau blocat într-o treaptă"])
    # Standard SAE codes: the brand text only adds Hyundai/Kia's solenoid names, so it overrides
    # the generic text. The transmission may be run by a separate TCM (7E9) or the engine PCM (7E8).
    for module in ("transmission", "engine"):
        add(code, module, "official", src, en, ro, ("mil",), key=code + "@" + module)
        PACK[code + "@" + module]["override"] = True


def main():
    pack = {
        "brand": "hyundai-kia",
        "label": "Obdaryn's Hyundai/Kia pack",
        "label_ro": "pachetul Obdaryn pentru Hyundai/Kia",
        "brandName": "Hyundai/Kia",
        "source": {
            "license": "GPL-3.0-or-later (text written for Obdaryn)",
            "note": "Meanings compiled from public Hyundai/Kia documents filed with NHTSA and independent sources; "
                    "each code lists its sources. Verify with a workshop before replacing parts.",
            "scope": "2010-2016 Hyundai/Kia: Elantra MD/UD, i30 GD, Ceed JD, Cerato/Forte",
        },
        "codes": dict(sorted(PACK.items())),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(pack, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"wrote {len(PACK)} codes to {OUT}")


if __name__ == "__main__":
    main()
