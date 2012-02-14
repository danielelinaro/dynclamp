#ifndef NEURONS_H
#define NEURONS_H

#include "dynamical_entity.h"
#include "types.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#ifdef HAVE_LIBCOMEDI
#include <comedilib.h>
#include "analog_io.h"
#include "aec.h"
#endif // HAVE_LIBCOMEDI
#endif // HAVE_CONFIG_H

#define VM       m_state[0]

#define LIF_C    m_parameters[0]
#define LIF_TAU  m_parameters[1]
#define LIF_TARP m_parameters[2]
#define LIF_ER   m_parameters[3]
#define LIF_E0   m_parameters[4]
#define LIF_VTH  m_parameters[5]
#define LIF_IEXT m_parameters[6]
//#define LIF_ARTIFICIAL_SPIKE

#define CBN_C           m_parameters[0]
#define CBN_GL          m_parameters[1]
#define CBN_EL          m_parameters[2]
#define CBN_IEXT        m_parameters[3]
#define CBN_AREA        m_parameters[4]
#define CBN_THRESH      m_parameters[5]

namespace dynclamp {

namespace neurons {

class Neuron : public DynamicalEntity {
public:
        Neuron(double Vm0, uint id = GetId(), double dt = GetGlobalDt());
        double Vm() const;
        virtual double output() const;
protected:
        void emitSpike() const;
};

class LIFNeuron : public Neuron {
public:

        /**
         * \param C membrane capacitance.
         * \param tau membrane time constant.
         * \param tarp absolute refractory period.
         * \param Er reset voltage.
         * \param E0 resting potential.
         * \param Vth threshold.
         * \param Iext externally applied current.
         */
        LIFNeuron(double C, double tau, double tarp,
                  double Er, double E0, double Vth, double Iext,
                  uint id = GetId(), double dt = GetGlobalDt());

protected:
        virtual void evolve();

private:
        double m_tPrevSpike;
};

class ConductanceBasedNeuron : public Neuron {
public:
        /**
         * \param C membrane capacitance (uF/cm^2)
         * \param gl leak conductance (S/cm^2)
         * \param El leak reversal potential (mV)
         * \param Iext externally applied current (pA)
         * \param area surface of the membrane (um^2)
         * \param spikeThreshold threshold for emitting a spike (mV)
         * \param V0 initial value of the membrane potential (mV)
         */
        ConductanceBasedNeuron(double C, double gl, double El, double Iext,
                               double area, double spikeThreshold, double V0,
                               uint id = GetId(), double dt = GetGlobalDt());
protected:
        virtual void evolve();
};

#ifdef HAVE_LIBCOMEDI

#define RN_VM_PREV                  m_state[1]
#define RN_SPIKE_THRESH             m_parameters[0]

class RealNeuron : public Neuron {
public:
        RealNeuron(const char *kernelFile,
                   const char *deviceFile,
                   uint inputSubdevice, uint outputSubdevice,
                   uint readChannel, uint writeChannel,
                   double inputConversionFactor, double outputConversionFactor,
                   double spikeThreshold, double V0,
                   uint id = GetId(), double dt = GetGlobalDt());

        RealNeuron(const double *AECKernel, size_t kernelSize,
                   const char *deviceFile,
                   uint inputSubdevice, uint outputSubdevice,
                   uint readChannel, uint writeChannel,
                   double inputConversionFactor, double outputConversionFactor,
                   double spikeThreshold, double V0,
                   uint id = GetId(), double dt = GetGlobalDt());

        virtual const double* metadata(size_t *dims, char *label) const;

protected:
        virtual void evolve();

private:
        ComediAnalogInput  m_input;
        ComediAnalogOutput m_output;
        AEC m_aec;
};
#endif // HAVE_LIBCOMEDI

} // namespace neurons

} // namespace dynclamp


/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

dynclamp::Entity* LIFNeuronFactory(dictionary& args);
#ifdef HAVE_LIBCOMEDI
dynclamp::Entity* RealNeuronFactory(dictionary& args);
#endif
	
#ifdef __cplusplus
}
#endif

#endif

