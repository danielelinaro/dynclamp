import time
import os
import numpy as np
import tables as tbl
import lcg
import subprocess as sub
import sys
from datetime import date

########## Functions that write configuration files ##########

def conductanceBurstStim(dur, g, tau, R0, dR, Tb, taub):
    Tb.append(dur)
    tau_sec = tau*1e-3
    stim = [[Tb[0], 2, g*tau*R0, g*(R0*tau/2)**0.5, tau, 0, 0, 1, int(np.random.uniform(high=10000)), 0, 0, 1]]
    for i in range(1,len(Tb)):
        stim.append([Tb[i]-Tb[i-1], -4, 0, 1, tau, 0, 0, 1, int(np.random.uniform(high=50000)), 2, 0, 1])
        stim.append([0, -4, dR, 1, taub, 0, R0, 0, 0, 12, 2, 0.5])
        stim.append([0, -4, g*(tau_sec/2)**0.5, 0, 0, 0, 0, 0, 0, 1, 2, 1])
        stim.append([0, -4, g*tau_sec*dR, 1, taub, 0, g*tau_sec*R0, 0, 0, 12, 1, 1])
    return stim

def writePulsesStimFile(f, dur, amp, N=10, delay=1, pulsesInBurst=1, withRecovery=True, filename='pulses.stim'):
    """
    Writes a stimulation file containing a series of pulses, with an optional "recovery" pulse.
    
    Parameters:
                 f - frequency of the stimulation
               dur - duration of the stimulation (!!! in ms !!!)
               amp - amplitude of the stimulation
                 N - number of repetitions
             delay - initial and final delay
     pulsesInBurst - number of pulses in each group
      withRecovery - whether or not to include a recovery pulse
          filename - file to write to

    Returns:
       The duration of the stimulation.

    """
    stimulus = [[delay,1,0,0,0,0,0,0,5061983,0,0,1]]
    if type(f) != list:
        f = [f]
    if len(f) == 1:
        f = f*2
    stimulus.append([N/f[1],-2,amp,-f[0],dur,0,0,0,5061983,8,0,1])
    stimulus.append([0,-2,1,-f[1],pulsesInBurst*(1000./f[0]),0,0,0,5061983,8,2,1])
    if withRecovery:
        stimulus.append([0.5,1,0,0,0,0,0,0,5061983,0,0,1])
        stimulus.append([dur,1,amp,0,0,0,0,0,5061983,0,0,1])
    stimulus.append([delay,1,0,0,0,0,0,0,5061983,0,0,1])
    dur = lcg.writeStimFile(filename, stimulus, None)
    return dur

def writeGStimFiles(Gexc, Ginh, duration, before, after, outfiles = ['gexc.stim','ginh.stim']):
    G = [[[before,1,0,0,0,0,0,0,0,0,0,1],
          [duration,2,Gexc['m'],Gexc['s'],Gexc['tau'],0,0,1,np.random.poisson(10000),0,0,1],
          [after,1,0,0,0,0,0,0,0,0,0,1]],
         [[before,1,0,0,0,0,0,0,0,0,0,1],
          [duration,2,Ginh['m'],Ginh['s'],Ginh['tau'],0,0,1,np.random.poisson(10000),0,0,1],
          [after,1,0,0,0,0,0,0,0,0,0,1]]]
    if 'seed' in Gexc:
        G[0][1][8] = Gexc['seed']
    if 'seed' in Ginh:
        G[1][1][8] = Ginh['seed']
    for i,filename in enumerate(outfiles):
        lcg.writeStimFile(filename,G[i],None)

def writeIPlusBgGConfig(I, Gexc, Ginh, ai, ao, duration, sampling_rate, outfile):
    config = lcg.XMLConfigurationFile(sampling_rate, duration+3.61)
    config.add_entity(lcg.entities.H5Recorder(id=0, connections=(), compress=True))
    config.add_entity(lcg.entities.RealNeuron(id=1, connections=(0), spikeThreshold=-20, V0=-65, deviceFile=os.environ['COMEDI_DEVICE'],
                                              inputSubdevice=os.environ['AI_SUBDEVICE'],
                                              outputSubdevice=os.environ['AO_SUBDEVICE'],
                                              readChannel=ai, writeChannel=ao,
                                              inputConversionFactor=os.environ['AI_CONVERSION_FACTOR_CC'],
                                              outputConversionFactor=os.environ['AO_CONVERSION_FACTOR_CC'],
                                              inputRange=os.environ['RANGE'], reference=os.environ['GROUND_REFERENCE'],
                                              kernelFile='kernel.dat'))
    config.add_entity(lcg.entities.Waveform(id=2, connections=(0,1), filename='current.stim', units='pA'))
    config.add_entity(lcg.entities.Waveform(id=3, connections=(0,5), filename='gexc.stim', units='nS'))
    config.add_entity(lcg.entities.Waveform(id=4, connections=(0,6), filename='ginh.stim', units='nS'))
    config.add_entity(lcg.entities.ConductanceStimulus(id=5, connections=(1), E=0.))
    config.add_entity(lcg.entities.ConductanceStimulus(id=6, connections=(1), E=-80.))
    config.write(outfile)

    current = [[0.5,1,0,0,0,0,0,0,0,0,0,1],
               [0.01,1,-300,0,0,0,0,0,0,0,0,1],
               [0.5,1,0,0,0,0,0,0,0,0,0,1],
               [0.6,1,-100,0,0,0,0,0,0,0,0,1],
               [1,1,0,0,0,0,0,0,0,0,0,1],
               [duration,1,I,0,0,0,0,0,0,0,0,1],
               [1,1,0,0,0,0,0,0,0,0,0,1]]
    conductance = [[2.61,1,0,0,0,0,0,0,0,0,0,1],
                   [duration,2,1,1,1,0,0,1,0,0,0,1],
                   [1,1,0,0,0,0,0,0,0,0,0,1]]

    lcg.writeStimFile('current.stim',current,None)

    conductance[1][2] = Gexc['m']
    conductance[1][3] = Gexc['s']
    conductance[1][4] = Gexc['tau']
    if 'seed' in Gexc:
        conductance[1][8] = Gexc['seed']
    else:
        conductance[1][8] = np.random.poisson(10000)
    lcg.writeStimFile('gexc.stim',conductance,None)

    conductance[1][2] = Ginh['m']
    conductance[1][3] = Ginh['s']
    conductance[1][4] = Ginh['tau']
    if 'seed' in Ginh:
        conductance[1][8] = Ginh['seed']
    else:
        conductance[1][8] = np.random.poisson(10000)
    lcg.writeStimFile('ginh.stim',conductance,None)

def writeSpontaneousConfig(I, G0_exc, sigma_exc, G0_inh, sigma_inh, ai=0, ao=0, duration=10, sampling_rate=20000, outfile='spontaneous.xml'):
    writeIPlusBgGConfig(I, {'m': G0_exc, 's': sigma_exc, 'tau': 5},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10},
                        ai, ao, duration, sampling_rate, outfile)

def writeSinusoidsConfig(bg_current, modulating_current, G0_exc, sigma_exc, G0_inh, sigma_inh,
                         ai=0, ao=0, duration=30, outfile='sinusoids.xml', infile=''):
    import shutil
    writeIPlusBgGConfig(0, {'m': G0_exc, 's': sigma_exc, 'tau': 5, 'seed': 5061983},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10, 'seed': 7051983},
                        ai, ao, duration, outfile, infile)
    if isinstance(bg_current, float) or isinstance(bg_current,int):
        if bg_current == 0:
            # we have only the sinusoidal modulating current
            current = [[duration,3,modulating_current,'F',0,0,0,0,0,0,0,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
        else:
            # sinusoid on top of a DC current
            current = [[duration,-2,bg_current,0,0,0,0,0,0,1,0,1],
                       [0,-2,modulating_current,'F',0,0,0,0,0,3,1,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
    else:
        if bg_current['mean'] == 0 and bg_current['std'] == 0:
            # we have only the sinusoidal modulating current
            current = [[duration,3,modulating_current,'F',0,0,0,0,0,0,0,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
        else:
            # sinusoid on top of a noisy (OU) current
            current = [[duration,-2,bg_current['mean'],bg_current['std'],bg_current['tau'],0,0,1,5061983,2,0,1],
                       [0,-2,modulating_current,'F',0,0,0,0,0,3,1,1],
                       [1,1,0,0,0,0,0,0,0,0,0,1]]
    os.remove('current.stim')
    shutil.move('gexc.stim','gexc_template.stim')
    shutil.move('ginh.stim','ginh_template.stim')
    lcg.writeStimFile('current_template.stim',current,True)

def writeGainModulationConfig(G0_exc, sigma_exc, G0_inh, sigma_inh, ai=0, ao=0, duration=5, outfile='gain_modulation.xml', infile=''):
    writeIPlusBgGConfig(0, {'m': G0_exc, 's': sigma_exc, 'tau': 5},
                        {'m': G0_inh, 's': sigma_inh, 'tau': 10},
                        ai, ao, duration+3, outfile, infile)
    os.remove('current.stim')
    current = [[0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.01,1,-300,0,0,0,0,0,3532765,0,0,1],
               [0.5,1,0,0,0,0,0,0,3532765,0,0,1],
               [0.6,1,-100,0,0,0,0,0,3532765,0,0,1],
               [4,1,0,0,0,0,0,0,3532765,0,0,1],
               [duration,1,'I',0,0,0,0,0,3532765,0,0,1],
               [1,1,0,0,0,0,0,0,3532765,0,0,1]]
    lcg.writeStimFile('template.stim',current,None)

def writefIStim(Imin,Imax,Istep,noisy=False):
    A = [[0.5,1,0,0,0,0,0,0,3532765,0,0,1],
         [0.01,1,-300,0,0,0,0,0,3532765,0,0,1],
         [0.5,1,0,0,0,0,0,0,3532765,0,0,1],
         [0.6,1,-100,0,0,0,0,0,3532765,0,0,1],
         [1,1,0,0,0,0,0,0,3532765,0,0,1],
         [5,1,500,0,0,0,0,0,3532765,0,0,1],
         [0.1,1,0,0,0,0,0,0,3532765,0,0,1]]
    if noisy:
        A[4,0] = 4
    for k,i in np.arange(Imin,Imax+Istep,Istep):
        A[5,2] = i
        lcg.writeStimFile('fi_%02d.stim' % (k+1), A, None)

def writeNoisyBackgroundConfig(filename='ou.xml', Rm=0, Vm=-57.6, R_exc=7000, tau_exc=5, tau_inh=10, E_exc=0, E_inh=-80):
    ratio = computeRatesRatio(Vm, Rin=Rm, tau_exc=tau_exc, tau_inh=tau_inh, E_exc=E_exc, E_inh=E_inh)
    Gm_exc,Gm_inh,Gs_exc,Gs_inh = computeSynapticBackgroundCoefficients(ratio, R_exc, Rin=Rm, tau_exc=tau_exc, tau_inh=tau_inh)
    fid = open(filename,'w')
    fid.write('<lcg>\n<entities>\n');
    fid.write('<ou>\n');
    fid.write('<G0>%g</G0>\n' % Gm_exc);
    fid.write('<sigma>%g</sigma>\n' % Gs_exc);
    fid.write('<tau>%g</tau>\n' % (tau_exc*1e-3));
    fid.write('<E>%g</E>\n' % E_exc);
    fid.write('</ou>\n');
    fid.write('<ou>\n');
    fid.write('<G0>%g</G0>\n' % Gm_inh);
    fid.write('<sigma>%g</sigma>\n' % Gs_inh);
    fid.write('<tau>%g</tau>\n' % (tau_inh*1e-3));
    fid.write('<E>%g</E>\n' % E_inh);
    fid.write('</ou>\n');
    fid.write('</entities>\n</lcg>\n');
    fid.close()

########## Utility functions ##########

def makeOutputFilename(prefix='', extension='.out'):
    filename = prefix
    if prefix != '' and prefix[-1] != '_':
        filename = filename + '_'
    now = time.localtime(time.time())
    filename = filename + '%d%02d%02d-%02d%02d%02d' % \
        (now.tm_year, now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec)
    if extension[0] != '.':
        extension = '.' + extension
    suffix = ''
    k = 0
    while os.path.exists(filename + suffix + extension):
        k = k+1
        suffix = '_%d' % k
    return filename + suffix + extension



def makeIncrementingFolders(folderName = '', folderPattern = 'YYYYMMDD[A]01',dryRun=False):
    ''' Create a directory (incremented following a pattern).
    Note there is no check that folderName corresponds to the folderPattern.
    Pattern supports the YYYY MM and DD keywords. Specify the incrementing part of 
the string by placing it between square brackets.
    '''
    if len(folderName):
        tmpfolder = folderName
    else:
        # Are there dates in the pattern?
        today = date.today()
        tmpfolder = folderPattern
        if 'YYYY' in tmpfolder:
            tmpfolder = folderPattern.replace(
                'YYYY', '{0:04d}'.format(today.year))
        if 'MM' in tmpfolder:
            tmpfolder = tmpfolder.replace(
                'MM', '{0:02d}'.format(today.month))
        if 'DD' in tmpfolder:
            tmpfolder = tmpfolder.replace(
                'DD', '{0:02d}'.format(today.day))
        if '[' in tmpfolder:
            tmpfolder = tmpfolder.replace('[','')
            tmpfolder = tmpfolder.replace(']','')
        else:
            print('Increments must be denoted by square brackets.')
            print(' There must be one increment in the pattern.')
    # Are there increments? Increments are specified between 
        # square brackets. Only one increment is allowed.
    if folderPattern.count('[') > 0:
        if folderPattern.count('[') > 1:
            print('''Only one increment [] is allowed in the experiment 
name pattern: {0}'''.format(folder))
        else:
            idx1 = folderPattern.index('[')
            idx2 = folderPattern.index(']')
    while os.path.isdir(tmpfolder):
        tmp = incrementLetterOrInteger(tmpfolder[idx1:idx2-1])
        tmpfolder = list(tmpfolder)
        tmpfolder[idx1:idx2-1] = tmp
        tmpfolder = ''.join(tmpfolder)
    if not dryRun: 
        os.makedirs(os.path.abspath(tmpfolder))
    return tmpfolder


def incrementLetterOrInteger(string):
    try:
        # Try first to increment number
        number = int(string) + 1
        returnstr = '{0:0'+str(len(string))+'d}'
        if number < 10**len(string):
            string = returnstr.format(number)
        else:
            returnstr = '{0:0d}'
            string = returnstr.format(number)
    except ValueError:
        # Then it must be a letter...
        for ii in reversed(range(len(string))):
            returnstr = chr(ord(string[ii])+1)
            if returnstr < chr(ord('Z')+1):
                tmp = list(string)
                tmp[ii] = returnstr
                string = ''.join(tmp)
                break
            elif ii == 0:
                print('Can not increment letter (< Z). Try adding another (AAA) for instance.')
                raise
    return string
            
def computeRatesRatio(Vm=-57.6, g0_exc=50, g0_inh=190, Rin=0, tau_exc=5, tau_inh=10, E_exc=0, E_inh=-80):
    """
    Parameters:
    Vm - membrane potential (mV), at which the mean injected current is 0.
    g0_exc - single synaptic excitatory conductance (pS).
    g0_inh - single synaptic inhibitory conductance (pS).
    Rin - input resistance of the cell(MOhm). If this value is not 0, g0_exc and
         g0_inh are ignored and the values 0.02/Rin and 0.06/Rin
         are used, respectively.
    tau_exc - time constant of excitatory inputs (msec).
    tau_inh - time constant of inhibitory inputs (msec).
    E_exc - reversal potential of excitatory inputs (mV).
    E_inh - reversal potential of inhibitory inputs (mV).
    """ 
    Vm = Vm * 1e-3         # (V)
    Rin = Rin * 1e6          # (Ohm)
    tau_exc = tau_exc*1e-3 # (s)
    tau_inh = tau_inh*1e-3 # (s)
    E_exc = E_exc * 1e-3   # (V)
    E_inh = E_inh * 1e-3   # (V)

    g_exc = g0_exc*1e-12   # (S)
    g_inh = g0_inh*1e-12   # (S)

    if Rin != 0.0:
        g_exc = 0.02 / Rin  # (S)
        g_inh = 0.06 / Rin  # (S)
        
    return (g_inh * tau_inh * (E_inh - Vm)) / (g_exc * tau_exc * (Vm - E_exc))

def computeSynapticBackgroundCoefficients(ratio, R_exc=7000, g0_exc=50, g0_inh=190, N=1, Rin=0, tau_exc=5, tau_inh=10):
    """
    Parameters:
    ratio - ratio between excitatory and inhibitory inputs.
    R_exc - rate of excitatory inputs (Hz).
    g0_exc - single synaptic excitatory conductance (pS).
    g0_inh - single synaptic inhibitory conductance (pS).
    N - number of synaptic contacts.
    Rin - input resistance of the cell(MOhm). If this value is not 0, g0_exc and
         g0_inh are ignored and the values 0.02/Rin and 0.06/Rin
         are used, respectively.
    tau_exc - time constant of excitatory inputs (msec).
    tau_inh - time constant of inhibitory inputs (msec).
    """
    Rin = Rin * 1e6              # (Ohm)
    tau_exc = tau_exc*1e-3     # (s)
    tau_inh = tau_inh*1e-3     # (s)
    if Rin == 0:
        g_exc = N*g0_exc*1e-12   # (S)
        g_inh = N*g0_inh*1e-12   # (S)
    else:
        g_exc = 0.02 / Rin      # (S)
        g_inh = 0.06 / Rin      # (S)

    R_inh = R_exc/ratio;
    G_exc = g_exc * tau_exc * R_exc;
    G_inh = g_inh * tau_inh * R_inh;
    Gm_exc = G_exc * 1e9;        # (nS)
    Gm_inh = G_inh * 1e9;        # (nS)
    D_exc = 0.5 * g_exc**2 * tau_exc**2 * R_exc;
    D_inh = 0.5 * g_inh**2 * tau_inh**2 * R_inh;
    Gs_exc = np.sqrt(D_exc / tau_exc) * 1e9;   # (nS)
    Gs_inh = np.sqrt(D_inh / tau_inh) * 1e9;   # (nS)

    return (Gm_exc,Gm_inh,Gs_exc,Gs_inh)

def computeElectrodeKernel(filename, Kdur=5e-3, interval=[], saveFile=True, fullOutput=False):
    import matplotlib.pyplot as p
    import aec
    p.ion()
    entities,info = loadH5Trace(filename)
    Ksize = int(Kdur/info['dt'])
    Kt = np.arange(0,Kdur,info['dt'])[:Ksize]
    for ntt in entities:
        if 'metadata' in ntt and ntt['units'] == 'pA':
            I = ntt['data']
            stimtimes = np.cumsum(ntt['metadata'][:,0])
            pulse = np.nonzero(ntt['metadata'][:,0] == 0.01)[0][0]
            if len(interval) != 2:
                # Find the gaussian noise
                idx = np.where(ntt['metadata'][:,1] == 11)[0][0]
                interval = stimtimes[idx-1:idx+1]
        elif (ntt['name'] == 'AnalogInput' or ntt['name'] == 'InputChannel') and ntt['units'] == 'mV':
            V = ntt['data']
    t = np.arange(len(V)) * info['dt']
    idx = np.intersect1d(np.nonzero(t >= interval[0])[0], np.nonzero(t <= interval[1])[0])
    Vn = V[idx]
    V[-Ksize:] = 0
    In = I[idx]
    K,V0 = aec.full_kernel(Vn,In,Ksize,True)

    # Kernels plot
    fig = p.figure(1,figsize=[5,5],facecolor='w',edgecolor=None)
    p.plot(Kt*1e3,K*1e3,'k.-',label='Full')
    p.xlabel('t (ms)')
    p.ylabel('R (MOhm)')
   
    while True:
        try:
            startTail = int(raw_input('Enter index of tail start (1 ms = %d samples): ' % int(1e-3/info['dt'])))
            if startTail >= len(K):
                print('The index must be smaller than %d.' % len(K))
                continue
        except:
            continue
        Ke,Km = aec.electrode_kernel(K,startTail,True)
        print('R = %g MOhm.\nV0 = %g mV.' % (np.sum(Ke*1e3),V0))

        # Print kernels and compensated traces
        p.close()
        fig = p.figure(1,figsize=[10,5],facecolor='w',edgecolor=None)
        plt = []
        plt.append(fig.add_axes([0.1,0.1,0.3,0.8]))
        
        plt[0].plot(Kt*1e3,K*1e3,'k.-',label='Full')
        plt[0].set_xlabel('t (ms)')
        plt[0].set_ylabel('R (MOhm)')
        plt.append(fig.add_axes([0.5,0.1,0.4,0.8]))
        plt[0].plot([Kt[0]*1e3,Kt[-1]*1e3],[0,0],'g')
        plt[0].plot(Kt*1e3,Km*1e3,'b',label='Membrane')
        plt[0].plot(Kt[:len(Ke)]*1e3,Ke*1e3,'r',label='Electrode')
        plt[0].set_xlabel('t (ms)')
        plt[0].set_ylabel('R (MOhm)')
        plt[0].legend(loc='best')
        # AEC offline
        ndx = np.intersect1d(np.nonzero(t > max(stimtimes[pulse]-20e-3,0))[0], 
                             np.nonzero(t < min(stimtimes[pulse]+80e-3,t[-1]))[0])
        Vc = aec.compensate(V[ndx],I[ndx],Ke)

        plt[1].plot(t[ndx]-t[ndx[0]], V[ndx], 'k', label='Recorded')
        plt[1].plot(t[ndx]-t[ndx[0]], Vc, 'r', label='Compensated')
        plt[1].set_xlabel('t (ms)')
        plt[1].set_ylabel('V (mV)')
        plt[1].legend(loc='best')
        fig.show()
        # Ask user what to do
        ok = raw_input('Ke[-1] / max(Ke) = %.2f %%. Ok? [Y/n] ' % (Ke[-1]/np.max(Ke)*100))
        if len(ok) == 0 or ok.lower() == 'y' or ok.lower() == 'yes':
            break
    p.close()

    if saveFile:
        np.savetxt(filename[:-3] + '_kernel.dat', Ke*1e9, '%.10e')
    
    if fullOutput:
        return Ke*1e3,V0,data.values()[0][idx],Vm
    else:
        return Ke*1e3   

########## Support functions #########
def runCommand(command, mode = None, *args):
    process = sub.Popen(command, shell=True)
    if mode == 'progress':
        printProgressBar(process, *args)
    if mode == 'timer':
        printTerminalTimer(process, *args)
    elif mode == 'percent_bar':
        printTerminalPercentageBar(process, *args)
    # Print errors and outputs
    out, err = process.communicate()
    if not out is None:
        sys.stdout.write(out)
        sys.stdout.flush()
    if not err is None:
        sys.stderr.write(err)
        sys.stderr.flush()
    return process
    
def printTerminalTimer(process, duration, timerString = '\rElapsed time: {0:.2f}s ',
                       refreshPeriod = 0.0005):
    '''Note: timerString must have space to format the time in seconds.'''
    times = np.arange(0,duration-refreshPeriod,refreshPeriod)
    for tt in times:
        if process.poll() is None:
            sys.stdout.write(timerString.format(tt))
            sys.stdout.flush()
            time.sleep(refreshPeriod)
    sys.stdout.write('\r'+' '*len(timerString) + '')
    sys.stdout.write(timerString.format(duration))
    sys.stdout.flush()
    return process

def printProgressBar(process, duration, string = '\r', nBins = 30):
    bins  = np.linspace(0,duration,nBins)
    if np.diff(bins[:2]) <0.01:
        nBins = 3
        bins  = np.linspace(0,duration,nBins)
    refreshTime = bins[1]-bins[0]

    for cnt in range(1,nBins ):
        if process.poll() is None:
            sys.stdout.write(string + '[' +
                             '='*(cnt-1) + '>' +
                             ' '*(nBins - cnt) + '] ')
            sys.stdout.flush()
            time.sleep(refreshTime)

    sys.stdout.write(string + '[' + '='*(nBins-1) + '>] ')
    sys.stdout.flush()

    return process

def printTerminalPercentageBar(process, count, total, 
                               percentString = '\rTrial %02d/%02d ',
                               max_steps = 50):
    ''' Note: percentString must have space to format 2 integers '''
    sys.stdout.write(percentString % (count,total) + ' [')
    percent = float(count)/total
    n_steps = int(round(percent*max_steps))

    for i in range(n_steps-1):
        sys.stdout.write('=')
    sys.stdout.write('>')
    for i in range(max_steps-n_steps):
        sys.stdout.write(' ')
    sys.stdout.write('] ')
    sys.stdout.flush()
    return process

########## Data analysis functions ##########

def findSpikes(t, V, thresh=0):
    spks = []
    for i in xrange(len(t)-1):
        if V[i] < thresh and V[i+1] >= thresh:
            spks.append(t[i])
    return np.array(spks)

########## Data loading functions ##########

def loadH5Trace(filename):
    try:
        fid = tbl.openFile(filename,mode='r')
    except:
        print('%s: not a valid H5 file.' % filename)
        fid.close()
        return None,None
    try:
        version = fid.root.Info._v_attrs.version
    except:
        if fid.root.__contains__('Metadata'):
            version = 0
        else:
            version = 1
    fid.close()
    if version == 0:
        return loadH5TraceV0(filename)
    if version == 1:
        return loadH5TraceV1(filename)
    if version == 2:
        return loadH5TraceV2(filename)
    print('Unknown H5 file version (%d).' % version)

def loadH5TraceV0(filename):
    fid = tbl.openFile(filename,mode='r')
    entities = []
    map = {}
    for k,node in enumerate(fid.root.Data):
        id = int(node.name.split('-')[1])
        map[id] = k
        entities.append({'data': node.read(), 'id': id})
    for node in fid.root.Metadata:
        id = int(node.name.split('-')[1])
        entities[map[id]]['metadata'] = node.read()
    for node in fid.root.Parameters:
        id = int(node.name.split('-')[1])
        entities[map[id]]['parameters'] = node.read()
    info = {'dt': fid.root.Misc.Simulation_properties.attrs.dt,
            'tend': fid.root.Misc.Simulation_properties.attrs.tend,
            'version': 0}
    fid.close()
    return entities,info

def loadH5TraceV1(filename):
    fid = tbl.openFile(filename,mode='r')
    entities = []
    for node in fid.root.Data:
        id = int(node.name.split('-')[1])
        entities.append({
                'id': id,
                'data': node.read()})
        for attrName in node.attrs._v_attrnames:
            if len(attrName) > 8 and attrName[:8].lower() == 'metadata':
                entities[-1]['metadata'] = node.attrs[attrName]
            else:
                entities[-1][attrName.lower()] = node.attrs[attrName]
    info = {'dt': fid.root.Misc.Simulation_properties.attrs.dt,
            'tend': fid.root.Misc.Simulation_properties.attrs.tend,
            'version': 1}
    fid.close()
    return entities,info

def loadH5TraceV2(filename):
    fid = tbl.openFile(filename,mode='r')
    try:
        if fid.root.Info._v_attrs.version != 2:
            print('Version not supported: %d.' % version)
            return
    except:
        print('Unknown version.')
        return

    entities = []
    for node in fid.root.Entities:
        entities.append({
                'id': int(node._v_name),
                'data': node.Data.read()})
        try:
            entities[-1]['metadata'] = node.Metadata.read()
        except:
            pass
        for attrName in node._v_attrs._v_attrnames:
            try:
                entities[-1][attrName.lower()] = node._v_attrs[attrName]
            except:
                entities[-1][attrName.lower()] = getattr(node._v_attrs, attrName)
    info = {}
    for a in fid.root.Info._v_attrs._g_listAttr(fid.root.Info):
        info[a] = getattr(fid.root.Info._v_attrs,a)
    # Read events
    try:
        node = fid.root.Events
        events = {'timestamp': node.Timestamp.read(),
                  'sender': node.Sender.read(),
                  'code': node.Code.read()}
        info['events'] = events
    except:
        pass
    fid.close()
    return entities,info
