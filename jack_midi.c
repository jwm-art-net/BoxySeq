#include "jack_midi.h"

#include "common.h"
#include "debug.h"

#include "pattern.h"
#include "jack_transport.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jack/midiport.h>
#include <jack/ringbuffer.h>


struct jack_midi_data
{
    char*               client_name;

    jack_client_t*      client;

    jack_ringbuffer_t * out_jack_ring;
    jack_port_t *       out_jack_port;

    jtransp*    tr;
    boxyseq*    bs;

    bbt_t oph;       /* old play head tick       */
    bbt_t onph;       /* old next play head tick  */
};


static int  jmidi_process(jack_nframes_t nframes, void* arg);
static void jmidi_jack_shutdown(void *arg);


jmidi* jmidi_new(void)
{
    jmidi* jm = malloc(sizeof(*jm));

    if (!jm)
        goto fail;

    jm->client_name = 0;

    jm->out_jack_ring = 0;
    jm->out_jack_port = 0;

    if (!(jm->tr = jtransp_new()))
        goto fail;

    MESSAGE("jm->tr = %p\n", jm->tr);

    jm->bs = 0;

    jm->oph = -1;
    jm->onph = 0;

    return jm;

fail:
    free(jm);
    WARNING("out of memory for jack midi data\n");
    return 0;
}


void jmidi_free(jmidi* jm)
{
    if (!jm)
        return;

    jtransp_free(jm->tr);
    free(jm);
}


_Bool jmidi_startup(jmidi* jm, boxyseq* bs)
{
    const char *server_name = NULL;
    jack_options_t options = JackNullOption;
    jack_status_t status;

    jm->client = jack_client_open(  boxyseq_basename(bs),
                                    options,
                                    &status,
                                    server_name );

    if (!jm->client)
    {
        WARNING("jack_client_open() failed: status = 0x%2.0x\n", status);
        if (status & JackServerFailed)
            WARNING("Unable to connect to JACK server\n");
        return 0;
    }

    jm->bs = bs;

    jm->client_name = jack_get_client_name(jm->client);

    if (status & JackServerStarted)
        MESSAGE("JACK server started\n");

    if (status & JackNameNotUnique)
        MESSAGE("unique name `%s' assigned\n", jm->client_name);

    jtransp_startup(jm->tr, jm->client);

    if (jack_set_process_callback(jm->client, jmidi_process, jm) != 0)
    {
        WARNING("failed to init jack process callback\n");
        return 0;
    }

    jack_on_shutdown(jm->client, jmidi_jack_shutdown, jm);

    jm->out_jack_port = jack_port_register( jm->client,
                                            "output",
                                            JACK_DEFAULT_MIDI_TYPE,
                                            JackPortIsOutput, 0   );

    if (!jm->out_jack_port)
    {
        WARNING("no more jack ports available\n");
        return 0;
    }

    if (jack_activate(jm->client))
    {
        WARNING("failed to activate jack client\n");
        return 0;
    }

    return 1;
}


void jmidi_shutdown(jmidi* jm)
{
    jtransp_shutdown(jm->tr);
    jack_client_close(jm->client);
}

const char* jmidi_client_name(jmidi* jm)
{
    return jm->client_name;
}

jack_client_t*  jmidi_jack_client(jmidi* jm)
{
    return jm->client;
}


jtransp* jmidi_jtransp(jmidi* jm)
{
    return jm->tr;
}

/*
void midi_RT_queue_note(int port, pnote*)
{

}
*/


static int jmidi_process(jack_nframes_t nframes, void* arg)
{
    jmidi* jm = (jmidi*)arg;
    jtransp* tr = jm->tr;

    jtransp_rt_poll(tr, nframes);

    if (!jtransp_rt_is_valid(tr))
        return 0;

    if (!jtransp_rt_is_rolling(tr))
        return 0;

    const bbt_t ph =  (bbt_t)(0 + jtransp_rt_ticks(tr));
    const bbt_t nph = (bbt_t)
                        (0 + trunc(ph + jtransp_rt_ticks_per_period(tr)));

    if (ph && ph == jm->oph)
        return 0;

    if (ph != jm->onph)
    {
        if (jm->onph > ph)
            WARNING("duplicated %lu ticks\n",
                    (long unsigned int)(jm->onph - ph));
        else
            WARNING("dropped %lu ticks\n",
                    (long unsigned int)(ph - jm->onph));
    }

    jm->onph = nph;

    boxyseq_rt_play(jm->bs, ph, nph);

    jm->oph = ph;

    return 0;
}


static void jmidi_jack_shutdown(void *arg)
{
    WARNING("jack_shutdown callback called\n");
    exit(1);
}
