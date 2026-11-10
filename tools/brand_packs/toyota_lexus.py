#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Source of the Toyota/Lexus trouble-code pack.

    python3 tools/brand_packs/toyota_lexus.py      # writes src/core/data/dtc/brands/toyota-lexus.json
    python3 tools/check_brand_pack.py src/core/data/dtc/brands/toyota-lexus.json

Edit THIS file, not the JSON. Families of codes (wheel-speed sensors, airbag
squib circuits, tyre-pressure transmitters, ...) are written once and expanded,
so English and Romanian stay consistent.

Scope: 2008-2020 Toyota/Lexus sold in Europe and the US (Corolla, Auris, Yaris,
RAV4, C-HR, Prius and the other hybrids, Avensis). Every meaning comes from
public sources listed per code. "official" means a Toyota/Lexus document filed
with NHTSA (technical service bulletin, tech tip or warranty notice) states it;
"two-sources" means at least two independent sources agree. The independent
sources are Toyota repair-manual pages for the 2006+ RAV4, the 2007 FJ Cruiser
and the Yaris, republished on the web, and the toyota-club.net code tables.
All descriptions are written for Obdaryn in our own words; nothing is copied
from service manuals. Codes whose meaning was uncertain, contradictory or
model-dependent were left out on purpose (see docs/brand-packs.md).
"""

import json
import pathlib

OUT = pathlib.Path(__file__).resolve().parents[2] / "src/core/data/dtc/brands/toyota-lexus.json"

# ---- Sources ---------------------------------------------------------------
# toyota-club.net code tables (independent enthusiast site, compiled from Toyota data).
TC = "https://toyota-club.net/files/techdata/dtc/"
TC_VSC = TC + "br_vsc.htm"
TC_VSC_SAE = TC + "br_vsc_sae.htm"
TC_ABS_SAE = TC + "br_abs_sae.htm"
TC_ECB = TC + "br_ecb_4d.htm"
TC_ECB6 = TC + "br_ecb_6d.htm"
TC_EPS = TC + "ch_eps_4d.htm"
TC_SRS = TC + "be_srs.htm"
TC_SRS2 = TC + "be_srs_rav4.htm"
TC_TPMS = TC + "be_tpms_4d.htm"
TC_ESS = TC + "be_ess.htm"
TC_HV = TC + "hv_p410.htm"
TC_2ZR = TC + "ems_2zrfe.htm"
TC_1NZX = TC + "ems_1nzfxe.htm"
TC_2ZRX = TC + "ems_2zrfxe.htm"

# Toyota repair-manual pages republished on the web (2006+ RAV4, 2007 FJ Cruiser, Yaris).
RAV4 = "https://share.qclt.com/%E4%B8%B0%E7%94%B0Toyota%20RAV4%20Repair%20Manual/"
RAV4_VSC = RAV4 + "Brake%20Control/Vehicle%20Stability%20Control%20System/0370010.pdf"
RAV4_SRS = RAV4 + "Supplemental%20Restraint%20System/Airbag%20System/043005.pdf"
RAV4_TPMS = RAV4 + "Tire%20and%20Wheel/Tire%20Pressure%20Warning%20System/036007.pdf"
RAV4_IMMO = RAV4 + "Engine%20Immobiliser/Engine%20Immobiliser%20System/045004.pdf"
RAV4_EPS = RAV4 + "Power%20Steering/Electronic%20Power%20Steering%20System/041004.pdf"
FJ = "https://www.purefjcruiser.com/docs/2007%20Toyota%20FJ%20Cruiser%20Repair%20Manual/"
FJ_VSC = FJ + "Brake%20Control/Vehicle%20Stability%20Control%20System/0280012.pdf"
FJ_SRS = FJ + "Supplemental%20Restraint%20System/Airbag%20System/034005.pdf"
YARIS_EPS = "https://mgaguru.com/mgtech/steering/pdf/power_steering_yaris.pdf"
MTD_CHR = "https://www.moderntiredealer.com/retail/article/11546521/tpms-toyota-ch-r-2018-2020"

# Toyota/Lexus documents filed with NHTSA.
NHTSA = "https://static.nhtsa.gov/odi/tsbs/"
SRS_TSB = NHTSA + "2013/MC-10132282-9999.pdf"         # T-SB-0035-13: 2012 Corolla/Prius SRS DTC cross reference
B1000_TSB = NHTSA + "2017/MC-10132448-9999.pdf"       # L-SB-0148-17: B1000 on NX/RX
KNEE_TSB = NHTSA + "2019/MC-10155240-9999.pdf"        # L-SB-0007-19: B1861 knee airbag
ECB_TSB = NHTSA + "2014/MC-10109482-9999.pdf"         # T-SB-0041-14: Camry HV brake actuator (C1391, C1252)
PRIUS_BOOSTER = NHTSA + "2019/MC-10158575-9999.pdf"   # T-SB-0024-19: 2011-2015 Prius brake booster
STROKE_TSB = NHTSA + "2014/MC-10132768-9999.pdf"      # T-SB-0174-12: Prius C1247
YAW_TSB = NHTSA + "2018/MC-10131753-9999.pdf"         # T-SB-0012-18: C1234 yaw rate sensor
SAS_TSB = NHTSA + "2013/MC-10132241-9999.pdf"         # T-SB-0086-13: C1231 steering angle sensor
P3000_TT = NHTSA + "2015/MC-10132527-9999.pdf"        # T-TT-0303-14: P3000 information code 123
IPM_TSB = NHTSA + "2016/MC-10140583-9999.pdf"         # T-SB-0036-16: 2010-2014 Prius inverter
IPM_ZE3 = NHTSA + "2014/MC-10109602-9999.pdf"         # ZE3 warranty enhancement (inverter IPM)
PURGE_TSB = NHTSA + "2015/MC-10111144-9999.pdf"       # T-SB-0041-15: Prius P0A0F/P3190/P3191
P3190_TSB = NHTSA + "2018/MC-10143923-9999.pdf"       # T-SB-0056-18: P3190 Highlander HV
FLEX_TSB = NHTSA + "2014/MC-10132332-9999.pdf"        # T-SB-0058-13: P1604/P1605 definitions
MISFIRE_TSB = NHTSA + "2018/MC-10143946-9999.pdf"     # T-SB-0071-18: P160500 rough idling
CARBON_TSB = NHTSA + "2016/MC-10132874-9999.pdf"      # S-SB-0008-16: P1604/P1605 carbon build-up
P148F_TSB = NHTSA + "2019/MC-10166037-9999.pdf"       # T-SB-0135-19: 2016-2019 Prius P148F
BOOSTER_CSP = NHTSA + "2024/MC-11011632-0001.pdf"     # 24TE01: brake booster coverage, Prius c / Highlander HV

OFFICIAL = {SRS_TSB, B1000_TSB, KNEE_TSB, ECB_TSB, PRIUS_BOOSTER, STROKE_TSB, YAW_TSB, SAS_TSB, P3000_TT,
            IPM_TSB, IPM_ZE3, PURGE_TSB, P3190_TSB, FLEX_TSB, MISFIRE_TSB, CARBON_TSB, P148F_TSB, BOOSTER_CSP}

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
    refs = [{"u": u, "o": True} if u in OFFICIAL or "static.nhtsa.gov" in u else {"u": u}
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
MIL = ("mil",)

# ============================================================================
# ABS / VSC (skid control ECU)
# ============================================================================
VSC_EN = ("While this fault is stored, the skid control unit may switch off VSC (stability control) and traction "
          "control, and sometimes ABS; normal braking still works. Drive gently and have it checked.")
VSC_RO = ("Cât timp acest defect este memorat, unitatea de control al frânării poate dezactiva VSC-ul (controlul "
          "stabilității) și controlul tracțiunii, uneori și ABS-ul; frânarea normală funcționează în continuare. "
          "Condu cu grijă și verifică mașina.")
VSC_LAMPS_EN = ["VSC / slip indicator or ABS warning light"]
VSC_LAMPS_RO = ["Martorul VSC / de derapaj sau martorul ABS aprins"]
ABS_SRC = [RAV4_VSC, FJ_VSC, TC_VSC]


def vsc(code, src, t_en, t_ro, d_en, d_ro, c_en, c_ro, s_en=None, s_ro=None, conf="two-sources", note=True):
    add(code, "abs", conf, src,
        dict(t=t_en, d=d_en + (" " + VSC_EN if note else ""), c=c_en, s=s_en or VSC_LAMPS_EN),
        dict(t=t_ro, d=d_ro + (" " + VSC_RO if note else ""), c=c_ro, s=s_ro or VSC_LAMPS_RO), LAMP)


vsc("C1201", ABS_SRC + [TC_ECB],
    "Engine control system fault (VSC switched off)", "Defect în sistemul de control al motorului (VSC dezactivat)",
    "The engine computer has a fault of its own, so the skid control unit switches VSC and traction control off as a "
    "precaution: it can no longer rely on reducing engine power. This code points elsewhere; read and repair the "
    "engine codes first and it usually clears by itself afterwards.",
    "Calculatorul motorului are un defect propriu, așa că unitatea de control al frânării dezactivează din precauție "
    "VSC-ul și controlul tracțiunii: nu se mai poate baza pe reducerea puterii motorului. Codul indică în altă parte; "
    "citește și repară întâi codurile de motor, iar acesta dispare de obicei singur după aceea.",
    [("medium", "Any stored engine (P) code, for example a misfire or an EVAP fault"), ("low", "Engine computer fault")],
    [("medium", "Orice cod de motor (P) memorat, de exemplu rateuri sau un defect EVAP"), ("low", "Defect al calculatorului motorului")],
    ["Check-engine light together with the VSC / slip light"], ["Martorul de motor aprins împreună cu martorul VSC / de derapaj"])
vsc("C1203", [RAV4_VSC, FJ_VSC, TC_VSC_SAE],
    "No valid communication with the engine computer", "Comunicație invalidă cu calculatorul motorului",
    "The skid control unit doesn't get the data it expects from the engine computer over the CAN network.",
    "Unitatea de control al frânării nu primește prin rețeaua CAN datele pe care le așteaptă de la calculatorul motorului.",
    [("medium", "CAN wiring or connectors"), ("low", "Engine computer, or a wrong computer fitted after a replacement")],
    [("medium", "Cablaj sau conectori CAN"), ("low", "Calculatorul motorului sau un calculator nepotrivit montat la înlocuire")])
for code, src, en_s, ro_s in (("C1210", [RAV4_VSC, FJ_VSC, TC_VSC_SAE], "yaw rate sensor", "senzorul de rotație (yaw)"),
                              ("C1336", [RAV4_VSC, FJ_VSC, TC_ECB], "acceleration (deceleration) sensor", "senzorul de accelerație (decelerație)")):
    vsc(code, src, f"Zero point of the {en_s} not calibrated", f"Punctul zero pentru {ro_s} necalibrat",
        f"The skid control unit has no stored zero point for the {en_s}. This is normal after the skid control unit or the "
        "sensor was replaced, or after some battery and alignment work. A workshop clears it with a scan tool and a short "
        "calibration, done with the car level, stationary and the steering straight. C1210 and C1336 often appear together.",
        f"Unitatea de control al frânării nu are memorat punctul zero pentru {ro_s}. Este normal după înlocuirea unității "
        "sau a senzorului, ori după unele lucrări la baterie și geometrie. Un service îl reface cu un tester printr-o "
        "calibrare scurtă, cu mașina pe teren plat, oprită și cu volanul drept. C1210 și C1336 apar adesea împreună.",
        [("medium", "Calibration not done after a part was replaced"), ("low", "Yaw rate / acceleration sensor fault")],
        [("medium", "Calibrarea nu a fost făcută după înlocuirea unei piese"), ("low", "Defect al senzorului de rotație / accelerație")])
vsc("C1223", [RAV4_VSC, FJ_VSC, TC_VSC_SAE],
    "ABS control system fault", "Defect în sistemul de control ABS",
    "The ABS side of the skid control system reports a fault, so VSC is switched off too. Look for other ABS codes "
    "stored at the same time; they say what actually failed.",
    "Partea ABS a sistemului de control al frânării raportează un defect, așa că și VSC-ul este dezactivat. Caută alte "
    "coduri ABS memorate în același timp; ele arată ce s-a defectat de fapt.",
    [("medium", "Another ABS fault, such as a wheel-speed sensor or low battery voltage"), ("low", "Skid control unit")],
    [("medium", "Alt defect ABS, de exemplu un senzor de turație sau tensiune scăzută a bateriei"), ("low", "Unitatea de control al frânării")])
vsc("C1231", [RAV4_VSC, FJ_VSC, TC_ECB, SAS_TSB],
    "Steering angle sensor circuit fault", "Defect la circuitul senzorului de unghi al volanului",
    "The steering angle signal the stability control needs is missing or faulty. The sensor sits in the steering "
    "column and talks to the skid control unit over CAN. On some 2009-2011 Camry with Bosch ABS, Toyota updated the "
    "ABS unit logic for this code.",
    "Semnalul de unghi al volanului de care are nevoie controlul stabilității lipsește sau este defect. Senzorul este "
    "în coloana de direcție și comunică prin CAN cu unitatea de control al frânării. La unele Camry 2009-2011 cu ABS "
    "Bosch, Toyota a actualizat logica unității ABS pentru acest cod.",
    [("medium", "Steering angle sensor or its wiring"), ("medium", "CAN communication fault"), ("low", "Skid control unit")],
    [("medium", "Senzorul de unghi al volanului sau cablajul lui"), ("medium", "Defect de comunicație CAN"), ("low", "Unitatea de control al frânării")],
    conf="official")
vsc("C1232", [RAV4_VSC, FJ_VSC],
    "Deceleration sensor stuck", "Senzorul de decelerație blocat",
    "The signal from the sensor that measures how fast the car slows down or speeds up does not change as it should.",
    "Semnalul senzorului care măsoară cât de repede încetinește sau accelerează mașina nu se schimbă cum ar trebui.",
    [("medium", "Yaw rate / deceleration sensor"), ("medium", "Sensor wiring or connector")],
    [("medium", "Senzorul de rotație / decelerație"), ("medium", "Cablajul sau conectorul senzorului")])
vsc("C1234", ABS_SRC + [YAW_TSB],
    "Yaw rate sensor fault", "Defect la senzorul de rotație (yaw)",
    "The sensor that tells the stability control how fast the car is turning gives a faulty signal. A bad yaw rate "
    "signal can make VSC brake a wheel when it shouldn't, so it is switched off.",
    "Senzorul care îi spune controlului stabilității cât de repede virează mașina dă un semnal defect. Un semnal greșit "
    "poate face VSC-ul să frâneze o roată fără motiv, așa că este dezactivat.",
    [("medium", "Yaw rate sensor"), ("medium", "Sensor wiring or connector")],
    [("medium", "Senzorul de rotație (yaw)"), ("medium", "Cablajul sau conectorul senzorului")],
    ["VSC / slip indicator light", "Possible unexpected VSC activation"], ["Martorul VSC / de derapaj aprins", "Posibile intervenții VSC neașteptate"],
    conf="official")
vsc("C1241", ABS_SRC + [TC_ECB],
    "Skid control supply voltage out of range", "Tensiunea de alimentare a unității de frânare în afara limitelor",
    "The skid control unit measured too low (on some systems also too high) a supply voltage. Check the battery and "
    "the charging system before suspecting the ABS unit.",
    "Unitatea de control al frânării a măsurat o tensiune de alimentare prea mică (la unele sisteme și prea mare). "
    "Verifică bateria și sistemul de încărcare înainte să bănuiești unitatea ABS.",
    [("medium", "Weak or discharged battery"), ("medium", "Charging-system fault"), ("low", "Fuse, power or ground connection")],
    [("medium", "Baterie slabă sau descărcată"), ("medium", "Defect al sistemului de încărcare"), ("low", "Siguranță, conexiune de alimentare sau de masă")],
    ["ABS and VSC warning lights", "Often other low-voltage codes at the same time"],
    ["Martorii ABS și VSC aprinși", "Adesea alte coduri de tensiune scăzută în același timp"])
vsc("C1246", ABS_SRC,
    "Master cylinder pressure sensor fault", "Defect la senzorul de presiune din pompa centrală",
    "The sensor that measures brake-fluid pressure from the pedal gives a wrong or missing signal. The stop light "
    "switch signal is checked against it, so a faulty switch can also set this code.",
    "Senzorul care măsoară presiunea lichidului de frână dată de pedală are un semnal greșit sau lipsă. Semnalul "
    "contactorului stopurilor este comparat cu el, așa că și un contactor defect poate seta acest cod.",
    [("medium", "Pressure sensor (in the brake actuator or booster)"), ("medium", "Stop light switch or its circuit")],
    [("medium", "Senzorul de presiune (în actuatorul sau servofrâna)"), ("medium", "Contactorul stopurilor sau circuitul lui")])
vsc("C1249", ABS_SRC + [TC_ECB],
    "Stop light switch circuit open", "Circuitul contactorului stopurilor întrerupt",
    "The skid control unit does not see the brake pedal signal from the stop light switch.",
    "Unitatea de control al frânării nu vede semnalul de pedală de frână de la contactorul stopurilor.",
    [("medium", "Stop light switch faulty or out of adjustment"), ("medium", "STOP fuse, wiring or connector"), ("low", "Brake lamp bulbs or stop light relay")],
    [("medium", "Contactorul stopurilor defect sau dereglat"), ("medium", "Siguranța STOP, cablaj sau conector"), ("low", "Becurile stopurilor sau releul stopurilor")],
    ["VSC / ABS warning light", "Brake lights may not work"], ["Martorul VSC / ABS aprins", "Stopurile s-ar putea să nu funcționeze"])
vsc("C1251", [RAV4_VSC, FJ_VSC],
    "Pump motor circuit open", "Circuitul motorului pompei întrerupt",
    "The electric pump motor in the brake actuator (or in the brake booster pump, on cars with a hydraulic booster) "
    "has an open circuit.",
    "Motorul electric al pompei din actuatorul de frână (sau al pompei servofrânei, la mașinile cu servofrână "
    "hidraulică) are circuitul întrerupt.",
    [("medium", "Pump motor wiring or connector"), ("medium", "Brake actuator or booster pump assembly")],
    [("medium", "Cablajul sau conectorul motorului pompei"), ("medium", "Actuatorul de frână sau ansamblul pompei servofrânei")])

# Hydraulic brake booster / electronically controlled brakes (hybrids and some 4x4s).
ECB_EN = ("Braking still works, but the brake assist may be reduced and the pedal can feel different. "
          "If the red brake warning light is on, have the car checked before driving further.")
ECB_RO = ("Frâna funcționează în continuare, dar asistarea poate fi redusă și pedala se poate simți altfel. "
          "Dacă martorul roșu de frână este aprins, verifică mașina înainte să mai circuli.")
ECB_S_EN = ["Brake, ABS and/or VSC warning lights", "Pump running more often or longer than usual"]
ECB_S_RO = ["Martorii de frână, ABS și/sau VSC aprinși", "Pompa pornește mai des sau mai mult decât de obicei"]
vsc("C1252", [FJ_VSC, ECB_TSB, PRIUS_BOOSTER, BOOSTER_CSP],
    "Brake booster pump runs too long", "Pompa servofrânei funcționează prea mult",
    "The pump that keeps the brake booster's accumulator under pressure ran for longer than it should, which means it "
    "can't build or hold the pressure. On 2011-2015 Prius and 2007-2011 Camry Hybrid, Toyota links this code, "
    "together with C1256 and C1391, to a small internal fluid leak in the brake booster or actuator. " + ECB_EN,
    "Pompa care ține sub presiune acumulatorul servofrânei a funcționat mai mult decât ar trebui, adică nu reușește să "
    "creeze sau să mențină presiunea. La Prius 2011-2015 și la Camry Hybrid 2007-2011, Toyota leagă acest cod, "
    "împreună cu C1256 și C1391, de o mică scurgere internă de lichid în servofrână sau în actuator. " + ECB_RO,
    [("medium", "Internal leak in the brake booster / actuator"), ("medium", "Brake booster pump"), ("low", "Accumulator pressure sensor")],
    [("medium", "Scurgere internă în servofrână / actuator"), ("medium", "Pompa servofrânei"), ("low", "Senzorul de presiune al acumulatorului")],
    ECB_S_EN, ECB_S_RO, conf="official", note=False)
vsc("C1256", [FJ_VSC, TC_ECB6, PRIUS_BOOSTER, BOOSTER_CSP],
    "Brake accumulator pressure low", "Presiune scăzută în acumulatorul de frână",
    "The pressure stored in the brake booster's accumulator is lower than it should be. On 2011-2015 Prius, Toyota "
    "lists it with C1252 and C1391 as a sign of an internal leak in the brake booster. " + ECB_EN,
    "Presiunea din acumulatorul servofrânei este mai mică decât ar trebui. La Prius 2011-2015, Toyota îl enumeră "
    "împreună cu C1252 și C1391 ca semn al unei scurgeri interne în servofrână. " + ECB_RO,
    [("medium", "Internal leak in the brake booster / actuator"), ("medium", "Brake booster pump"), ("low", "Accumulator pressure sensor")],
    [("medium", "Scurgere internă în servofrână / actuator"), ("medium", "Pompa servofrânei"), ("low", "Senzorul de presiune al acumulatorului")],
    ECB_S_EN, ECB_S_RO, conf="official", note=False)
vsc("C1391", [ECB_TSB, TC_ECB6, PRIUS_BOOSTER, BOOSTER_CSP],
    "Brake accumulator leak detected", "Scurgere detectată la acumulatorul de frână",
    "The skid control unit sees the accumulator pressure falling when it should hold steady, which points to fluid "
    "leaking inside the brake booster or actuator. Toyota has run support programs that cover the brake booster and pump "
    "on some hybrids (for example 2015-2017 Prius c and 2014-2016 Highlander Hybrid), using this code as proof. " + ECB_EN,
    "Unitatea de control al frânării vede presiunea din acumulator scăzând când ar trebui să rămână constantă, ceea ce "
    "indică o scurgere de lichid în interiorul servofrânei sau al actuatorului. Toyota a avut programe de suport care "
    "acoperă servofrâna și pompa la unele hibride (de exemplu Prius c 2015-2017 și Highlander Hybrid 2014-2016), "
    "folosind acest cod ca dovadă. " + ECB_RO,
    [("medium", "Internal leak in the brake booster / actuator"), ("low", "External brake-fluid leak")],
    [("medium", "Scurgere internă în servofrână / actuator"), ("low", "Scurgere exterioară de lichid de frână")],
    ECB_S_EN, ECB_S_RO, conf="official", note=False)
vsc("C1247", [STROKE_TSB],
    "Brake pedal stroke sensor fault", "Defect la senzorul de cursă al pedalei de frână",
    "On hybrids with electronically controlled brakes, a sensor measures how far the brake pedal moves. The skid "
    "control unit considers its signal faulty. On 2004-2009 Prius Toyota found the monitoring logic too sensitive and "
    "released an updated skid control unit. " + ECB_EN,
    "La hibridele cu frânare controlată electronic, un senzor măsoară cât de mult se deplasează pedala de frână. "
    "Unitatea de control al frânării consideră semnalul lui defect. La Prius 2004-2009, Toyota a constatat că logica "
    "de supraveghere era prea sensibilă și a lansat o unitate de control actualizată. " + ECB_RO,
    [("medium", "Over-sensitive monitoring logic (updated unit available)"), ("medium", "Stroke sensor or its wiring")],
    [("medium", "Logică de supraveghere prea sensibilă (există unitate actualizată)"), ("medium", "Senzorul de cursă sau cablajul lui")],
    ["Check-engine, brake, ABS and VSC warning lights"], ["Martorii de motor, frână, ABS și VSC aprinși"], conf="official", note=False)
vsc("C1290", [RAV4_VSC, FJ_VSC, TC_VSC_SAE],
    "Steering angle sensor zero point fault", "Defect la punctul zero al senzorului de unghi al volanului",
    "The steering angle reading doesn't agree with where the car actually goes when driving straight. Typical after "
    "a wheel alignment with the steering wheel not centred, or when the yaw rate zero point was not calibrated.",
    "Unghiul citit al volanului nu se potrivește cu direcția reală a mașinii când merge drept. Apare de obicei după o "
    "geometrie cu volanul necentrat sau când punctul zero al senzorului de rotație nu a fost calibrat.",
    [("medium", "Steering wheel not centred or wheel alignment off"), ("medium", "Zero point calibration not done")],
    [("medium", "Volan necentrat sau geometrie dereglată"), ("medium", "Calibrarea punctului zero nefăcută")])
vsc("C1380", ABS_SRC,
    "Stop light control relay fault", "Defect la releul de comandă al stopurilor",
    "The relay the skid control unit uses to switch the brake lights on during automatic braking has a fault.",
    "Releul prin care unitatea de control al frânării aprinde stopurile la frânările automate are un defect.",
    [("medium", "Stop light control relay or its wiring"), ("low", "Stop light switch circuit")],
    [("medium", "Releul de comandă al stopurilor sau cablajul lui"), ("low", "Circuitul contactorului stopurilor")])
vsc("C1381", ABS_SRC,
    "Yaw rate / acceleration sensor supply voltage fault", "Defect la alimentarea senzorului de rotație / accelerație",
    "The power supply to the yaw rate and acceleration sensor is out of range.",
    "Alimentarea senzorului de rotație și accelerație este în afara limitelor.",
    [("medium", "Sensor wiring or connector"), ("low", "Low battery voltage"), ("low", "Yaw rate / acceleration sensor")],
    [("medium", "Cablajul sau conectorul senzorului"), ("low", "Tensiune scăzută a bateriei"), ("low", "Senzorul de rotație / accelerație")])

# Wheel-speed sensors. Toyota uses RH = right, LH = left.
WHEELS = [("front right", "față dreapta"), ("front left", "față stânga"), ("rear right", "spate dreapta"), ("rear left", "spate stânga")]
for code, (wen, wro) in zip(("C1235", "C1236", "C1238", "C1239"), WHEELS):
    vsc(code, [RAV4_VSC, FJ_VSC],
        f"Foreign matter on the {wen} wheel-speed sensor", f"Corp străin pe senzorul de turație al roții {wro}",
        f"The {wen} wheel-speed signal looks as if dirt or metal particles are stuck to the sensor tip or its tone ring.",
        f"Semnalul de turație al roții {wro} arată ca și cum murdărie sau particule metalice s-au lipit de vârful "
        "senzorului sau de inelul lui dințat.",
        [("medium", "Dirt or rust on the sensor tip or tone ring"), ("medium", "Sensor not seated correctly"), ("low", "Sensor wiring")],
        [("medium", "Murdărie sau rugină pe vârful senzorului ori pe inelul dințat"), ("medium", "Senzor montat incorect"), ("low", "Cablajul senzorului")])
# Test-mode codes: only appear when a workshop runs Toyota's sensor check.
TEST_EN = "This is a test-mode code: it only appears during the sensor check a workshop runs with a scan tool."
TEST_RO = "Este un cod din modul de test: apare doar la verificarea senzorilor pe care o face un service cu testerul."
for i, (wen, wro) in enumerate(WHEELS):
    vsc("C%04d" % (1271 + i), ABS_SRC,
        f"Low output from the {wen} wheel-speed sensor (test mode)", f"Semnal slab de la senzorul de turație {wro} (mod test)",
        f"During the sensor check the {wen} wheel-speed signal was too weak. " + TEST_EN,
        f"La verificarea senzorilor, semnalul de turație al roții {wro} a fost prea slab. " + TEST_RO,
        [("medium", "Sensor too far from the tone ring, or loose"), ("medium", "Dirty or damaged tone ring"), ("low", "Sensor or wiring")],
        [("medium", "Senzor prea departe de inelul dințat sau slăbit"), ("medium", "Inel dințat murdar sau deteriorat"), ("low", "Senzor sau cablaj")],
        ["ABS warning light blinks in test mode"], ["Martorul ABS clipește în modul de test"], note=False)
    vsc("C%04d" % (1275 + i), ABS_SRC,
        f"Irregular signal from the {wen} wheel-speed sensor (test mode)", f"Semnal neregulat de la senzorul de turație {wro} (mod test)",
        f"During the sensor check the {wen} wheel-speed signal changed abnormally. " + TEST_EN,
        f"La verificarea senzorilor, semnalul de turație al roții {wro} s-a schimbat anormal. " + TEST_RO,
        [("medium", "Damaged or dirty tone ring"), ("medium", "Sensor installation"), ("low", "Sensor or wiring")],
        [("medium", "Inel dințat deteriorat sau murdar"), ("medium", "Montajul senzorului"), ("low", "Senzor sau cablaj")],
        ["ABS warning light blinks in test mode"], ["Martorul ABS clipește în modul de test"], note=False)

# ============================================================================
# Electric power steering (EPS)
# ============================================================================
STEER_EN = "When the power steering limits or stops its assistance, the steering becomes heavy but still works mechanically."
STEER_RO = "Când servodirecția își limitează sau oprește asistența, volanul devine greu, dar direcția funcționează în continuare mecanic."
EPS_SRC = [YARIS_EPS, RAV4_EPS, TC_EPS]
EPS_S_EN = ["P/S (power steering) warning light", "Heavy steering"]
EPS_S_RO = ["Martorul P/S (servodirecție) aprins", "Volan greu"]


def eps(code, src, t_en, t_ro, d_en, d_ro, c_en, c_ro, s_en=EPS_S_EN, s_ro=EPS_S_RO):
    add(code, "steering", "two-sources", src,
        dict(t=t_en, d=d_en + " " + STEER_EN, c=c_en, s=list(s_en)),
        dict(t=t_ro, d=d_ro + " " + STEER_RO, c=c_ro, s=list(s_ro)), LAMP)


TORQUE_C_EN = [("medium", "Torque sensor in the steering column (usually replaced with the column)"), ("medium", "Wiring between column and EPS unit"), ("low", "EPS control unit")]
TORQUE_C_RO = [("medium", "Senzorul de cuplu din coloana de direcție (de obicei se înlocuiește cu coloana)"), ("medium", "Cablajul dintre coloană și unitatea EPS"), ("low", "Unitatea de control EPS")]
for code, src, t_en, t_ro in (
        ("C1511", EPS_SRC, "Torque sensor 1 fault", "Defect la senzorul de cuplu 1"),
        ("C1512", EPS_SRC, "Torque sensor 2 fault", "Defect la senzorul de cuplu 2"),
        ("C1513", EPS_SRC, "Torque sensor signals disagree", "Semnalele senzorului de cuplu nu concordă"),
        ("C1514", EPS_SRC, "Torque sensor supply voltage fault", "Defect la alimentarea senzorului de cuplu"),
        ("C1517", [YARIS_EPS, TC_EPS], "Torque sensor signal frozen", "Semnalul senzorului de cuplu blocat")):
    eps(code, src, t_en, t_ro,
        "The sensor in the steering column that measures how hard you turn the wheel reports a fault, so the power "
        "steering limits or stops its assistance.",
        "Senzorul din coloana de direcție care măsoară cât de tare rotești volanul raportează un defect, așa că "
        "servodirecția își limitează sau oprește asistența.",
        TORQUE_C_EN, TORQUE_C_RO)
eps("C1515", [YARIS_EPS, TC_EPS], "Torque sensor zero point not set", "Punctul zero al senzorului de cuplu nesetat",
    "The torque sensor has no zero point stored. This is expected after the EPS unit or the steering column was "
    "replaced; a workshop sets it with a scan tool, with the wheels straight and the steering wheel untouched.",
    "Senzorul de cuplu nu are memorat punctul zero. Este normal după înlocuirea unității EPS sau a coloanei de "
    "direcție; un service îl setează cu un tester, cu roțile drepte și fără să atingă volanul.",
    [("medium", "Zero point calibration not done after a replacement")], [("medium", "Calibrarea punctului zero nefăcută după o înlocuire")],
    ["P/S warning light", "Steering may pull or feel uneven left and right"], ["Martorul P/S aprins", "Volanul poate trage sau poate fi inegal stânga-dreapta"])
eps("C1516", [YARIS_EPS, TC_EPS], "Torque sensor zero point calibration failed", "Calibrarea punctului zero al senzorului de cuplu a eșuat",
    "A zero point calibration of the torque sensor was started but did not finish correctly, often because the "
    "steering wheel was touched during the procedure. Repeat it.",
    "O calibrare a punctului zero al senzorului de cuplu a început, dar nu s-a încheiat corect, adesea pentru că "
    "volanul a fost atins în timpul procedurii. Trebuie repetată.",
    [("medium", "Calibration disturbed or interrupted"), ("low", "Steering column (torque sensor)")],
    [("medium", "Calibrare perturbată sau întreruptă"), ("low", "Coloana de direcție (senzorul de cuplu)")],
    ["P/S warning light"], ["Martorul P/S aprins"])
MOTOR_C_EN = [("medium", "Wiring or connector between EPS unit and motor"), ("medium", "Assist motor (part of the column)"), ("low", "EPS control unit")]
MOTOR_C_RO = [("medium", "Cablajul sau conectorul dintre unitatea EPS și motor"), ("medium", "Motorul de asistare (parte din coloană)"), ("low", "Unitatea de control EPS")]
eps("C1521", [RAV4_EPS, TC_EPS], "Short in the assist motor circuit", "Scurtcircuit în circuitul motorului de asistare",
    "The EPS unit detected a short circuit in the electric assist motor circuit and stops assisting.",
    "Unitatea EPS a detectat un scurtcircuit în circuitul motorului electric de asistare și oprește asistența.", MOTOR_C_EN, MOTOR_C_RO)
eps("C1524", [YARIS_EPS, RAV4_EPS], "Assist motor circuit fault", "Defect la circuitul motorului de asistare",
    "The EPS unit sees a wrong voltage or current in the electric assist motor circuit and stops assisting.",
    "Unitatea EPS vede o tensiune sau un curent greșit în circuitul motorului electric de asistare și oprește asistența.", MOTOR_C_EN, MOTOR_C_RO)
eps("C1528", [RAV4_EPS, TC_EPS], "Motor rotation angle sensor fault", "Defect la senzorul de unghi de rotație al motorului",
    "The sensor that tells the EPS unit the position of the assist motor gives a faulty signal.",
    "Senzorul care îi spune unității EPS poziția motorului de asistare dă un semnal defect.", MOTOR_C_EN, MOTOR_C_RO)
for code, src, t_en, t_ro, d_en, d_ro in (
        ("C1531", EPS_SRC, "EPS control unit internal fault", "Defect intern al unității de control EPS",
         "The power steering control unit found a fault in its own electronics.", "Unitatea de control a servodirecției a găsit un defect în propria electronică."),
        ("C1532", EPS_SRC, "EPS control unit internal fault", "Defect intern al unității de control EPS",
         "The power steering control unit found a fault in its own electronics.", "Unitatea de control a servodirecției a găsit un defect în propria electronică."),
        ("C1533", EPS_SRC, "EPS unit temperature sensor fault", "Defect la senzorul de temperatură al unității EPS",
         "The temperature sensor inside the power steering unit reads out of range, so assistance is reduced to protect the system.",
         "Senzorul de temperatură din unitatea servodirecției citește în afara limitelor, așa că asistența este redusă pentru protecția sistemului."),
        ("C1534", EPS_SRC, "EPS unit memory (EEPROM) fault", "Defect de memorie (EEPROM) al unității EPS",
         "The power steering unit can't read or write its internal memory.", "Unitatea servodirecției nu își poate citi sau scrie memoria internă.")):
    eps(code, src, t_en, t_ro, d_en + " Check its power, ground and connectors before replacing it.",
        d_ro + " Verifică alimentarea, masa și conectorii înainte s-o înlocuiești.",
        [("medium", "EPS control unit"), ("low", "Poor power or ground connection")], [("medium", "Unitatea de control EPS"), ("low", "Conexiune de alimentare sau masă proastă")])
for code, en_s, ro_s, c_en, c_ro in (
        ("C1551", "ignition (IG) supply", "alimentarea de la contact (IG)",
         [("medium", "ECU-IG fuse or IG supply wiring"), ("medium", "Weak battery"), ("low", "EPS control unit")],
         [("medium", "Siguranța ECU-IG sau cablajul alimentării IG"), ("medium", "Baterie slabă"), ("low", "Unitatea de control EPS")]),
        ("C1552", "main power (PIG) supply", "alimentarea principală (PIG)",
         [("medium", "Weak battery or charging fault"), ("medium", "EPS fuse or main power wiring"), ("low", "EPS control unit")],
         [("medium", "Baterie slabă sau defect la încărcare"), ("medium", "Siguranța EPS sau cablajul alimentării principale"), ("low", "Unitatea de control EPS")])):
    eps(code, [YARIS_EPS, RAV4_EPS, TC_EPS], f"EPS {en_s} voltage fault", f"Defect de tensiune la {ro_s} EPS",
        f"The power steering unit measured its {en_s} voltage out of range, so assistance is reduced.",
        f"Unitatea servodirecției a măsurat tensiunea la {ro_s} în afara limitelor, așa că asistența este redusă.", c_en, c_ro)
eps("C1554", EPS_SRC, "EPS power supply relay fault", "Defect la releul de alimentare EPS",
    "The relay that powers the assist motor inside the EPS unit doesn't work correctly, so assistance stops.",
    "Releul care alimentează motorul de asistare în unitatea EPS nu funcționează corect, așa că asistența se oprește.",
    [("medium", "EPS fuse or main power wiring"), ("medium", "EPS control unit")], [("medium", "Siguranța EPS sau cablajul alimentării principale"), ("medium", "Unitatea de control EPS")])
eps("C1555", EPS_SRC, "EPS motor relay stuck (welded)", "Releul motorului EPS blocat (lipit)",
    "The motor relay inside the EPS unit appears stuck closed, so assistance stops.",
    "Releul motorului din unitatea EPS pare blocat închis, așa că asistența se oprește.",
    [("medium", "EPS control unit")], [("medium", "Unitatea de control EPS")])
eps("C1581", EPS_SRC, "EPS assist map not written", "Harta de asistare EPS nescrisă",
    "The power steering unit has no assist map stored for this car. It happens when a new or replacement EPS unit "
    "was fitted without loading the map with a scan tool.",
    "Unitatea servodirecției nu are memorată harta de asistare pentru această mașină. Se întâmplă când a fost montată "
    "o unitate EPS nouă sau de schimb fără încărcarea hărții cu un tester.",
    [("medium", "Assist map not written after the EPS unit was replaced")], [("medium", "Harta de asistare nescrisă după înlocuirea unității EPS")])

# ============================================================================
# Airbag (SRS)
# ============================================================================
SRS_EN = ("While an airbag fault is stored, the affected airbag or pretensioner may not deploy in a crash. "
          "Have it repaired before relying on it. Never probe airbag connectors with a meter: that can fire the airbag.")
SRS_RO = ("Cât timp un defect de airbag este memorat, airbagul sau pretensionerul afectat s-ar putea să nu se declanșeze la impact. "
          "Repară-l înainte să te bazezi pe el. Nu măsura niciodată conectorii de airbag cu un aparat: poate declanșa airbagul.")
SRS_LAMP_EN = ["SRS (airbag) warning light"]
SRS_LAMP_RO = ["Martorul SRS (airbag) aprins"]
SRS_SRC = [RAV4_SRS, FJ_SRS, TC_SRS]


def srs(code, src, title_en, title_ro, d_en, d_ro, causes_en, causes_ro, conf="two-sources"):
    add(code, "airbag", conf, list(src),
        dict(t=title_en, d=d_en + " " + SRS_EN, c=causes_en, s=SRS_LAMP_EN),
        dict(t=title_ro, d=d_ro + " " + SRS_RO, c=causes_ro, s=SRS_LAMP_RO), LAMP)


srs("B1000", SRS_SRC + [B1000_TSB], "Airbag control unit (center airbag sensor) fault", "Defect al unității de control airbag (senzorul central)",
    "The center airbag sensor assembly, which is the airbag control unit, found an internal fault. On some 2015-2017 "
    "Lexus NX and RX Toyota issued a software update for an intermittent B1000 stored in history.",
    "Senzorul central de airbag, adică unitatea de control airbag, a găsit un defect intern. La unele Lexus NX și RX "
    "2015-2017, Toyota a emis o actualizare software pentru un B1000 intermitent memorat în istoric.",
    [("medium", "Center airbag sensor assembly"), ("low", "Software fault on some Lexus models (update available)")],
    [("medium", "Senzorul central de airbag"), ("low", "Defect software la unele modele Lexus (există actualizare)")], conf="official")
for code, side_en, side_ro, src in (("B1603", "right", "dreapta", [SRS_TSB, RAV4_SRS, FJ_SRS]),
                                    ("B1608", "left", "stânga", [SRS_TSB, RAV4_SRS, FJ_SRS])):
    srs(code, src, f"Front crash sensor line ({side_en}) not initialised", f"Linia senzorului de impact frontal ({side_ro}) neinițializată",
        f"The airbag control unit could not finish setting up the {side_en} front crash sensor line at start-up.",
        f"Unitatea de control airbag nu a putut finaliza inițializarea liniei senzorului de impact frontal din {side_ro} la pornire.",
        [("medium", "Wiring or connector to the front crash sensor"), ("low", "Front crash sensor"), ("low", "Airbag control unit")],
        [("medium", "Cablaj sau conector spre senzorul de impact frontal"), ("low", "Senzorul de impact frontal"), ("low", "Unitatea de control airbag")], conf="official")
for code, side_en, side_ro in (("B1610", "right", "dreapta"), ("B1615", "left", "stânga")):
    srs(code, [RAV4_SRS, FJ_SRS, TC_SRS, TC_SRS2], f"Front crash sensor ({side_en}) fault", f"Defect la senzorul de impact frontal ({side_ro})",
        f"The {side_en} front crash sensor, at the front of the engine bay, or its circuit has a fault.",
        f"Senzorul de impact frontal din {side_ro}, aflat în fața compartimentului motor, sau circuitul lui are un defect.",
        [("medium", "Front crash sensor (often after front-end damage or repair)"), ("medium", "Sensor wiring or connector"), ("low", "Airbag control unit")],
        [("medium", "Senzorul de impact frontal (adesea după avarii sau reparații în față)"), ("medium", "Cablajul sau conectorul senzorului"), ("low", "Unitatea de control airbag")])
for code, side_en, side_ro, kind in (("B1612", "right", "dreapta", "comm"), ("B1613", "right", "dreapta", "init"),
                                     ("B1617", "left", "stânga", "comm"), ("B1618", "left", "stânga", "init")):
    srs(code, [SRS_TSB, RAV4_SRS, FJ_SRS, TC_SRS2],
        f"Front crash sensor ({side_en}): " + ("no communication" if kind == "comm" else "not initialised"),
        f"Senzorul de impact frontal ({side_ro}): " + ("fără comunicație" if kind == "comm" else "neinițializat"),
        f"The airbag control unit " + ("lost communication with" if kind == "comm" else "could not finish initialising") + f" the {side_en} front crash sensor.",
        "Unitatea de control airbag " + ("a pierdut comunicația cu" if kind == "comm" else "nu a putut finaliza inițializarea pentru") + f" senzorul de impact frontal din {side_ro}.",
        [("medium", "Wiring or connector to the front crash sensor"), ("low", "Front crash sensor"), ("low", "Airbag control unit")],
        [("medium", "Cablaj sau conector spre senzorul de impact frontal"), ("low", "Senzorul de impact frontal"), ("low", "Unitatea de control airbag")], conf="official")
srs("B1650", [RAV4_SRS, FJ_SRS, TC_SRS2], "Occupant classification system fault", "Defect la sistemul de clasificare a pasagerului",
    "The system under the front passenger seat that decides whether the passenger airbag may deploy reports a fault. "
    "Diagnose the occupant classification system itself; it may also need a zero point calibration after seat work.",
    "Sistemul de sub scaunul pasagerului din față, care decide dacă airbagul pasagerului se poate declanșa, raportează "
    "un defect. Diagnostichează sistemul de clasificare propriu-zis; poate avea nevoie și de o calibrare a punctului "
    "zero după lucrări la scaun.",
    [("medium", "Seat wiring or connector under the passenger seat"), ("medium", "Occupant classification sensor or ECU"), ("low", "Airbag control unit")],
    [("medium", "Cablajul sau conectorul de sub scaunul pasagerului"), ("medium", "Senzorul sau calculatorul de clasificare a pasagerului"), ("low", "Unitatea de control airbag")])
srs("B1653", [RAV4_SRS, FJ_SRS, TC_SRS2], "Driver seat position sensor circuit fault", "Defect la circuitul senzorului de poziție al scaunului șoferului",
    "The sensor that tells the airbag unit whether the driver's seat is far forward has a fault; the unit uses it to "
    "adapt how the driver airbag deploys.",
    "Senzorul care îi spune unității airbag dacă scaunul șoferului este mult în față are un defect; unitatea îl "
    "folosește ca să adapteze declanșarea airbagului șoferului.",
    [("medium", "Seat wiring or connector under the driver's seat"), ("medium", "Seat position sensor")],
    [("medium", "Cablajul sau conectorul de sub scaunul șoferului"), ("medium", "Senzorul de poziție al scaunului")])
srs("B1660", [RAV4_SRS, FJ_SRS, TC_SRS, TC_SRS2], "Passenger airbag ON/OFF indicator circuit fault", "Defect la circuitul indicatorului airbag pasager ON/OFF",
    "The indicator that shows whether the front passenger airbag is on or off has a circuit fault, so you can't trust "
    "what it shows.",
    "Indicatorul care arată dacă airbagul pasagerului din față este activ sau dezactivat are un defect de circuit, "
    "așa că nu te poți baza pe ce afișează.",
    [("medium", "Indicator wiring or connector"), ("low", "Indicator / panel assembly"), ("low", "Airbag control unit")],
    [("medium", "Cablajul sau conectorul indicatorului"), ("low", "Indicatorul / ansamblul panoului"), ("low", "Unitatea de control airbag")])

# Squib (firing) circuits: four codes per airbag.
SQUIB_KINDS = [
    ("short in the circuit", "scurtcircuit în circuit", "The two wires of the firing circuit for {w} are shorted together.",
     "Cele două fire ale circuitului de declanșare pentru {w} sunt în scurtcircuit între ele."),
    ("open circuit", "circuit întrerupt", "The firing circuit for {w} is open.", "Circuitul de declanșare pentru {w} este întrerupt."),
    ("short to ground", "scurtcircuit la masă", "The firing circuit for {w} is shorted to ground.",
     "Circuitul de declanșare pentru {w} este în scurtcircuit la masă."),
    ("short to battery voltage", "scurtcircuit la plus", "The firing circuit for {w} is shorted to battery voltage.",
     "Circuitul de declanșare pentru {w} este în scurtcircuit la tensiunea bateriei."),
]
SQUIBS = [
    ("B1800", "Driver airbag", "Airbag șofer", "the driver airbag in the steering wheel", "airbagul șoferului din volan",
     [("medium", "Spiral cable (clock spring) behind the steering wheel"), ("medium", "Connector at the steering wheel airbag"), ("low", "Driver airbag or airbag control unit")],
     [("medium", "Cablul spiralat (clock spring) din spatele volanului"), ("medium", "Conectorul airbagului din volan"), ("low", "Airbagul șoferului sau unitatea de control airbag")]),
    ("B1805", "Front passenger airbag", "Airbag pasager față", "the front passenger airbag in the dashboard", "airbagul pasagerului din față din bord",
     [("medium", "Wiring or connector behind the dashboard"), ("low", "Passenger airbag or airbag control unit")],
     [("medium", "Cablaj sau conector din spatele bordului"), ("low", "Airbagul pasagerului sau unitatea de control airbag")]),
]
for first, nen, nro, wen, wro, cen, cro in SQUIBS:
    base = int(first[1:])
    for i, (ken, kro, den, dro) in enumerate(SQUIB_KINDS):
        srs("B%04d" % (base + i), SRS_SRC + [TC_SRS2], f"{nen} firing circuit: {ken}", f"{nro}, circuit de declanșare: {kro}",
            den.format(w=wen), dro.format(w=wro), cen, cro)
srs("B1861", [KNEE_TSB, TC_SRS, TC_SRS2], "Driver knee airbag firing circuit open", "Circuitul de declanșare al airbagului de genunchi al șoferului întrerupt",
    "The firing circuit of the knee airbag under the steering column is open. For 2006-2008 Lexus IS, Lexus issued a "
    "wiring-harness repair for this code.",
    "Circuitul de declanșare al airbagului de genunchi de sub coloana de direcție este întrerupt. Pentru Lexus IS "
    "2006-2008, Lexus a emis o reparație a cablajului pentru acest cod.",
    [("medium", "Connector or wiring at the knee airbag"), ("low", "Knee airbag or airbag control unit")],
    [("medium", "Conectorul sau cablajul airbagului de genunchi"), ("low", "Airbagul de genunchi sau unitatea de control airbag")], conf="official")

# ============================================================================
# Tyre pressure (TPMS)
# ============================================================================
TPMS_EN = ("Toyota numbers the sensors in the order they were registered (ID1, ID2, ...), not by wheel position, so a "
           "workshop identifies the wheel from the stored sensor IDs. Until it is fixed, check tyre pressures yourself with a gauge.")
TPMS_RO = ("Toyota numerotează senzorii în ordinea în care au fost înregistrați (ID1, ID2, ...), nu după poziția roții, "
           "așa că un service identifică roata după ID-urile memorate. Până la remediere, verifică singur presiunea în anvelope cu un manometru.")
TPMS_S_EN = ["Tyre-pressure warning light blinks, then stays on"]
TPMS_S_RO = ["Martorul de presiune în anvelope clipește, apoi rămâne aprins"]
TPMS_SRC = [RAV4_TPMS, TC_TPMS]


def tpms(code, src, t_en, t_ro, d_en, d_ro, c_en, c_ro, per_sensor=True):
    add(code, "tpms", "two-sources", src,
        dict(t=t_en, d=d_en + (" " + TPMS_EN if per_sensor else " Until it is fixed, check tyre pressures yourself with a gauge."), c=c_en, s=TPMS_S_EN),
        dict(t=t_ro, d=d_ro + (" " + TPMS_RO if per_sensor else " Până la remediere, verifică singur presiunea în anvelope cu un manometru."), c=c_ro, s=TPMS_S_RO),
        LAMP)


for n in range(1, 5):
    tpms("C211%d" % n, TPMS_SRC, f"Tyre-pressure sensor ID{n} stopped transmitting", f"Senzorul de presiune ID{n} nu mai transmite",
         f"The tyre-pressure sensor registered as ID{n} has stopped transmitting. If it starts sending again, the code clears.",
         f"Senzorul de presiune înregistrat ca ID{n} nu mai transmite. Dacă reîncepe să transmită, codul se șterge.",
         [("medium", "Tyre-pressure sensor (valve and transmitter)"), ("low", "Tyre-pressure control unit")],
         [("medium", "Senzorul de presiune (valvă și emițător)"), ("low", "Unitatea de control a presiunii")])
    tpms("C212%d" % n, TPMS_SRC, f"No signal from tyre-pressure sensor ID{n}", f"Niciun semnal de la senzorul de presiune ID{n}",
         f"The receiver doesn't hear the tyre-pressure sensor registered as ID{n}.",
         f"Receptorul nu primește semnal de la senzorul de presiune înregistrat ca ID{n}.",
         [("medium", "Wheel without a sensor fitted (for example winter wheels)"), ("medium", "Tyre-pressure sensor"),
          ("low", "Radio interference, receiver or wiring")],
         [("medium", "Roată montată fără senzor (de exemplu roțile de iarnă)"), ("medium", "Senzorul de presiune"),
          ("low", "Interferențe radio, receptorul sau cablajul")])
    tpms("C214%d" % n, TPMS_SRC, f"Tyre-pressure sensor ID{n} internal fault", f"Defect intern la senzorul de presiune ID{n}",
         f"The tyre-pressure sensor registered as ID{n} reports a fault in its own electronics.",
         f"Senzorul de presiune înregistrat ca ID{n} raportează un defect în propria electronică.",
         [("medium", "Tyre-pressure sensor (valve and transmitter)")], [("medium", "Senzorul de presiune (valvă și emițător)")])
tpms("C2171", [RAV4_TPMS, TC_TPMS, MTD_CHR], "Tyre-pressure sensor IDs not registered", "ID-urile senzorilor de presiune neînregistrate",
     "The tyre-pressure system has no sensor IDs stored, typically after the control unit was replaced or new sensors "
     "were fitted. The IDs must be registered with a scan tool.",
     "Sistemul de presiune în anvelope nu are memorat niciun ID de senzor, de obicei după înlocuirea unității de "
     "control sau montarea unor senzori noi. ID-urile trebuie înregistrate cu un tester.",
     [("medium", "IDs not registered after a sensor or control unit replacement")], [("medium", "ID-uri neînregistrate după înlocuirea senzorilor sau a unității de control")],
     per_sensor=False)
tpms("C2176", TPMS_SRC, "Tyre-pressure receiver fault", "Defect la receptorul de presiune în anvelope",
     "The receiver that picks up the tyre-pressure sensors' radio signals, or its connection to the control unit, has a fault.",
     "Receptorul care preia semnalele radio ale senzorilor de presiune, sau legătura lui cu unitatea de control, are un defect.",
     [("medium", "Receiver wiring or connector"), ("medium", "Receiver"), ("low", "Tyre-pressure control unit")],
     [("medium", "Cablajul sau conectorul receptorului"), ("medium", "Receptorul"), ("low", "Unitatea de control a presiunii")], per_sensor=False)
tpms("C2177", [TC_TPMS, MTD_CHR], "Tyre-pressure initialisation not completed", "Inițializarea presiunii în anvelope nefinalizată",
     "After the tyre pressures were set and the system was reset, it could not finish learning the new pressures while "
     "driving. Set the pressures cold and repeat the reset.",
     "După reglarea presiunilor și resetarea sistemului, acesta nu a reușit să învețe noile presiuni în timpul mersului. "
     "Reglează presiunile la rece și repetă resetarea.",
     [("medium", "Reset not completed or interrupted"), ("low", "A sensor not received during learning")],
     [("medium", "Resetare nefinalizată sau întreruptă"), ("low", "Un senzor nerecepționat în timpul învățării")], per_sensor=False)

# ============================================================================
# Hybrid system
# ============================================================================
HV_EN = "A hybrid warning shows the system protecting itself; if the car limits power or won't go into READY, stop somewhere safe and get help."
HV_RO = "Un avertisment de hibrid arată că sistemul se protejează; dacă mașina limitează puterea sau nu intră în READY, oprește într-un loc sigur și cere ajutor."
HV_S_EN = ["Check-engine light", "Hybrid system warning on the display"]
HV_S_RO = ["Martorul de motor aprins", "Avertisment de sistem hibrid pe afișaj"]
add("P3000", "hybrid", "official", [P3000_TT, TC_HV],
    dict(t="Hybrid battery control system fault", d="The hybrid control unit stored this code because the high-voltage battery system reported a fault. "
         "The information code (sub-code) stored with it matters: with 123 it only records that the battery computer set its own code, so "
         "read and repair the battery computer's codes, not this one. " + HV_EN,
         c=[("medium", "Fault reported by the high-voltage battery computer"), ("low", "Hybrid battery or its sensors")], s=HV_S_EN),
    dict(t="Defect în sistemul de control al bateriei hibride", d="Unitatea de control hibrid a memorat acest cod pentru că sistemul bateriei de înaltă tensiune a raportat un defect. "
         "Contează codul de informație (subcodul) memorat împreună cu el: cu 123 doar înregistrează că calculatorul bateriei și-a setat propriul cod, așa că "
         "citește și repară codurile calculatorului bateriei, nu pe acesta. " + HV_RO,
         c=[("medium", "Defect raportat de calculatorul bateriei de înaltă tensiune"), ("low", "Bateria hibridă sau senzorii ei")], s=HV_S_RO), MIL)
for code, t_en, t_ro in (("P3004", "High-voltage power source fault (hybrid inverter)", "Defect al sursei de înaltă tensiune (invertor hibrid)"),
                         ("P324E", "Hybrid inverter control (MG ECU) internal fault", "Defect intern al controlului invertorului hibrid (MG ECU)")):
    add(code, "hybrid", "official", [IPM_TSB, IPM_ZE3, TC_HV],
        dict(t=t_en, d="A fault inside the hybrid inverter, the unit that converts battery power for the electric motors. On 2010-2014 Prius and "
             "2012-2014 Prius v, Toyota names this code (alone or with P0A94 and P0A1A) as a sign of a failed power module in the inverter, "
             "covered by an extended warranty; check with a Toyota dealer. The car usually goes into a fail-safe mode with reduced power. " + HV_EN,
             c=[("medium", "Inverter power module (IPM) or MG control board"), ("low", "Inverter wiring or connectors")], s=HV_S_EN + ["Reduced power (fail-safe mode)"]),
        dict(t=t_ro, d="Un defect în interiorul invertorului hibrid, unitatea care transformă energia bateriei pentru motoarele electrice. La Prius 2010-2014 și "
             "Prius v 2012-2014, Toyota indică acest cod (singur sau cu P0A94 și P0A1A) ca semn al unui modul de putere defect în invertor, "
             "acoperit de o garanție extinsă; verifică la un dealer Toyota. De obicei mașina intră într-un mod de avarie cu putere redusă. " + HV_RO,
             c=[("medium", "Modulul de putere al invertorului (IPM) sau placa de control MG"), ("low", "Cablajul sau conectorii invertorului")], s=HV_S_RO + ["Putere redusă (mod de avarie)"]), MIL)
# P3190/P3191 are judged by the hybrid system from the engine's behaviour; depending on the car they
# can be read from the hybrid unit (7E2) or the engine computer (7E0).
for module in ("hybrid", "engine"):
    add("P3190", module, "official", [PURGE_TSB, P3190_TSB],
        dict(t="Poor engine power", d="The hybrid system found that the petrol engine delivers less power than expected. The cause is on the engine side: "
             "air, fuel or ignition. On 2010-2013 Prius and 2012-2013 Prius v, Toyota found a detached purge (EVAP) hose at the throttle body "
             "causing this together with P0A0F and P3191; on 2017-2018 Highlander Hybrid an engine software update addresses it at high altitude and heat.",
             c=[("medium", "Air leak, such as a detached vacuum or purge hose"), ("medium", "Fuel supply problem, such as low fuel or a weak pump"),
                ("low", "Dirty air filter or throttle body")], s=["Check-engine light", "Weak acceleration"]),
        dict(t="Putere slabă a motorului", d="Sistemul hibrid a constatat că motorul pe benzină dă mai puțină putere decât se așteaptă. Cauza este la motor: "
             "aer, combustibil sau aprindere. La Prius 2010-2013 și Prius v 2012-2013, Toyota a găsit un furtun de purjare (EVAP) desprins de la "
             "clapeta de accelerație, care producea acest cod împreună cu P0A0F și P3191; la Highlander Hybrid 2017-2018 o actualizare software "
             "a motorului rezolvă problema la altitudine mare și căldură.",
             c=[("medium", "Aer fals, de exemplu un furtun de vacuum sau de purjare desprins"), ("medium", "Problemă de alimentare, de exemplu combustibil puțin sau pompă slabă"),
                ("low", "Filtru de aer sau clapetă murdare")], s=["Martorul de motor aprins", "Accelerare slabă"]), MIL, key="P3190@" + module)
    add("P3191", module, "official", [PURGE_TSB],
        dict(t="Engine does not start (hybrid)", d="The hybrid system tried to start the petrol engine but it didn't start running. The car may still drive "
             "on the battery for a short while. Check fuel level first; on 2010-2013 Prius and 2012-2013 Prius v Toyota also found a detached "
             "purge (EVAP) hose at the throttle body behind this code.",
             c=[("medium", "Fuel supply: empty tank, fuel pump or filter"), ("medium", "Air leak, such as a detached purge hose"), ("low", "Ignition or sensor fault")],
             s=["Check-engine light", "Engine does not run; limited driving on the battery"]),
        dict(t="Motorul nu pornește (hibrid)", d="Sistemul hibrid a încercat să pornească motorul pe benzină, dar acesta nu a pornit. Mașina poate merge încă puțin "
             "pe baterie. Verifică întâi nivelul combustibilului; la Prius 2010-2013 și Prius v 2012-2013 Toyota a găsit și un furtun de purjare (EVAP) "
             "desprins de la clapeta de accelerație în spatele acestui cod.",
             c=[("medium", "Alimentare: rezervor gol, pompa sau filtrul de combustibil"), ("medium", "Aer fals, de exemplu un furtun de purjare desprins"), ("low", "Defect de aprindere sau de senzor")],
             s=["Martorul de motor aprins", "Motorul nu funcționează; mers limitat pe baterie"]), MIL, key="P3191@" + module)
# Without module context (e.g. a manual lookup) the same text applies.
for code in ("P3190", "P3191"):
    PACK[code] = dict(PACK[code + "@hybrid"])

# ============================================================================
# Engine
# ============================================================================
add("P1604", "engine", "official", [FLEX_TSB, CARBON_TSB, TC_2ZR, TC_1NZX],
    dict(t="Startability malfunction (hard or long start)", d="The engine computer noticed that the engine took too long to start or did not start "
         "properly. It is a symptom code: look at other codes stored with it. Toyota has linked it to carbon build-up in the combustion chambers on "
         "the 2012-2013 iQ and to a fuel-quality learning error on flex-fuel V8 engines.",
         c=[("medium", "Fuel supply problem (pressure, pump, injectors)"), ("medium", "Air leak or dirty throttle body"), ("low", "Carbon build-up in the engine")],
         s=["Check-engine light", "Long cranking before the engine starts"]),
    dict(t="Defect de pornire (pornire grea sau lungă)", d="Calculatorul motorului a observat că motorul a pornit prea greu sau nu a pornit corect. "
         "Este un cod de simptom: uită-te la celelalte coduri memorate împreună cu el. Toyota l-a legat de depunerile de carbon din camerele de ardere "
         "la iQ 2012-2013 și de o eroare de învățare a calității combustibilului la motoarele V8 flex-fuel.",
         c=[("medium", "Problemă de alimentare (presiune, pompă, injectoare)"), ("medium", "Aer fals sau clapetă murdară"), ("low", "Depuneri de carbon în motor")],
         s=["Martorul de motor aprins", "Demarorul învârte mult până pornește motorul"]), MIL)
add("P1605", "engine", "official", [FLEX_TSB, MISFIRE_TSB, CARBON_TSB, TC_2ZR, TC_1NZX],
    dict(t="Rough idling", d="The engine computer saw the idle speed drop or become unstable. It is a symptom code that often comes with misfire or "
         "mixture codes, so diagnose those first.",
         c=[("medium", "Misfire (spark plugs, ignition coils)"), ("medium", "Air leak or dirty throttle body"), ("low", "Carbon build-up in the engine")],
         s=["Check-engine light", "Shaky idle or idle dips"]),
    dict(t="Ralanti instabil", d="Calculatorul motorului a văzut turația de ralanti scăzând sau devenind instabilă. Este un cod de simptom care apare adesea "
         "cu coduri de rateuri sau de amestec, așa că diagnostichează-le întâi pe acelea.",
         c=[("medium", "Rateuri (bujii, bobine de inducție)"), ("medium", "Aer fals sau clapetă murdară"), ("low", "Depuneri de carbon în motor")],
         s=["Martorul de motor aprins", "Ralanti tremurat sau cu scăderi"]), MIL)
add("P148F", "engine", "official", [P148F_TSB, TC_1NZX, TC_2ZRX],
    dict(t="Electric coolant pump running too fast", d="The electric engine coolant pump on Toyota hybrids spins faster than it should, which can happen "
         "when it pumps air because the coolant level is low. On 2016-2019 Prius and Prius Prime, Toyota traced this to a coolant leak at "
         "the exhaust heat exchanger. Check the coolant level only with the engine cold.",
         c=[("medium", "Low coolant level or a coolant leak"), ("low", "Air in the cooling system after a repair"), ("low", "Coolant pump")],
         s=["Check-engine light", "Low coolant level", "Poor cabin heating"]),
    dict(t="Pompa electrică de lichid de răcire se rotește prea repede", d="Pompa electrică de lichid de răcire a motorului, la hibridele Toyota, se rotește "
         "mai repede decât ar trebui, ceea ce se poate întâmpla când pompează aer pentru că nivelul lichidului este scăzut. La Prius și Prius Prime "
         "2016-2019, Toyota a găsit cauza într-o scurgere la schimbătorul de căldură de pe evacuare. Verifică nivelul doar cu motorul rece.",
         c=[("medium", "Nivel scăzut de lichid de răcire sau o scurgere"), ("low", "Aer în sistemul de răcire după o reparație"), ("low", "Pompa de lichid de răcire")],
         s=["Martorul de motor aprins", "Nivel scăzut al lichidului de răcire", "Încălzire slabă în habitaclu"]), MIL)

# ============================================================================
# Body: engine immobiliser
# ============================================================================
add("B2784", "body", "two-sources", [RAV4_IMMO, TC_ESS],
    dict(t="Immobiliser antenna coil open or shorted", d="The antenna coil around the ignition (or the start button) that reads the key's chip has an "
         "open or short circuit, so the key may not be recognised and the engine may not start.",
         c=[("medium", "Antenna coil / key amplifier or its wiring"), ("low", "Immobiliser (transponder key) control unit")],
         s=["Engine may not start", "Security light behaves abnormally"]),
    dict(t="Bobina antenei imobilizatorului întreruptă sau în scurtcircuit", d="Bobina antenei din jurul contactului (sau al butonului de pornire), care "
         "citește cipul cheii, are circuitul întrerupt sau în scurtcircuit, așa că cheia s-ar putea să nu fie recunoscută și motorul să nu pornească.",
         c=[("medium", "Bobina antenei / amplificatorul cheii sau cablajul lor"), ("low", "Unitatea de control a imobilizatorului (transponder)")],
         s=["Motorul s-ar putea să nu pornească", "Martorul de securitate se comportă anormal"]))
# B2799 is stored by the engine computer (the RAV4 manual lists it under the ECM).
add("B2799", "engine", "two-sources", [RAV4_IMMO, TC_ESS],
    dict(t="Engine immobiliser system fault", d="The engine computer and the immobiliser could not agree that a valid key is present, so the engine "
         "is blocked from starting or stops right after starting. Try your spare key; if it starts with that one, the first key is the problem.",
         c=[("medium", "Key not recognised (damaged key, or not registered)"), ("medium", "Wiring between the immobiliser unit and the engine computer"),
            ("low", "Immobiliser control unit or engine computer")],
         s=["Engine does not start, or stops after a second or two", "Security light blinks"]),
    dict(t="Defect în sistemul imobilizatorului", d="Calculatorul motorului și imobilizatorul nu au căzut de acord că există o cheie validă, așa că motorul "
         "este blocat la pornire sau se oprește imediat după pornire. Încearcă cheia de rezervă; dacă pornește cu ea, problema este la prima cheie.",
         c=[("medium", "Cheie nerecunoscută (cheie deteriorată sau neînregistrată)"), ("medium", "Cablajul dintre unitatea imobilizatorului și calculatorul motorului"),
            ("low", "Unitatea de control a imobilizatorului sau calculatorul motorului")],
         s=["Motorul nu pornește sau se oprește după o secundă-două", "Martorul de securitate clipește"]))


def main():
    pack = {
        "brand": "toyota-lexus",
        "label": "Obdaryn's Toyota/Lexus pack",
        "label_ro": "pachetul Obdaryn pentru Toyota/Lexus",
        "brandName": "Toyota/Lexus",
        "source": {
            "license": "GPL-3.0-or-later (text written for Obdaryn)",
            "note": "Meanings compiled from public Toyota/Lexus documents filed with NHTSA, republished Toyota repair-manual "
                    "pages and independent code tables; each code lists its sources. Verify with a workshop before replacing parts.",
            "scope": "2008-2020 Toyota/Lexus sold in Europe and the US: Corolla, Auris, Yaris, RAV4, C-HR, Prius and hybrids, Avensis",
        },
        "codes": dict(sorted(PACK.items())),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(pack, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"wrote {len(PACK)} codes to {OUT}")


if __name__ == "__main__":
    main()
