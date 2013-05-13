#ifndef DELAY_H
#define DELAY_H

#include "entity.h"
#include "utils.h"

namespace lcg {

class Delay : public Entity {
public:
        Delay(uint nSamples = 1, uint id = GetId());
        Delay(double delay, uint id = GetId());
        virtual ~Delay();

        virtual bool initialise();
        virtual void step();
        virtual double output();

private:
        void allocateBuffer();

private:
        uint m_bufferLength;
        double *m_buffer;
        uint m_bufferPosition;
};

} // namespace dinclamp

/***
 *   FACTORY METHODS
 ***/
#ifdef __cplusplus
extern "C" {
#endif

lcg::Entity* DelayFactory(string_dict& args);
	
#ifdef __cplusplus
}
#endif

#endif

