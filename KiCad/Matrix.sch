EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector:Screw_Terminal_01x02 J5
U 1 1 60AD6606
P 7600 4650
F 0 "J5" H 7680 4642 50  0000 L CNN
F 1 "Screw_Terminal_01x02" H 7680 4551 50  0000 L CNN
F 2 "TerminalBlock_RND:TerminalBlock_RND_205-00012_1x02_P5.00mm_Horizontal" H 7600 4650 50  0001 C CNN
F 3 "~" H 7600 4650 50  0001 C CNN
	1    7600 4650
	1    0    0    -1  
$EndComp
$Comp
L Connector:Screw_Terminal_01x02 J3
U 1 1 60AD7602
P 5450 4750
F 0 "J3" H 5530 4742 50  0000 L CNN
F 1 "Screw_Terminal_01x02" H 5530 4651 50  0000 L CNN
F 2 "TerminalBlock_RND:TerminalBlock_RND_205-00012_1x02_P5.00mm_Horizontal" H 5450 4750 50  0001 C CNN
F 3 "~" H 5450 4750 50  0001 C CNN
	1    5450 4750
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x06_Male J1
U 1 1 60AEFAD5
P 4850 2950
F 0 "J1" H 4958 3331 50  0000 C CNN
F 1 "Conn_01x06_Male" H 4958 3240 50  0000 C CNN
F 2 "Connector_JST:JST_EH_S6B-EH_1x06_P2.50mm_Horizontal" H 4850 2950 50  0001 C CNN
F 3 "~" H 4850 2950 50  0001 C CNN
	1    4850 2950
	1    0    0    -1  
$EndComp
Wire Wire Line
	7400 4750 6600 4750
Wire Wire Line
	5700 3850 5700 4650
Wire Wire Line
	5650 4650 5700 4650
Connection ~ 5700 4650
Wire Wire Line
	5700 4650 6600 4650
Wire Wire Line
	5700 3750 5600 3750
Wire Wire Line
	5600 3750 5600 4750
Wire Wire Line
	5600 4750 5650 4750
Connection ~ 5650 4750
Wire Wire Line
	5050 2750 5700 2750
Wire Wire Line
	5050 2850 5250 2850
Wire Wire Line
	5250 2850 5250 3050
Wire Wire Line
	5050 2950 5700 2950
Wire Wire Line
	5050 3050 5250 3050
Connection ~ 5250 3050
Wire Wire Line
	5250 3050 5250 3750
Wire Wire Line
	5050 3150 5350 3150
Wire Wire Line
	5350 3150 5350 2350
Wire Wire Line
	5350 2350 5700 2350
Wire Wire Line
	5050 3250 5450 3250
Wire Wire Line
	5450 3250 5450 2850
Wire Wire Line
	5450 2850 5700 2850
Wire Wire Line
	5250 3850 5700 3850
$Comp
L power:+5V #PWR02
U 1 1 60B2838B
P 6600 4750
F 0 "#PWR02" H 6600 4600 50  0001 C CNN
F 1 "+5V" H 6615 4923 50  0000 C CNN
F 2 "" H 6600 4750 50  0001 C CNN
F 3 "" H 6600 4750 50  0001 C CNN
	1    6600 4750
	-1   0    0    1   
$EndComp
Connection ~ 6600 4750
Wire Wire Line
	6600 4750 5650 4750
$Comp
L power:GND #PWR01
U 1 1 60B293BE
P 6600 4650
F 0 "#PWR01" H 6600 4400 50  0001 C CNN
F 1 "GND" H 6605 4477 50  0000 C CNN
F 2 "" H 6600 4650 50  0001 C CNN
F 3 "" H 6600 4650 50  0001 C CNN
	1    6600 4650
	-1   0    0    1   
$EndComp
Connection ~ 6600 4650
Wire Wire Line
	6600 4650 7350 4650
Wire Wire Line
	7200 4050 7350 4050
Wire Wire Line
	7350 4050 7350 4650
Connection ~ 7350 4650
Wire Wire Line
	7350 4650 7400 4650
Wire Wire Line
	7200 2350 7350 2350
Wire Wire Line
	7350 2350 7350 2950
Connection ~ 7350 4050
Wire Wire Line
	7200 2950 7350 2950
Connection ~ 7350 2950
Wire Wire Line
	7350 2950 7350 3050
Wire Wire Line
	7200 3050 7350 3050
Connection ~ 7350 3050
Wire Wire Line
	7350 3050 7350 4050
$Comp
L Connector:Conn_01x04_Male J2
U 1 1 60B0490B
P 4850 3650
F 0 "J2" H 4822 3532 50  0000 R CNN
F 1 "Conn_01x04_Male" H 4822 3623 50  0000 R CNN
F 2 "Connector_JST:JST_EH_S4B-EH_1x04_P2.50mm_Horizontal" H 4850 3650 50  0001 C CNN
F 3 "~" H 4850 3650 50  0001 C CNN
	1    4850 3650
	1    0    0    1   
$EndComp
Wire Wire Line
	5050 3450 5700 3450
Wire Wire Line
	5050 3550 5700 3550
Wire Wire Line
	5050 3650 5700 3650
Wire Wire Line
	5050 3750 5250 3750
Connection ~ 5250 3750
Wire Wire Line
	5250 3750 5250 3850
Connection ~ 5700 3850
$Comp
L symbols:Lolin32 U1
U 1 1 60AEB9DC
P 6200 3300
F 0 "U1" H 6450 4497 60  0000 C CNN
F 1 "Lolin32" H 6450 4391 60  0000 C CNN
F 2 "footprints:Lolin32" H 6050 3150 60  0001 C CNN
F 3 "" H 6050 3150 60  0001 C CNN
	1    6200 3300
	1    0    0    -1  
$EndComp
Wire Wire Line
	5550 3250 5700 3250
Wire Wire Line
	6300 4250 7200 4250
Wire Wire Line
	8100 2750 7200 2750
Wire Wire Line
	8200 3150 7200 3150
Wire Wire Line
	8300 3450 7200 3450
Wire Wire Line
	8700 3650 7200 3650
Wire Wire Line
	8600 3750 7200 3750
Wire Wire Line
	8500 3850 7200 3850
Wire Wire Line
	8400 4150 7200 4150
Wire Wire Line
	6800 950  6850 950 
Wire Wire Line
	8100 1050 8100 2750
Wire Wire Line
	8200 1150 8200 3150
Wire Wire Line
	8300 1250 8300 3450
Wire Wire Line
	8700 1350 8700 3650
Wire Wire Line
	8600 1450 8600 3750
Wire Wire Line
	8500 1550 8500 3850
Wire Wire Line
	8400 1650 8400 4150
Wire Wire Line
	6850 950  6850 3150
Wire Wire Line
	6200 2850 7200 2850
Wire Wire Line
	6100 3350 5700 3350
Wire Wire Line
	6150 3350 7200 3350
Wire Wire Line
	6850 3150 5700 3150
Connection ~ 5250 2850
Wire Wire Line
	5550 1050 5550 3250
Wire Wire Line
	6300 1650 6300 4250
Wire Wire Line
	5250 1550 5250 2850
Connection ~ 5250 1550
Wire Wire Line
	6300 1550 5250 1550
Wire Wire Line
	6300 1450 6100 1450
Wire Wire Line
	6150 1250 6150 3350
Wire Wire Line
	6100 1450 6100 3350
Wire Wire Line
	5250 1350 5250 1550
Wire Wire Line
	5250 950  5250 1350
Connection ~ 5250 1350
Wire Wire Line
	6300 1350 5250 1350
Wire Wire Line
	6200 1150 6200 2850
Wire Wire Line
	6300 1250 6150 1250
Wire Wire Line
	6300 1150 6200 1150
Wire Wire Line
	6300 1050 5550 1050
Wire Wire Line
	6300 950  5250 950 
Text Label 6950 1650 0    50   ~ 0
R1
Text Label 6950 1550 0    50   ~ 0
B1
Text Label 6950 1450 0    50   ~ 0
R2
Text Label 6950 1350 0    50   ~ 0
B2
Text Label 6950 1250 0    50   ~ 0
A
Text Label 6950 1150 0    50   ~ 0
C
Text Label 6950 1050 0    50   ~ 0
CLK
Text Label 6850 950  0    50   ~ 0
OE
Text Label 6300 1800 0    50   ~ 0
G1
Text Label 6250 1450 0    50   ~ 0
G2
Text Label 6250 1250 0    50   ~ 0
B
Text Label 6250 1150 0    50   ~ 0
D
Text Label 6200 1050 0    50   ~ 0
LAT
Wire Wire Line
	6800 1350 8700 1350
Wire Wire Line
	6800 1450 8600 1450
Wire Wire Line
	6800 1550 8500 1550
Wire Wire Line
	6800 1650 8400 1650
Wire Wire Line
	6800 1050 8100 1050
Wire Wire Line
	6800 1150 8200 1150
Wire Wire Line
	6800 1250 8300 1250
$Comp
L Connector_Generic:Conn_02x08_Odd_Even J4
U 1 1 60B1EF7F
P 6600 1350
F 0 "J4" H 6650 725 50  0000 C CNN
F 1 "Conn_02x08_Odd_Even" H 6650 816 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x08_P2.54mm_Horizontal" H 6600 1350 50  0001 C CNN
F 3 "~" H 6600 1350 50  0001 C CNN
	1    6600 1350
	-1   0    0    1   
$EndComp
$Comp
L Mechanical:MountingHole H2
U 1 1 60B2845C
P 8950 1300
F 0 "H2" H 9050 1346 50  0000 L CNN
F 1 "MountingHole" H 9050 1255 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3_DIN965_Pad" H 8950 1300 50  0001 C CNN
F 3 "~" H 8950 1300 50  0001 C CNN
	1    8950 1300
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H4
U 1 1 60B28B3F
P 8950 1700
F 0 "H4" H 9050 1746 50  0000 L CNN
F 1 "MountingHole" H 9050 1655 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3_DIN965_Pad" H 8950 1700 50  0001 C CNN
F 3 "~" H 8950 1700 50  0001 C CNN
	1    8950 1700
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H1
U 1 1 60B29268
P 8950 1100
F 0 "H1" H 9050 1146 50  0000 L CNN
F 1 "MountingHole" H 9050 1055 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3_DIN965_Pad" H 8950 1100 50  0001 C CNN
F 3 "~" H 8950 1100 50  0001 C CNN
	1    8950 1100
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H3
U 1 1 60B29581
P 8950 1500
F 0 "H3" H 9050 1546 50  0000 L CNN
F 1 "MountingHole" H 9050 1455 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3_DIN965_Pad" H 8950 1500 50  0001 C CNN
F 3 "~" H 8950 1500 50  0001 C CNN
	1    8950 1500
	1    0    0    -1  
$EndComp
Text Label 5050 3450 0    50   ~ 0
E1
Text Label 5050 3550 0    50   ~ 0
E2
Text Label 5050 3650 0    50   ~ 0
BTN
Text Label 5050 2750 0    50   ~ 0
SCK
Text Label 5050 2950 0    50   ~ 0
SD
Text Label 5050 3150 0    50   ~ 0
VDD
Text Label 5050 3250 0    50   ~ 0
WS
$Comp
L Mechanical:MountingHole H5
U 1 1 60B5A8FD
P 8950 2050
F 0 "H5" H 9050 2096 50  0000 L CNN
F 1 "OH logo" H 9050 2005 50  0000 L CNN
F 2 "Symbol:OSHW-Logo2_7.3x6mm_SilkScreen" H 8950 2050 50  0001 C CNN
F 3 "~" H 8950 2050 50  0001 C CNN
	1    8950 2050
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H6
U 1 1 60B5ADEA
P 8950 2250
F 0 "H6" H 9050 2296 50  0000 L CNN
F 1 "KiCad logo" H 9050 2205 50  0000 L CNN
F 2 "Symbol:KiCad-Logo_5mm_SilkScreen" H 8950 2250 50  0001 C CNN
F 3 "~" H 8950 2250 50  0001 C CNN
	1    8950 2250
	1    0    0    -1  
$EndComp
$EndSCHEMATC
