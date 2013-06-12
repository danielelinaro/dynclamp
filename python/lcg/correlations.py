#!/usr/bin/env python

import sys
import os
import getopt
import numpy as np
import lcg
import time
import subprocess as sub

stim_files = {'gampa': 'gampa.stim', 'gampa_common': 'gampa_common.stim',
              'ggaba': 'ggaba.stim', 'ggaba_common': 'ggaba_common.stim',
              'gnmda': 'gnmda.stim', 'gnmda_common': 'gnmda_common.stim'}

config_file = 'correlations.xml'

def usage():
    print('\nUsage: %s [--option <value>]' % os.path.basename(sys.argv[0]))
    print('\nThe options are:\n')
    print('     -h   Display this help message and exit.')
    print('     -v   Values of voltage at which the background activity should be balanced (comma-separated values).')
    print('     -c   Correlation coefficients (comma-separated values).')
    print('     -f   Firing frequency of the excitatory background population (default 7000 Hz).')
    print('     -R   Input resistance of the cell (in MOhm).')
    print('     -n   Number of repetitions (default 100).')
    print('     -i   Interval between trials (default 2 s).')
    print('     -d   Duration of the stimulation (default 1.1 sec).')
    print('     -I   Input channel (default 0).')
    print('     -O   Output channel (default 0).')
    print('     -F   Sampling rate (default 20000 Hz).')
    print('     -k   Frequency at which a new kernel should be computed (the default is just at the beginning).')
    print('     -b   Time before stimulus onset (default 0.1 sec).')
    print('     -a   Time after stimulus offset (default 0.3 sec).')
    print('Additionally, if the --with-nmda option is set, the following options are accepted:\n')
    print('     -m   Mean of the NMDA conductance (in nS).')
    print('     -s   Standard deviation of the NMDA conductance (in nS).')
    print('     -t   Time constant of the NMDA conductance (default 100 ms).')
    print('     -K   Coefficients for the magnesium block of the NMDA conductance (default 0.6,0.06)')
    print('')

def parseArgs():
    try:
        opts,args = getopt.getopt(sys.argv[1:],'hc:n:i:d:I:O:k:R:f:F:v:b:a:m:s:t:K:',['help','with-nmda'])
    except getopt.GetoptError, err:
        print(str(err))
        usage()
        sys.exit(1)

    options = {'reps': 100,
               'interval': 2,   # [s]
               'duration': 1.1,   # [s]
               'kernel_frequency': 0,
               'correlation_coefficients': None,
               'balanced_voltages': None,
               'input_resistance': None,
               'R_exc': 7000, # [Hz]
               'sampling_rate': 20000, # [Hz]
               'with_nmda': False,
               'nmda_mean': None, 'nmda_std': None, 'nmda_tau': 100, 'nmda_K': [0.6,0.06],
               'before': 0.1, 'after': 0.3, # [s]
               'ai': 0, 'ao': 0}

    for o,a in opts:
        if o in ('-h','--help'):
            usage()
            sys.exit(1)
        elif o == '-n':
            options['reps'] = int(a)
        elif o == '-i':
            options['interval'] = float(a)
        elif o == '-b':
            options['before'] = float(a)
        elif o == '-a':
            options['after'] = float(a)
        elif o == '-I':
            options['ai'] = int(a)
        elif o == '-O':
            options['ao'] = int(a)
        elif o == '-d':
            options['duration'] = float(a)
        elif o == '-k':
            options['kernel_frequency'] = int(a)
        elif o == '-R':
            options['input_resistance'] = float(a)
        elif o == '-F':
            options['sampling_rate'] = float(a)
        elif o == '-f':
            options['R_exc'] = float(a)
        elif o == '-v':
            options['balanced_voltages'] = []
            for v in a.split(','):
                options['balanced_voltages'].append(float(v))
        elif o == '-c':
            options['correlation_coefficients'] = []
            for c in a.split(','):
                options['correlation_coefficients'].append(float(c))
        elif o == '--with-nmda':
            options['with_nmda'] = True
        elif o == '-m':
            options['nmda_mean'] = float(a)
        elif o == '-s':
            options['nmda_std'] = float(a)
        elif o == '-t':
            options['nmda_tau'] = float(a)
        elif o == '-K':
            for i,k in enumerate(a.split(',')):
                options['nmda_K'][i] = float(k)

    if not options['input_resistance']:
        print('You must specify the input resistance of the cell (-R switch).')
        sys.exit(1)
    
    if options['input_resistance'] <= 0:
        print('The input resistance must be positive.')
        sys.exit(1)

    if options['R_exc'] <= 0:
        print('The firing frequency of the excitatory population must be positive.')
        sys.exit(1)
        
    if options['sampling_rate'] <= 0:
        print('The sampling rate must be greater than 0.')
        sys.exit(1)

    if not options['balanced_voltages']:
        print('You must specify the balanced voltage (-v switch)')
        sys.exit(1)

    if min(options['balanced_voltages']) < -80 or max(options['balanced_voltages']) > 0:
        plural = ''
        if len(options['balanced_voltages']) > 1:
            plural = 's'
        print('The balanced voltage' + plural + ' must be between -80 and 0 mV.')
        sys.exit(1)

    if not options['correlation_coefficients']:
        print('You must specify the correlation coefficient(s) (-c switch)')
        sys.exit(1)

    if min(options['correlation_coefficients']) < 0 or max(options['correlation_coefficients']) > 1:
        plural = ''
        if len(options['correlation_coefficients']) > 1:
            plural = 's'
        print('The correlation coefficient' + plural + ' must be between 0 and 1.')
        sys.exit(1)

    if options['kernel_frequency'] <= 0:
        options['kernel_frequency'] = options['reps']

    if options['with_nmda']:
        if options['nmda_mean'] == None or options['nmda_mean'] < 0:
            print('You must specify a non-negative mean for the NMDA conductance.')
            sys.exit(1)
        if options['nmda_std'] == None or options['nmda_std'] < 0:
            print('You must specify a non-negative standard deviation for the NMDA conductance.')
            sys.exit(1)
        if options['nmda_tau'] <= 0:
            print('The time constant of the NMDA conductance must be positive')
            sys.exit(1)
        if not np.all(np.array(options['nmda_K']) > 0):
            print('The K1 and K2 coefficients must be positive.')
            sys.exit(1)
        if options['nmda_mean'] == 0 and options['nmda_std'] == 0:
            print('Both mean and standard deviation of the NMDA conductance are zero, I cowardly refuse to continue.')
            sys.exit(0)

    return options

def writeConfigFile(options):
    config = lcg.XMLConfigurationFile(options['sampling_rate'],options['duration']+options['before']+options['after'])
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0), spikeThreshold=-20, V0=-65, deviceFile=os.environ['COMEDI_DEVICE'],
                                              inputSubdevice=os.environ['AI_SUBDEVICE'],
                                              outputSubdevice=os.environ['AO_SUBDEVICE'],
                                              readChannel=options['ai'], writeChannel=options['ao'],
                                              inputConversionFactor=os.environ['AI_CONVERSION_FACTOR'],
                                              outputConversionFactor=os.environ['AO_CONVERSION_FACTOR'],
                                              inputRange='[-10,+10]', reference=os.environ['GROUND_REFERENCE'],
                                              kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.Waveform(id=2, connections=(0,3), filename=stim_files['gampa'], units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=3, connections=(1), E=0.))
    config.add_entity(lcg.entities.Waveform(id=4, connections=(0,5), filename=stim_files['ggaba'], units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=5, connections=(1), E=-80.))
    config.add_entity(lcg.entities.Waveform(id=6, connections=(0,7), filename=stim_files['gampa_common'], units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=7, connections=(1), E=0.))
    config.add_entity(lcg.entities.Waveform(id=8, connections=(0,9), filename=stim_files['ggaba_common'], units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=9, connections=(1), E=-80.))
    if options['with_nmda']:
        config.add_entity(lcg.entities.Waveform(id=10, connections=(0,11), filename=stim_files['gnmda'], units='nS'))
        config.add_entity(lcg.entities.NMDAConductanceStimulus(id=11, connections=(1),
                                                               E=0., K1=options['nmda_K'][0], K2=options['nmda_K'][1]))
        config.add_entity(lcg.entities.Waveform(id=12, connections=(0,13), filename=stim_files['gnmda_common'], units='nS'))
        config.add_entity(lcg.entities.NMDAConductanceStimulus(id=13, connections=(1),
                                                               E=0., K1=options['nmda_K'][0], K2=options['nmda_K'][1]))
    config.write(config_file)

def main():
    opts = parseArgs()
    print opts
    writeConfigFile(opts)

    cnt = 0
    tot = len(opts['balanced_voltages']) * len(opts['correlation_coefficients']) * opts['reps']
    gampa = {'m': 0, 's': 0, 'mc': 0, 'sc': 0}
    ggaba = {'m': 0, 's': 0, 'mc': 0, 'sc': 0}

    stim = [[opts['before'], 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
            [opts['duration'], 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1],
            [opts['after'], 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]]

    np.random.seed(5061983)
    ampa_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))
    np.random.seed(7051983)
    gaba_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))
    if opts['with_nmda']:
        np.random.seed(723587)
        nmda_seeds = map(int, np.random.uniform(low=0, high=10000, size=opts['reps']))

    np.random.seed(int(time.time()))

    np.random.shuffle(opts['balanced_voltages'])
    np.random.shuffle(opts['correlation_coefficients'])

    for v in opts['balanced_voltages']:
        ratio = lcg.computeRatesRatio(Vm=v, Rin=opts['input_resistance'])
        for c in opts['correlation_coefficients']:
            gampa['m'],ggaba['m'],gampa['s'],ggaba['s'] = lcg.computeSynapticBackgroundCoefficients(
                ratio,R_exc=(1-c)*opts['R_exc'],Rin=opts['input_resistance'])
            gampa['mc'],ggaba['mc'],gampa['sc'],ggaba['sc'] = lcg.computeSynapticBackgroundCoefficients(
                ratio,R_exc=c*opts['R_exc'],Rin=opts['input_resistance'])
            for k in range(opts['reps']):
                stim[1][2] = gampa['m']
                stim[1][3] = gampa['s']
                stim[1][4] = 5
                stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                lcg.writeStimFile(stim_files['gampa'], stim, False)
                stim[1][2] = ggaba['m']
                stim[1][3] = ggaba['s']
                stim[1][4] = 10
                stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                lcg.writeStimFile(stim_files['ggaba'], stim, False)
                stim[1][2] = gampa['mc']
                stim[1][3] = gampa['sc']
                stim[1][4] = 5
                stim[1][8] = ampa_seeds[k]
                lcg.writeStimFile(stim_files['gampa_common'], stim, False)
                stim[1][2] = ggaba['mc']
                stim[1][3] = ggaba['sc']
                stim[1][4] = 10
                stim[1][8] = gaba_seeds[k]
                lcg.writeStimFile(stim_files['ggaba_common'], stim, False)
                if opts['with_nmda']:
                    stim[1][2] = (1-c)*opts['nmda_mean']
                    stim[1][3] = (1-c)*opts['nmda_std']
                    stim[1][4] = opts['nmda_tau']
                    stim[1][8] = int(np.random.uniform(low=0, high=100*opts['reps']))
                    lcg.writeStimFile(stim_files['gnmda'], stim, False)
                    stim[1][2] = c*opts['nmda_mean']
                    stim[1][3] = c*opts['nmda_std']
                    stim[1][4] = opts['nmda_tau']
                    stim[1][8] = nmda_seeds[k]
                    lcg.writeStimFile(stim_files['gnmda_common'], stim, False)
                if cnt%opts['kernel_frequency'] == 0:
                    sub.call('lcg kernel -I ' + str(opts['ai']) + ' -O ' + str(opts['ao']), shell=True)
                cnt = cnt+1
                sub.call(lcg.common.prog_name + ' -V 4 -c ' + config_file, shell=True)
                sub.call(['sleep', str(opts['interval'])])
                if cnt%10 == 0:
                    print('[%02d/%02d]' % (cnt,tot))

if __name__ == '__main__':
    main()

