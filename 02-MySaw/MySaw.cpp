/*
    Example implementation of the PADsynth basic algorithm
    By: Nasca O. Paul, Tg. Mures, Romania

    Ported to pure C by Paul Batchelor

    This implementation and the algorithm are released under Public Domain
    Feel free to use it into your projects or your products ;-)

    This implementation is tested under GCC/Linux, but it's
    very easy to port to other compiler/OS.
*/

#include <stdlib.h>
#include <math.h>

#include "sndfile.h"
#include "sndfile.hh"
#include "soundpipe.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#include "SC_PlugIn.h"

// InterfaceTable contains pointers to functions in the host (server).
static InterfaceTable *ft;

typedef struct user_data {
		sp_ftbl *ft, *amps;
		sp_osc *osc;
		SPFLOAT fc;
} UserData;

// declare struct to hold unit generator state
struct MySaw : public Unit
{
		double mPhase; // phase of the oscillator, from -1 to 1.
		float mFreqMul; // a constant for multiplying frequency
		UserData ud;
		sp_data *sp;
};

// declare unit generator functions
static void MySaw_next_a(MySaw *unit, int inNumSamples);
static void MySaw_next_k(MySaw *unit, int inNumSamples);
static void MySaw_Ctor(MySaw* unit);


void process(sp_data *sp, void *udata) {
		UserData* ud = (UserData*)udata;
		//sp->out = ud->ft->tbl[sp->pos % ud->ft->size];
		sp_osc_compute(sp, ud->osc, NULL, &sp->out[0]);
}

//////////////////////////////////////////////////////////////////

// Ctor is called to initialize the unit generator.
// It only executes once.

// A Ctor usually does 3 things.
// 1. set the calculation function.
// 2. initialize the unit generator state variables.
// 3. calculate one sample of output.
void MySaw_Ctor(MySaw* unit)
{
    int i;

    // 1. set the calculation function.
    if (INRATE(0) == calc_FullRate) {
        // if the frequency argument is audio rate
        SETCALC(MySaw_next_a);
    } else {
        // if the frequency argument is control rate (or a scalar).
        SETCALC(MySaw_next_k);
    }

    sp_create(&unit->sp);
    sp_ftbl_create(unit->sp, &unit->ud.amps, 64);

    sp_ftbl_create(unit->sp, &unit->ud.ft, 262144);
    // just creates a buffer of length ...
    sp_osc_create(&unit->ud.osc);

		// 2. initialize the unit generator state variables.
		// initialize a constant for multiplying the frequency
		unit->mFreqMul = 2.0 * SAMPLEDUR;
		// get initial phase of oscillator
		unit->mPhase = IN0(1);

    unit->sp->sr = SAMPLERATE;
    //sp->len = ud.ft->size;
    // This would generate a 5 second sample
    //sp->len = sp->sr * 5;
    // we want an infinite generator so set len to zero
    unit->sp->len = 0;

    unit->ud.amps->tbl[0] = 0.0;

    for(i = 1; i < unit->ud.amps->size; i++){
        unit->ud.amps->tbl[i] = 1.0 / i;
        if((i % 2) == 0) unit->ud.amps->tbl[i] *= 2.0;
    }

    /* Discovered empirically. multiply frequency by this constant. */
    unit->ud.fc = 1 / (6.0 * 440);

    sp_gen_padsynth(unit->sp, unit->ud.ft, unit->ud.amps, sp_midi2cps(60.0), 40.0);

    sp_osc_init(unit->sp, unit->ud.osc, unit->ud.ft, 0.0);
    unit->ud.osc->freq = sp_midi2cps(70.0) * unit->ud.fc;
    unit->ud.osc->amp = 1.0;

    // 3. calculate one sample of output.
    MySaw_next_k(unit, 1);
}


//////////////////////////////////////////////////////////////////

// The calculation function executes once per control period
// which is typically 64 samples.

// calculation function for an audio rate frequency argument
void MySaw_next_a(MySaw *unit, int inNumSamples)
{
    // get the pointer to the output buffer
    float *out = OUT(0);

    // get the pointer to the input buffer
    float *freq = IN(0);

    // perform a loop for the number of samples in the control period.
    // If this unit is audio rate then inNumSamples will be 64 or whatever
    // the block size is. If this unit is control rate then inNumSamples will
    // be 1.
    for (int i=0; i < inNumSamples; ++i)
    {
        // write the output
        out[i] = unit->ud.ft->tbl[unit->sp->pos % unit->ud.ft->size];
        unit->sp->pos++;
    }
}

//////////////////////////////////////////////////////////////////

// calculation function for a control rate frequency argument
void MySaw_next_k(MySaw *unit, int inNumSamples)
{
    // get the pointer to the output buffer
    float *out = OUT(0);

    // freq is control rate, so calculate it once.
    float freq = IN0(0) * unit->mFreqMul;

    // get phase from struct and store it in a local variable.
    // The optimizer will cause it to be loaded it into a register.
    double phase = unit->mPhase;

    // perform a loop for the number of samples in the control period.
    // If this unit is audio rate then inNumSamples will be 64 or whatever
    // the block size is. If this unit is control rate then inNumSamples will
    // be 1.
    for (int i=0; i < inNumSamples; ++i)
    {
    		// write the output
    		out[i] = unit->ud.ft->tbl[unit->sp->pos % unit->ud.ft->size];
    		unit->sp->pos++;
    }
}


// the entry point is called by the host when the plug-in is loaded
PluginLoad(MySaw)
{
    // InterfaceTable *inTable implicitly given as argument to the load function
    ft = inTable; // store pointer to InterfaceTable

    DefineSimpleUnit(MySaw);
}
