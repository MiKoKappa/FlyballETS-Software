#test ESP32
import sys
import serial
import time
import string
from termcolor import colored
from colorama import init, Fore

exitfile = open("D:\\Users\\konri\\Kurs\\ESP32\\dataESP.txt", "wb")
outputfile = open("D:\\Users\\konri\\Kurs\\ESP32\\stabilityLOG.txt", "wb")

ser = serial.Serial('COM9', 115200)
time.sleep(2)
if ser.isOpen():
    print("COM port is available")
else:
    print("COM port is not available")

#ser.write(b"STOP" + b"\n")
#ser.write(b"RESET" + b"\n")
time.sleep(5)
ammountofraces = 1
racenumber = 0
endless = False
#selectedrace = input("Select race: ") # 0 or 1 or 2
selectedrace = str(sys.argv[1])
#print(type(selectedrace))
if selectedrace.isdigit() == True:
    bselectedrace = selectedrace.encode('utf-8')
    ser.write(b"RACE " + bselectedrace + b"\n")
    racefile = open("D:\\Users\\konri\\Kurs\\ESP32\\RACE" + selectedrace + ".txt", "r")
elif selectedrace == "-all":
    ser.write(b"RACE 0" + b"\n")
    for a in range(2):
        numofraces = ser.readline()[:-2]
        #print(numofraces)
    splitnumofraces = numofraces.split(b"races: ")
    ammountofraces = int(splitnumofraces[1])
    exitfile.write(b"//RACE 0" + b'\n')
    racefile = open("D:\\Users\\konri\\Kurs\\ESP32\\RACE0.txt", "r")
elif selectedrace == "-stab":
    endless = True
    ser.write(b"RACE 0" + b"\n")
    for a in range(2):
        numofraces = ser.readline()[:-2]
    splitnumofraces = numofraces.split(b"races: ")
    #print(splitnumofraces)
    ammountofraces = int(splitnumofraces[1])
    exitfile.write(b"//RACE 0" + b'\n')
    racefile = open("D:\\Users\\konri\\Kurs\\ESP32\\RACE0.txt", "r")
else:
    print("Error: Invalid input")
    racenumber = ammountofraces
#print(ammountofraces)
readline = ser.readline()

bytetime = b'0'
raceEND = False
while racenumber < ammountofraces:
    ser.write(b"RESET" + b"\n") #\x52\x45\x53\x45\x54\x0a (utf-8)
    if raceEND == True:
        raceEND = False
        racefile.close()
        if racenumber <= ammountofraces:
            bracenumber = b'%i' % racenumber
            #print(bracenumber)
            strracenumber = str(racenumber)
            racefile = open("D:\\Users\\konri\\Kurs\\ESP32\\RACE" + strracenumber + ".txt", "r")
            ser.write(b"RACE " + bracenumber + b'\n')
            for x in range(3):
                ser.readline()
            exitfile.write(b"//RACE " + bracenumber + b'\n')
    ser.write(b"START" + b"\n") #\x53\x54\x41\x52\x54\x0a (utf-8)

    while raceEND != True:
        readline = ser.readline()[:-2]
        decodeline = readline.decode('utf-8')
        splitdecodeline = decodeline.split("(): ")
        #print(splitdecodeline[1])
        if endless == True:
            outputfile.write(readline + b'\n')
            if splitdecodeline[1] == "RS:  STOP  ":
                outputfile.write(bytetime + b'\n')
        if((splitdecodeline[1].startswith("D0") or splitdecodeline[1].startswith("D1") 
        or splitdecodeline[1].startswith("D2") or splitdecodeline[1].startswith("D3") 
        or splitdecodeline[1].startswith("RT") or splitdecodeline[1].startswith("RS") 
        or splitdecodeline[1].startswith("Dog "))
        and splitdecodeline[1] != "Dog 0:   0.000|CR:  " and splitdecodeline[1] != "RT:  0.000"):
            #exitfile.write(splitdecodeline[1].encode('utf-8') + b'\n')
            lengthofline = len(splitdecodeline[1])
            normdecodeline = splitdecodeline[1]
            for x in range(30-lengthofline):
                normdecodeline = normdecodeline + " "
            expectedline = racefile.readline()[:-1]
            if expectedline.startswith("//RACE") == True:
                expectedline = racefile.readline()[:-1]
            if splitdecodeline[1] == expectedline:
                print(normdecodeline, colored("OK", 'green'))                    
                exitfile.write(normdecodeline.encode('utf-8')+ b'  OK' + b'\n')
            else:
                print(normdecodeline, colored("NOK", 'red'), " Exp: ", Fore.YELLOW + expectedline, Fore.RESET)
                exitfile.write(normdecodeline.encode('utf-8')+ b'  NOK' + b'   Exp: ' + expectedline.encode('utf-8') + b'\n')      

        if splitdecodeline[1] == "RS:  STOP  ":
            raceEND = True
            racenumber += 1
            if racenumber == ammountofraces and endless == True:
                racefile.seek(0)
                racenumber = 0
        del splitdecodeline
    if endless == True:
        ser.write(b"TIME" + b'\n')
        timecheck = ser.readline()[:-2]
        timecheck = ser.readline()[:-2]
        #print(timecheck)
        #if timecheck.startswith(b"[I]"):
        splittimecheck = timecheck.split(b" ")
        splittimecheck = splittimecheck[5].split(b".")
        time = int(splittimecheck[0])
        bytetime = bytes(splittimecheck[0])
        #exitfile.write(bytetime + b'\n')
        print(time)
        if time > 21600000:
            racenumber = ammountofraces
        #print(racenumber)
if endless == True:
    print(time)
racefile.close()
exitfile.close()
outputfile.close()