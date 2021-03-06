#include <stdio.h>
#include <sys/stat.h>
#include "neurons.h"
#include "events.h"
#include "thread_safe_queue.h"
#include "utils.h"

lcg::Entity* LIFNeuronFactory(string_dict& args)
{
        uint id;
        double C, tau, tarp, Er, E0, Vth, Iext;
	bool holdLastValue;
        std::string filename;

        id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractDouble(args, "C", &C) ||
             ! lcg::CheckAndExtractDouble(args, "tau", &tau) ||
             ! lcg::CheckAndExtractDouble(args, "tarp", &tarp) ||
             ! lcg::CheckAndExtractDouble(args, "Er", &Er) ||
             ! lcg::CheckAndExtractDouble(args, "E0", &E0) ||
             ! lcg::CheckAndExtractDouble(args, "Vth", &Vth) ||
             ! lcg::CheckAndExtractDouble(args, "Iext", &Iext)) {
                lcg::Logger(lcg::Critical, "Unable to build a LIF neuron.\n");
                return NULL;
        }
	
        if (! lcg::CheckAndExtractBool(args, "holdLastValue", &holdLastValue)) 
                holdLastValue = false;
	else {
        	if (!lcg::CheckAndExtractValue(args, "holdLastValueFilename", filename))
                	filename = LOGFILE;
	}
        return new lcg::neurons::LIFNeuron(C, tau, tarp, Er, E0, Vth, Iext, holdLastValue,filename, id);
}

lcg::Entity* IzhikevichNeuronFactory(string_dict& args)
{
        uint id;
        double a, b, c, d, Vspk, Iext;

        id = lcg::GetIdFromDictionary(args);

		if (! lcg::CheckAndExtractDouble(args, "a", &a))
                a = 0.02;
		if (! lcg::CheckAndExtractDouble(args, "b", &b))
                b = 0.2;
		if (! lcg::CheckAndExtractDouble(args, "c", &c))
                c = -65;
		if (! lcg::CheckAndExtractDouble(args, "d", &d))
                Vspk = 2;
		if (! lcg::CheckAndExtractDouble(args, "Vspk", &Vspk))
                Vspk = 30;
		if (! lcg::CheckAndExtractDouble(args, "Iext", &Iext))
                Iext = 0;

        return new lcg::neurons::IzhikevichNeuron(a, b, c, d, Vspk, Iext, id);
}

lcg::Entity* ConductanceBasedNeuronFactory(string_dict& args)
{
        uint id;
        double C, gl, El, Iext, area, spikeThreshold, V0;
        id = lcg::GetIdFromDictionary(args);
        if ( ! lcg::CheckAndExtractDouble(args, "C", &C) ||
             ! lcg::CheckAndExtractDouble(args, "gl", &gl) ||
             ! lcg::CheckAndExtractDouble(args, "El", &El) ||
             ! lcg::CheckAndExtractDouble(args, "Iext", &Iext) ||
             ! lcg::CheckAndExtractDouble(args, "area", &area) ||
             ! lcg::CheckAndExtractDouble(args, "spikeThreshold", &spikeThreshold) ||
             ! lcg::CheckAndExtractDouble(args, "V0", &V0)) {
                lcg::Logger(lcg::Critical, "Unable to build a conductance based neuron.\n");
                return NULL;
        }

        return new lcg::neurons::ConductanceBasedNeuron(C, gl, El, Iext, area, spikeThreshold, V0, id);
}

#ifdef HAVE_LIBCOMEDI

lcg::Entity* RealNeuronFactory(string_dict& args)
{
        uint inputSubdevice, outputSubdevice, readChannel, writeChannel, inputRange, reference, id;
        std::string kernelFile, deviceFile, inputRangeStr, referenceStr;
        double inputConversionFactor, outputConversionFactor, spikeThreshold, V0;
        bool holdLastValue, adaptiveThreshold;
        std::string filename;
        
	id = lcg::GetIdFromDictionary(args);

        if ( ! lcg::CheckAndExtractValue(args, "deviceFile", deviceFile) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "inputSubdevice", &inputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "outputSubdevice", &outputSubdevice) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "readChannel", &readChannel) ||
             ! lcg::CheckAndExtractUnsignedInteger(args, "writeChannel", &writeChannel) ||
             ! lcg::CheckAndExtractDouble(args, "inputConversionFactor", &inputConversionFactor) ||
             ! lcg::CheckAndExtractDouble(args, "outputConversionFactor", &outputConversionFactor) ||
             ! lcg::CheckAndExtractDouble(args, "spikeThreshold", &spikeThreshold) ||
             ! lcg::CheckAndExtractDouble(args, "V0", &V0)) {
                lcg::Logger(lcg::Critical, "Unable to build a real neuron.\n");
                return NULL;
        }


        if (! lcg::CheckAndExtractValue(args, "kernelFile", kernelFile))
                kernelFile = "";

        if (! lcg::CheckAndExtractValue(args, "inputRange", inputRangeStr)) {
                inputRange = PLUS_MINUS_TEN;
        }
        else {
                if (inputRangeStr.compare("PlusMinusTen") == 0 ||
                    inputRangeStr.compare("[-10,+10]") == 0 ||
                    inputRangeStr.compare("+-10") == 0) {
                        inputRange = PLUS_MINUS_TEN;
                }
                else if (inputRangeStr.compare("PlusMinusFive") == 0 ||
                         inputRangeStr.compare("[-5,+5]") == 0 ||
                         inputRangeStr.compare("+-5") == 0) {
                        inputRange = PLUS_MINUS_FIVE;
                }
                else if (inputRangeStr.compare("PlusMinusOne") == 0 ||
                         inputRangeStr.compare("[-1,+1]") == 0 ||
                         inputRangeStr.compare("+-1") == 0) {
                        inputRange = PLUS_MINUS_ONE;
                }
                else if (inputRangeStr.compare("PlusMinusZeroPointTwo") == 0 ||
                         inputRangeStr.compare("[-0.2,+0.2]") == 0 ||
                         inputRangeStr.compare("+-0.2") == 0) {
                        inputRange = PLUS_MINUS_ZERO_POINT_TWO;
                }
                else {
                        lcg::Logger(lcg::Critical, "Unknown input range: [%s].\n", inputRangeStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build a real neuron.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractValue(args, "reference", referenceStr)) {
                reference = GRSE;
        }
        else {
                if (referenceStr.compare("GRSE") == 0) {
                        reference = GRSE;
                }
                else if (referenceStr.compare("NRSE") == 0) {
                        reference = NRSE;
                }
                else {
                        lcg::Logger(lcg::Critical, "Unknown reference mode: [%s].\n", referenceStr.c_str());
                        lcg::Logger(lcg::Critical, "Unable to build a real neuron.\n");
                        return NULL;
                }
        }

        if (! lcg::CheckAndExtractBool(args, "holdLastValue", &holdLastValue)) 
                holdLastValue = false;
	else {
        	if (!lcg::CheckAndExtractValue(args, "holdLastValueFilename", filename))
                	filename = LOGFILE;
	}

        if (! lcg::CheckAndExtractBool(args, "adaptiveThreshold", &adaptiveThreshold))
                adaptiveThreshold = false;

        return new lcg::neurons::RealNeuron(spikeThreshold, V0,
                                                 deviceFile.c_str(),
                                                 inputSubdevice, outputSubdevice, readChannel, writeChannel,
                                                 inputConversionFactor, outputConversionFactor,
                                                 inputRange, reference,
                                                 (kernelFile.compare("") == 0 ? NULL : kernelFile.c_str()),
                                                 holdLastValue,filename, adaptiveThreshold, id);
}

#endif

namespace lcg {

extern ThreadSafeQueue<Event*> eventsQueue;

integration_method integr_algo = EULER;

void SetIntegrationMethod(integration_method algo) {
        integr_algo = algo;
}

namespace neurons {

Neuron::Neuron(double Vm0, uint id)
        : DynamicalEntity(id), m_Vm0(Vm0)
{
        m_state.push_back(Vm0); // m_state[0] -> membrane potential
        setName("Neuron");
        setUnits("mV");
}

bool Neuron::initialise()
{
        VM = m_Vm0;
        return true;
}

double Neuron::Vm() const
{
        return VM;
}

double Neuron::Vm0() const
{
        return m_Vm0;
}

double Neuron::output()
{
        return VM;
}

void Neuron::emitSpike() const
{
        emitEvent(new SpikeEvent(this));
}

LIFNeuron::LIFNeuron(double C, double tau, double tarp,
                     double Er, double E0, double Vth, double Iext,
		     bool holdLastValue, const std::string& holdLastValueFilename, uint id)
        : Neuron(E0, id), m_holdLastValue(holdLastValue), m_holdLastValueFilename(holdLastValueFilename)
{
        double dt = GetGlobalDt();
        LIF_C = C;
        LIF_TAU = tau;
        LIF_TARP = tarp;
        LIF_ER = Er;
        LIF_E0 = E0;
        LIF_VTH = Vth;
        LIF_IEXT = Iext;
        LIF_LAMBDA = -1.0 / LIF_TAU;
        LIF_RL = LIF_TAU / LIF_C;
	
        setName("LIFNeuron");
        setUnits("mV");
}

LIFNeuron::~LIFNeuron()
{
        terminate();
}

bool LIFNeuron::initialise()
{
        if (! Neuron::initialise())
                return false;
        m_tPrevSpike = -1000.0;

        struct stat buf;
        if (m_holdLastValue && stat(m_holdLastValueFilename.c_str(),&buf) != -1) {
                FILE *fid = fopen(m_holdLastValueFilename.c_str(), "r");
                if (fid == NULL) {
                        Logger(Important, "Unable to open [%s].\n", m_holdLastValueFilename.c_str());
                        m_Iinj = 0;
                }
                else {
                        fscanf(fid, "%le", &m_Iinj);
                        Logger(Info, "Successfully read holding value (%lf pA) from [%s].\n", m_Iinj, m_holdLastValueFilename.c_str());
                        fclose(fid);
                }
        }
        else {
                m_Iinj = 0.0;
        }

        return true;
}

void LIFNeuron::evolve()
{
        double t = GetGlobalTime();

        if ((t - m_tPrevSpike) <= LIF_TARP) {
                VM = LIF_ER;
        }
        else {
                m_Iinj = LIF_IEXT;
		double Vinf;
                int nInputs = m_inputs.size();
                for (int i=0; i<nInputs; i++)
                        m_Iinj += m_inputs[i];
		
                Vinf = LIF_RL*m_Iinj + LIF_E0;
                VM = Vinf - (Vinf - VM) * exp(LIF_LAMBDA * GetGlobalDt());
        }
        if(VM > LIF_VTH) {
#ifdef LIF_ARTIFICIAL_SPIKE
                /* an ``artificial'' spike is added when the membrane potential reaches threshold */
                VM = -LIF_VTH;
#else
                /* no ``artificial'' spike is added when the membrane potential reaches threshold */
                VM = LIF_E0;
#endif
                m_tPrevSpike = t;
                emitSpike();
        }
}

void LIFNeuron::terminate()
{
        Neuron::terminate();
        if (!m_holdLastValue)
                m_Iinj = 0;
	else {
		// Remove the neuron's intrinsic baseline current
		m_Iinj -= LIF_IEXT;
        	FILE *fid = fopen(m_holdLastValueFilename.c_str(), "w");
        	if (fid != NULL) {
                	fprintf(fid, "%le", m_Iinj);
                	fclose(fid);
                	Logger(Debug, "Written holding value (%lf pA) to [%s].\n", m_Iinj, m_holdLastValueFilename.c_str());
        	}
	}
}

void IzhiStepEuler(double *V, double *U, double dt, double I, double a, double b)
{
        double v = *V, u = *U;
        *V = v + dt * ((0.04 * v * v) + 5 * v + 140 - u + I);
	*U = u + dt * (a * (b * v - u));
}

void IzhiStepRK4(double *V, double *U, double dt, double I, double a, double b)
{
        double v = *V, u = *U;
        double k1, k2, k3, k4, l1, l2, l3, l4,x,y;
        k1 = (0.04 * v * v) + 5 * v + 140 - u + I;
        l1 = a * (b * v - u);
        x = v + dt * 0.5 * k1; 
        y = u + dt * 0.5 * l1;
        k2 = (0.04 * (x * x) + 5 * x + 140 - y + I); 
        l2 = a * (b * x - y);
        x = v + dt * 0.5 * k2; 
        y = u + dt * 0.5 * l2;
        k3 = (0.04 * (x * x) + 5 * x + 140 - y + I); 
        l3 = a * (b * x - y);
        x = v + dt * k3; 
        y = u + dt * l3;
        k4 = (0.04 * (x * x) + 5 * x + 140 - y + I); 
        l4 = a * (b * x - y);
        *V = v + dt * ONE_OVER_SIX * (k1+2*k2+2*k3+k4);
        *U = u + dt * ONE_OVER_SIX * (l1+2*l2+2*l3+l4);
}

IzhikevichNeuron::IzhikevichNeuron(double a, double b, double c,
                     double d, double Vspk, double Iext,
                     uint id)
        : Neuron(c, id)
{
        m_state.push_back(b*VM);                  // m_state[1] -> u the membrane recovery variable
        IZH_A = a;
        IZH_B = b;
        IZH_C = c;
        IZH_D = d;
        IZH_VSPK = Vspk;
        IZH_IEXT = Iext;
        setName("IzhikevichNeuron");
        setUnits("mV");
        switch (lcg::integr_algo) {
                case EULER:
                        doStep = IzhiStepEuler;
                        break;
                case RK4:
                        doStep = IzhiStepRK4;
                        break;
                default:
                        doStep = IzhiStepEuler;
                        break;
        }
}

bool IzhikevichNeuron::initialise()
{
        if (! Neuron::initialise())
                return false;
        return true;
}

void IzhikevichNeuron::evolve()
{
        double v = VM, u = IZH_U, Iinj = IZH_IEXT;
        for (int i=0; i<m_inputs.size(); i++)
             Iinj += m_inputs[i];
        doStep(&v, &u, GetGlobalDt()*1e3, IZH_IEXT, IZH_A, IZH_B);
        VM = v;
        IZH_U = u;
	if(VM >= IZH_VSPK) {
                VM = IZH_C;
                IZH_U += IZH_D;
                emitSpike();
        }
}

double CBNStepEuler(double V, double dt, double I, double C, double area)
{
        return V + dt / (C*1e-5*area) * I;
}

ConductanceBasedNeuron::ConductanceBasedNeuron(double C, double gl, double El, double Iext,
                                               double area, double spikeThreshold, double V0,
                                               uint id)
        : Neuron(V0, id)
{
        m_state.push_back(V0);                  // m_state[1] -> previous membrane voltage (for spike detection)

        CBN_C = C;
        CBN_GL = gl;
        CBN_EL = El;
        CBN_IEXT = Iext;
        CBN_AREA = area;
        CBN_SPIKE_THRESH = spikeThreshold;
        CBN_GL_NS = gl*10*area; // leak conductance in nS
        CBN_COEFF = GetGlobalDt() / (C*1e-5*area); // coefficient

        setName("ConductanceBasedNeuron");
        setUnits("mV");

        switch (lcg::integr_algo) {
                case EULER:
                        doStep = CBNStepEuler;
                        break;
                case RK4:
                        // FIX!!!
                        doStep = CBNStepEuler;
                        break;
                default:
                        doStep = CBNStepEuler;
                        break;
        }
}

void ConductanceBasedNeuron::evolve()
{
        double Iinj, v;
        // externally applied current plus leak current
	Iinj = CBN_IEXT + CBN_GL_NS * (CBN_EL - VM);	// (pA)
        // sum all the ionic currents
	for(uint i=0; i<m_inputs.size(); i++)
		Iinj += m_inputs[i];	        // (pA)
        CBN_VM_PREV = VM;
        VM = doStep(VM, GetGlobalDt(), Iinj, CBN_C, CBN_AREA);
        if (VM >= CBN_SPIKE_THRESH && CBN_VM_PREV < CBN_SPIKE_THRESH)
                emitSpike();
}

#ifdef HAVE_LIBCOMEDI

RealNeuron::RealNeuron(double spikeThreshold, double V0,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor,
                       uint inputRange, uint reference, const char *kernelFile,
                       bool holdLastValue, const std::string& holdLastValueFilename,
		       bool adaptiveThreshold, uint id)
        : Neuron(V0, id), m_aec(kernelFile),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, inputRange, reference),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, reference),
          m_holdLastValue(holdLastValue),m_holdLastValueFilename(holdLastValueFilename),
	  m_adaptiveThreshold(adaptiveThreshold)
{
        m_state.push_back(V0);        // m_state[1] -> previous membrane voltage (for spike detection)
        RN_SPIKE_THRESH = spikeThreshold;
        setName("RealNeuron");
        setUnits("mV");
}

RealNeuron::RealNeuron(double spikeThreshold, double V0,
                       const char *deviceFile,
                       uint inputSubdevice, uint outputSubdevice,
                       uint readChannel, uint writeChannel,
                       double inputConversionFactor, double outputConversionFactor,
                       uint inputRange, uint reference,
                       const double *AECKernel, size_t kernelSize,
                       bool holdLastValue, const std::string& holdLastValueFilename,
		       bool adaptiveThreshold, uint id)
        : Neuron(V0, id), m_aec(AECKernel, kernelSize),
          m_input(deviceFile, inputSubdevice, readChannel, inputConversionFactor, inputRange, reference),
          m_output(deviceFile, outputSubdevice, writeChannel, outputConversionFactor, reference),
          m_holdLastValue(holdLastValue),m_holdLastValueFilename(holdLastValueFilename),
	  m_adaptiveThreshold(adaptiveThreshold)
{
        m_state.push_back(V0);        // m_state[1] -> previous membrane voltage (for spike detection)
        RN_SPIKE_THRESH = spikeThreshold;
        setName("RealNeuron");
        setUnits("mV");
}

RealNeuron::~RealNeuron()
{
        terminate();
}

bool RealNeuron::initialise()
{
        if (! Neuron::initialise()  ||
            ! m_input.initialise()  ||
            ! m_output.initialise())
                return false;

        struct stat buf;

        if (m_holdLastValue && stat(m_holdLastValueFilename.c_str(),&buf) != -1) {
                FILE *fid = fopen(m_holdLastValueFilename.c_str(), "r");
                if (fid == NULL) {
                        Logger(Important, "Unable to open [%s].\n", m_holdLastValueFilename.c_str());
                        m_Iinj = 0;
                }
                else {
                        fscanf(fid, "%le", &m_Iinj);
                        Logger(Important, "Successfully read holding value (%lf pA) from [%s].\n", m_Iinj, m_holdLastValueFilename.c_str());
                        fclose(fid);
                }
        }
        else {
                m_Iinj = 0.0;
        }

        double Vr = m_input.read();
        if (! m_aec.initialise(m_Iinj,Vr))
                return false;
        VM = m_aec.compensate(Vr);
        m_aec.pushBack(m_Iinj);

        /*
        if (!m_aec.hasKernel()) {
                VM = Vr;
        }
        else {
                VM = m_aec.compensate(Vr);
                m_aec.pushBack(m_Iinj);
                for (int i=0; i<m_delaySteps; i++)
                        m_VrDelay[i] = Vr;
        }
        */

        m_Vth = RN_SPIKE_THRESH;
        m_Vmin = -80;
        m_Vmax = RN_SPIKE_THRESH + 20;

        return true;
}
        
void RealNeuron::terminate()
{
        Neuron::terminate();
        if (!m_holdLastValue)
                m_output.write(m_Iinj = 0);
        FILE *fid = fopen(m_holdLastValueFilename.c_str(), "w");
        if (fid != NULL) {
                fprintf(fid, "%le", m_Iinj);
                fclose(fid);
                Logger(Debug, "Written holding value (%lf pA) to [%s].\n", m_Iinj, m_holdLastValueFilename.c_str());
        }
}

void RealNeuron::evolve()
{
        // compute the total input current
        m_Iinj = 0.0;
        size_t nInputs = m_inputs.size();
        for (int i=0; i<nInputs; i++)
                m_Iinj += m_inputs[i];
//#ifndef TRIM_ANALOG_OUTPUT
        /*** BE SAFE! ***/
        if (m_Iinj < -10000)
                m_Iinj = -10000;
//#endif
        // read current value of the membrane potential
        double Vr = m_input.read();
        // inject the total input current into the neuron
        m_output.write(m_Iinj);
        // store the previous value of the membrane potential
        RN_VM_PREV = VM;
        // compensate the recorded voltage
        VM = m_aec.compensate(Vr);
        // store the injected current into the buffer of the AEC
        m_aec.pushBack(m_Iinj);

        /*** SPIKE DETECTION ***/
        if (VM >= m_Vth && RN_VM_PREV < m_Vth) {
                emitSpike();
                if (m_adaptiveThreshold) {
                        m_Vmin = VM;
                        m_Vmax = VM;
                }
        }
        if (m_adaptiveThreshold) {
                if (VM < m_Vmin) {
                        m_Vmin = VM;
                }
                else if (VM > m_Vmax) {
                        m_Vmax = VM;
                        m_Vth = m_Vmax - 0.15 * (m_Vmax - m_Vmin);
                }
        }
}

bool RealNeuron::hasMetadata(size_t *ndims) const
{
        //if (m_aec.hasKernel())
        *ndims = 1;
        return m_aec.hasKernel();
}

const double* RealNeuron::metadata(size_t *dims, char *label) const
{
        //if (m_aec.hasKernel()) {
        sprintf(label, "Electrode_Kernel");
        dims[0] = m_aec.kernelLength();
        return m_aec.kernel();
        //}
        //return NULL;
}

#endif // HAVE_LIBCOMEDI

} // namespace neurons

} // namespace lcg

