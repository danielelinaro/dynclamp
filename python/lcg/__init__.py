"""

=====
 lcg
=====

Collection of functions used during experiments with lcg.

Authors:
        Daniele Linaro - daniele.linaro@ua.ac.be
        Joao Couto - joao@tnb.ua.ac.be

----------------------------------------------------------

"""

__all__ = ['writePulsesStimFile','writeIPlusBgGConfig', 'writeGainModulationConfig',
           'writeSinusoidsConfig', 'writeFClampConfig', 'writeSpontaneousConfig',
           'writePulsesStimFile', 'writefIStim','writeNoisyBackgroundConfig',
           'writeGStimFiles','writeStimFile',
           'substituteStrings',
           'computeRatesRatio', 'computeSynapticBackgroundCoefficients','computeElectrodeKernel',
           'findSpikes','loadH5Trace',
           'lcg_xml']

import common
import utils
from utils import writePulsesStimFile,writeIPlusBgGConfig, writeGainModulationConfig,writeSinusoidsConfig, writeFClampConfig, writeSpontaneousConfig,writePulsesStimFile, writefIStim,writeNoisyBackgroundConfig,writeGStimFiles,writeStimFile,computeRatesRatio,computeSynapticBackgroundCoefficients,computeElectrodeKernel,findSpikes,loadH5Trace,substituteStrings
from config_writer import XMLConfigurationFile
import entities

### the scripts
import extract_kernels
import Rin_protocol
import cv
import disynaptic_microcircuit
import extracellular_protocol
import fopt
import fclamp
import psp_opt
import pulses_protocol
import reliability_disynaptic
import sinusoids_protocol
import steps_protocol

