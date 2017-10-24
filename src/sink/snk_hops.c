    
    #include "snk_hops.h"

    snk_hops_obj * snk_hops_construct(const snk_hops_cfg * snk_hops_config, const msg_hops_cfg * msg_hops_config) {

        snk_hops_obj * obj;

        obj = (snk_hops_obj *) malloc(sizeof(snk_hops_obj));

        obj->timeStamp = 0;

        obj->hopSize = msg_hops_config->hopSize;
        obj->nChannels = msg_hops_config->nChannels;
        obj->fS = snk_hops_config->fS;
        
        obj->format = format_clone(snk_hops_config->format);
        obj->interface = interface_clone(snk_hops_config->interface);

        memset(obj->bytes, 0x00, 4 * sizeof(char));

        switch (obj->format->type) {
            
            case format_binary_int08: break;
            case format_binary_int16: break;
            case format_binary_int24: break;
            case format_binary_int32: break;
            default:

                printf("Invalid format.\n");
                exit(EXIT_FAILURE);

            break;

        }

        obj->smessage = (char *) malloc(sizeof(char) * msg_hops_config->nChannels * msg_hops_config->hopSize * 4);
        memset(obj->smessage, 0x00, sizeof(char) * msg_hops_config->nChannels * msg_hops_config->hopSize * 4);

        obj->in = (msg_hops_obj *) NULL;

        return obj;

    }

    void snk_hops_destroy(snk_hops_obj * obj) {

        format_destroy(obj->format);
        interface_destroy(obj->interface);
        free((void *) obj->smessage);

        free((void *) obj);

    }

    void snk_hops_connect(snk_hops_obj * obj, msg_hops_obj * in) {

        obj->in = in;

    }

    void snk_hops_disconnect(snk_hops_obj * obj) {

        obj->in = (msg_hops_obj *) NULL;

    }

    int snk_hops_open(snk_hops_obj * obj) {

        switch(obj->interface->type) {

            case interface_blackhole:

                // Empty

            break;

            case interface_file:

                obj->fp = fopen(obj->interface->fileName, "wb");

            break;

            case interface_socket:

                obj->sserver.sin_family = AF_INET;
                obj->sserver.sin_port = htons(obj->interface->port);
                inet_aton(obj->interface->ip, &(obj->sserver.sin_addr));

                obj->sid = socket(AF_INET, SOCK_STREAM, 0);

                if ( (connect(obj->sid, (struct sockaddr *) &(obj->sserver), sizeof(obj->sserver))) < 0 ) {

                    printf("Cannot connect to server\n");
                    exit(EXIT_FAILURE);

                }

            break;

            case interface_soundcard:

                printf("Not implemented yet.\n");
                exit(EXIT_FAILURE);

            break;

            default:

                printf("Invalid interface type.\n");
                exit(EXIT_FAILURE);

            break;           

        }

    }

    int snk_hops_close(snk_hops_obj * obj) {

        switch(obj->interface->type) {

            case interface_blackhole:

                // Empty

            break;

            case interface_file:

                fclose(obj->fp);

            break;

            case interface_socket:

                close(obj->sid);

            break;

            case interface_soundcard:

                printf("Not implemented yet.\n");
                exit(EXIT_FAILURE);

            break;

            default:

                printf("Invalid interface type.\n");
                exit(EXIT_FAILURE);

            break;

        }

    }

    int snk_hops_process(snk_hops_obj * obj) {

        int rtnValue;

        switch(obj->interface->type) {

            case interface_blackhole:

                rtnValue = snk_hops_process_blackhole(obj);

            break;  

            case interface_file:

                rtnValue = snk_hops_process_file(obj);

            break;

            case interface_socket:

                rtnValue = snk_hops_process_socket(obj);

            break;

            case interface_soundcard:

                rtnValue = snk_hops_process_soundcard(obj);

            break;

            default:

                printf("Invalid interface type.\n");
                exit(EXIT_FAILURE);

            break;

        }

        return rtnValue;

    }

    int snk_hops_process_blackhole(snk_hops_obj * obj) {

        int rtnValue;

        if (obj->in->timeStamp != 0) {

            rtnValue = 0;

        }
        else {

            rtnValue = -1;

        }

        return rtnValue;

    }

    int snk_hops_process_file(snk_hops_obj * obj) {

        unsigned int iSample;
        unsigned int iChannel;
        float sample;
        unsigned int nBytes;
        int rtnValue;

        if (obj->in->timeStamp != 0) {

            switch (obj->format->type) {

                case format_binary_int08: nBytes = 1; break;
                case format_binary_int16: nBytes = 2; break;
                case format_binary_int24: nBytes = 3; break;
                case format_binary_int32: nBytes = 4; break;

            }

            for (iSample = 0; iSample < obj->hopSize; iSample++) {
                
                for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {

                    sample = obj->in->hops->array[iChannel][iSample];
                    pcm_normalized2signedXXbits(sample, nBytes, obj->bytes);
                    fwrite(&(obj->bytes[4-nBytes]), sizeof(char), nBytes, obj->fp);

                }

            }

            rtnValue = 0;

        }
        else {

            rtnValue = -1;

        }

        return rtnValue;

    }

    int snk_hops_process_socket(snk_hops_obj * obj) {

        unsigned int iChannel;
        unsigned int iSample;
        float sample;
        unsigned int nBytes;
        unsigned int nBytesTotal;
        int rtnValue;

        if (obj->in->timeStamp != 0) {

            switch (obj->format->type) {

                case format_binary_int08: nBytes = 1; break;
                case format_binary_int16: nBytes = 2; break;
                case format_binary_int24: nBytes = 3; break;
                case format_binary_int32: nBytes = 4; break;

            }

            nBytesTotal = 0;

            for (iSample = 0; iSample < obj->hopSize; iSample++) {

                for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {

                    nBytesTotal += nBytes;

                    sample = obj->in->hops->array[iChannel][iSample];
                    pcm_normalized2signedXXbits(sample, nBytes, obj->bytes);
                    memcpy(&(obj->smessage[nBytesTotal]), &(obj->bytes[4-nBytes]), sizeof(char) * nBytes);

                }

            }

            if (send(obj->sid, obj->smessage, nBytesTotal, 0) < 0) {
                printf("Could not send message.\n");
                exit(EXIT_FAILURE);
            }

            rtnValue = 0;

        }
        else {

            rtnValue = -1;

        }

        return rtnValue;

    }

    int snk_hops_process_soundcard(snk_hops_obj * obj) {

        printf("Not implemented\n");
        exit(EXIT_FAILURE);

    }

    snk_hops_cfg * snk_hops_cfg_construct(void) {

        snk_hops_cfg * cfg;

        cfg = (snk_hops_cfg *) malloc(sizeof(snk_hops_cfg));

        cfg->fS = 0;
        cfg->format = (format_obj *) NULL;
        cfg->interface = (interface_obj *) NULL;

        return cfg;

    }

    void snk_hops_cfg_destroy(snk_hops_cfg * snk_hops_config) {

        if (snk_hops_config->format != NULL) {
            format_destroy(snk_hops_config->format);
        }
        if (snk_hops_config->interface != NULL) {
            interface_destroy(snk_hops_config->interface);
        }

        free((void *) snk_hops_config);

    }