# Calculateur caracteristiques moteurs
from math import pi

# -- Variables --
Nm = 0
Ns = 0
Rse = 0
Rr = 0
Um = 0
Im = 0
CmAtSpeed = 0
Cn = 0
Pe = 0
Cr = 0

rState = None
calcState = 0
ex = 0

# fonction


def calcul():
    Nm = input("Vitesse de rotation nominale du moteur en tr/min : ")
    if Nm == "def":
        Nm = 1850
        Ns = 210
        Rse = Ns / Nm
        Rr = 0.10
        Um = 24
        Im = 12.5
        CmAtSpeed = 0.834
        Cn = 1.6
        rState = None

    else:
        Ns = float(
            input("Vitesse de rotation du moteur en sortie de réducteur en tr/min : ")
        )
        Rse = float(Ns / float(Nm))
        Rr = float(input("Rayon de la roue en m: "))
        Um = float(input("Tension nominale du moteur en V : "))
        Im = float(input("Intensité nominale du moteur en A : "))
        CmAtSpeed = float(
            input("Couple du moteur à sa vitesse de rotation nominale en N.m : ")
        )
        Cn = float(input("Couple nominal du moteur en N.m : "))
        rState = None

    Pe = Um * Im
    Pmm = Cn * (float(Nm) * (pi / 30))
    Pm = CmAtSpeed * (float(Nm) * (pi / 30))
    Vr = float(Nm) * Rse
    VrRad = Vr * (pi / 30)
    Cr = CmAtSpeed * (1 / Rse)
    Vav = VrRad * Rr
    r = Pm / Pmm

    if r < 0.20:
        rState = str("Rendement Mauvais - Vérifier les valeurs")
    elif 0.20 <= r < 0.65:
        rState = str("Rendement médiocre")
    elif r >= 1:
        print(
            "You've made somthing that creates energy. (Maybe you weren't expecting to do that) Kindly check your values and calculate again."
        )
    else:
        rState = str("Rendement bon")

    print(
        "----------------------------------------------------------------------------------------"
    )
    print("Rapport de réduction du moteur : ", Rse)
    print("Puissance électrique du moteur : ", Pe, "W")
    print("Puissance mécanique du moteur : ", Pmm, "W")
    print("Puissance mécanique en sortie du réducteur : ", Pm, "W")
    print("Vitesse de sortie : ", Vr, "tr/min | ", VrRad, "rad/s")
    print("Couple de sorte : ", Cr, "N.m")
    print("Vitesse d'avance théorique du système : ", Vav, "m/s | ", Vav * 3.6, "km/h")
    print("Rendement du système : ", r)
    print(rState)
    print(
        "----------------------------------------------------------------------------------------"
    )


def exit():
    ex = input("exit ? [y/n]")
    if ex == "y":
        pass
    elif ex == "n":
        main()
    else:
        print("Error, answer not recognized")
        exit()


def main():
    print("   ")
    print("Welcome to EngineCharacteristics.py 1.0")
    print(
        "After the confirmation you'll be asked to enter characteristics of your electric motor."
    )
    print(
        "Then you'll be able to see Differents characteristics your system will have, such as forward speed and torque."
    )
    print(
        "WARNING : Those calculations doesn't take into account physics phenomen, but is rather based on simple mathematic equation so you have an idea of the performance of your system."
    )
    print(
        "To have a more realistic simulation, be free to head to MatLab to proprely simulate your system"
    )
    print(
        "----------------------------------------------------------------------------------------"
    )
    calcul()
    exit()


main()
