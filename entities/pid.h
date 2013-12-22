#ifndef PID_H
#define PID_H

#include "entity.h"
#include "utils.h"

namespace lcg {

#define PID_BASELINE m_parameters["baseline"]
#define PID_GP       m_parameters["gp"]
#define PID_GI       m_parameters["gi"]
#define PID_GD       m_parameters["gd"]

/**
* PID controller entity
* @param baseline - baseline current pick a value that makes the neuron fire.
* @param gp - proportional gain.
* @param gi - integral gain.
* @param gd - derivative gain.

* The controller changes its output value when receiving a SPIKE or a TRIGGER event.
* It requires two waveform inputs. The waveforms are compared (w2-w1).
* The output follows the equation:
* /f[
* $\fraq{ (x_2-x_1)^2 + (y_2 - y_1)^2 }{1}$
* /f]
*/

class PID : public Entity {
public:
        PID(double baseline, double gp, double gi, double gd = 0.0, uint id = GetId());
        bool state();
        void changeState();
        virtual double output();
        virtual bool initialise();
        virtual void step();
        void handleEvent(const Event *event);
private:
        bool m_state;
        double m_output;
        double m_erri;
        double m_errpPrev;
};

} // namespace lcg

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* PIDFactory(string_dict& args);
        
#ifdef __cplusplus
}
#endif

#endif
