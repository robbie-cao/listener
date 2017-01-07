#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>
#include "sndfile_pars.h"

sf_key_value_t formats[] = {
    {"wav", SF_FORMAT_WAV}
    ,
    {"aiff", SF_FORMAT_AIFF}
    ,
    {"au", SF_FORMAT_AU}
    ,
    {"raw", SF_FORMAT_RAW}
    ,
    {"paf", SF_FORMAT_PAF}
    ,
    {"svx", SF_FORMAT_SVX}
    ,
    {"nist", SF_FORMAT_NIST}
    ,
    {"voc", SF_FORMAT_VOC}
    ,
    {"ircam", SF_FORMAT_IRCAM}
    ,
    {"w64", SF_FORMAT_W64}
    ,
    {"mat4", SF_FORMAT_MAT4}
    ,
    {"mat5", SF_FORMAT_MAT5}
    ,
    {"pvf", SF_FORMAT_PVF}
    ,
    {"xi", SF_FORMAT_XI}
    ,
    {"htk", SF_FORMAT_HTK}
    ,
    {"sds", SF_FORMAT_SDS}
    ,
    {"avr", SF_FORMAT_AVR}
    ,
    {"wavex", SF_FORMAT_WAVEX}
    ,
    {"sd2", SF_FORMAT_SD2}
    ,
    {"flac", SF_FORMAT_FLAC}
    ,
    {"caf", SF_FORMAT_CAF}
    ,
    {"wve", SF_FORMAT_WVE}
    ,
    {"ogg", SF_FORMAT_OGG}
    ,
    {"mpc2k", SF_FORMAT_MPC2K}
    ,
    {"rf64", SF_FORMAT_RF64}
    ,
    {NULL, -1}
    ,
};

sf_key_value_t *sf_get_formats()
{
    return formats;
}

sf_key_value_t subtypes[] = {
    {"pcm_s8", SF_FORMAT_PCM_S8}
    ,
    {"pcms8", SF_FORMAT_PCM_S8}
    ,
    {"pcm8", SF_FORMAT_PCM_S8}
    ,
    {"pcm_16", SF_FORMAT_PCM_16}
    ,
    {"pcm16", SF_FORMAT_PCM_16}
    ,
    {"pcm_24", SF_FORMAT_PCM_24}
    ,
    {"pcm24", SF_FORMAT_PCM_24}
    ,
    {"pcm_32", SF_FORMAT_PCM_32}
    ,
    {"pcm32", SF_FORMAT_PCM_32}
    ,
    {"pcm_u8", SF_FORMAT_PCM_U8}
    ,
    {"pcmu8", SF_FORMAT_PCM_U8}
    ,
    {"float", SF_FORMAT_FLOAT}
    ,
    {"double", SF_FORMAT_DOUBLE}
    ,
    {"ulaw", SF_FORMAT_ULAW}
    ,
    {"alaw", SF_FORMAT_ALAW}
    ,
    {"ima_adpcm", SF_FORMAT_IMA_ADPCM}
    ,
    {"ms_adpcm", SF_FORMAT_MS_ADPCM}
    ,
    {"gsm610", SF_FORMAT_GSM610}
    ,
    {"vox_adpcm", SF_FORMAT_VOX_ADPCM}
    ,
    {"g721_32", SF_FORMAT_G721_32}
    ,
    {"g723_24", SF_FORMAT_G723_24}
    ,
    {"g723_40", SF_FORMAT_G723_40}
    ,
    {"dwvw_12", SF_FORMAT_DWVW_12}
    ,
    {"dwvw_16", SF_FORMAT_DWVW_16}
    ,
    {"dwvw_24", SF_FORMAT_DWVW_24}
    ,
    {"dwvw_n", SF_FORMAT_DWVW_N}
    ,
    {"dpcm_8", SF_FORMAT_DPCM_8}
    ,
    {"dpcm_16", SF_FORMAT_DPCM_16}
    ,
    {"vorbis", SF_FORMAT_VORBIS}
    ,
    {NULL, -1}
    ,
};

sf_key_value_t *sf_get_subtypes()
{
    return subtypes;
}

SF_INFO *sf_parse_settings(char *settings)
{
    SF_INFO *result;
    char *work = strdup(settings), *wptr, *wptr2 = NULL;
    int len = strlen(work);
    int index;

    int sample_rate = 44100;
    int channels = 2;
    int file_format = SF_FORMAT_WAV;
    int subtype = SF_FORMAT_PCM_16;
    int endianness = SF_ENDIAN_FILE;

    for (index = 0; index < len; index++)
        work[index] = tolower(work[index]);

    wptr = work;
    for (;;) {
        char *token = strtok_r(wptr, ",", &wptr2);
        wptr = NULL;
        if (!token)
            break;

        if (strstr(token, "khz"))
            sample_rate = (int)(atof(token) * 1000.0);
        else if (strstr(token, "hz"))
            sample_rate = atoi(token);
        else if (strcmp(token, "endian_file") == 0)
            endianness = SF_ENDIAN_FILE;
        else if (strcmp(token, "endian_little") == 0 || strcmp(token, "little") == 0)
            endianness = SF_ENDIAN_LITTLE;
        else if (strcmp(token, "endian_big") == 0 || strcmp(token, "big") == 0)
            endianness = SF_ENDIAN_BIG;
        else if (strcmp(token, "endian_cpu") == 0)
            endianness = SF_ENDIAN_CPU;
        else {
            int value = atoi(token);
            int cur_len = strlen(token);
            char is_number = 1, found = 0;
            for (index = 0; index < cur_len; index++) {
                if (!isdigit(token[index])) {
                    is_number = 0;
                    break;
                }
            }
            if (is_number) {
                if (value < 0) {
                    fprintf(stderr, "Value (%s) cannot be less than 0\n", token);
                    return NULL;
                }
                if (value <= 16) {
                    channels = value;
                    continue;
                }
                if (value >= 1000) {
                    sample_rate = value;
                    continue;
                }

                fprintf(stderr, "Value %s is not understood (too big for channel number, too small for sample rate)\n",
                        token);
                return NULL;
            }

            for (index = 0; formats[index].key != NULL; index++) {
                if (strcmp(token, formats[index].key) == 0) {
                    file_format = formats[index].value;
                    found = 1;
                    break;
                }
            }
            if (found)
                continue;

            for (index = 0; subtypes[index].key != NULL; index++) {
                if (strcmp(token, subtypes[index].key) == 0) {
                    subtype = subtypes[index].value;
                    found = 1;
                    break;
                }
            }
            if (found)
                continue;

            fprintf(stderr, "Parameter '%s' is not understood\n", token);
            return NULL;
        }
    }

    free(work);

    result = (SF_INFO *) calloc(1, sizeof(SF_INFO));

    result->samplerate = sample_rate;
    result->channels = channels;
    result->format = file_format | subtype | endianness;

    return result;
}
