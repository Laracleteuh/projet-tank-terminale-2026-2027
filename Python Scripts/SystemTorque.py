# -- Variables --
mSysteme = 0
rRoue = 0
a = 0
Crr = 0
nMotor = 0
Frr = 0
g = 9.806
Fa = 0
Ftot = 0
C = 0
CperEngine = 0


def calcul():
    mSysteme = input("Masse totale du système en kg : ")
    if mSysteme == "def":
        mSysteme = 65
        rRoue = 0.1325
        a = 0.5
        Crr = 0.12
        nMotor = 2
    else:
        mSysteme = float(mSysteme)
        rRoue = float(input("Rayon de la roue motrice en m : "))
        a = float(input("Accélération désirée en m/s^-2 : "))
        Crr = float(input("Coefficient de résistance aux frottements (Crr) : "))
        nMotor = int(input("Nombre de moteurs dans le système : "))

    Frr = mSysteme * g * Crr
    Fa = mSysteme * a
    Ftot = Fa + Frr
    C = Ftot * rRoue
    CperEngine = C / nMotor

    print(
        "----------------------------------------------------------------------------------------"
    )
    print("Force de résistance au mouvements : ", Frr, "N")
    print("Force nécéssaire pour l'accélération du char : ", Fa, "N")
    print(
        "Force totale nécéssaire pour démarrer et faire avancer le système en N : ",
        Ftot,
        "N",
    )
    print(
        "Couple total nécéssaire pour démarrer et faire avancer le système : ", C, "N.m"
    )
    print(
        "Couple total nécéssaire pour démarrer et faire avancer le système par moteur : ",
        CperEngine,
        "N.m",
    )
    print(
        "----------------------------------------------------------------------------------------"
    )


def exit():
    ex = input("exit ? [y/n]").lower()
    if ex == "y":
        pass
    elif ex == "n":
        main()
    else:
        print("Error, answer not recognized")
        exit()


def main():
    print("   ")
    print("Welcome to SystemTorque.py 1.0")
    print(
        "After the confirmation you'll be asked to enter characteristics of your system."
    )
    print(
        "Then you'll be able to have an idea about how much torque is necessary to make your system go. (Applicable for wheel-driven systems only)"
    )
    print(
        "WARNING : Those calculations doesn't take into account physics phenomen, but is rather based on simple mathematic equations so you have an idea of the performance of your system."
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
