COMEDI_DEVICE="/dev/comedi0"
# Choose here the appropriate amplifier for your setup
AMPLIFIER="HEKA-EPC10"
#AMPLIFIER="AXON-700B"
case $AMPLIFIER in
    "HEKA-EPC10")
        AI_CONVERSION_FACTOR_CC=100
        AO_CONVERSION_FACTOR_CC=0.001
        AI_CONVERSION_FACTOR_VC=1000
        AO_CONVERSION_FACTOR_VC=0.01
	;;
    "AXON-700B")
        AI_CONVERSION_FACTOR_CC=20
        AO_CONVERSION_FACTOR_CC=0.00025
        AI_CONVERSION_FACTOR_VC=400
        AO_CONVERSION_FACTOR_VC=0.05
	;;
    *)
        AI_CONVERSION_FACTOR_CC=100
        AO_CONVERSION_FACTOR_CC=0.001
        AI_CONVERSION_FACTOR_VC=1000
        AO_CONVERSION_FACTOR_VC=0.01
	;;
esac
RANGE="[-10,+10]"
AI_SUBDEVICE=0
AI_CHANNEL=0
AO_SUBDEVICE=1
AO_CHANNEL=0
AI_UNITS_CC="mV"
AO_UNITS_CC="pA"
AI_UNITS_VC="pA"
AO_UNITS_VC="mV"
SAMPLING_RATE=20000
GROUND_REFERENCE="GRSE"
export COMEDI_DEVICE
export AI_CONVERSION_FACTOR_CC
export AO_CONVERSION_FACTOR_CC
export AI_CONVERSION_FACTOR_VC
export AO_CONVERSION_FACTOR_VC
export RANGE
export AI_SUBDEVICE
export AI_CHANNEL
export AO_SUBDEVICE
export AO_CHANNEL
export AI_UNITS_CC
export AO_UNITS_CC
export AI_UNITS_VC
export AO_UNITS_VC
export SAMPLING_RATE
export GROUND_REFERENCE
export AMPLIFIER
