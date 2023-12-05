#include <stdlib.h>
#include <stdio.h>
#include <jansson.h>
#include <unistd.h>

#include "basedef.h"

int main(int argc, char **argv)
{
    const char content[] =
                "{\
                \"CCOTimeOut\":90,\
                \"ReadMeterMode\":1,\
                \"ReadintervalMs\":100,\
                \"WaitCount\":10,\
                \"PortConfig\": {\
                    \"PortType\": 0,\
                    \"PortPara\": {\
                        \"number\": 7,\
                        \"BaudRate\": 9600,\
                        \"Parity\": 0,\
                        \"DataBits\": 8,\
                        \"Stop\": 1\
                    }\
                },\
                \"MeterList\": [\
                    {\
                        \"Sequence\": 0,\
                        \"Address\": \"202101010001\",\
                        \"Model\": \"Multimeter\",\
                         \"ModbusSeq\":0,\
                         \"LinkNo\":4,\
                         \"LinkDevNo\":1\
                    },\
                    {\
                        \"Sequence\": 1,\
                        \"Address\": \"202101010002\",\
                        \"Model\": \"Multimeter\",\
                         \"ModbusSeq\":1,\
                         \"LinkNo\":5,\
                         \"LinkDevNo\":1\
                    },\
                    {\
                        \"Sequence\": 2,\
                        \"Address\": \"202101010003\",\
                        \"Model\": \"Multimeter\",\
                         \"ModbusSeq\":0,\
                         \"LinkNo\":6,\
                         \"LinkDevNo\":1\
                    }\
                ]\
            }";

    json_t *root = json_loads(content, 0, NULL);
    json_decref(root);

    return 0;
}
